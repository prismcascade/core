#pragma once
//---------------------------------------------------------------------------
// 単純 lock-free Audio RingBuffer
//   • tail_stamp() で “最も新しいサンプルのタイムスタンプ” を取得
//---------------------------------------------------------------------------
#include <cstdint>
#include <prismcascade/scheduler/time.hpp>
#include <vector>

namespace prismcascade::scheduler {
class audio_ring {
   public:
    explicit audio_ring(std::size_t capacity_samples = 48000 * 10);
    std::size_t write(const double* src, std::size_t n, timestamp_t ts);
    std::size_t read(double* dst, std::size_t n);
    std::size_t size() const noexcept { return len_; }
    timestamp_t tail_stamp() const noexcept { return tail_stamp_; }

   private:
    std::vector<double> buf_;
    std::size_t         cap_, head_{0}, tail_{0}, len_{0};
    timestamp_t         tail_stamp_{0};
};
}  // namespace prismcascade::scheduler
