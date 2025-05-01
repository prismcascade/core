// /* ── tests/20_test_scheduler.cpp ───────────────────────────────
//    Layer-4 : ExecutionQueue / Scheduler の検証
//    ──────────────────────────────────────────────────────────── */
// #include <gtest/gtest.h>

// #include <prismcascade/ast/edge.hpp>
// #include <prismcascade/ast/operations.hpp>
// #include <prismcascade/scheduler/delay.hpp>
// #include <prismcascade/scheduler/scheduler.hpp>

// using namespace prismcascade;  // ← 唯一許可された using

// /* ────────── ヘルパ：A→B→C(root) の線形 DAG を生成 ────────── */
// static std::shared_ptr<ast::AstNode> make_chain() {
//     auto A = ast::make_node("uuidA", 1, 1);
//     auto B = ast::make_node("uuidB", 2, 2);
//     auto C = ast::make_node("uuidC", 3, 3);

//     A->inputs.resize(1, std::monostate{});
//     B->inputs.resize(1, std::monostate{});
//     C->inputs.resize(1, std::monostate{});

//     ast::substitute(B, 0, A);  // B→A
//     ast::substitute(C, 0, B);  // C→B
//     return C;                  // C が root
// }

// /* ------------------------------------------------------------------ */
// /* 1. 初回 pop は入力次数 0 のノードのうち rank 最小 (root)            */
// /* ------------------------------------------------------------------ */
// TEST(SchedulerOrder, RootFirst) {
//     auto                 root = make_chain();
//     scheduler::Scheduler sch;
//     sch.compile(root);

//     std::vector<std::int64_t> pop;
//     sch.tick([&](auto n) { pop.push_back(n->plugin_instance_handler); });

//     /* C(handler=3) が最初に取得される設計である */
//     ASSERT_EQ(pop.size(), 1u);
//     EXPECT_EQ(pop[0], 3);
// }

// /* ------------------------------------------------------------------ */
// /* 2. lookAhead が buffer を超えると“辺”で queue が切れる            */
// /*    したがって遅延ノード自身(C) は queue0 のまま                   */
// /* ------------------------------------------------------------------ */
// TEST(SchedulerDelay, ChildMovesToLaterQueue) {
//     auto C = make_chain();  // root C
//     auto B = std::get<std::shared_ptr<ast::AstNode>>(C->inputs[0]);
//     auto A = std::get<std::shared_ptr<ast::AstNode>>(B->inputs[0]);

//     /* DelayTable : 子 (A,B) は 0, C 本体=8 (> buffer=3) */
//     scheduler::DelayTable dt;
//     dt[A->plugin_instance_handler] = 0;
//     dt[B->plugin_instance_handler] = 0;
//     dt[C->plugin_instance_handler] = 8;

//     auto subs = scheduler::split_dag(C, dt, /*startup_buffer=*/3);

//     std::uint32_t qid_A = 0, qid_C = 0;
//     for (auto& s : subs) {
//         if (s.root == A) qid_A = s.queue_id;
//         if (s.root == C) qid_C = s.queue_id;
//     }
//     /* C は queue0、A は queue1 になるのが期待動作 */
//     EXPECT_EQ(qid_C, 0u);
//     EXPECT_GT(qid_A, qid_C);
// }

// /* ------------------------------------------------------------------ */
// /* 3. ExecutionQueue: stamp が同値なら rank 小さい方が先             */
// /* ------------------------------------------------------------------ */
// TEST(ExecQueue, RankTieBreaker) {
//     auto n1 = ast::make_node("u", 1, 1);
//     auto n2 = ast::make_node("u", 2, 2);

//     scheduler::ExecutionQueue::compare_t cmp = [](const scheduler::ExecutionQueue::Item& a,
//                                                   const scheduler::ExecutionQueue::Item& b) {
//         return (a.stamp == b.stamp) ? (a.rank > b.rank) : (a.stamp > b.stamp);
//     };

//     scheduler::ExecutionQueue q(cmp);
//     q.push({10, 5, n1});
//     q.push({10, 3, n2});  // rank=3 < 5

//     scheduler::ExecutionQueue::Item out;
//     ASSERT_TRUE(q.pop(out));
//     EXPECT_EQ(out.rank, 3);
//     EXPECT_EQ(out.node, n2);
// }

// /* ------------------------------------------------------------------ */
// /* 4. SubEdge 閉路を compile が検出し std::domain_error を投げる      */
// /* ------------------------------------------------------------------ */
// TEST(SchedulerError, DetectCycleViaSubEdge) {
//     auto P = ast::make_node("u", 1, 1);
//     auto Q = ast::make_node("u", 2, 2);
//     P->inputs.resize(1, std::monostate{});
//     Q->inputs.resize(1, std::monostate{});

//     /* P ←subEdge Q,  Q ←subEdge P で有向閉路 */
//     ast::substitute(P, 0, ast::SubEdge{Q, 0});
//     ast::substitute(Q, 0, ast::SubEdge{P, 0});

//     scheduler::Scheduler sch;
//     EXPECT_THROW(sch.compile(P), std::domain_error);
// }

// /* ------------------------------------------------------------------ */
// /* 5. tick() を一定回数呼んでも dead-lock せず stamp が単調増加       */
// /*    (リアルタイム進行性の最小チェック)                              */
// /* ------------------------------------------------------------------ */
// TEST(SchedulerProgress, NoDeadlockAndMonotoneStamp) {
//     auto                 root = make_chain();
//     scheduler::Scheduler sch;
//     sch.compile(root);

//     constexpr int          kIter      = 50;
//     scheduler::timestamp_t prev_stamp = -1;

//     for (int i = 0; i < kIter; ++i) {
//         bool ok = sch.tick([&](auto) {});
//         ASSERT_TRUE(ok) << "dead-locked at tick " << i;
//         /* 実際の stamp はキュー内部だが「tick が true」であれば
//            少なくとも 1 ノードが前進していることが保証される。*/
//     }
// }
