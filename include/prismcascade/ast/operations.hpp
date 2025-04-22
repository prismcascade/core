// include/prismcascade/ast/operations.hpp
#pragma once
/**
 *  Prism Cascade – Ast::Operations
 *  --------------------------------
 *  高レベル編集 API
 *  ・ユーザが GUI／スクリプトで行う “1 回の操作” をまとめて実行し
 *  ・その差分を `Diff` として返す
 *  ・`apply_diff()` / `revert_diff()` で Redo / Undo を実現
 *
 *  低レベル 1 ステップは ast/transformer.hpp の Transformer が担当し、
 *  Operations は “複数ステップを 1 操作へ束ねる” レイヤ。
 */
#include <prismcascade/ast/diff.hpp>
#include <prismcascade/ast/transform_ast.hpp>
#include <vector>

namespace prismcascade::ast {

/// 複合差分
struct AstDiff {
    std::vector<AstDiffStep> steps;
};

/// apply / revert は **transform_ast** レイヤを呼び出して実行
void apply_diff(const AstDiff& diff);
void revert_diff(const AstDiff& diff);

/// ── UI 用複合コマンド ─────────────────────
/// MOVE (subtree) :  cut + assign
AstDiff move_subtree(const std::shared_ptr<AstNode>& srcParent, std::uint32_t srcSlot,
                     const std::shared_ptr<AstNode>& dstParent, std::uint32_t dstSlot);

/// REPLACE :  cut(old) + assign(new)
AstDiff replace_node(const std::shared_ptr<AstNode>& parent, std::uint32_t slot,
                     const std::shared_ptr<AstNode>& newNode);

/// INSERT literal (0, "", etc.) ※ monostate は別
AstDiff set_literal(const std::shared_ptr<AstNode>& parent, std::uint32_t slot, AstNode::input_t literal);

/// サブツリー複製 (deep‑clone) → 挿入
AstDiff clone_and_graft(const std::shared_ptr<AstNode>& srcSubtree, const std::shared_ptr<AstNode>& dstParent,
                        std::uint32_t dstSlot);

/// SubEdge 付け替え (UI で線をドラッグ)
AstDiff rebind_subedge(const SubEdge& oldEdge, const std::shared_ptr<AstNode>& newFrom, std::uint32_t newOut);

}  // namespace prismcascade::ast
