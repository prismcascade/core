#pragma once
//---------------------------------------------------------------------------
// VideoLRU  : 共有ポインタ VideoFrameMemory を LRU で保持し、
//             RAM 使用量が上限を超えた場合に PNG へスワップ。
//   • key = (node_id , stamp , port)
//   • swap はオプションで無効可
//---------------------------------------------------------------------------
#include <cstdint>
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <prismcascade/memory/types_internal.hpp>
#include <prismcascade/scheduler/time.hpp>
#include <string>
#include <unordered_map>

namespace prismcascade::scheduler {
struct video_cache_key {
    std::int64_t  node_id;
    timestamp_t   stamp;
    std::uint32_t port;

    bool operator==(const video_cache_key& o) const noexcept {
        return node_id == o.node_id && stamp == o.stamp && port == o.port;
    }
};
struct video_key_hash {
    std::size_t operator()(const video_cache_key& k) const noexcept {
        return std::hash<std::int64_t>{}(k.node_id) ^ std::size_t(k.stamp) ^ k.port;
    }
};

class video_cache {
   public:
    struct options {
        std::size_t           max_ram_bytes = 256 * 1024 * 1024;  // 256 MiB
        bool                  enable_swap   = false;
        std::filesystem::path swap_dir      = "./prismcache";
    };

    explicit video_cache(const options& opt = {});
    std::shared_ptr<memory::VideoFrameMemory> fetch(const video_cache_key&);
    void                                      insert(const video_cache_key&, std::shared_ptr<memory::VideoFrameMemory>);
    void                                      drop(const video_cache_key&);
    std::size_t                               ram_bytes() const noexcept { return cur_ram_; }

   private:
    struct entry {
        std::shared_ptr<memory::VideoFrameMemory> mem;
        std::filesystem::path                     swap_path;
        std::list<video_cache_key>::iterator      lru_it;
    };

    void evict_if_needed();
    bool fetch(const video_cache_key& k, std::shared_ptr<memory::VideoFrameMemory>& out);

    options                                                    opt_;
    std::mutex                                                 m_;
    std::unordered_map<video_cache_key, entry, video_key_hash> map_;
    std::list<video_cache_key>                                 lru_;  // back = most-recent
    std::size_t                                                cur_ram_{0};
};
}  // namespace prismcascade::scheduler
