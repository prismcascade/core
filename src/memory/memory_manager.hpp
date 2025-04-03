#pragma once
#include <map>
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <mutex>
#include <cstdint>
#include <optional>
#include <core/project_data.hpp>

namespace PrismCascade {

struct DllMemoryManager {
    // handle -> data
    std::map<TextParam*, std::shared_ptr<std::string>> text_instances;
    std::map<VectorParam*, std::shared_ptr<void>> vector_instances;
    std::map<VideoFrame*, std::shared_ptr<std::vector<std::uint8_t>>> video_instances;
    std::map<AudioParam*, std::shared_ptr<std::vector<double>>> audio_instances;  // TODO: 型は後から考える
    // plugin_handler -> type_info
    std::map<std::int64_t, PluginMetaDataInternal> plugin_metadata_instances;
    std::map<std::int64_t, std::array<std::vector<std::tuple<std::string, std::vector<VariableType>>>, 2>> parameter_type_informations;
    // plugin_*instance*_handler -> plugin_handler, [(parameter_adapter, [parameter_buffer])]
    std::map<std::int64_t, std::pair<std::int64_t, std::array<std::pair<std::shared_ptr<Parameter>, std::vector<std::shared_ptr<void>>>, 2>>> parameter_pack_instances;

    bool allocate_param(std::int64_t plugin_handler, bool is_output, VariableType type, const char* name, std::optional<VariableType> type_inner);
    static bool allocate_param_static(void* ptr, std::int64_t plugin_handler, bool is_output, VariableType type, const char* name, ...);

    bool add_required_handler(std::int64_t plugin_handler, const char* effect_name);
    static bool add_required_handler_static(void* ptr, std::int64_t plugin_handler, const char* effect_name);

    bool add_handleable_effect(std::int64_t plugin_handler, const char* effect_name);
    static bool add_handleable_effect_static(void* ptr, std::int64_t plugin_handler, const char* effect_name);

    bool load_video_buffer(VideoFrame* target, std::uint64_t frame);
    static bool load_video_buffer_static(void* ptr, VideoFrame* target, std::uint64_t frame);

    ParameterPack allocate_parameter(std::int64_t plugin_handler, std::int64_t plugin_instance_handler, bool is_output);

    // -------- 受け渡し用コンテナのメモリ解放責任は，ホスト側の (.+)Param を持っている存在が負う -------- //

    bool assign_text(TextParam* buffer, const char* text);
    static bool assign_text_static(void* ptr, TextParam* buffer, const char* text);
    void copy_text(TextParam* dst, TextParam* src);
    void free_text(TextParam* buffer);

    // vector は内容の解放責任を負う
    bool allocate_vector(VectorParam* buffer, int size);
    static bool allocate_vector_static(void* ptr, VectorParam* buffer, int size);
    void copy_vector(VectorParam* dst, VectorParam* src);
    void free_vector(VectorParam* buffer);

    bool allocate_video(VideoFrame* buffer,  VideoMetaData metadata);
    static bool allocate_video_static(void* ptr, VideoFrame* buffer,  VideoMetaData metadata);
    void copy_video(VideoFrame* dst, VideoFrame* src);
    void free_video(VideoFrame* buffer);

    bool allocate_audio(AudioParam* buffer);
    static bool allocate_audio_static(void* ptr, AudioParam* buffer);
    void copy_audio(AudioParam* dst, AudioParam* src);
    void free_audio(AudioParam* buffer);

private:
    std::mutex mutex_;
};

}
