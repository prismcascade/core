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

// ---------- property test ----------
RC_GTEST_PROP(DelayAlgoProp, AllCoreProperties, ()) {
    /* 1) ランダム DAG を生成 ------------- */
    std::size_t                                n = *rc::gen::inRange(std::size_t{4}, std::size_t{10});
    std::vector<std::shared_ptr<ast::AstNode>> v;
    v.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        v.push_back(mk(200 + i));
        v[i]->resize_inputs({{VariableType::Int}, {VariableType::Int}});
        v[i]->resize_outputs({{VariableType::Int}});
    }
    // 方向は child(i) -> parent(j)  (i<j)
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = i + 1; j < n; ++j) {
            if (*rc::gen::arbitrary<bool>() && *rc::gen::inRange(0, 4) == 0)
                ast::substitute(v[i], 0, ast::SubEdge{v[j], 0});
            if (*rc::gen::arbitrary<bool>() && *rc::gen::inRange(0, 4) == 0)
                ast::substitute(v[i], 1, ast::SubEdge{v[j], 0});
        }

    // root wiring
    std::vector<std::vector<VariableType>> types(n - 1, {VariableType::Int});
    v[0]->resize_inputs(types);
    for (std::size_t i = 1; i < n; ++i) ast::substitute(v[0], i - 1, ast::SubEdge{v[i], 0});

    /* 2) random window / weight ------------- */
    for (auto& parent : v) {
        for (std::size_t idx = 0; idx < parent->inputs.size(); ++idx) {
            if (auto se = std::get_if<ast::SubEdge>(&parent->inputs[idx])) {
                bool has_window = *rc::gen::arbitrary<bool>();
                if (has_window) {
                    parent->input_window[idx] = ast::AstNode::InputWindow{*rc::gen::inRange<std::int64_t>(-1000, 1000)};
                } else {
                    parent->input_window[idx] = std::nullopt;
                }
            }
        }
    }

    // /* 3) topo sort & our algorithm ---------- */
    // auto topoRes = scheduler::topological_sort(v[0]);

    // RC_PRE(topoRes.cycle_path.empty());             // guard (should always hold)
    // auto topo = topoRes.ranksInTopologicalOrder();  // 仮にこう取れるとする

    // auto edges = collectEdges(v);

    // /* P1: 可行性 --------------------------- */
    // for (auto& e : edges) {
    //     if (!e.has_window) continue;
    //     auto d_child  = info.at(e.child->plugin_instance_handler).delay;
    //     auto d_parent = info.at(e.parent->plugin_instance_handler).delay;
    //     RC_ASSERT(d_parent >= d_child + e.w);
    // }

    // /* P3: min==0 --------------------------- */
    // std::int64_t mn = LLONG_MAX;
    // for (auto& node : v) {
    //     auto d = info.at(node->plugin_instance_handler).delay;
    //     if (d) mn = std::min(mn, *d);
    // }
    // RC_ASSERT(mn == 0);

    // /* P2/P4: tightness or minimality ------- */
    // for (auto& n : v) {
    //     auto& dOpt = info.at(n->plugin_instance_handler).delay;
    //     if (!dOpt) continue;  // unconstrained subtree
    //     auto orig = *dOpt;
    //     --(*dOpt);  // 1 減らす
    //     bool feasible = true;
    //     for (auto& e : edges) {
    //         if (!e.has_window) continue;
    //         auto d_c = info.at(e.child->plugin_instance_handler).delay;
    //         auto d_p = info.at(e.parent->plugin_instance_handler).delay;
    //         if (!(d_p >= d_c + e.w)) {
    //             feasible = false;
    //             break;
    //         }
    //     }
    //     *dOpt = orig;          // 戻す
    //     RC_ASSERT(!feasible);  // 1 減らすと破綻 ⇒ minimal
    // }
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
