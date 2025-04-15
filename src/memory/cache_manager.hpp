#pragma once
#include <mutex>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <optional>
#include <plugin/data_types.hpp>
#include "memory_allocator.hpp"

namespace PrismCascade {

// TODO: 以下の宣言を確認

class CacheManager {
public:
    CacheManager(std::shared_ptr<MemoryAllocator> allocator);
    ~CacheManager();

    // 例: 映像フレームをキャッシュして取り出す
    bool store_video_frame(std::uint64_t key, const VideoFrame& frame);
    std::optional<VideoFrame> retrieve_video_frame(std::uint64_t key);

    // Dirty フラグ管理など
    void mark_as_dirty(std::uint64_t key);
    bool is_dirty(std::uint64_t key) const;
    void clear_dirty(std::uint64_t key);

    // メモリ確保ラッパ
    void* allocate(std::size_t size);
    void  deallocate(void* ptr);

private:
    std::shared_ptr<MemoryAllocator> allocator_;

    struct VideoFrameCache {
        VideoFrame frame;  // deep copy or buffer pointer
        bool dirty = false;
    };

    std::map<std::uint64_t, VideoFrameCache> video_map_;
    mutable std::mutex mutex_;
};

}
