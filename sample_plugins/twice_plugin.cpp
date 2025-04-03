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
#include <cassert>
#include <iostream>
#include <cstdint>

using namespace PrismCascade;

extern "C" {

// input/output のパラメータ一覧と型, requiredEffect, handleableEffect 等を返す
EXPORT bool API_CALL getMetaInfo(
    void* host_handler,
    std::int64_t plugin_handler,
    PluginMetaData* metadata,
    bool(*allocate_param)(void* host_handler, std::int64_t plugin_handler, bool is_output, VariableType type, const char* name, ...),
    bool(*assign_text)(void* host_handler, TextParam* buffer, const char* text),
    bool(*add_required_handler)(void* host_handler, std::int64_t plugin_handler, const char* effect_name),
    bool(*add_handleable_effect)(void* host_handler, std::int64_t plugin_handler, const char* effect_name)) {
        allocate_param(host_handler, plugin_handler, false, VariableType::Int, "入力");
        allocate_param(host_handler, plugin_handler, true,  VariableType::Vector, "ベクタ出力", VariableType::Int);
        allocate_param(host_handler, plugin_handler, true,  VariableType::Int, "出力");
        add_handleable_effect(host_handler, plugin_handler, "twice_effect");
        metadata->protocol_version = 1;
        metadata->type = PluginType::Effect;
        assign_text(host_handler, &metadata->uuid, "f0000000-0000-0000-0000-000000000000");
        assign_text(host_handler, &metadata->name, "The greatest twice plugin");
    return true;
}

EXPORT bool API_CALL onLoadPlugin() {
    std::cerr << "plugin loaded" << std::endl;
    return true;
}

EXPORT void API_CALL onDestroyPlugin() {
    std::cerr << "plugin destroyed" << std::endl;
}

// レンダリング開始時に1回呼ばれ，出力するビデオ/オーディオクリップやの長さ等を出力する （メイン出力が video であるエフェクトのみ）
// video が引数に含まれるとき， load_buffer コールバックにそのポインタと要求フレームを送ると，そこのフレームバッファがセットされる。
// 複数のビデオクリップに関する処理が混ざって流れてくる可能性もあるため，バッファの用意等の副作用は非推奨。
// 注意点として，allocate するのは出力のみ (入力は直前の出力のメモリを使いまわすため)
// 出力には allocate 以外の書き込み禁止 (したところで無意味なはず)
EXPORT bool API_CALL onStartRendering(
    void* host_handler,
    VideoMetaData* estimated_video_meta_data,
    AudioMetaData* estimated_audio_meta_data,
    ParameterPack* input,
    ParameterPack* output,
    bool(*load_video_buffer)(void* host_handler, VideoFrame* target, std::uint64_t frame),
    bool(*assign_text)(void* host_handler, TextParam* buffer, const char* text),
    bool(*allocate_vector)(void* host_handler, VectorParam* buffer, int size),
    bool(*allocate_video)(void* host_handler, VideoFrame* buffer,  VideoMetaData metadata),
    bool(*allocate_audio)(void* host_handler, AudioParam* buffer /* TODO: 必要なパラメータを考える */)) {
        allocate_vector(host_handler, reinterpret_cast<VectorParam*>(output->parameters[0].value), 3);
    // Do Nothing
    return true;
}

EXPORT void API_CALL onFinishRendering() {
}

// 毎フレーム呼ばれる。
// レンダリング開始時に推定した長さ等をもとに，順次呼ばれる。
// video が引数に含まれるとき， load_buffer コールバックにそのポインタと要求フレームを送ると，そこのフレームバッファがセットされる。
EXPORT bool API_CALL renderFrame(
    void* host_handler,
    ParameterPack* input,
    ParameterPack* output,
    const VideoMetaData estimated_video_meta_data,
    const AudioMetaData estimated_audio_meta_data,
    int frame,
    bool(*load_video_buffer)(void* host_handler, VideoFrame* target, std::uint64_t frame),
    bool(*assign_text)(void* host_handler, TextParam* buffer, const char* text)) {
        // 一応確認
        assert(input->size == 1);
        Parameter input_num = input->parameters[0];
        assert(input_num.type == VariableType::Int);

        assert(output->size == 2);
        Parameter output_vec = output->parameters[0];
        assert(output_vec.type == VariableType::Vector);
        Parameter output_num = output->parameters[1];
        assert(output_num.type == VariableType::Int);

		// 入力を取得
        int input_int = *reinterpret_cast<int*>(input_num.value);

        // 3, 4, 5 倍
        VectorParam* output_vec_ptr = reinterpret_cast<VectorParam*>(output_vec.value);
		assert(output_vec_ptr->type == VariableType::Int);
        for(int i=0; i<output_vec_ptr->size; ++i){
            reinterpret_cast<int*>(output_vec_ptr->buffer)[i] = input_int * (i + 3);
        }

        // 2倍して返すだけ
        *reinterpret_cast<int*>(output_num.value) = input_int * 2;

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
