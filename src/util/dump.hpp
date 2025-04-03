#pragma once
#include <cassert>
#include <memory/memory_manager.hpp>

namespace PrismCascade {

inline void dump_plugins(const DllMemoryManager& dll_memory_manager){
    std::cerr << " ======== [Plugin List] ========" << std::endl;
    assert(dll_memory_manager.plugin_metadata_instances.size() == dll_memory_manager.parameter_type_informations.size());
    for(auto&& [plugin_handler, meta_data] : dll_memory_manager.plugin_metadata_instances){
        std::string type_string = to_string(meta_data.type);
        std::cerr << "    -------- [" << plugin_handler << "] " << meta_data.name << " --------" << std::endl;
        std::cerr << "      [version] " << meta_data.protocol_version << std::endl;
        std::cerr << "      [type]    " << type_string << std::endl;
        std::cerr << "      [uuid]    " << meta_data.uuid << std::endl;
        std::cerr << "      [input]" << std::endl;
        const std::array<std::vector<std::tuple<std::string, std::vector<VariableType>>>, 2>& type_info = dll_memory_manager.parameter_type_informations.at(plugin_handler);
        for(auto&& [name, type] : type_info[0]){
            if(type[0] == VariableType::Vector)
                std::cerr << "         -> " << name << " (Vector<" << to_string(type[1]) << ">)" << std::endl;
            else
                std::cerr << "         -> " << name << " (" << to_string(type[0]) << ")" << std::endl;
        }
        std::cerr << "      [output]" << std::endl;
        for(auto&& [name, type] : type_info[1]){
            if(type[0] == VariableType::Vector)
                std::cerr << "        <-  " << name << " (Vector<" << to_string(type[1]) << ">)" << std::endl;
            else
                std::cerr << "        <-  " << name << " (" << to_string(type[0]) << ")" << std::endl;
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;
}

inline void dump_parameters(const DllMemoryManager& dll_memory_manager){
    auto param_to_string = [](auto param_to_string, const Parameter& param, int offset = 0) -> std::string{
        switch(param.type){
            case VariableType::Int:
                return { std::to_string(reinterpret_cast<int*>(param.value)[offset]) };
            case VariableType::Bool:
                return { (reinterpret_cast<bool*>(param.value)[offset] ? "true" : "false") };
            case VariableType::Float:
                return { std::to_string(reinterpret_cast<double*>(param.value)[offset]) };
            case VariableType::Text:
                return { reinterpret_cast<TextParam*>(param.value)[offset].buffer };
            case VariableType::Vector:
            {
                // vector の vector には，今のところ非対応

                VectorParam* vector_param = reinterpret_cast<VectorParam*>(param.value);
                std::string result;
                result += "[";
                const char* delimiter = "";
                for(int i=0; i<vector_param->size; ++i){
                    Parameter param_tmp;
                    param_tmp.type = vector_param->type;
                    param_tmp.value = vector_param->buffer;
                    result += std::exchange(delimiter, ", ");
                    result += param_to_string(param_to_string, param_tmp, i);
                }
                result += "]";
                return result;
            }
            break;
            case VariableType::Video:
            {
                VideoFrame* video_param = reinterpret_cast<VideoFrame*>(param.value) + offset;
                return "(Video)";
            }
            break;
            case VariableType::Audio:
            {
                AudioParam* audio_param = reinterpret_cast<AudioParam*>(param.value) + offset;
                return "(Audio)";
            }
            default:
                return "(unknown type)";
        }
    };

    std::cerr << " ======== [Parameter List] ========" << std::endl;
    for(auto&& [plugin_instance_handler, meta_parameters] : dll_memory_manager.parameter_pack_instances){
        auto&& [plugin_handler, io_parameters] = meta_parameters;
        auto&& [input_parameters, output_parameters] = io_parameters;

        // メタデータの取得
        const PluginMetaDataInternal& plugin_data = dll_memory_manager.plugin_metadata_instances.at(plugin_handler);
        const std::array<std::vector<std::tuple<std::string, std::vector<VariableType>>>, 2>& type_info = dll_memory_manager.parameter_type_informations.at(plugin_handler);

        // 表示
        std::cerr << "    -------- [" << plugin_instance_handler << "] " << plugin_data.name << " --------" << std::endl;
        std::cerr << "      [input]" << std::endl;
        for(int i = 0; i < type_info[0].size(); ++i){
            auto&& [name, type] = type_info[0][i];
            if(type[0] == VariableType::Vector)
                std::cerr << "         -> " << name << " (vector<" << to_string(type[1]) << ">) : " << param_to_string(param_to_string, input_parameters.first.get()[i]) << std::endl;
            else
                std::cerr << "         -> " << name << " (" << to_string(type[0]) << ") : " << param_to_string(param_to_string, input_parameters.first.get()[i]) << std::endl;
        }
        std::cerr << "      [output]" << std::endl;
        for(int i = 0; i < type_info[1].size(); ++i){
            auto&& [name, type] = type_info[1][i];
            if(type[0] == VariableType::Vector)
                std::cerr << "        <-  " << name << " (vector<" << to_string(type[1]) << ">) : " << param_to_string(param_to_string, output_parameters.first.get()[i]) << std::endl;
            else
                std::cerr << "        <-  " << name << " (" << to_string(type[0]) << ") : " << param_to_string(param_to_string, output_parameters.first.get()[i]) << std::endl;
        }
        std::cerr << std::endl;
    }
    std::cerr << std::endl;
}

}
