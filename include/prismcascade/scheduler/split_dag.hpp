#pragma once
#include <cstdint>
#include <memory>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/scheduler/delay.hpp>
#include <vector>

namespace prismcascade::scheduler {
struct subdag_info {
    std::shared_ptr<ast::AstNode> root;
    std::uint32_t                 queue_id;
};

/// gdelay が startup_buffer を超えないノードまでを “live DAG” とし、
/// 残りは別 queue 番号を振って返す。
std::vector<subdag_info> split_dag(const std::shared_ptr<ast::AstNode>& root, const DelayTable& delay,
                                   std::uint32_t startup_buffer);

}  // namespace prismcascade::scheduler
