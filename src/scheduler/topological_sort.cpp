#include <algorithm>
#include <limits>
#include <prismcascade/memory/time.hpp>
#include <prismcascade/scheduler/topological_sort.hpp>
#include <unordered_map>
#include <vector>

namespace prismcascade::scheduler {
namespace {

enum class TopSortDfsMark : std::uint8_t { initial, visiting, done };

// 再帰 DFS
bool dfs(const std::shared_ptr<ast::AstNode>& v, std::unordered_map<std::int64_t, TopSortDfsMark>& mark,
         std::unordered_map<std::int64_t, RankInfo>& info, std::vector<std::shared_ptr<ast::AstNode>>& callstack,
         std::vector<std::shared_ptr<ast::AstNode>>& topo, std::vector<std::shared_ptr<ast::AstNode>>& cycle,
         memory::timestamp_t& counter) {
    if (!v) return true;
    const auto id = v->plugin_instance_handler;

    switch (mark[id]) {
        case TopSortDfsMark::visiting: {  // back-edge → cycle
            auto it = std::find_if(callstack.begin(), callstack.end(), [&](const std::shared_ptr<ast::AstNode>& p) {
                return p->plugin_instance_handler == id;
            });
            cycle.assign(it, callstack.end());
            cycle.push_back(v);
            return false;
        }
        case TopSortDfsMark::done:
            return true;
        default:;
    }

    mark[id] = TopSortDfsMark::visiting;
    callstack.push_back(v);

    RankInfo& ri = info[id];
    ri.pre_ts    = ++counter;  // pre-order

    auto recur = [&](const std::shared_ptr<ast::AstNode>& n) {
        return dfs(n, mark, info, callstack, topo, cycle, counter);
    };

    for (const auto& in : v->inputs) {
        if (auto p = std::get_if<std::shared_ptr<ast::AstNode>>(&in)) {
            if (*p && !recur(*p)) return false;
        } else if (auto se = std::get_if<ast::SubEdge>(&in)) {
            if (auto src = se->source.lock())
                if (!recur(src)) return false;
        }
    }

    ri.post_ts = ++counter;  // post-order
    mark[id]   = TopSortDfsMark::done;
    callstack.pop_back();
    topo.push_back(v);  // 葉→根の順
    return true;
}

}  // namespace

TopoResult topological_sort(const std::shared_ptr<ast::AstNode>& root) {
    TopoResult res;
    if (!root) return res;

    std::unordered_map<std::int64_t, TopSortDfsMark> mark;
    std::unordered_map<std::int64_t, RankInfo>       info;
    std::vector<std::shared_ptr<ast::AstNode>>       callstack, topo;
    memory::timestamp_t                              counter = 0;

    // 普通にDFS
    const bool success = dfs(root, mark, info, callstack, topo, res.cycle_path, counter);
    if (!success) return res;  // ranks 空・cycle_path に閉路

    // fps_lcm の計算
    {
        // res.fps_lcm = 1;
        // auto lcm = [](memory::timestamp_t a, memory::timestamp_t b) { return a / boost::multiprecision::gcd(a, b) *
        // b; }; for (auto& n : topo)
        //     for (auto& winOpt : n->input_window)
        //         if (winOpt && winOpt->fps.num)  // fps == 0 は除外
        //             res.fps_lcm = lcm(res.fps_lcm, winOpt->fps.num);
    }

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
        auto update        = [&](const std::shared_ptr<ast::AstNode>& prod) {
            if (!prod) return;
            auto& pi              = info[prod->plugin_instance_handler];
            pi.last_consumer_rank = std::max(pi.last_consumer_rank, consumer_rank);
        };
        for (const auto& input : node->inputs) {
            if (auto child = std::get_if<std::shared_ptr<ast::AstNode>>(&input); child && *child)
                update(*child);
            else if (auto sub_edge = std::get_if<ast::SubEdge>(&input))
                if (auto sub_edge_source = sub_edge->source.lock()) update(sub_edge_source);
        }
    }

    // delayを伝搬
    {
        // calculate upper bound
        std::unordered_map<std::int64_t, std::optional<std::int64_t>> delay_upper_bound;
        delay_upper_bound[topo.back()->plugin_instance_handler] = 0;  // root固定
        for (auto node_iterator = topo.crbegin(); node_iterator != topo.crend(); ++node_iterator) {
            const auto& node             = *node_iterator;
            auto        assign_min_delay = [&delay_upper_bound](std::optional<std::int64_t> source_delay,
                                                         std::int64_t                maybe_diff,
                                                         std::int64_t                child_plugin_instance_handler) {
                if (source_delay) {
                    auto& child_delay_upper_bound = delay_upper_bound[child_plugin_instance_handler];
                    if (child_delay_upper_bound)
                        child_delay_upper_bound = std::min(*child_delay_upper_bound, *source_delay - maybe_diff);
                    else
                        child_delay_upper_bound = *source_delay - maybe_diff;
                }
            };
            for (std::size_t input_index = 0; input_index < node->inputs.size(); ++input_index) {
                const auto& input_value  = node->inputs.at(input_index);
                const auto& maybe_window = node->input_window.at(input_index);
                if (auto source = std::get_if<std::shared_ptr<ast::AstNode>>(&input_value); source && *source) {
                    if (maybe_window) {
                        assign_min_delay(delay_upper_bound[node->plugin_instance_handler], maybe_window->look_ahead,
                                         (*source)->plugin_instance_handler);
                    }
                } else if (auto sub_edge = std::get_if<ast::SubEdge>(&input_value)) {
                    if (auto source = sub_edge->source.lock()) {
                        if (maybe_window) {
                            assign_min_delay(delay_upper_bound[node->plugin_instance_handler], maybe_window->look_ahead,
                                             source->plugin_instance_handler);
                        }
                    }
                }
            }
        }

        // calculate delay offset
        std::int64_t delay_offset = std::numeric_limits<std::int64_t>::max();
        for (const auto& node : topo) {
            const auto& maybe_upper_bound = delay_upper_bound[node->plugin_instance_handler];
            if (maybe_upper_bound) delay_offset = std::min(delay_offset, *maybe_upper_bound);
        }

        // store (min{D} = 0)
        for (const auto& node : topo) {
            const auto& maybe_upper_bound = delay_upper_bound[node->plugin_instance_handler];
            if (maybe_upper_bound) info.at(node->plugin_instance_handler).delay = *maybe_upper_bound - delay_offset;
        }
    }

    // Rank Table を構築する
    for (const auto& node : topo) res.ranks.emplace(node->plugin_instance_handler, info[node->plugin_instance_handler]);

    return res;
}

}  // namespace prismcascade::scheduler
