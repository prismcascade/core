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

    // とりあえず最初に読み込めたUUIDを取る
    std::string sample_target_uuid = plugin_manager.dll_memory_manager.plugin_uuid_to_handler.begin()->first;
    std::shared_ptr<AstNode> root = plugin_manager.make_node(sample_target_uuid);

    // 入力のセット
    plugin_manager.assign_input(root, 0, 25);

    // 準備
    plugin_manager.invoke_finish_rendering(root);
    // 呼ぶ
    bool ok = plugin_manager.invoke_render_frame(root, 7);
    // 出力を見る
    std::cout << ok << std::endl;
    int result = *reinterpret_cast<int*>(plugin_manager.dll_memory_manager.parameter_pack_instances.at(root->plugin_instance_handler).second[true].first->value);
    std::cout << result << std::endl;

    // 完了
    plugin_manager.invoke_finish_rendering(root);

    std::cout << "[Finished]" << std::endl << std::endl;

    // 値のダンプ
    dump_parameters(plugin_manager.dll_memory_manager);

    std::cout << "[Finished]" << std::endl << std::endl;

}
