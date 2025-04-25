// ───────────────────────────────────────────────────────────────
//   VideoLRU  – RAM-bounded LRU cache + optional raw-file swap
//     • key = (node_id , stamp , port)
//     • swap file layout = [ VideoMetaData (binary) ][ raw RGBA bytes ]
//     • thread-safe via single mutex
// ───────────────────────────────────────────────────────────────
#include <fstream>
#include <iterator>
#include <mutex>
#include <prismcascade/scheduler/video_cache.hpp>

namespace prismcascade::scheduler {
//--------------------------------------------------------------------
// util: compute byte size of a frame
//--------------------------------------------------------------------
static std::size_t bytes_of(const memory::VideoFrameMemory& m) {
    // RGBA ⇒ size() already reports byte length
    return m.size();
}

//--------------------------------------------------------------------
video_cache::video_cache(const options& o) : opt_(o) {
    if (opt_.enable_swap) std::filesystem::create_directories(opt_.swap_dir);
}

//--------------------------------------------------------------------
std::shared_ptr<memory::VideoFrameMemory> video_cache::fetch(const video_cache_key& k) {
    std::lock_guard lock(m_);

    auto it = map_.find(k);
    if (it == map_.end()) return {};  // miss

    // LRU: move to newest
    lru_.splice(lru_.end(), lru_, it->second.lru_it);

    // In-RAM hit
    if (it->second.mem) return it->second.mem;

    // On-disk hit – read back
    const std::filesystem::path& path = it->second.swap_path;
    if (path.empty()) return {};  // corrupted entry

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return {};  // I/O error → treat as miss

    // read metadata
    VideoMetaData md{};
    ifs.read(reinterpret_cast<char*>(&md), sizeof(md));

    // allocate VideoFrameMemory with correct buffer length
    auto mem = std::make_shared<memory::VideoFrameMemory>();
    mem->update_metadata(md);

    // assert consistency
    ifs.seekg(0, std::ios::end);
    const std::size_t payload = static_cast<std::size_t>(ifs.tellg()) - sizeof(md);
    if (payload != mem->size()) return {};  // corrupted size

    // read pixel bytes
    ifs.seekg(sizeof(md), std::ios::beg);
    for (std::size_t i = 0; i < payload; ++i) ifs.read(reinterpret_cast<char*>(&mem->at(i)), 1);

    // restore in map
    it->second.mem = mem;
    cur_ram_ += bytes_of(*mem);

    return mem;
}

//--------------------------------------------------------------------
void video_cache::insert(const video_cache_key& k, std::shared_ptr<memory::VideoFrameMemory> mem) {
    std::lock_guard lock(m_);

    // map insert / update
    auto [it, fresh] = map_.try_emplace(k);
    if (!fresh) {
        if (it->second.mem)  // replace existing; adjust ram usage
            cur_ram_ -= bytes_of(*it->second.mem);
        lru_.erase(it->second.lru_it);
    }
    lru_.push_back(k);
    it->second = {mem, {}, std::prev(lru_.end())};

    cur_ram_ += bytes_of(*mem);
    evict_if_needed();
}

//--------------------------------------------------------------------
void video_cache::drop(const video_cache_key& k) {
    std::lock_guard lock(m_);

    auto it = map_.find(k);
    if (it == map_.end()) return;

    if (it->second.mem) cur_ram_ -= bytes_of(*it->second.mem);
    if (!it->second.swap_path.empty()) std::filesystem::remove(it->second.swap_path);

    lru_.erase(it->second.lru_it);
    map_.erase(it);
}

//--------------------------------------------------------------------
void video_cache::evict_if_needed() {
    while (cur_ram_ > opt_.max_ram_bytes && !lru_.empty()) {
        const video_cache_key victim = lru_.front();
        lru_.pop_front();
        auto& ent = map_.at(victim);

        if (!ent.mem) continue;  // already swapped or dropped

        if (opt_.enable_swap) {
            // build unique filename
            ent.swap_path =
                opt_.swap_dir
                / (std::to_string(victim.node_id) + "_" + std::to_string(static_cast<unsigned long long>(victim.stamp))
                   + "_" + std::to_string(victim.port) + ".raw");

            std::ofstream ofs(ent.swap_path, std::ios::binary);
            if (!ofs) {  // I/O error → give up swapping, just drop
                cur_ram_ -= bytes_of(*ent.mem);
                ent.mem.reset();
                continue;
            }

            // write metadata header
            const auto& md = ent.mem->metadata();
            ofs.write(reinterpret_cast<const char*>(&md), sizeof(md));

            // write pixel bytes
            for (std::size_t i = 0; i < ent.mem->size(); ++i)
                ofs.write(reinterpret_cast<const char*>(&ent.mem->at(i)), 1);
        }

        // in any case, remove from RAM
        cur_ram_ -= bytes_of(*ent.mem);
        ent.mem.reset();
    }
}

}  // namespace prismcascade::scheduler
