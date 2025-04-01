#include <iostream>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <plugin/dynamic_library.hpp>
#include <core/project_data.hpp>
#include <render/rendering_scheduler.hpp>

// handle -> data
std::map<int, PluginMetaDataInternal> plugin_metadata_instance;
std::map<const char*, std::shared_ptr<std::string>> text_instance;
std::map<int, std::vector<std::tuple<std::string, VariableType>>> ParameterTypeInfo;

bool allocate_param(int plugin_handler, bool is_output, VariableType type, const char* name, void* metadata){
    ParameterTypeInfo[plugin_handler].push_back({std::string(name), type});
    return true;
}

// TODO: ptr -> 実体のshared_ptr のmapを持つことで参照カウント管理をする
bool assign_text(TextParam* buffer, const char* text){
    auto text_buffer = std::make_shared<std::string>(text);
    // TODO: ここではなく，そのさらに根本の参照先を key にするべきな気がする (要修正)
    text_instance[text_buffer->c_str()] = text_buffer;

    buffer->size = static_cast<int>(text_buffer->size());
    buffer->buffer = text_buffer->c_str();
    return true;
}

bool add_required_handler(int plugin_handler, const char* effect_name){
    // TODO: implement
    return true;
}

bool add_handleable_effect(int plugin_handler, const char* effect_name){
    // TODO: implement
    return true;
}

bool load_video_buffer(VideoFrame* target, std::uint64_t size){
    // TODO: implement
    return true;
}

// TODO: ptr -> 実体のshared_ptr のmapを持つことで参照カウント管理をする
bool allocate_vector(VectorParam* buffer, VariableType type, const char* text){
    // TODO: implement
    return true;
}

int main(){
    try{
        int handler = HandleManager::freshHandler();
        DynamicLibrary lib("debug_twice_plugin.dll");
        auto& plugin_metadata = plugin_metadata_instance[handler];
        PluginMetaData plugin_metadata_dll;

        // メタ情報を取得
        reinterpret_cast<bool(*)(int, PluginMetaData*, void*, void*, void*, void*, void*)>(lib["getMetaInfo"])(
            handler,
            &plugin_metadata_dll,
            reinterpret_cast<void*>(allocate_param),
            reinterpret_cast<void*>(assign_text),
            reinterpret_cast<void*>(allocate_vector),
            reinterpret_cast<void*>(add_required_handler),
            reinterpret_cast<void*>(add_handleable_effect)
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
        reinterpret_cast<bool(*)(VideoMetaData*, ParameterPack*, void*, void*, void*)>(lib["onStartRendering"])(
            &clip_meta_data,
            &input_params,
            reinterpret_cast<void*>(load_video_buffer),
            reinterpret_cast<void*>(assign_text),
            reinterpret_cast<void*>(allocate_vector)
        );

        // レンダリング
        // 実際にはループを回す
        int frame = 7;
        bool ok = reinterpret_cast<bool(*)(ParameterPack*, ParameterPack*, const VideoMetaData, int, void*, void*)>(lib["renderFrame"])(
            &input_params,
            &output_params,
            clip_meta_data,
            frame,
            reinterpret_cast<void*>(load_video_buffer),
            reinterpret_cast<void*>(assign_text)
        );
        std::cout << ok << std::endl;
        std::cout << b << std::endl;

    } catch(...) {
        std::cout << "Error (Loading File)" << std::endl;
    }
}
