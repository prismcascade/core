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

    void collect_nodes(const std::shared_ptr<AstNode>& node, std::map<std::int64_t, std::shared_ptr<AstNode>>& nodes, std::map<std::int64_t, std::int64_t>& in_degrees, std::map<std::int64_t, std::vector<std::int64_t>>& depended_by){
        auto link = [&](std::int64_t parent_handler, std::int64_t child_handler){
            depended_by[child_handler].push_back(parent_handler);
            ++in_degrees[parent_handler];
        };
        if(!node) return;
        if(nodes.count(node->plugin_instance_handler)) return;
        nodes[node->plugin_instance_handler] = node;
        for(auto&& input : node->children){
            if(std::holds_alternative<std::shared_ptr<AstNode>>(input)){
                const auto& child_instance = std::get<std::shared_ptr<AstNode>>(input);
                link(node->plugin_instance_handler, child_instance->plugin_instance_handler);
                collect_nodes(child_instance, nodes, in_degrees, depended_by);
            } else if(std::holds_alternative<std::shared_ptr<AstNode>>(input)){
                const auto& sub_edge = std::get<AstNode::SubEdge>(input);
                if(auto child_instance = sub_edge.from_node.lock()){
                    link(node->plugin_instance_handler, child_instance->plugin_instance_handler);
                    collect_nodes(child_instance, nodes, in_degrees, depended_by);
                } else
                    throw std::runtime_error("[RenderingScheduler::topological_sort/collect_nodes] dungling pointer (sub_edge)");
            }
        }
    }
}

    // NOTE: macro によるノード展開で得られたASTについては，個別に topological sort する必要がある
    // Ast -> (トポロジカルソートしたもの（循環部分を除く）, 循環があったノード一覧（plugin_instance_handler順）)
    std::pair<std::vector<std::shared_ptr<AstNode>>, std::vector<std::shared_ptr<AstNode>>> RenderingScheduler::topological_sort(const std::shared_ptr<AstNode>& ast_root){
        std::vector<std::shared_ptr<AstNode>> sorted_ast;
        // plugin_instance_handler -> (.*)
        std::map<std::int64_t, std::shared_ptr<AstNode>> nodes;
        std::map<std::int64_t, std::int64_t> in_degrees;  // 入り次数
        std::map<std::int64_t, std::vector<std::int64_t>> depended_by;  // 依存されている元
        collect_nodes(ast_root, nodes, in_degrees, depended_by);

        // topological sort
        std::vector<std::int64_t> topologically_sorted_node_handlers;
        std::queue<std::int64_t> nodes_without_in_degree;

        // NOTE: in_degrees に全ての node がセットされているわけではないので，これだとバグる
        // for(const auto& [plugin_instance_handler, in_degree] : in_degrees)
        for(const auto& [plugin_instance_handler, _] : nodes)
            if(in_degrees[plugin_instance_handler] == 0)
                nodes_without_in_degree.push(plugin_instance_handler);

        while(!nodes_without_in_degree.empty()){
            std::int64_t plugin_instance_handler = nodes_without_in_degree.front();
            nodes_without_in_degree.pop();

            topologically_sorted_node_handlers.push_back(plugin_instance_handler);
            sorted_ast.push_back(nodes.at(plugin_instance_handler));

            for(auto&& parent_handler : depended_by[plugin_instance_handler]){
                --in_degrees[parent_handler];
                if(in_degrees[parent_handler] == 0)
                    nodes_without_in_degree.push(parent_handler);
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
            return { sorted_ast, nodes_in_cycles };
        }

        // 循環が無かった場合
        return { sorted_ast, {} };
    }

}
