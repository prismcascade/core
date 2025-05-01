#pragma once
#include <cstdint>
#include <memory>
#include <prismcascade/ast/node.hpp>
#include <unordered_map>

namespace prismcascade::scheduler {
// ノード単位の「累積 look-ahead」
using DelayTable = std::unordered_map<std::int64_t, std::int64_t>;  // handler → gdelay(frame)

// ルートから上流方向へ最大 lookAhead を伝搬して求める。
DelayTable build_delay_table(const std::shared_ptr<ast::AstNode>& root);

}  // namespace prismcascade::scheduler
