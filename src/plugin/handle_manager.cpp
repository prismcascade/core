#include <prismcascade/plugin/handle_manager.hpp>

namespace prismcascade {

std::string HandleManager::fresh_id() {
    char buffer[15]{};
    snprintf(buffer, std::size(buffer), "%012ld", static_cast<long>(current_id++));
    return {buffer};
    // return std::format("{:012d}", current_id++);  // C++20
}

std::int64_t HandleManager::fresh_handler() { return current_id++; }

}  // namespace prismcascade
