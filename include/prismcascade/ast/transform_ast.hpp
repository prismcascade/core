#pragma once

#include <memory>
#include <prismcascade/ast/node.hpp>

namespace prismcascade {
namespace ast {
namespace inner_transform {

struct AstDiffStep {
    std::shared_ptr<AstNode> node;       // 影響ノード
    std::uint64_t            index = 0;  // スロット or 出力 index
    AstNode::input_t         old_value;  // Assign/Remove 用
    AstNode::input_t         new_value;  // Assign 用
};

// 部分木もしくは即値を切り離す
std::vector<AstDiffStep> cut(const std::shared_ptr<AstNode>& parent, std::uint64_t index);
// 部分木もしくは即値を割り当てる
std::vector<AstDiffStep> assign(const std::shared_ptr<AstNode>& parent, std::uint64_t index, AstNode::input_t value);
// subtree境界をまたぐ sub_edge を全て切断する
std::vector<AstDiffStep> detach_cross_edges(const std::shared_ptr<ast::AstNode>& sub_root);

}
}
}
