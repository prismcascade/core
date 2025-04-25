#include <prismcascade/scheduler/live_controller.hpp>
namespace prismcascade::scheduler {
live_controller::live_controller(timestamp_t maxl) : max_lag_(maxl) {}

bool live_controller::need_drop(timestamp_t next_vid, timestamp_t now, std::size_t root_cnt) {
    cur_lag_ = now - next_vid;
    return (cur_lag_ > max_lag_) && (root_cnt > 1);
}
void live_controller::commit(timestamp_t vid, timestamp_t aud) {
    last_vid_ = vid;
    last_aud_ = aud;
}
}  // namespace prismcascade::scheduler
