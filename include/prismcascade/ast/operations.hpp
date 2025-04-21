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
#include <prismcascade/ast/transformer.hpp>
#include <vector>

namespace prismcascade::ast {

class Operations {
   public:
    Operations()  = default;
    ~Operations() = default;

    /* -------- 代表的な操作 ---------------------------------- */

    /** ノード parent の inputs[index] に new_effect を追加 */
    Diff add_effect(const std::shared_ptr<Node>& parent, int index, const std::shared_ptr<Node>& new_effect);

    /** target ノードを AST から除去 */
    Diff remove_node(const std::shared_ptr<Node>& target);

    /** 汎用：Diff を適用（Redo） */
    void apply_diff(const Diff& diff);

    /** 汎用：Diff を逆転（Undo）  */
    void revert_diff(const Diff& diff);

   private:
    Transformer transformer_;

    /* 内部ヘルパ : 単一ステップの適用 / 逆適用 */
    void apply_step(const DiffStep& step, bool forward);
    void revert_step(const DiffStep& step) { apply_step(step, /*forward=*/false); }
};

}  // namespace prismcascade::ast
