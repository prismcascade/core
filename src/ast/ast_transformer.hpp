#pragma once
#include <memory>
#include <ast/ast_node.hpp>
#include <ast/ast_diff.hpp>

namespace PrismCascade {

//------------------------------------------
// AST変形の低レベル関数群 (Undo/Redo を考慮しない)
//------------------------------------------
class AstTransformer {
public:
    AstTransformer() = default;
    ~AstTransformer() = default;

    // InsertNode: ノードをAST内に差し込む
    // 例: parent->inputs[index] = newNode
    AstDiffStep insert_node(std::shared_ptr<AstNode> parent,
                           int index,
                           std::shared_ptr<AstNode> new_node);

    // DeleteNode: ノードを切り離し、参照を外す
    AstDiffStep delete_node(std::shared_ptr<AstNode> target);

    // ReplaceNode: parent->inputs[index] を別ノードor別値に差し替え
    AstDiffStep replace_node(std::shared_ptr<AstNode> parent,
                            int index,
                            AstInput new_value);

    AstDiffStep connect_input(std::shared_ptr<AstNode> parent, int index, AstInput child);
    AstDiffStep disconnect_input(std::shared_ptr<AstNode> parent, int index);

    AstDiffStep reconnect_sub_edge(std::shared_ptr<AstNode> node, int index, SubEdge new_edge);

private:
    void remove_parent_ref(std::shared_ptr<AstNode> child, std::shared_ptr<AstNode> parent);
    void add_parent_ref(std::shared_ptr<AstNode> child, std::shared_ptr<AstNode> parent);
};

}
