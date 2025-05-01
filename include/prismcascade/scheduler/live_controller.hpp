#pragma once
//---------------------------------------------------------------------------
// ライブ配信ラグ監視 & ドロップ判定
//---------------------------------------------------------------------------
#include <prismcascade/memory/time.hpp>

namespace prismcascade::scheduler {
class live_controller {
   public:
    explicit live_controller(memory::timestamp_t max_lag_frames = 3);

    bool need_drop(memory::timestamp_t next_vid, memory::timestamp_t now, std::size_t root_count);
    void commit(memory::timestamp_t vid, memory::timestamp_t aud);

    memory::timestamp_t lag() const noexcept { return cur_lag_; }

   private:
    memory::timestamp_t max_lag_, cur_lag_{0};
    memory::timestamp_t last_vid_{0}, last_aud_{0};
};
}  // namespace prismcascade::scheduler
