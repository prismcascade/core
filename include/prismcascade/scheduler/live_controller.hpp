#pragma once
//---------------------------------------------------------------------------
// ライブ配信ラグ監視 & ドロップ判定
//---------------------------------------------------------------------------
#include <prismcascade/scheduler/time.hpp>

namespace prismcascade::scheduler {
class live_controller {
   public:
    explicit live_controller(timestamp_t max_lag_frames = 3);

    bool need_drop(timestamp_t next_vid, timestamp_t now, std::size_t root_count);
    void commit(timestamp_t vid, timestamp_t aud);

    timestamp_t lag() const noexcept { return cur_lag_; }

   private:
    timestamp_t max_lag_, cur_lag_{0};
    timestamp_t last_vid_{0}, last_aud_{0};
};
}  // namespace prismcascade::scheduler
