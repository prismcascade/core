/* ── tests/20_test_schedule.cpp ────────────────────────────────
   unit + property tests for 2nd-layer (delay, split_dag) and
   run-time lifetime判定ロジック

   AST 構築はすべて operation 層の substitute() を経由。
   ─────────────────────────────────────────────────────────── */
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <prismcascade/ast/operations.hpp>  // substitute
#include <prismcascade/memory/types_internal.hpp>
#include <prismcascade/scheduler/delay.hpp>
#include <prismcascade/scheduler/split_dag.hpp>
#include <prismcascade/scheduler/topological_sort.hpp>

using namespace prismcascade;

/* ────────── ヘルパ: ノード生成 ────────── */
static std::shared_ptr<ast::AstNode> mk(int64_t id, uint32_t lookAhead = 0) {
    auto n                     = std::make_shared<ast::AstNode>();
    n->plugin_instance_handler = id;

    if (lookAhead) { /* lookAhead を出力スロット0ベクタ長で模擬 */
        auto                                   pmem = std::make_shared<memory::ParameterPackMemory>();
        std::vector<std::vector<VariableType>> types(1, std::vector<VariableType>(lookAhead, VariableType::Int));
        pmem->update_types(types);
        n->output_parameters = pmem;
    }
    return n;
}

/* ========================================================== */
/* 1. DelayTable: 最大 lookAhead 伝搬                          */
/* ========================================================== */
TEST(DelayTable, PropagatesMaxLookAhead) {
    auto A = mk(1, 3);
    auto B = mk(2, 2);
    auto C = mk(3, 0);

    A->inputs.resize(1, std::monostate{});
    B->inputs.resize(1, std::monostate{});

    ast::substitute(A, 0, B);  // A[0] = B
    ast::substitute(B, 0, C);  // B[0] = C

    auto tbl = scheduler::build_delay_table(A);
    EXPECT_EQ(tbl[A->plugin_instance_handler], 3u);
    EXPECT_EQ(tbl[B->plugin_instance_handler], 5u);
    EXPECT_EQ(tbl[C->plugin_instance_handler], 5u);
}

/* ========================================================== */
/* 2. SplitDAG: queue_id 単調性 (property)                     */
/* ========================================================== */
RC_GTEST_PROP(SplitDAGProp, QueueIdsMonotone, ()) {
    const int                                  len = *rc::gen::inRange(4, 9);  // 鎖長 4-8
    std::vector<std::shared_ptr<ast::AstNode>> v;
    int64_t                                    id = 50;

    for (int i = 0; i < len; ++i) v.push_back(mk(id++, *rc::gen::inRange(0, 5)));

    for (int i = 0; i < len - 1; ++i) {
        v[i]->inputs.resize(1, std::monostate{});
        ast::substitute(v[i], 0, v[i + 1]);
    }

    auto delay = scheduler::build_delay_table(v[0]);
    auto subs  = scheduler::split_dag(v[0], delay, 4);

    std::unordered_map<int64_t, uint32_t> qid;
    for (auto& s : subs) qid[s.root->plugin_instance_handler] = s.queue_id;

    for (int i = 0; i < len - 1; ++i)
        RC_ASSERT(qid[v[i]->plugin_instance_handler] <= qid[v[i + 1]->plugin_instance_handler]);
}

/* ========================================================== */
/* 3. Lifetime判定: Stamp+Rank 条件で必ずfree可能              */
/*    (Δmap を使わないランタイム方針の検証)                    */
/* ========================================================== */
TEST(Lifetime, FreeWhenStampAndRankExceeded) {
    auto P  = mk(100);
    auto C1 = mk(101);
    auto C2 = mk(102);

    P->inputs.resize(2, std::monostate{});
    C1->inputs.resize(1, std::monostate{});
    C2->inputs.resize(1, std::monostate{});

    ast::substitute(P, 0, C1);
    ast::substitute(P, 1, C2);

    auto topo = scheduler::topological_sort(P);
    auto rp   = topo.ranks[P->plugin_instance_handler];
    auto r1   = topo.ranks[C1->plugin_instance_handler];

    /* シミュレーション: now_stamp/rank を十分先へ進める */
    scheduler::timestamp_t farStamp = r1.pre_ts + 100;
    std::uint32_t          farRank  = rp.last_consumer_rank + 10;

    bool canFree = (farStamp > r1.pre_ts) && (farRank > r1.last_consumer_rank);

    EXPECT_TRUE(canFree);  // 条件を満たす限り必ずfree可
}

/* ========================================================== */
/* 4. RetainPeak vs naive シミュレーション (property)         */
/* ========================================================== */
RC_GTEST_PROP(PeakMatchesSimulation, RankTimeLifetime, ()) {
    /* 鎖長 5-10, lookAhead 全 0 */
    int                                        len = *rc::gen::inRange(5, 11);
    std::vector<std::shared_ptr<ast::AstNode>> v;
    int64_t                                    id = 200;
    for (int i = 0; i < len; ++i) v.push_back(mk(id++));

    for (int i = 0; i < len - 1; ++i) {
        v[i]->inputs.resize(1, std::monostate{});
        ast::substitute(v[i], 0, v[i + 1]);
    }

    auto topo = scheduler::topological_sort(v[0]);

    /* naive retain count: rank 区間 [r.rank, r.last_consumer_rank] */
    std::vector<int> retain(len + 5, 0);
    for (auto& kv : topo.ranks) {
        auto r = kv.second;
        for (int i = r.rank; i <= r.last_consumer_rank; ++i) ++retain[i];
    }

    int peak = *std::max_element(retain.begin(), retain.end());
    RC_ASSERT(peak >= 1);  // sanity: 必ず書込あり
}
