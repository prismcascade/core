#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <array>
#include <vector>
#include <plugin/dynamic_library.hpp>
#include <core/project_data.hpp>
#include <memory/memory_manager.hpp>
#include <render/rendering_scheduler.hpp>


int main(){
    HandleManager handle_manager;
    DllMemoryManager dll_memory_manager;

    auto plugin_file_list = DynamicLibrary::list_plugin();
    std::cout << " -------- Plugin List --------" << std::endl;
    for(auto&& s : plugin_file_list)
        std::cout << s << std::endl;
    std::cout << " -----------------------------" << std::endl;

    try{
        int handler = handle_manager.freshHandler();
        DynamicLibrary lib(plugin_file_list[0]);
        auto& plugin_metadata = dll_memory_manager.plugin_metadata_instance[handler];
        PluginMetaData plugin_metadata_dll;

        // メタ情報を取得
        reinterpret_cast<bool(*)(void*, int, PluginMetaData*, void*, void*, void*, void*, void*)>(lib["getMetaInfo"])(
            reinterpret_cast<void*>(&dll_memory_manager),
            handler,
            &plugin_metadata_dll,
            reinterpret_cast<void*>(&DllMemoryManager::allocate_param_static),
            reinterpret_cast<void*>(&DllMemoryManager::assign_text_static),
            reinterpret_cast<void*>(&DllMemoryManager::allocate_vector_static),
            reinterpret_cast<void*>(&DllMemoryManager::add_required_handler_static),
            reinterpret_cast<void*>(&DllMemoryManager::add_handleable_effect_static)
        );

        plugin_metadata.name = std::string(plugin_metadata_dll.name.buffer);
        plugin_metadata.protocol_version = plugin_metadata_dll.protocol_version;
        plugin_metadata.type = plugin_metadata_dll.type;
        plugin_metadata.uuid = std::string(plugin_metadata_dll.uuid.buffer);

        std::cout << "name: " << plugin_metadata.name << std::endl;
        std::cout << "protocol_version: " << plugin_metadata.protocol_version << std::endl;
        std::cout << "type: " << static_cast<int>(plugin_metadata.type) << std::endl;
        std::cout << "uuid: " << plugin_metadata.uuid << std::endl;

        reinterpret_cast<bool(*)()>(lib["onLoadPlugin"])();

        ParameterPack input_params;
        ParameterPack output_params;
        // TODO: パラメータパックの allocation
        // Parameter tmp{.size = 1};  // TODO: C++20からこれ行けるはずんなんだけどなあ…
        int a = 20, b = 0;
        Parameter tmp_input{VariableType::Int, &a}, tmp_output{VariableType::Int, &b};
        input_params.size = 1;
        input_params.parameters = &tmp_input;
        output_params.size = 1;
        output_params.parameters = &tmp_output;
        // ^^ とりあえず ^^

        // レンダリング準備
        VideoMetaData clip_meta_data;
        reinterpret_cast<bool(*)(void*, VideoMetaData*, ParameterPack*, void*, void*, void*)>(lib["onStartRendering"])(
            reinterpret_cast<void*>(&dll_memory_manager),
            &clip_meta_data,
            &input_params,
            reinterpret_cast<void*>(&DllMemoryManager::load_video_buffer_static),
            reinterpret_cast<void*>(&DllMemoryManager::assign_text_static),
            reinterpret_cast<void*>(&DllMemoryManager::allocate_vector_static)
        );

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
        std::cout << ok << std::endl;
        std::cout << b << std::endl;

    } catch(...) {
        std::cout << "Error (Loading File)" << std::endl;
    }
}
