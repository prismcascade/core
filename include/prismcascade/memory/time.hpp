#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <limits>

namespace prismcascade::memory {

/* 128-bit 符号無し整数を “時間座標” として使う。
   Boost の checked_int128_t は
     – GCC/Clang, MSVC いずれでもサポート
     – 加算・乗算でオーバーフロー検出
 */
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
    timestamp_t step{0};

    constexpr bool is_on_demand() const noexcept { return step == 0; }
};

/* fps → frame_step_t への変換。
   引数 lcm_den は，すべてのソース fps 分母の LCM
       step = lcm_den / fps.num * fps.den
   が必ず整数になるため，割り算は最後に 1 回だけ。                                           */
inline frame_step_t to_step(rational_fps_t fps, std::uint64_t lcm_den) noexcept {
    if (fps.num == 0) return {};  // ガード：未設定 → on-demand
    timestamp_t s = timestamp_t(lcm_den) * fps.den / fps.num;
    return {s};
}

/* ─────────────────────────────────────────────── */
/* 形式的性質メモ                                   */
/*   1) timestamp_t は 128-bit (checked) 整数なので     */
/*      32-bit 同士の LCM×分母×倍率 (≦2^96) が安全。   */
/*   2) step 変換は符号無し整数演算のみ → 量子化誤差ゼロ */
/*   3) step==0 が on-demand の一意表現で衝突しない。   */
/* ─────────────────────────────────────────────── */
static_assert(std::numeric_limits<timestamp_t>::digits >= 127, "timestamp_t width must be ≥ 128 bits");

}  // namespace prismcascade::memory
