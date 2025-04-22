#pragma once

#include <memory>
#include <prismcascade/ast/edge.hpp>
#include <prismcascade/ast/node.hpp>

namespace prismcascade {

namespace ast {

struct AstDiffStep {
    std::shared_ptr<AstNode> node;       // 影響ノード
    std::uint64_t            index = 0;  // スロット or 出力 index
    AstNode::input_t         old_value;  // Assign/Remove 用
    AstNode::input_t         new_value;  // Assign 用
};

// 部分木もしくは即値を切り離す
std::vector<AstDiffStep> cut(const std::shared_ptr<AstNode>& parent, std::int32_t index);
// 部分木もしくは即値を割り当てる
std::vector<AstDiffStep> assign(const std::shared_ptr<AstNode>& parent, std::int32_t index, AstNode::input_t value);

}  // namespace ast
}  // namespace prismcascade
