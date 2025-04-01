#if defined(_WIN32) || defined(_WIN64)
#define _IS_WINDOWS_BUILD
#endif

#ifdef _IS_WINDOWS_BUILD
    #define EXPORT __declspec(dllexport)
    #define API_CALL __cdecl
#else
    #define EXPORT __attribute__((visibility("default")))
    #define API_CALL
#endif

#include <string>
#include "project_data.hpp"

extern "C" {

// input/output のパラメータ一覧と型, requiredEffect, handlableEffect 等を返す
EXPORT bool API_CALL getMetaInfo(
    int plugin_handler,
    PluginMetaData metadata,
    bool(*allocate_param)(int plugin_handler, bool is_output, VariableType type, const char* name, void* metadata),
    bool(*assign_text)(TextParam* buffer, const char* text),
    bool(*allocate_vector)(VectorParam* buffer, VariableType type, const char* text),
    bool(*add_required_handler)(int plugin_handler, const char* effect_name),
    bool(*add_handleable_effect)(int plugin_handler, const char* effect_name)) {
    return true;
}

EXPORT bool API_CALL onLoadPlugin() {
    return true;
}

EXPORT void API_CALL onDestroyPlugin() {
}

// レンダリング開始時に1回呼ばれ，出力するビデオクリップの長さ等を出力する （出力が video であるエフェクトのみ）
// video が引数に含まれるとき， load_buffer コールバックにそのポインタと要求フレームを送ると，そこのフレームバッファがセットされる。
// 複数のビデオクリップに関する処理が混ざって流れてくる可能性もあるため，バッファの用意等の副作用は非推奨。
EXPORT bool API_CALL onStartRendering(
    VideoMetaData* estimated_meta_data,
    ParameterPack* input,
    bool(*load_video_buffer)(VideoFrame* target, std::uint64_t size),
    bool(*assign_text)(TextParam* buffer, const char* text),
    bool(*allocate_vector)(VectorParam* buffer, VariableType type, const char* text)) {
    // TODO: set: VideoMetaData
    return true;
}

EXPORT void API_CALL onFinishRendering() {
}

// 毎フレーム呼ばれる。
// レンダリング開始時に推定した長さ等をもとに，順次呼ばれる。
// video が引数に含まれるとき， load_buffer コールバックにそのポインタと要求フレームを送ると，そこのフレームバッファがセットされる。
EXPORT bool API_CALL renderFrame(
    ParameterPack* input,
    ParameterPack* output,
    const VideoMetaData estimated_meta_data,
    int frame,
    bool(*load_video_buffer)(VideoFrame*, std::uint64_t),
    bool(*assign_text)(TextParam* buffer, const char* text)) {
    return true;
}

// effectを発生させるもの
EXPORT bool API_CALL applyMacro(/* TODO: outputの方法を考える */) {
    return true;
}
// effectのhandleをするもの
EXPORT bool API_CALL handleEffect(const char* effect_name /* TODO: outputの方法を考える */) {
    return true;
}


}
