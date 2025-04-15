#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <common/types.hpp>

namespace PrismCascade {

extern "C" {

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
// パラメータとして受け渡される型 (int, bool, float はプリミティブ型を使う)

struct VideoMetaData {
    std::uint32_t width{};
    std::uint32_t height{};
    double fps{};
    std::uint64_t total_frames{};
};

struct AudioMetaData {
    std::uint32_t sample_rate{};
    std::uint32_t channels{};
    std::uint64_t total_samples{};
};

// 必ずホスト側がallocして渡し，プラグイン側では大きさの操作は不能
struct VideoFrame {
    VideoMetaData metadata;
    std::uint64_t current_frame{};
    std::uint8_t* frame_buffer = nullptr;  // RGBA
};

struct AudioBuffer {
    AudioMetaData metadata;
    std::uint64_t current_sample{};
    double* buffer = nullptr;
};

struct TextParam {
    std::int32_t size = 0;
    const char* buffer = nullptr;
};

struct VectorParam {
    VariableType element_type{};
    std::int32_t size = 0;
    void* buffer = nullptr;
};

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
// プラグイン受け渡し時の一時的な入れ物

struct Parameter {
    VariableType type{};
    void* value = nullptr;  // VideoFrame*, AudioBuffer*, double*, int*, etc.
};

struct ParameterPack {
    std::int32_t size = 0;
    Parameter* parameters = nullptr;
};

}
}
