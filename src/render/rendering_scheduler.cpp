#include "rendering_scheduler.hpp"
#include <core/project_data.hpp>
#include <memory>
#include <queue>
#include <map>
#include <cassert>
#include <vector>
#include <algorithm>
#include <iostream>

namespace PrismCascade {

namespace {

    void collect_nodes(
        const std::shared_ptr<AstNode>& node,
        std::map<std::int64_t, std::shared_ptr<AstNode>>& nodes,
        std::map<std::int64_t, std::int64_t>& in_degrees,
        std::map<std::int64_t, std::vector<std::int64_t>>& depended_by,
        std::map<std::int64_t, std::vector<std::pair<std::int64_t, std::int32_t>>>& depends_to
    ){
        auto link = [&](std::int64_t parent_handler, std::int64_t child_handler, std::int32_t output_index){
            depended_by[child_handler].push_back(parent_handler);
            depends_to[parent_handler].push_back({child_handler, output_index});
            ++in_degrees[parent_handler];
        };
        if(!node) return;
        if(nodes.count(node->plugin_instance_handler)) return;
        nodes[node->plugin_instance_handler] = node;
        for(auto&& input : node->children){
            if(std::holds_alternative<std::shared_ptr<AstNode>>(input)){
                const auto& child_instance = std::get<std::shared_ptr<AstNode>>(input);
                link(node->plugin_instance_handler, child_instance->plugin_instance_handler, 0);
                collect_nodes(child_instance, nodes, in_degrees, depended_by, depends_to);
            } else if(std::holds_alternative<AstNode::SubEdge>(input)){
                const auto& sub_edge = std::get<AstNode::SubEdge>(input);
                if(auto child_instance = sub_edge.from_node.lock()){
                    link(node->plugin_instance_handler, child_instance->plugin_instance_handler, sub_edge.index);
                    collect_nodes(child_instance, nodes, in_degrees, depended_by, depends_to);
                } else
                    throw std::runtime_error("[RenderingScheduler::topological_sort/collect_nodes] dungling pointer (sub_edge)");
            }
        }
    }
}

    // NOTE: macro によるノード展開で得られたASTサブツリーについては，個別に topological sort する必要がある
    // Ast -> (トポロジカルソートしたもの（循環部分を除く）, 循環があったノード一覧（plugin_instance_handler順）)
    std::tuple<std::vector<std::shared_ptr<AstNode>>, std::vector<std::shared_ptr<AstNode>>, std::vector<std::map<std::pair<std::int64_t, std::int32_t>, std::int64_t>>>
    RenderingScheduler::topological_sort(const std::shared_ptr<AstNode>& ast_root){
        std::vector<std::shared_ptr<AstNode>> sorted_ast;
        std::map<std::int64_t, std::size_t> sorted_ranking;
        // [(from_instance_name, id)->count_diff] （要求された時点で+, 消費した時点で-）
        std::vector<std::map<std::pair<std::int64_t, std::int32_t>, std::int64_t>> variable_lifetime_differences;
        // plugin_instance_handler -> (.*)
        std::map<std::int64_t, std::shared_ptr<AstNode>> nodes;
        std::map<std::int64_t, std::int64_t> in_degrees;  // 入り次数
        // ↓ 変数名が良くない気がする。
        std::map<std::int64_t, std::vector<std::int64_t>> depended_by;  // 依存されている元: 依存先(child) -> 依存元(parent)
        std::map<std::int64_t, std::vector<std::pair<std::int64_t, std::int32_t>>> depends_to;   // 依存している先: 依存元(parent) -> (依存先(child), 出力index)
        collect_nodes(ast_root, nodes, in_degrees, depended_by, depends_to);
        // 全て入るとは限らないため，メモリ予約のみ
        sorted_ast.reserve(nodes.size());
        variable_lifetime_differences.reserve(nodes.size());

        // topological sort
        std::vector<std::int64_t> topologically_sorted_node_handlers;
        std::queue<std::int64_t> nodes_without_in_degree;

        // NOTE: in_degrees に全ての node がセットされているわけではないので， in_degrees に対して回すとバグる
        for(const auto& [plugin_instance_handler, _] : nodes)
            if(in_degrees[plugin_instance_handler] == 0)
                nodes_without_in_degree.push(plugin_instance_handler);

        while(!nodes_without_in_degree.empty()){
            std::int64_t plugin_instance_handler = nodes_without_in_degree.front();
            nodes_without_in_degree.pop();

            topologically_sorted_node_handlers.push_back(plugin_instance_handler);
            sorted_ranking[plugin_instance_handler] = sorted_ast.size();
            // NOTE: ↑↓ 順番依存コード
            sorted_ast.push_back(nodes.at(plugin_instance_handler));
            variable_lifetime_differences.emplace_back();

            // これに加えて， sorted_ranking の全ての値は variable_lifetime_differences.size() 未満
            assert(sorted_ast.size() == variable_lifetime_differences.size());

            for(auto&& parent_handler : depended_by[plugin_instance_handler]){
                --in_degrees[parent_handler];
                if(in_degrees[parent_handler] == 0)
                    nodes_without_in_degree.push(parent_handler);
            }
            // 変数寿命管理のため，変数の生成(+)と要求(-)を記録
            for(auto&& [child_handler, output_index] : depends_to[plugin_instance_handler]){
                const auto child_sort_index = sorted_ranking.at(child_handler);  // ここで落ちた場合にはアルゴリズムが壊れている
                const auto current_sort_index = sorted_ranking.at(plugin_instance_handler);
                // NOTE: 重複して依存している場合， depends_to には重複して記録されているので，通常通り ++ しておけば問題ない
                ++variable_lifetime_differences.at(child_sort_index)[{child_sort_index, output_index}];
                --variable_lifetime_differences.at(current_sort_index)[{child_sort_index, output_index}];
            }
        }

        // 循環があればそれも返却
        if(sorted_ast.size() < nodes.size()){
            std::vector<std::int64_t> all_node_handlers;
            for(const auto& [plugin_instance_handler, _] : nodes)
                all_node_handlers.push_back(plugin_instance_handler);
            std::sort(topologically_sorted_node_handlers.begin(), topologically_sorted_node_handlers.end());
            std::sort(all_node_handlers.begin(), all_node_handlers.end());  // これはstd::mapの特性上要らないはずだけど一応ソート

            std::vector<std::int64_t> node_handlers_in_cycles;
            std::set_difference(
                all_node_handlers.begin(), all_node_handlers.end(),
                topologically_sorted_node_handlers.begin(), topologically_sorted_node_handlers.end(),
                std::back_inserter(node_handlers_in_cycles)
            );
            std::vector<std::shared_ptr<AstNode>> nodes_in_cycles;
            nodes_in_cycles.reserve(node_handlers_in_cycles.size());
            for(std::int64_t plugin_instance_handler : node_handlers_in_cycles)
                nodes_in_cycles.push_back(nodes[plugin_instance_handler]);
            return { sorted_ast, nodes_in_cycles, variable_lifetime_differences };
        }

        // 循環が無かった場合
        return { sorted_ast, {}, variable_lifetime_differences };
    }

}
