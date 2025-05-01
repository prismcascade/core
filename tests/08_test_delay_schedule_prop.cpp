#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <prismcascade/ast/operations.hpp>  // substitute
#include <prismcascade/memory/types_internal.hpp>
#include <prismcascade/scheduler/delay.hpp>
#include <prismcascade/scheduler/split_dag.hpp>
#include <prismcascade/scheduler/topological_sort.hpp>

using namespace prismcascade;

static std::shared_ptr<ast::AstNode> mk(uint64_t id) { return ast::make_node("dummy", id, id); }

static bool ranks_uniquely_topo(const std::unordered_map<std::int64_t, scheduler::RankInfo>& tbl,
                                const std::shared_ptr<ast::AstNode>&                         n) {
    if (!n) return true;                                    // 空なら常にOK
    uint32_t rk = tbl.at(n->plugin_instance_handler).rank;  // rank取得
    for (const auto& input : n->inputs) {
        if (auto child = std::get_if<std::shared_ptr<ast::AstNode>>(&input); child && *child) {
            // 葉のほうが先に計算される（値が小さい）
            if (!(rk > tbl.at((*child)->plugin_instance_handler).rank)) return false;
            if (!ranks_uniquely_topo(tbl, *child)) return false;
        } else if (auto sub_edge = std::get_if<ast::SubEdge>(&input)) {
            if (auto sub_edge_source = sub_edge->source.lock()) {
                // 葉のほうが先に計算される（値が小さい）
                if (!(rk > tbl.at(sub_edge_source->plugin_instance_handler).rank)) return false;
                if (!ranks_uniquely_topo(tbl, sub_edge_source)) return false;
            }
        }
    }
    return true;
}

// ---------- 補助: 辺列挙と weight 生成 ----------
struct EdgeInfo {
    std::shared_ptr<ast::AstNode> child;
    std::shared_ptr<ast::AstNode> parent;
    std::int64_t                  w;
    bool                          has_window;
};
static std::vector<EdgeInfo> collectEdges(const std::vector<std::shared_ptr<ast::AstNode>>& v) {
    std::vector<EdgeInfo> edges;
    for (auto& parent : v) {
        for (std::size_t idx = 0; idx < parent->inputs.size(); ++idx) {
            if (auto se = std::get_if<ast::SubEdge>(&parent->inputs[idx])) {
                if (auto child = se->source.lock()) {
                    bool         has_win = parent->input_window[idx].has_value();
                    std::int64_t w       = has_win ? parent->input_window[idx]->look_ahead : 0;
                    edges.push_back({child, parent, w, has_win});
                }
            }
        }
    }
    return edges;
}

// デバッグ用

auto show_delay_impl = [&](auto result, const char* name, const std::shared_ptr<ast::AstNode>& node) {
    auto maybe_delay = result.ranks.at(node->plugin_instance_handler).delay;
    std::cout << "delay(" << name << ") = ";
    if (maybe_delay)
        std::cout << *maybe_delay << std::endl;
    else
        std::cout << "(empty)" << std::endl;
};

////////
/*-------------------------------------------------------------
 * print_graph
 *   nodes   : この DAG に含まれる AstNode (順不同で OK)
 *   os      : 出力ストリーム (既定は std::cerr)
 *   show_w  : true なら Window::look_ahead も併記
 *-----------------------------------------------------------*/
void print_graph(const std::vector<std::shared_ptr<ast::AstNode>>& nodes, std::ostream& os = std::cerr,
                 bool show_w = true) {
    struct EdgeRec {
        std::uint64_t               src;
        std::uint64_t               dst;
        std::size_t                 input_idx;
        std::optional<std::int64_t> w;
    };

    std::vector<EdgeRec> edges;
    edges.reserve(32);

    /*------------- collect edges -------------*/
    for (const auto& dst_node : nodes) {
        const auto& inputs  = dst_node->inputs;
        const auto& windows = dst_node->input_window;

        for (std::size_t idx = 0; idx < inputs.size(); ++idx) {
            const auto& val = inputs[idx];

            if (auto sp = std::get_if<std::shared_ptr<ast::AstNode>>(&val); sp && *sp) {
                edges.push_back(
                    EdgeRec{(*sp)->plugin_instance_handler, dst_node->plugin_instance_handler, idx,
                            windows[idx] ? std::optional<std::int64_t>{windows[idx]->look_ahead} : std::nullopt});
            } else if (auto se = std::get_if<ast::SubEdge>(&val)) {
                if (auto src_ptr = se->source.lock()) {
                    edges.push_back(
                        EdgeRec{src_ptr->plugin_instance_handler, dst_node->plugin_instance_handler, idx,
                                windows[idx] ? std::optional<std::int64_t>{windows[idx]->look_ahead} : std::nullopt});
                }
            }
        }
    }

    /*------------- pretty print -------------*/
    os << '\n' << "=== DAG DEBUG DUMP ===\n";

    os << "Nodes (" << nodes.size() << "): ";
    bool first = true;
    for (const auto& n : nodes) {
        os << (first ? "" : ", ") << n->plugin_instance_handler;
        first = false;
    }
    os << "\n\nEdges (" << edges.size() << "):\n";

    for (const auto& e : edges) {
        os << "  " << e.src << " ──";
        if (show_w && e.w)
            os << '[' << *e.w << "]─▶ ";
        else
            os << "──▶ ";
        os << e.dst << "  (input#" << e.input_idx << ")\n";
    }
    os << "========================\n";
}

////////

#define show_delay(x) show_delay_impl(result, #x, x)

TEST(DelayAlgoProp, MixedEdges01) {
    auto R = mk(20), X = mk(21), Y = mk(22);
    R->resize_inputs({{VariableType::Int}, {VariableType::Int}});
    X->resize_inputs({{VariableType::Int}});
    Y->resize_inputs({{VariableType::Int}});
    ast::substitute(R, 0, X);
    ast::substitute(R, 1, Y);
    // ast::substitute(Y, 0, ast::SubEdge{X, 0});
    R->input_window[0] = ast::AstNode::InputWindow{100, -100};
    R->input_window[1] = ast::AstNode::InputWindow{0, 0};

    auto result = scheduler::topological_sort(R);
    EXPECT_TRUE(result.cycle_path.empty());
    EXPECT_EQ(result.ranks.size(), 3u);

    ASSERT_TRUE(result.ranks.at(R->plugin_instance_handler).delay);
    EXPECT_EQ(*result.ranks.at(R->plugin_instance_handler).delay, 0);

    ASSERT_TRUE(result.ranks.at(X->plugin_instance_handler).delay);
    EXPECT_EQ(*result.ranks.at(X->plugin_instance_handler).delay, 100);

    ASSERT_TRUE(result.ranks.at(Y->plugin_instance_handler).delay);
    EXPECT_EQ(*result.ranks.at(Y->plugin_instance_handler).delay, 0);

    // print_graph({R, X, Y});
    // std::cerr << "--------" << std::endl;
    // show_delay(R);
    // show_delay(X);
    // show_delay(Y);
}

TEST(DelayAlgoProp, MixedEdges02) {
    auto R = mk(20), X = mk(21), Y = mk(22);
    R->resize_inputs({{VariableType::Int}, {VariableType::Int}});
    X->resize_inputs({{VariableType::Int}});
    Y->resize_inputs({{VariableType::Int}});
    ast::substitute(R, 0, X);
    ast::substitute(R, 1, Y);
    ast::substitute(X, 0, ast::SubEdge{Y, 0});
    R->input_window[0] = ast::AstNode::InputWindow{0, 0};
    R->input_window[1] = ast::AstNode::InputWindow{0, 0};
    X->input_window[0] = ast::AstNode::InputWindow{750, -750};

    auto result = scheduler::topological_sort(R);
    EXPECT_TRUE(result.cycle_path.empty());
    EXPECT_EQ(result.ranks.size(), 3u);

    // print_graph({R, X, Y});
    // std::cerr << "--------" << std::endl;
    // show_delay(R);
    // show_delay(X);
    // show_delay(Y);
}

// ---------- property test ----------
RC_GTEST_PROP(DelayAlgoProp, AllCoreProperties, ()) {
    /* 1) ランダム DAG を生成 ------------- */
    // std::size_t                                n = *rc::gen::inRange(std::size_t{4}, std::size_t{10});
    std::size_t                                n = 5;
    std::vector<std::shared_ptr<ast::AstNode>> vertex_vec;
    vertex_vec.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        vertex_vec.push_back(mk(200 + i));
        vertex_vec[i]->resize_inputs({{VariableType::Int}, {VariableType::Int}, {VariableType::Int}});
        vertex_vec[i]->resize_outputs({{VariableType::Int}});
    }
    // 方向は child(i) -> parent(j)  (i<j)
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = i + 1; j < n; ++j) {
            if (*rc::gen::arbitrary<bool>() && *rc::gen::inRange(0, 4) == 0)
                ast::substitute(vertex_vec[i], 0, ast::SubEdge{vertex_vec[j], 0});
            if (*rc::gen::arbitrary<bool>() && *rc::gen::inRange(0, 4) == 0)
                ast::substitute(vertex_vec[i], 1, ast::SubEdge{vertex_vec[j], 0});
            if (*rc::gen::arbitrary<bool>() && *rc::gen::inRange(0, 4) == 0)
                ast::substitute(vertex_vec[i], 2, ast::SubEdge{vertex_vec[j], 0});
        }

    // root wiring
    std::vector<std::vector<VariableType>> types(n - 1, {VariableType::Int});
    vertex_vec[0]->resize_inputs(types);
    for (std::size_t i = 1; i < n; ++i) ast::substitute(vertex_vec[0], i - 1, ast::SubEdge{vertex_vec[i], 0});

    /* 2) random window / weight ------------- */
    for (auto& parent : vertex_vec) {
        for (std::size_t idx = 0; idx < parent->inputs.size(); ++idx) {
            if (std::holds_alternative<ast::SubEdge>(parent->inputs[idx])
                || std::holds_alternative<std::shared_ptr<ast::AstNode>>(parent->inputs[idx])) {
                // bool has_window = *rc::gen::arbitrary<bool>();
                bool has_window = true;  // TODO: 各出力のメタデータをセットできるようになったらここを任意にする
                if (has_window) {
                    parent->input_window[idx] = ast::AstNode::InputWindow{*rc::gen::inRange<std::int64_t>(-1000, 1000),
                                                                          *rc::gen::inRange<std::int64_t>(-750, 750)};
                } else {
                    parent->input_window[idx] = std::nullopt;
                }
            }
        }
    }
    scheduler::TopoResult result = scheduler::topological_sort(vertex_vec[0]);
    RC_ASSERT(result.cycle_path.empty());

    bool has_zero_delay = false;
    for (const auto& node : vertex_vec) {
        auto rank_info = result.ranks.at(node->plugin_instance_handler);
        if (rank_info.delay) {
            if (*rank_info.delay == 0) { has_zero_delay = true; }
            RC_ASSERT(rank_info.delay >= 0);
            for (int input_index = 0; input_index < node->inputs.size(); ++input_index) {
                const auto& input_value  = node->inputs.at(input_index);
                const auto& maybe_window = node->input_window.at(input_index);
                if (auto source = std::get_if<std::shared_ptr<ast::AstNode>>(&input_value); source && *source) {
                    auto child_rank_info = result.ranks.at((*source)->plugin_instance_handler);
                    if (child_rank_info.delay && maybe_window) {
                        // if (!(maybe_window->look_ahead + *child_rank_info.delay <= *rank_info.delay)) {
                        //     std::cout << " --- 01 ---" << std::endl;
                        //     print_graph(vertex_vec);
                        //     show_delay(vertex_vec[0]);
                        //     show_delay(vertex_vec[1]);
                        //     show_delay(vertex_vec[2]);
                        //     show_delay(vertex_vec[3]);
                        //     show_delay(vertex_vec[4]);
                        //     throw std::exception{};
                        // }
                        RC_ASSERT(maybe_window->look_ahead + *child_rank_info.delay <= *rank_info.delay);
                    }
                } else if (auto sub_edge = std::get_if<ast::SubEdge>(&input_value)) {
                    if (auto source = sub_edge->source.lock()) {
                        auto child_rank_info = result.ranks.at(source->plugin_instance_handler);
                        if (child_rank_info.delay && maybe_window) {
                            // if (!(maybe_window->look_ahead + *child_rank_info.delay <= *rank_info.delay)) {
                            //     std::cout << " --- 02 ---" << std::endl;
                            //     print_graph(vertex_vec);
                            //     show_delay(vertex_vec[0]);
                            //     show_delay(vertex_vec[1]);
                            //     show_delay(vertex_vec[2]);
                            //     show_delay(vertex_vec[3]);
                            //     show_delay(vertex_vec[4]);
                            //     throw std::exception{};
                            // }
                            RC_ASSERT(maybe_window->look_ahead + *child_rank_info.delay <= *rank_info.delay);
                        }
                    }
                }
            }
        }
    }
    RC_ASSERT(has_zero_delay);
}

// /* ── tests/20_test_schedule.cpp ────────────────────────────────
//    unit + property tests for 2nd-layer (delay, split_dag) and
//    run-time lifetime判定ロジック

//    AST 構築はすべて operation 層の substitute() を経由。
//    ─────────────────────────────────────────────────────────── */
// #include <gtest/gtest.h>
// #include <rapidcheck/gtest.h>

// #include <prismcascade/ast/operations.hpp>  // substitute
// #include <prismcascade/memory/types_internal.hpp>
// #include <prismcascade/scheduler/delay.hpp>
// #include <prismcascade/scheduler/split_dag.hpp>
// #include <prismcascade/scheduler/topological_sort.hpp>

// using namespace prismcascade;

// /* ────────── ヘルパ: ノード生成 ────────── */
// static std::shared_ptr<ast::AstNode> mk(int64_t id, uint32_t lookAhead = 0) {
//     auto n                     = std::make_shared<ast::AstNode>();
//     n->plugin_instance_handler = id;

//     if (lookAhead) { /* lookAhead を出力スロット0ベクタ長で模擬 */
//         auto                                   pmem = std::make_shared<memory::ParameterPackMemory>();
//         std::vector<std::vector<VariableType>> types(1, std::vector<VariableType>(lookAhead, VariableType::Int));
//         pmem->update_types(types);
//         n->output_parameters = pmem;
//     }
//     return n;
// }

// /* ========================================================== */
// /* 1. DelayTable: 最大 lookAhead 伝搬                          */
// /* ========================================================== */
// TEST(DelayTable, PropagatesMaxLookAhead) {
//     auto A = mk(1, 3);
//     auto B = mk(2, 2);
//     auto C = mk(3, 0);

//     A->inputs.resize(1, std::monostate{});
//     B->inputs.resize(1, std::monostate{});

//     ast::substitute(A, 0, B);  // A[0] = B
//     ast::substitute(B, 0, C);  // B[0] = C

//     auto tbl = scheduler::build_delay_table(A);
//     EXPECT_EQ(tbl[A->plugin_instance_handler], 3u);
//     EXPECT_EQ(tbl[B->plugin_instance_handler], 5u);
//     EXPECT_EQ(tbl[C->plugin_instance_handler], 5u);
// }

// /* ========================================================== */
// /* 2. SplitDAG: queue_id 単調性 (property)                     */
// /* ========================================================== */
// RC_GTEST_PROP(SplitDAGProp, QueueIdsMonotone, ()) {
//     const int                                  len = *rc::gen::inRange(4, 9);  // 鎖長 4-8
//     std::vector<std::shared_ptr<ast::AstNode>> v;
//     int64_t                                    id = 50;

//     for (int i = 0; i < len; ++i) v.push_back(mk(id++, *rc::gen::inRange(0, 5)));

//     for (int i = 0; i < len - 1; ++i) {
//         v[i]->inputs.resize(1, std::monostate{});
//         ast::substitute(v[i], 0, v[i + 1]);
//     }

//     auto delay = scheduler::build_delay_table(v[0]);
//     auto subs  = scheduler::split_dag(v[0], delay, 4);

//     std::unordered_map<int64_t, uint32_t> qid;
//     for (auto& s : subs) qid[s.root->plugin_instance_handler] = s.queue_id;

//     for (int i = 0; i < len - 1; ++i)
//         RC_ASSERT(qid[v[i]->plugin_instance_handler] <= qid[v[i + 1]->plugin_instance_handler]);
// }

// /* ========================================================== */
// /* 3. Lifetime判定: Stamp+Rank 条件で必ずfree可能              */
// /*    (Δmap を使わないランタイム方針の検証)                    */
// /* ========================================================== */
// TEST(Lifetime, FreeWhenStampAndRankExceeded) {
//     auto P  = mk(100);
//     auto C1 = mk(101);
//     auto C2 = mk(102);

//     P->inputs.resize(2, std::monostate{});
//     C1->inputs.resize(1, std::monostate{});
//     C2->inputs.resize(1, std::monostate{});

//     ast::substitute(P, 0, C1);
//     ast::substitute(P, 1, C2);

//     auto topo = scheduler::topological_sort(P);
//     auto rp   = topo.ranks[P->plugin_instance_handler];
//     auto r1   = topo.ranks[C1->plugin_instance_handler];

//     /* シミュレーション: now_stamp/rank を十分先へ進める */
//     scheduler::timestamp_t farStamp = r1.pre_ts + 100;
//     std::uint32_t          farRank  = rp.last_consumer_rank + 10;

//     bool canFree = (farStamp > r1.pre_ts) && (farRank > r1.last_consumer_rank);

//     EXPECT_TRUE(canFree);  // 条件を満たす限り必ずfree可
// }

// /* ========================================================== */
// /* 4. RetainPeak vs naive シミュレーション (property)         */
// /* ========================================================== */
// RC_GTEST_PROP(PeakMatchesSimulation, RankTimeLifetime, ()) {
//     /* 鎖長 5-10, lookAhead 全 0 */
//     int                                        len = *rc::gen::inRange(5, 11);
//     std::vector<std::shared_ptr<ast::AstNode>> v;
//     int64_t                                    id = 200;
//     for (int i = 0; i < len; ++i) v.push_back(mk(id++));

//     for (int i = 0; i < len - 1; ++i) {
//         v[i]->inputs.resize(1, std::monostate{});
//         ast::substitute(v[i], 0, v[i + 1]);
//     }

//     auto topo = scheduler::topological_sort(v[0]);

//     /* naive retain count: rank 区間 [r.rank, r.last_consumer_rank] */
//     std::vector<int> retain(len + 5, 0);
//     for (auto& kv : topo.ranks) {
//         auto r = kv.second;
//         for (int i = r.rank; i <= r.last_consumer_rank; ++i) ++retain[i];
//     }

//     int peak = *std::max_element(retain.begin(), retain.end());
//     RC_ASSERT(peak >= 1);  // sanity: 必ず書込あり
// }
