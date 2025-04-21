// include/prismcascade/ast/transformer.hpp
#pragma once
/**
 *  Prism Cascade – Ast::Transformer
 *  --------------------------------
 *  AST を破壊的に変形する **低レベル API**。
 *  ・Undo/Redo を意識せず 1 操作＝1 DiffStep を返すだけ
 *  ・高レベル操作は ast/operations.hpp でラップする
 */
#include <prismcascade/ast/diff.hpp>
#include <prismcascade/ast/edge.hpp>
#include <prismcascade/ast/node.hpp>

namespace prismcascade::ast {

/**
 *  Transformer
 *  ------------
 *  * insert_node        : parent->inputs[index] に新ノードを挿入
 *  * delete_node        : target ノードを木から切り離す
 *  * replace_node       : 親の入力スロットを別値に置換
 *  * connect_input      : 入力スロットに子ノード／SubEdge を接続
 *  * disconnect_input   : 入力スロットを空値にする
 *  * reconnect_sub_edge : SubEdge を差し替える
 *
 *  いずれも “**成功時のみ** DiffStep を返す”。
 *  エラーは std::runtime_error で通知。
 */
class Transformer {
   public:
    Transformer()  = default;
    ~Transformer() = default;

    DiffStep insert_node(const std::shared_ptr<Node>& parent, int index, std::shared_ptr<Node> new_node);

    DiffStep delete_node(std::shared_ptr<Node> target);

    DiffStep replace_node(const std::shared_ptr<Node>& parent, int index, Node::Input new_value);

    DiffStep connect_input(std::shared_ptr<Node> parent, int index, Node::Input child);

    DiffStep disconnect_input(std::shared_ptr<Node> parent, int index);

    DiffStep reconnect_sub_edge(std::shared_ptr<Node> node, int index, SubEdge new_edge);

   private:
    /* 親子参照更新のヘルパ */
    static void remove_parent_ref(const std::shared_ptr<Node>& child, const std::shared_ptr<Node>& parent);

    static void add_parent_ref(const std::shared_ptr<Node>& child, const std::shared_ptr<Node>& parent);
};

}  // namespace prismcascade::ast
