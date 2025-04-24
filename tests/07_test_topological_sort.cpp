#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <prismcascade/ast/operations.hpp>
#include <prismcascade/scheduler/topological_sort.hpp>

using namespace prismcascade;

/* ------------- ヘルパ --------------- */
static std::shared_ptr<ast::AstNode> mk(uint64_t id) { return ast::make_node("dummy", id, id); }

static bool ranks_uniquely_topo(const std::unordered_map<std::int64_t, schedule::RankInfo>& tbl,
                                const std::shared_ptr<ast::AstNode>&                        n) {
    if (!n) return true;
    uint32_t rk = tbl.at(n->plugin_instance_handler).rank;
    for (const auto& in : n->inputs) {
        if (auto p = std::get_if<std::shared_ptr<ast::AstNode>>(&in); p && *p) {
            if (!(rk < tbl.at((*p)->plugin_instance_handler).rank)) return false;
            if (!ranks_uniquely_topo(tbl, *p)) return false;
        } else if (auto se = std::get_if<ast::SubEdge>(&in)) {
            if (auto s = se->source.lock()) {
                if (!(rk < tbl.at(s->plugin_instance_handler).rank)) return false;
                if (!ranks_uniquely_topo(tbl, s)) return false;
            }
        }
    }
    return true;
}

/* ========== テスト 1 : Linear chain ========= */
TEST(TopoSort, LinearChain) {
    auto A = mk(1), B = mk(2), C = mk(3);
    A->inputs.resize(1, std::monostate{});
    B->inputs.resize(1, std::monostate{});
    ast::substitute(A, 0, B);
    ast::substitute(B, 0, C);

    auto r = schedule::topological_sort(A);
    ASSERT_TRUE(r.cycle_path.empty());
    ASSERT_EQ(r.ranks.size(), 3u);

    std::unordered_set<uint32_t> uniq;
    for (auto& kv : r.ranks) uniq.insert(kv.second.rank);
    EXPECT_EQ(uniq.size(), 3u);
    EXPECT_TRUE(ranks_uniquely_topo(r.ranks, A));
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

    auto r = schedule::topological_sort(R);
    ASSERT_TRUE(r.cycle_path.empty());
    EXPECT_EQ(r.ranks.size(), 4u);
    EXPECT_TRUE(ranks_uniquely_topo(r.ranks, A));
}

/* ========== テスト 3 : SubEdge+MainTree ========= */
TEST(TopoSort, MixedEdges) {
    auto R = mk(20), X = mk(21), Y = mk(22);
    R->inputs.resize(2, std::monostate{});
    X->inputs.resize(1, std::monostate{});
    Y->inputs.resize(1, std::monostate{});
    ast::substitute(R, 0, X);
    ast::substitute(Y, 0, ast::SubEdge{X, 0});
    ast::substitute(R, 0, X);
    ast::substitute(R, 1, Y);

    auto r = schedule::topological_sort(R);
    EXPECT_TRUE(r.cycle_path.empty());
    EXPECT_EQ(r.ranks.size(), 3u);
    EXPECT_TRUE(ranks_uniquely_topo(r.ranks, R));
}

/* ========== テスト 4 : Rank uniqueness alone ========= */
TEST(TopoSort, RanksUnique) {
    auto A = mk(30), B = mk(31);
    A->inputs.resize(1, std::monostate{});
    ast::substitute(A, 0, B);
    auto                         r = schedule::topological_sort(A);
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

    auto r = schedule::topological_sort(A);
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

    auto r = schedule::topological_sort(A);
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
    auto r = schedule::topological_sort(v[0]);
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

    auto r = schedule::topological_sort(v[0]);
    RC_ASSERT(r.ranks.empty());
    RC_ASSERT(!r.cycle_path.empty());
    RC_ASSERT(r.cycle_path.front() == r.cycle_path.back());
}
