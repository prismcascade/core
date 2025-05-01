#pragma once

#include <prismcascade/ast/transform_ast.hpp>
#include <vector>
namespace prismcascade::ast {

/// (1) 新規ノード生成  — AST には未接続
std::shared_ptr<AstNode> make_node(const std::string& plugin_uuid, std::uint64_t plugin_handler,
                                   std::uint64_t plugin_instance_handler);

/// (2) substitute :  parent[slot] ← value
///      value が既に AST 内に居る場合は「安全に移動」
///      引数 allow_cross = true にすると、外部 SubEdge を温存して移動する
std::vector<AstDiffStep> substitute(const std::shared_ptr<ast::AstNode>& parent, std::uint32_t slot,
                                    ast::AstNode::input_t value, bool allow_cross = false);

void redo(const std::vector<AstDiffStep>& diff);
void undo(const std::vector<AstDiffStep>& diff);

}  // namespace prismcascade::ast
