#pragma once
#include <map>
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <core/project_data.hpp>

struct DllMemoryManager {
    // handle -> data
    std::map<int, PluginMetaDataInternal> plugin_metadata_instance;
    std::map<const char*, std::shared_ptr<std::string>> text_instance;
    std::map<int, std::array<std::vector<std::tuple<std::string, VariableType>>, 2>> ParameterTypeInfo;

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool allocate_param(int plugin_handler, bool is_output, VariableType type, const char* name, void* metadata){
        ParameterTypeInfo[plugin_handler][is_output].push_back({std::string(name), type});
        return true;
    }

    static bool allocate_param_static(void* ptr, int plugin_handler, bool is_output, VariableType type, const char* name, void* metadata){
        return reinterpret_cast<DllMemoryManager*>(ptr)->allocate_param(plugin_handler, is_output, type, name, metadata);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    // TODO: ptr -> 実体のshared_ptr のmapを持つことで参照カウント管理をする
    bool assign_text(TextParam* buffer, const char* text){
        auto text_buffer = std::make_shared<std::string>(text);
        // TODO: ここではなく，そのさらに根本の参照先を key にするべきな気がする (要修正)
        text_instance[text_buffer->c_str()] = text_buffer;

        buffer->size = static_cast<int>(text_buffer->size());
        buffer->buffer = text_buffer->c_str();
        return true;
    }

    static bool assign_text_static(void* ptr, TextParam* buffer, const char* text){
        return reinterpret_cast<DllMemoryManager*>(ptr)->assign_text(buffer, text);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool add_required_handler(int plugin_handler, const char* effect_name){
        // TODO: implement
        return true;
    }

    static bool add_required_handler_static(void* ptr, int plugin_handler, const char* effect_name){
        return reinterpret_cast<DllMemoryManager*>(ptr)->add_required_handler(plugin_handler, effect_name);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool add_handleable_effect(int plugin_handler, const char* effect_name){
        // TODO: implement
        return true;
    }

    static bool add_handleable_effect_static(void* ptr, int plugin_handler, const char* effect_name){
        return reinterpret_cast<DllMemoryManager*>(ptr)->add_handleable_effect(plugin_handler, effect_name);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    bool load_video_buffer(VideoFrame* target, std::uint64_t frame){
        // TODO: implement
        return true;
    }

    static bool load_video_buffer_static(void* ptr, VideoFrame* target, std::uint64_t frame){
        return reinterpret_cast<DllMemoryManager*>(ptr)->load_video_buffer(target, frame);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

    // TODO: ptr -> 実体のshared_ptr のmapを持つことで参照カウント管理をする
    bool allocate_vector(VectorParam* buffer, VariableType type, const char* text){
        // TODO: implement
        return true;
    }

    static bool allocate_vector_static(void* ptr, VectorParam* buffer, VariableType type, const char* text){
        return reinterpret_cast<DllMemoryManager*>(ptr)->allocate_vector(buffer, type, text);
    }

    // -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //

};
