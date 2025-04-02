#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <array>
#include <vector>
#include <cassert>
#include <plugin/dynamic_library.hpp>
#include <core/project_data.hpp>
#include <memory/memory_manager.hpp>
#include <render/rendering_scheduler.hpp>
#include <util/dump.hpp>


int main(){
    using namespace PrismCascade;
    HandleManager handle_manager;
    DllMemoryManager dll_memory_manager;

    auto plugin_file_list = DynamicLibrary::list_plugin();
    std::cout << " -------- Plugin List --------" << std::endl;
    for(auto&& s : plugin_file_list)
        std::cout << s << std::endl;
    std::cout << " -----------------------------" << std::endl;

    try{
        std::int64_t plugin_handler = handle_manager.freshHandler();
        std::int64_t plugin_instance_handler = handle_manager.freshHandler();
        DynamicLibrary lib(plugin_file_list[0]);
        PluginMetaData plugin_metadata_dll;

        // メタ情報を取得
        reinterpret_cast<bool(*)(void*, std::int64_t, PluginMetaData*, void*, void*, void*, void*)>(lib["getMetaInfo"])(
            reinterpret_cast<void*>(&dll_memory_manager),
            plugin_handler,
            &plugin_metadata_dll,
            reinterpret_cast<void*>(&DllMemoryManager::allocate_param_static),
            reinterpret_cast<void*>(&DllMemoryManager::assign_text_static),
            reinterpret_cast<void*>(&DllMemoryManager::add_required_handler_static),
            reinterpret_cast<void*>(&DllMemoryManager::add_handleable_effect_static)
        );

        // 取得したメタ情報を保存
        PluginMetaDataInternal plugin_metadata;
        plugin_metadata.name = std::string(plugin_metadata_dll.name.buffer);
        plugin_metadata.protocol_version = plugin_metadata_dll.protocol_version;
        plugin_metadata.type = plugin_metadata_dll.type;
        plugin_metadata.uuid = std::string(plugin_metadata_dll.uuid.buffer);
        dll_memory_manager.plugin_metadata_instances[plugin_handler] = plugin_metadata;

        // メモリ解放
        dll_memory_manager.free_text(&plugin_metadata_dll.name);
        dll_memory_manager.free_text(&plugin_metadata_dll.uuid);

        // プラグインのロードイベント発火
        reinterpret_cast<bool(*)()>(lib["onLoadPlugin"])();

        // パラメータパックの allocation
        ParameterPack input_params  = dll_memory_manager.allocate_parameter(plugin_handler, plugin_instance_handler, false);
        ParameterPack output_params = dll_memory_manager.allocate_parameter(plugin_handler, plugin_instance_handler, true);

        // レンダリング開始イベント発火 & アロケーション
        VideoMetaData clip_meta_data;
        reinterpret_cast<bool(*)(void*, VideoMetaData*, ParameterPack*, ParameterPack*, void*, void*, void*, void*, void*)>(lib["onStartRendering"])(
            reinterpret_cast<void*>(&dll_memory_manager),
            &clip_meta_data,
            &input_params,
            &output_params,
            reinterpret_cast<void*>(&DllMemoryManager::load_video_buffer_static),
            reinterpret_cast<void*>(&DllMemoryManager::assign_text_static),
            reinterpret_cast<void*>(&DllMemoryManager::allocate_vector_static),
            reinterpret_cast<void*>(&DllMemoryManager::allocate_video_static),
            reinterpret_cast<void*>(&DllMemoryManager::allocate_audio_static)
        );

        // 入力パラメータのセット
        *reinterpret_cast<int*>(dll_memory_manager.parameter_pack_instances.at(plugin_instance_handler).second[false].first->value) = 18;

        // レンダリング
        // 実際にはループを回す
        int frame = 7;
        bool ok = reinterpret_cast<bool(*)(void*, ParameterPack*, ParameterPack*, const VideoMetaData, int, void*, void*)>(lib["renderFrame"])(
            reinterpret_cast<void*>(&dll_memory_manager),
            &input_params,
            &output_params,
            clip_meta_data,
            frame,
            reinterpret_cast<void*>(&DllMemoryManager::load_video_buffer_static),
            reinterpret_cast<void*>(&DllMemoryManager::assign_text_static)
        );

        // 出力を見る
        std::cout << ok << std::endl;
        int result = *reinterpret_cast<int*>(dll_memory_manager.parameter_pack_instances.at(plugin_instance_handler).second[true].first->value);
        std::cout << result << std::endl;

        // レンダリング完了イベント発火
        reinterpret_cast<void(*)()>(lib["onFinishRendering"])();

        // プラグインの破棄イベント発火
        reinterpret_cast<void(*)()>(lib["onDestroyPlugin"])();

        std::cout << "[Finished]" << std::endl << std::endl;

        // 値のダンプ
        dump_plugins(dll_memory_manager);
        dump_parameters(dll_memory_manager);

        std::cout << "[Finished]" << std::endl << std::endl;

    } catch(...) {
        std::cout << "Error (Loading File)" << std::endl;
    }
}
