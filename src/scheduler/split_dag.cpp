// #include <prismcascade/scheduler/split_dag.hpp>
// #include <queue>

// namespace prismcascade::scheduler {
// std::vector<subdag_info> split_dag(const std::shared_ptr<ast::AstNode>& root, const DelayTable& delay,
//                                    std::uint32_t startup_buffer) {
//     std::vector<subdag_info> out;
//     if (!root) return out;

//     std::queue<std::pair<std::shared_ptr<ast::AstNode>, std::uint32_t>> q;
//     q.emplace(root, 0);  // queue_id = 0  がライブ DAG

//     while (!q.empty()) {
//         auto [n, qid] = q.front();
//         q.pop();
//         if (!n) continue;

//         out.push_back({n, qid});

//         auto next_qid = (delay.at(n->plugin_instance_handler) > startup_buffer) ? qid + 1 : qid;

//         auto push = [&](const std::shared_ptr<ast::AstNode>& c) {
//             if (c) q.emplace(c, next_qid);
//         };

//         for (const auto& in : n->inputs) {
//             if (auto p = std::get_if<std::shared_ptr<ast::AstNode>>(&in))
//                 push(*p);
//             else if (auto se = std::get_if<ast::SubEdge>(&in))
//                 if (auto src = se->source.lock()) push(src);
//         }
//     }
//     return out;
// }
// }  // namespace prismcascade::scheduler
