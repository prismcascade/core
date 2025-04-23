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

#include <cassert>
#include <cstdint>
#include <iostream>
#include <prismcascade/common/abi.hpp>
#include <prismcascade/common/types.hpp>
#include <string>

using namespace prismcascade;

extern "C" {

EXPORT bool API_CALL hello_world(int t) {
    std::cout << "Hello world: [" << t << "]" << std::endl;
    return true;
}

// // ─────────────────────────────────────────────
// // 1. メタ情報
// // ─────────────────────────────────────────────
// EXPORT bool API_CALL getMetaInfo(void* host, std::int64_t self, PluginMetaData* meta, allocate_param_fn
// allocate_param,
//                                  assign_text_fn           assign_text, add_required_fn /*unused*/,
//                                  add_handleable_effect_fn add_handleable) {
//     allocate_param(host, self, /*is_output=*/false, VariableType::Int, "入力");

//     allocate_param(host, self, true, VariableType::Vector, "ベクタ出力", VariableType::Int);
//     allocate_param(host, self, true, VariableType::Int, "出力");
//     allocate_param(host, self, true, VariableType::Text, "文字出力");

//     add_handleable(host, self, "twice_effect");

//     meta->protocol_version = 1;
//     meta->type             = PluginType::Effect;
//     assign_text(host, &meta->uuid, "f0000000-0000-0000-0000-000000000000");
//     assign_text(host, &meta->name, "The greatest twice plugin");
//     return true;
// }

// // ─────────────────────────────────────────────
// // 2. ライフサイクル
// // ─────────────────────────────────────────────
// EXPORT bool API_CALL onLoadPlugin() {
//     std::cerr << "[twice] loaded\n";
//     return true;
// }
// EXPORT void API_CALL onDestroyPlugin() { std::cerr << "[twice] destroyed\n"; }

// // ─────────────────────────────────────────────
// // 3. レンダリング開始
// // ─────────────────────────────────────────────
// EXPORT bool API_CALL onStartRendering(void* host, VideoMetaData* /*video_est*/, AudioMetaData* /*audio_est*/,
//                                       ParameterPack* /*input*/, ParameterPack*      output, load_video_fn /*unused*/,
//                                       assign_text_fn /*assign*/, allocate_vector_fn alloc_vec,
//                                       allocate_video_fn /*unused*/, allocate_audio_fn /*unused*/) {
//     // vector<Int> 出力だけ事前確保 (サイズ3)
//     alloc_vec(host, reinterpret_cast<VectorParam*>(output->parameters[0].value), 3);
//     return true;
// }

// EXPORT void API_CALL onFinishRendering() {}

// // ─────────────────────────────────────────────
// // 4. 毎フレーム処理
// // ─────────────────────────────────────────────
// EXPORT bool API_CALL renderFrame(void* host, ParameterPack* input, ParameterPack* output,
//                                  const VideoMetaData /*video_meta*/, const AudioMetaData /*audio_meta*/,
//                                  std::int64_t /*frame*/, load_video_fn /*unused*/, assign_text_fn assign) {
//     // 入力
//     assert(input->size == 1);
//     std::int64_t in_val = *static_cast<std::int64_t*>(input->parameters[0].value);

//     // vector 出力 (3,4,5 倍)
//     auto* vec = reinterpret_cast<VectorParam*>(output->parameters[0].value);
//     auto* buf = static_cast<std::int64_t*>(vec->buffer);
//     buf[0]    = in_val * 3;
//     buf[1]    = in_val * 4;
//     buf[2]    = in_val * 5;

//     // 整数出力 (×2)
//     *static_cast<std::int64_t*>(output->parameters[1].value) = in_val * 2;

//     // 文字列出力 (×10)
//     std::string txt = std::to_string(in_val * 10);
//     assign(host, reinterpret_cast<TextParam*>(output->parameters[2].value), txt.c_str());
//     return true;
// }

// // ─────────────────────────────────────────────
// // 5. Macro/Handler (今回はダミー)
// // ─────────────────────────────────────────────
// EXPORT bool API_CALL applyMacro() { return true; }
// EXPORT bool API_CALL handleEffect(const char*) { return true; }
}
