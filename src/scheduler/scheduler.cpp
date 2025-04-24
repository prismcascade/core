/* ── prismcascade/schedule/scheduler.cpp ─────────────────────────
   High-level compile / tick driver.
   Layers 0-3 build every table we need; here we only glue them and
   keep the runtime priority_queues.
   ────────────────────────────────────────────────────────────── */
#include <algorithm>
#include <functional>
#include <prismcascade/scheduler/scheduler.hpp>
#include <unordered_map>
#include <utility>

namespace prismcascade::scheduler {
// -----------------------------------------------------------------------------
// ctor
// -----------------------------------------------------------------------------
Scheduler::Scheduler(const Options& o) : opt_(o) {}

// -----------------------------------------------------------------------------
//  compile :  AST  →  (Rank, Delay, SubDAG, Exec-queue init)
// -----------------------------------------------------------------------------
void Scheduler::compile(const std::shared_ptr<ast::AstNode>& root) {
    if (!root) throw std::domain_error("compile: null root");

    /* 1)  トポロジカルソート  ───────────────────────────── */
    const auto topo = topological_sort(root);
    if (topo.ranks.empty()) throw std::domain_error("compile: cycle detected");

    /* 2)  gdelay table  ─────────────────────────────────── */
    const auto gdelay = build_delay_table(root);

    /* 3)  queue 付け SubDAG  ───────────────────────────── */
    subdags_ = split_dag(root, gdelay, opt_.startup_buffer);

    /* 4)  ExecQueue 配列を作る  ─────────────────────────── */
    std::uint32_t max_qid = 0;
    for (const auto& s : subdags_) max_qid = std::max(max_qid, s.queue_id);

    auto cmp = [](const ExecutionQueue::Item& a, const ExecutionQueue::Item& b) {
        return (a.stamp == b.stamp) ? (a.rank > b.rank)     // rank小さいほうを先に
                                    : (a.stamp > b.stamp);  // stamp小さいほうを先に
    };
    queues_.clear();
    queues_.reserve(max_qid + 1);
    for (std::uint32_t i = 0; i <= max_qid; ++i) queues_.emplace_back(cmp);

    /* 5)  “葉” を queue-0 に初期 push  ─────────────────── */
    std::unordered_map<std::int64_t, std::uint32_t> indeg;
    for (const auto& s : subdags_)
        for (const auto& in : s.root->inputs)
            if (auto p = std::get_if<std::shared_ptr<ast::AstNode>>(&in); p && *p)
                ++indeg[(*p)->plugin_instance_handler];
            else if (auto se = std::get_if<ast::SubEdge>(&in))
                if (auto src = se->source.lock()) ++indeg[src->plugin_instance_handler];

    for (const auto& s : subdags_)
        if (s.queue_id == 0 && indeg[s.root->plugin_instance_handler] == 0)
            queues_[0].push({0 /*stamp*/,  //
                             topo.ranks.at(s.root->plugin_instance_handler).rank, s.root});
}

// -----------------------------------------------------------------------------
//  tick : run one node (returns false when all queues empty)
// -----------------------------------------------------------------------------
bool Scheduler::tick(const std::function<void(const std::shared_ptr<ast::AstNode>&)>& run_cb) {
    for (auto& q : queues_) {
        ExecutionQueue::Item it;
        if (!q.pop(it)) continue;

        run_cb(it.node);  // ← 実際の render はコールバック側

        /* 次回実行 stamp 更新 (デモ用に +1).
           実際は node 種別ごとに step が決まる。 */
        it.stamp += 1;
        q.push(it);
        return true;
    }
    return false;  // 全 queue が空 → 終了
}

}  // namespace prismcascade::scheduler
