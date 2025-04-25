#include <algorithm>
#include <prismcascade/scheduler/audio_ring.hpp>
namespace prismcascade::scheduler {
audio_ring::audio_ring(std::size_t c) : buf_(c), cap_(c) {}

std::size_t audio_ring::write(const double* src, std::size_t n, timestamp_t ts) {
    n = std::min(n, cap_ - len_);
    for (std::size_t i = 0; i < n; ++i) buf_[(tail_ + i) % cap_] = src[i];
    tail_ = (tail_ + n) % cap_;
    len_ += n;
    tail_stamp_ = ts;
    return n;
}
std::size_t audio_ring::read(double* dst, std::size_t n) {
    n = std::min(n, len_);
    for (std::size_t i = 0; i < n; ++i) dst[i] = buf_[(head_ + i) % cap_];
    head_ = (head_ + n) % cap_;
    len_ -= n;
    return n;
}
}  // namespace prismcascade::scheduler
