// // tests/integration/test_render_pipeline.cpp
// //
// // フルパイプライン結合テスト
// //  ─ Twice → Sum → Count という 3 段 AST を構築
// //  ─ RenderingScheduler::topological_sort で
// //      ▸ ノードが 3 件並ぶこと
// //      ▸ cycles が空であること
// //  ─ PluginManager で start → frame → finish を一巡
// //
// // ※ invoke_* がまだスタブ実装の場合は true を返す想定
// //
// #include <gtest/gtest.h>

// #include <prismcascade/plugin/plugin_manager.hpp>
// #include <prismcascade/scheduler/rendering_scheduler.hpp>

// using namespace prismcascade;
// namespace sched = prismcascade::scheduler;
// using plugin::PluginManager;

// TEST(RenderPipeline, TwiceSumCount) {
//     PluginManager     pm;
//     const std::string twice_uuid = "f0000000-0000-0000-0000-000000000000";
//     const std::string sum_uuid   = "f0000000-0000-0000-0000-000000000002";
//     const std::string count_uuid = "f0000000-0000-0000-0000-000000000004";

//     // AST 構築: twice → sum → count
//     auto twice = pm.make_node(twice_uuid);
//     auto sum   = pm.make_node(sum_uuid);
//     auto count = pm.make_node(count_uuid);

//     pm.assign_input(sum, 0, twice);  // sum(x) = twice(x)
//     pm.assign_input(twice, 0, std::int64_t{3});
//     pm.assign_input(count, 0, sum);  // count(video, sum, 123)
//     pm.assign_input(count, 1, std::int64_t{123});

//     /* ---------- スケジューラ ---------- */
//     auto [sorted, cycles, life] = sched::topological_sort(count);

//     EXPECT_EQ(sorted.size(), 3u);
//     EXPECT_TRUE(cycles.empty());
//     ASSERT_EQ(life.size(), sorted.size());

//     /* ---------- ライフサイクル ---------- */
//     for (auto& n : sorted) EXPECT_TRUE(pm.invoke_start_rendering(n));
//     for (auto& n : sorted) EXPECT_TRUE(pm.invoke_render_frame(n, 0));
//     for (auto& n : sorted) pm.invoke_finish_rendering(n);
// }
