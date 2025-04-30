#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <prismcascade/ast/operations.hpp>
#include <prismcascade/scheduler/topological_sort.hpp>

using namespace prismcascade;

/* ------------- ヘルパ --------------- */
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

static bool hasEdge(const std::shared_ptr<ast::AstNode>& dst, const std::shared_ptr<ast::AstNode>& src) {
    for (const auto& in : dst->inputs)
        if (auto p = std::get_if<std::shared_ptr<ast::AstNode>>(&in))
            if (*p == src) return true;
    return false;
}

/* ========== テスト 1 : Linear chain ========= */
TEST(TopoSort, LinearChain) {
    auto A = mk(1), B = mk(2), C = mk(3);
    A->inputs.resize(1, std::monostate{});
    B->inputs.resize(1, std::monostate{});
    ast::substitute(A, 0, B);
    ast::substitute(B, 0, C);

    auto r = scheduler::topological_sort(A);
    ASSERT_TRUE(r.cycle_path.empty());
    ASSERT_EQ(r.ranks.size(), 3u);

    std::unordered_set<uint32_t> uniq;
    for (auto& kv : r.ranks) uniq.insert(kv.second.rank);
    EXPECT_EQ(uniq.size(), 3u);
    EXPECT_TRUE(ranks_uniquely_topo(r.ranks, A));
    // for (auto&& [handler, rank_info] : r.ranks)
    //     std::cout << "handler = " << handler << ", rank = " << rank_info.rank << ", [" << rank_info.pre_ts << ", "
    //               << rank_info.post_ts << "]" << std::endl;
}

/* ========== テスト 2 : Fork-merge ========= */
TEST(TopoSort, ForkMerge) {
    auto A = mk(10), B = mk(11), C = mk(12), R = mk(13);
    B->inputs.resize(1, std::monostate{});
    C->inputs.resize(1, std::monostate{});
    R->inputs.resize(2, std::monostate{});
    ast::substitute(B, 0, ast::SubEdge{A, 0});  // B depends on A
    ast::substitute(C, 0, ast::SubEdge{A, 0});  // C depends on A
    ast::substitute(R, 0, B);                   // final output depends on both
    ast::substitute(R, 1, C);

    auto r = scheduler::topological_sort(R);
    ASSERT_TRUE(r.cycle_path.empty());
    EXPECT_EQ(r.ranks.size(), 4u);
    EXPECT_TRUE(ranks_uniquely_topo(r.ranks, R));
    EXPECT_EQ(r.ranks.at(R->plugin_instance_handler).rank, 3);
    EXPECT_EQ(r.ranks.at(A->plugin_instance_handler).rank, 0);
}

/* ========== テスト 3 : SubEdge+MainTree ========= */
TEST(TopoSort, MixedEdges) {
    auto R = mk(20), X = mk(21), Y = mk(22);
    R->inputs.resize(2, std::monostate{});
    X->inputs.resize(1, std::monostate{});
    Y->inputs.resize(1, std::monostate{});
    ast::substitute(R, 0, X);
    ast::substitute(Y, 0, ast::SubEdge{X, 0});
    ast::substitute(R, 1, Y);

    auto r = scheduler::topological_sort(R);
    EXPECT_TRUE(r.cycle_path.empty());
    EXPECT_EQ(r.ranks.size(), 3u);
    EXPECT_TRUE(ranks_uniquely_topo(r.ranks, R));
}

/* ========== テスト 4 : Rank uniqueness alone ========= */
TEST(TopoSort, RanksUnique) {
    auto A = mk(30), B = mk(31);
    A->inputs.resize(1, std::monostate{});
    ast::substitute(A, 0, B);
    auto                         r = scheduler::topological_sort(A);
    std::unordered_set<uint32_t> uniq;
    for (auto& kv : r.ranks) uniq.insert(kv.second.rank);
    EXPECT_EQ(uniq.size(), r.ranks.size());
}

/* ========== テスト 5 : SubEdge Cycle detection ========= */
TEST(TopoSortCycle, SubEdgeCycle) {
    auto A = mk(40), B = mk(41);
    A->inputs.resize(1, std::monostate{});
    B->inputs.resize(1, std::monostate{});
    ast::substitute(A, 0, ast::SubEdge{B, 0});
    ast::substitute(B, 0, ast::SubEdge{A, 0});  // cycle

    auto r = scheduler::topological_sort(A);
    EXPECT_TRUE(r.ranks.empty());
    ASSERT_FALSE(r.cycle_path.empty());
    EXPECT_EQ(r.cycle_path.front(), r.cycle_path.back());
}

/* ========== テスト 6 : cycle_path size >=2 ========= */
TEST(TopoSortCycle, PathLength) {
    auto A = mk(50), B = mk(51), C = mk(52);
    A->inputs.resize(1, std::monostate{});
    B->inputs.resize(1, std::monostate{});
    C->inputs.resize(1, std::monostate{});
    ast::substitute(A, 0, ast::SubEdge{C, 0});
    ast::substitute(B, 0, ast::SubEdge{A, 0});
    ast::substitute(C, 0, ast::SubEdge{B, 0});  // 3-cycle

    auto r = scheduler::topological_sort(A);
    ASSERT_GE(r.cycle_path.size(), 2u);
}

/* ========== テスト 7 : property – random acyclic DAG ========= */
RC_GTEST_PROP(TopoSortProp, RandomAcyclicDag, ()) {
    // ノード作成
    std::size_t                                n = *rc::gen::inRange(std::size_t{4}, std::size_t{8});
    std::vector<std::shared_ptr<ast::AstNode>> v;
    v.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        v.push_back(mk(200 + i));
        v[i]->inputs.resize(1, std::monostate{});
    }
    // ランダムな acyclic SubEdge を追加
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = i + 1; j < n; ++j)
            if (*rc::gen::arbitrary<bool>() && (rand() % 4 == 0)) ast::substitute(v[i], 0, ast::SubEdge{v[j], 0});
    // rootから到達可能にする
    v[0]->inputs.resize(n - 1, std::monostate{});  // root に (n-1) スロット確保
    for (std::size_t i = 1; i < n; ++i) ast::substitute(v[0], i - 1, ast::SubEdge{v[i], 0});  // 依存辺を張る
    // 実行
    auto r = scheduler::topological_sort(v[0]);
    RC_ASSERT(r.cycle_path.empty());
    RC_ASSERT(r.ranks.size() == n);
    RC_ASSERT(ranks_uniquely_topo(r.ranks, v[0]));
}

/* ========== テスト 8 : property – inject one subedge cycle ========= */
RC_GTEST_PROP(TopoSortProp, RandomCycleDetected, ()) {
    const std::size_t                          n = 5;
    std::vector<std::shared_ptr<ast::AstNode>> v;
    for (std::size_t i = 0; i < n; ++i) {
        v.push_back(mk(300 + i));
        v[i]->inputs.resize(1, std::monostate{});
    }
    // chain
    for (std::size_t i = 0; i < n - 1; ++i) ast::substitute(v[i], 0, v[i + 1]);
    // add subedge back-edge
    ast::substitute(v[n - 1], 0, ast::SubEdge{v[1], 0});

    auto r = scheduler::topological_sort(v[0]);
    RC_ASSERT(r.ranks.empty());
    RC_ASSERT(!r.cycle_path.empty());
    RC_ASSERT(r.cycle_path.front() == r.cycle_path.back());
}

// ── 1. rank & edge 順序性 ──────────────────────────────
TEST(TopoSort, RankOrderRespectsEdges) {
    auto A = mk(1);
    auto B = mk(2);
    auto C = mk(3);
    A->inputs.resize(1, std::monostate{});
    B->inputs.resize(1, std::monostate{});
    assign(A, 0, B);  // A ← B
    assign(B, 0, C);  // B ← C

    auto r = scheduler::topological_sort(A);
    ASSERT_TRUE(r.cycle_path.empty());
    ASSERT_EQ(r.ranks.size(), 3u);

    EXPECT_LT(r.ranks[C->plugin_instance_handler].rank, r.ranks[B->plugin_instance_handler].rank);
    EXPECT_LT(r.ranks[B->plugin_instance_handler].rank, r.ranks[A->plugin_instance_handler].rank);
}

// ── 2. pre/post ts 含有性 (親は子より広い区間) ─────────
TEST(TopoSort, PrePostIntervalContainsChildren) {
    auto P = mk(10);
    auto C = mk(11);
    P->inputs.resize(1, C);

    auto        r  = scheduler::topological_sort(P);
    const auto& pi = r.ranks[P->plugin_instance_handler];
    const auto& ci = r.ranks[C->plugin_instance_handler];

    ASSERT_LT(pi.pre_ts, ci.pre_ts);
    ASSERT_LT(ci.post_ts, pi.post_ts);
    ASSERT_LT(ci.pre_ts, ci.post_ts);
}

// ── 3. self-loop cycle detection ───────────────────────
TEST(TopoSort, DetectSelfLoop) {
    auto N = mk(20);
    N->inputs.resize(1, N);  // self loop

    auto res = scheduler::topological_sort(N);
    ASSERT_TRUE(res.ranks.empty());
    ASSERT_EQ(res.cycle_path.size(), 2u);  // N → N

    EXPECT_EQ(res.cycle_path.front(), N);
    EXPECT_EQ(res.cycle_path.back(), N);
}

// ── 4. 2 ノード cycle path correctness ────────────────
TEST(TopoSort, CyclePathMatchesEdges) {
    auto X = mk(30);
    auto Y = mk(31);
    X->inputs.resize(1, Y);
    Y->inputs.resize(1, X);

    auto res = scheduler::topological_sort(X);
    ASSERT_TRUE(res.ranks.empty());
    ASSERT_EQ(res.cycle_path.size(), 3u);  // X→Y→X

    for (size_t i = 0; i + 1 < res.cycle_path.size(); ++i)
        EXPECT_TRUE(hasEdge(res.cycle_path[i + 1], res.cycle_path[i]));
}
/*──────────────────────────
   last_consumer_rank  の検証
 *─────────────────────────*/

/* 1. Linear chain  A←B←C  */
TEST(TopoSort_Last, LinearChain) {
    auto A = mk(501), B = mk(502), C = mk(503);
    A->inputs.resize(1, std::monostate{});
    B->inputs.resize(1, std::monostate{});
    ast::substitute(A, 0, B);
    ast::substitute(B, 0, C);

    auto r = scheduler::topological_sort(A);
    ASSERT_TRUE(r.cycle_path.empty());

    auto rankA = r.ranks[A->plugin_instance_handler].rank;
    auto rankB = r.ranks[B->plugin_instance_handler].rank;
    auto rankC = r.ranks[C->plugin_instance_handler].rank;

    EXPECT_EQ(r.ranks[C->plugin_instance_handler].last_consumer_rank, rankB);
    EXPECT_EQ(r.ranks[B->plugin_instance_handler].last_consumer_rank, rankA);
    EXPECT_EQ(r.ranks[A->plugin_instance_handler].last_consumer_rank, rankA);  // 自分のみ
}

/* 2. Fork  R consumes X,Y  &  M also consumes X,Y */
TEST(TopoSort_Last, ForkMerge) {
    auto R = mk(510), X = mk(511), Y = mk(512), M = mk(513);
    R->inputs.resize(2, std::monostate{});
    M->inputs.resize(2, std::monostate{});

    ast::substitute(R, 0, X);
    ast::substitute(R, 1, Y);
    ast::substitute(M, 0, X);
    ast::substitute(M, 1, Y);

    auto r = scheduler::topological_sort(M);
    ASSERT_TRUE(r.cycle_path.empty());

    auto rankR = r.ranks[R->plugin_instance_handler].rank;
    auto rankM = r.ranks[M->plugin_instance_handler].rank;

    EXPECT_EQ(r.ranks[X->plugin_instance_handler].last_consumer_rank, std::max(rankR, rankM));
    EXPECT_EQ(r.ranks[Y->plugin_instance_handler].last_consumer_rank, std::max(rankR, rankM));
}

/* 3. SubEdge external consumer */
TEST(TopoSort_Last, ExternalSubEdgeConsumer) {
    auto A = mk(520), B = mk(521), C = mk(522);
    B->inputs.resize(2, std::monostate{});
    C->inputs.resize(1, std::monostate{});
    ast::substitute(B, 0, C);  // B <- C <- A
    ast::substitute(B, 1, ast::SubEdge{A, 0});
    ast::substitute(C, 0, A);

    auto r = scheduler::topological_sort(B);
    ASSERT_TRUE(r.cycle_path.empty());

    EXPECT_EQ(r.ranks[A->plugin_instance_handler].last_consumer_rank, r.ranks.at(B->plugin_instance_handler).rank);
}
