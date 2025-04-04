#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <array>
#include <vector>
#include <cassert>
#include <plugin/dynamic_library.hpp>
#include <plugin/plugin_manager.hpp>
#include <core/project_data.hpp>
#include <memory/memory_manager.hpp>
#include <render/rendering_scheduler.hpp>
#include <util/dump.hpp>


int main(){
    using namespace PrismCascade;

    // プラグインを読み込む
    PluginManager plugin_manager;
    dump_plugins(plugin_manager.dll_memory_manager);

    auto run = [&](std::shared_ptr<AstNode> root, int debug_level = 2){
        auto&& [sorted_ast, nodes_in_cycles] = RenderingScheduler::topological_sort(root);

        for(auto&& node : sorted_ast){
            std::cerr << "sorted: (" << node->plugin_instance_handler << ") " << node->plugin_name << std::endl;
        }
        for(auto&& node : nodes_in_cycles){
            std::cerr << "cycle: (" << node->plugin_instance_handler << ") " << node->plugin_name << std::endl;
        }

        assert(nodes_in_cycles.size() == 0);

        // 準備
        for(auto&& node : sorted_ast)
            plugin_manager.invoke_start_rendering(node);

        // 呼ぶ
        for(auto&& node : sorted_ast){
            bool ok = plugin_manager.invoke_render_frame(node, 7);
            // 結果の表示
            std::cout << node->plugin_name << ": " << (ok ? "OK" : "Failed") << std::endl;
            if(debug_level > 1)
                dump_parameters(plugin_manager.dll_memory_manager);
        }

        // 完了
        for(auto&& node : sorted_ast)
            plugin_manager.invoke_finish_rendering(node);

        std::cout << " -------- [Finished] --------" << std::endl;

        // 値のダンプ
        if(debug_level > 0)
            dump_parameters(plugin_manager.dll_memory_manager);
    };

    ////////////////

    const std::string twice_plugin_uuid = "f0000000-0000-0000-0000-000000000000";
    const std::string   sum_plugin_uuid = "f0000000-0000-0000-0000-000000000002";
    const std::string count_plugin_uuid = "f0000000-0000-0000-0000-000000000004";

	std::cout << "----------------" << std::endl;

    std::shared_ptr<AstNode> root = plugin_manager.make_node(sum_plugin_uuid);

    // 入力のセット
    {
        std::shared_ptr<AstNode> child = plugin_manager.make_node(twice_plugin_uuid);
        plugin_manager.assign_input(root, 0, child);
        plugin_manager.assign_input(child, 0, 25);
        run(root, 0);
        plugin_manager.dll_memory_manager.dump_memory_usage();

        std::cout << "----------------" << std::endl;

        // count_plugin に繋げる
        std::shared_ptr<AstNode> new_root = plugin_manager.make_node(count_plugin_uuid);
        plugin_manager.assign_input(new_root, 1, AstNode::SubEdge{child, 2});
        plugin_manager.assign_input(new_root, 2, root);
        auto old_root = root;
        root = new_root;
        run(root, 1);
        plugin_manager.dll_memory_manager.dump_memory_usage();

        std::cout << "----------------" << std::endl;

        plugin_manager.assign_input(new_root, 2, 12);
        run(root, 2);
        plugin_manager.dll_memory_manager.dump_memory_usage();
    }

	std::cout << "----------------" << std::endl;

    std::cout << " -------- [完了！！] --------" << std::endl;

    // TODO: 値を付け替える

}
