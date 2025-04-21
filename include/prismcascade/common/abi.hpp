// include/prismcascade/common/abi.hpp
#pragma once
/* ────────────────────────────────────────────────────────────────
   Prism Cascade : C‑ABI shared header
   ---------------------------------------------------------------
   Defines every plain‑C structure, enum, and function typedef
   used between host and plug‑in DLL/SO.
   ─────────────────────────────────────────────────────────────── */
#include <prismcascade/common/types.hpp>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Callback typedefs supplied by host ===================== */
typedef bool (*allocate_param_fn)(void* host, std::int64_t self, bool is_output, prismcascade::VariableType type,
                                  const char* name, ...);

typedef bool (*assign_utf8_fn)(void* host, prismcascade::TextParam* dst, const char* utf8);

typedef bool (*add_required_fn)(void* host, std::int64_t self, const char* effect_name);

typedef bool (*add_handleable_effect_fn)(void* host, std::int64_t self, const char* effect_name);

typedef bool (*load_video_fn)(void* host, prismcascade::VideoFrame* dst, std::uint64_t frame_idx);

typedef bool (*allocate_vector_fn)(void* host, prismcascade::VectorParam* dst, std::int32_t size);

typedef bool (*allocate_video_fn)(void* host, prismcascade::VideoFrame* dst, prismcascade::VideoMetaData md);

typedef bool (*allocate_audio_fn)(void* host, prismcascade::AudioParam* dst);

/* ===== Plugin‑exported function typedefs ====================== */
typedef bool (*fn_getMetaInfo)(void* host, std::int64_t self, prismcascade::PluginMetaData* out_meta,
                               allocate_param_fn allocate_param, assign_utf8_fn assign_utf8,
                               add_required_fn add_required, add_handleable_effect_fn add_handleable);

typedef bool (*fn_onLoadPlugin)(void);
typedef void (*fn_onDestroyPlugin)(void);

typedef bool (*fn_onStartRendering)(void* host, prismcascade::VideoMetaData* est_video,
                                    prismcascade::AudioMetaData* est_audio, prismcascade::ParameterPack* input,
                                    prismcascade::ParameterPack* output, load_video_fn load_video,
                                    assign_utf8_fn assign_utf8, allocate_vector_fn alloc_vec,
                                    allocate_video_fn alloc_vid, allocate_audio_fn alloc_aud);

typedef void (*fn_onFinishRendering)(void);

typedef bool (*fn_renderFrame)(void* host, prismcascade::ParameterPack* input, prismcascade::ParameterPack* output,
                               const prismcascade::VideoMetaData est_video, const prismcascade::AudioMetaData est_audio,
                               std::int64_t frame_idx, load_video_fn load_video, assign_utf8_fn assign_utf8);

typedef bool (*fn_applyMacro)(void);
typedef bool (*fn_handleEffect)(const char* effect_name);

/* Recommended export names (case‑sensitive):
      getMetaInfo
      onLoadPlugin
      onDestroyPlugin
      onStartRendering
      onFinishRendering
      renderFrame
      applyMacro
      handleEffect
   The host resolves them via dlsym()/GetProcAddress.
   ------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
