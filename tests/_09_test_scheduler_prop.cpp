// // tests/property/test_scheduler_prop.cpp
// //
// // RapidCheck + GoogleTest で RenderingScheduler の
// // topological_sort が持つべき性質を検証する
// //
// //  ▸ acyclic な AST を生成 → cycles が空 / 並び順が親→子
// //  ▸ 1 本だけ自己ループを作る → cycles が空でない
// //
// #include <gtest/gtest.h>
// #include <rapidcheck/gtest.h>

// #include <prismcascade/scheduler/rendering_scheduler.hpp>

// using namespace prismcascade;
// namespace sched = prismcascade::scheduler;
// using ast::Node;

// /* ---------- ヘルパ: acyclic ランダム木生成 ------------------- */
// static std::shared_ptr<Node> make_random_tree(int depth = 0) {
//     auto n = std::make_shared<Node>();
//     if (depth > 2) return n;

//     int children = *rc::gen::inRange(0, 3);
//     n->inputs.reserve(children);
//     for (int i = 0; i < children; ++i) {
//         auto child = make_random_tree(depth + 1);
//         n->inputs.push_back(child);
//         child->parent = n;
//     }
//     return n;
// }

// /* ---------- ヘルパ: 単一自己ループを付与 -------------------- */
// static void add_self_cycle(const std::shared_ptr<Node>& root) {
//     if (root->inputs.empty())
//         root->inputs.emplace_back(root);  // self loop
//     else
//         root->inputs[0] = root;
// }

// /* ---------- プロパティ: DAG → cycles.empty ----------------- */
// RC_GTEST_PROP(SchedulerProp, AcyclicNoCycles, ()) {
//     auto root                = make_random_tree();
//     auto [sorted, cycles, _] = sched::topological_sort(root);

//     RC_ASSERT(cycles.empty());
//     // すべての子が親よりも後ろに並んでいる
//     std::unordered_map<std::shared_ptr<Node>, int> rank;
//     for (std::size_t i = 0; i < sorted.size(); ++i) rank[sorted[i]] = int(i);

//     for (auto& n : sorted) {
//         for (auto& in : n->inputs) {
//             if (std::holds_alternative<std::shared_ptr<Node>>(in)) {
//                 auto child = std::get<std::shared_ptr<Node>>(in);
//                 RC_ASSERT(rank[child] < rank[n]);  // parent after child
//             }
//         }
//     }
// }

// /* ---------- プロパティ: 自己ループ → cycles.nonEmpty ------- */
// RC_GTEST_PROP(SchedulerProp, SelfLoopDetected, ()) {
//     auto root = make_random_tree();
//     add_self_cycle(root);

//     auto [sorted, cycles, _] = sched::topological_sort(root);
//     RC_ASSERT(!cycles.empty());
// }
