#include <algorithm>
#include <prismcascade/scheduler/topological_sort.hpp>
#include <unordered_map>
#include <vector>

namespace prismcascade::scheduler {
namespace {

// ─── DFS colour marks ──────────────────────────────────────────
enum class Mark : std::uint8_t { initial, visiting, done };
using NodePtr = std::shared_ptr<ast::AstNode>;

// ------------- 再帰 DFS ----------------------------------------
bool dfs(const NodePtr& v, std::unordered_map<std::int64_t, Mark>& mark,
         std::unordered_map<std::int64_t, RankInfo>& info, std::vector<NodePtr>& callstack, std::vector<NodePtr>& topo,
         std::vector<NodePtr>& cycle, timestamp_t& counter) {
    if (!v) return true;
    const auto id = v->plugin_instance_handler;

    switch (mark[id]) {
        case Mark::visiting: {  // back-edge → cycle
            auto it = std::find_if(callstack.begin(), callstack.end(),
                                   [&](const NodePtr& p) { return p->plugin_instance_handler == id; });
            cycle.assign(it, callstack.end());
            cycle.push_back(v);
            return false;
        }
        case Mark::done:
            return true;
        default:;
    }

    mark[id] = Mark::visiting;
    callstack.push_back(v);

    RankInfo& ri = info[id];
    ri.pre_ts    = ++counter;  // pre-order

    auto recur = [&](const NodePtr& n) { return dfs(n, mark, info, callstack, topo, cycle, counter); };

    for (const auto& in : v->inputs) {
        if (auto p = std::get_if<NodePtr>(&in)) {
            if (*p && !recur(*p)) return false;
        } else if (auto se = std::get_if<ast::SubEdge>(&in)) {
            if (auto src = se->source.lock())
                if (!recur(src)) return false;
        }
    }

    ri.post_ts = ++counter;  // post-order
    mark[id]   = Mark::done;
    callstack.pop_back();
    topo.push_back(v);  // 葉→根の順
    return true;
}

}  // namespace

// =================================================================
TopoResult topological_sort(const std::shared_ptr<ast::AstNode>& root) {
    TopoResult res;
    if (!root) return res;

    std::unordered_map<std::int64_t, Mark>     mark;
    std::unordered_map<std::int64_t, RankInfo> info;
    std::vector<NodePtr>                       callstack, topo;
    timestamp_t                                counter = 0;

    // 普通にDFS
    const bool success = dfs(root, mark, info, callstack, topo, res.cycle_path, counter);
    if (!success) return res;  // ranks 空・cycle_path に閉路

    // Rank を振る
    std::int64_t rank = 0;
    for (const auto& node : topo) {  // topo は葉→根 順
        auto& ri              = info[node->plugin_instance_handler];
        ri.rank               = rank++;   // 葉 0, 親ほど大
        ri.last_consumer_rank = ri.rank;  // 最低限，自分自身は出力の寿命内であるべき
    }

    // last_consumer_rank を伝搬 (root 側から辿る)
    for (const auto& node : topo) {
        auto consumer_rank = info[node->plugin_instance_handler].rank;  // 現在ノード
        auto update        = [&](const NodePtr& prod) {
            if (!prod) return;
            auto& pi              = info[prod->plugin_instance_handler];
            pi.last_consumer_rank = std::max(pi.last_consumer_rank, consumer_rank);
        };
        for (const auto& input : node->inputs) {
            if (auto child = std::get_if<NodePtr>(&input); child && *child)
                update(*child);
            else if (auto sub_edge = std::get_if<ast::SubEdge>(&input))
                if (auto sub_edge_source = sub_edge->source.lock()) update(sub_edge_source);
        }
    }

    // Rank Table を構築する
    for (const auto& node : topo) res.ranks.emplace(node->plugin_instance_handler, info[node->plugin_instance_handler]);

    return res;
}

}  // namespace prismcascade::scheduler
