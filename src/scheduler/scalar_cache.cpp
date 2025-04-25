#include <prismcascade/scheduler/scalar_cache.hpp>
namespace prismcascade::scheduler {
void       scalar_cache::store(const scalar_key& k, scalar_val v) { map_[k] = std::move(v); }
scalar_val scalar_cache::fetch(const scalar_key& k) const {
    auto it = map_.find(k);
    return (it == map_.end()) ? scalar_val{} : it->second;
}
void scalar_cache::clear_before(timestamp_t t) {
    for (auto it = map_.begin(); it != map_.end();)
        if (it->first.stamp < t)
            it = map_.erase(it);
        else
            ++it;
}
}  // namespace prismcascade::scheduler
