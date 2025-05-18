#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <limits>

namespace prismcascade::memory {

// 整数化済み時間座標 （レンダリング時のみ存在）
using timestamp_t = boost::multiprecision::checked_int128_t;

// fps を num/den (例: 30/1, 30000/1001) で保持する
struct rational_fps_t {
    std::uint32_t num{0};  // 分子  (frames)
    std::uint32_t den{1};  // 分母  (seconds)

    constexpr double as_double() const noexcept { return static_cast<double>(num) / static_cast<double>(den); }
};

// 1 フレームあたりの整数ステップ値。
//   step == 0 は 「on-demand スカラー」 を表す特殊値
struct frame_step_t {
    timestamp_t    step{0};
    constexpr bool is_on_demand() const noexcept { return step == 0; }
};

// fps → frame_step_t の変換
// 引数 lcm_den は，すべてのソース fps 分母の LCM
inline frame_step_t to_step(rational_fps_t fps, std::uint64_t lcm_den) noexcept {
    if (fps.num == 0) return {};  // ガード：未設定 → on-demand
    timestamp_t s = timestamp_t(lcm_den) * fps.den / fps.num;
    return {s};
}

static_assert(std::numeric_limits<timestamp_t>::digits >= 127, "timestamp_t width must be ≥ 128 bits");

}  // namespace prismcascade::memory
