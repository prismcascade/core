#pragma once
#include <cstdint>

namespace prismcascade {

extern "C" {

// ── 列挙型 ─────────────────────────────
enum class VariableType { Int, Bool, Float, Text, Vector, Video, Audio };

enum class PluginType {
    Macro,
    Effect,
};

// ── メタデータ ─────────────────────────
struct VideoMetaData {
    std::uint32_t width        = 0;
    std::uint32_t height       = 0;
    double        fps          = 0.0;
    std::uint64_t total_frames = 0;
};

struct AudioMetaData {
    std::uint32_t channels      = 0;
    std::uint64_t total_samples = 0;
};

// 受け渡し用パラメータ

// Int: std::int64_t
// Bool: bool
// Float: double

struct VideoFrame {
    void*         handler{};
    VideoMetaData metadata{};
    std::uint64_t current_frame = 0;
    std::uint8_t* frame_buffer  = nullptr;  // RGBA
};

struct AudioParam {
    void*         handler{};
    AudioMetaData metadata{};
    double*       buffer = nullptr;  // LRLR...
};

struct TextParam {
    void*         handler{};
    std::uint32_t size   = 0;
    const char*   buffer = nullptr;
};

struct VectorParam {
    void*         handler{};
    VariableType  type{};
    std::uint32_t size   = 0;
    void*         buffer = nullptr;
};

// ラッパー
struct PluginMetaData {
    std::int32_t protocol_version = 1;
    PluginType   type;
    TextParam    uuid;
    TextParam    name;
};

struct Parameter {
    VariableType type{};
    void*        value = nullptr;
};

struct ParameterPack {
    std::int32_t size       = 0;
    Parameter*   parameters = nullptr;
};
}

}  // namespace prismcascade
