#pragma once
#include <cstdint>
#include <string>

namespace prismcascade {

extern "C" {

// ── 列挙型 ─────────────────────────────
enum class VariableType { Int, Bool, Float, Text, Vector, Video, Audio };

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

// ── 実データ構造 ───────────────────────
struct VideoFrame {
    VideoMetaData metadata;
    std::uint64_t current_frame = 0;
    std::uint8_t* frame_buffer  = nullptr;  // RGBA
};

struct AudioParam {
    double* buffer = nullptr;  // LRLR...
};

struct TextParam {
    std::int32_t size   = 0;
    const char*  buffer = nullptr;
};

struct VectorParam {
    VariableType type{};
    std::int32_t size   = 0;
    void*        buffer = nullptr;
};

// ── パラメータ渡し用 ───────────────────
struct Parameter {
    VariableType type{};
    void*        value = nullptr;
};

struct ParameterPack {
    std::int32_t size       = 0;
    Parameter*   parameters = nullptr;
};

// ── プラグインのメタデータ ──────────────
enum class PluginType {
    Macro,
    Effect,
};

struct PluginMetaData {
    std::int32_t protocol_version = 1;
    PluginType   type;
    TextParam    uuid;
    TextParam    name;
};
}

std::string to_string(VariableType variable_type);
std::string to_string(PluginType variable_type);

struct PluginMetaDataInternal {
    std::int32_t protocol_version = 1;
    PluginType   type{};
    std::string  uuid;
    std::string  name;
};

}  // namespace prismcascade
