#pragma once
#include <memory>
#include <vector>
#include <ast/ast_node.hpp>
#include <ast/ast_diff.hpp>
#include "ast_transformer.hpp"

namespace PrismCascade {

//-------------------------------------------
// 高レベル操作 (Undo/Redo対応)
//-------------------------------------------
class AstOperations {
public:
    AstOperations();
    ~AstOperations();

    // 例: 1つの“エフェクト追加”操作
    //  -> 低レベル操作(AstTransformer)を複数回呼び、差分をまとめて返す
    AstDiff add_effect(std::shared_ptr<AstNode> parent,
                      int index,
                      std::shared_ptr<AstNode> new_effect);

    // 例: “ノード削除”操作
    AstDiff remove_node(std::shared_ptr<AstNode> node);

    // applyDiff: 実際に差分を適用 (Redo相当)
    void apply_diff(const AstDiff& diff);

    // revertDiff: 差分を逆転 (Undo相当)
    void revert_diff(const AstDiff& diff);

private:
    AstTransformer transformer_;
    // Helper: applySingleStep / revertSingleStep
    void apply_single_step(const AstDiffStep& step);
    void revert_single_step(const AstDiffStep& step);
};

} // namespace PrismCascade
