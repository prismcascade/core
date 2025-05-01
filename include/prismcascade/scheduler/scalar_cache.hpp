#pragma once
//---------------------------------------------------------------------------
// スカラー値キャッシュ (key = node_id + stamp)
//   • clear_before(t) で retain_window を実装
//---------------------------------------------------------------------------
#include <prismcascade/memory/time.hpp>
#include <unordered_map>
#include <variant>

namespace prismcascade::scheduler {
using scalar_val = std::variant<std::monostate, std::int64_t, bool, double, std::string>;

struct scalar_key {
    std::int64_t        node;
    memory::timestamp_t stamp;
    bool                operator==(const scalar_key& o) const noexcept { return node == o.node && stamp == o.stamp; }
};
struct scalar_h {
    std::size_t operator()(const scalar_key& k) const noexcept { return k.node ^ std::size_t(k.stamp); }
};

class scalar_cache {
   public:
    void       store(const scalar_key&, scalar_val);
    scalar_val fetch(const scalar_key&) const;
    void       clear_before(memory::timestamp_t t);

   private:
    std::unordered_map<scalar_key, scalar_val, scalar_h> map_;
};
}  // namespace prismcascade::scheduler
