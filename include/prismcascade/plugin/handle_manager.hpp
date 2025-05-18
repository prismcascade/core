#include <atomic>
#include <string>

namespace prismcascade {
class HandleManager {
   public:
    std::string  fresh_id();
    std::int64_t fresh_handler();

   private:
    std::atomic_int64_t current_id{};
};
}  // namespace prismcascade
