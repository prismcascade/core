// tests/unit/test_operations.cpp
//
// 範囲: ast::Operations ― 高レベル操作の整合性テスト
//
// シナリオ
//   1) add_effect が複数 DiffStep を生成
//   2) apply_diff → revert_diff で AST が元に戻る
//   3) remove_node も同様
//
// Operations が Transformer を内部で呼び出す前提
//
#include <gtest/gtest.h>

#include <prismcascade/ast/operations.hpp>

using namespace prismcascade;
using ast::Node;
using ast::Operations;

namespace {

// AST が構造的に同じかを簡易比較 (親子と variant 種別だけ見る)
bool is_isomorphic(const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b) {
    if (!a && !b) return true;
    if (!a || !b) return false;

    if (a->inputs.size() != b->inputs.size()) return false;

    for (std::size_t i = 0; i < a->inputs.size(); ++i) {
        const auto& ia = a->inputs[i];
        const auto& ib = b->inputs[i];

        if (ia.index() != ib.index()) return false;

        if (std::holds_alternative<std::shared_ptr<Node>>(ia)) {
            if (!is_isomorphic(std::get<std::shared_ptr<Node>>(ia), std::get<std::shared_ptr<Node>>(ib))) return false;
        }
    }
    return true;
}

}  // namespace

TEST(Operations, AddEffectUndoRedo) {
    Operations op;
    auto       parent = std::make_shared<Node>();
    auto       effect = std::make_shared<Node>();

    // original clone
    auto original = std::make_shared<Node>(*parent);

    // add_effect
    auto diff = op.add_effect(parent, 0, effect);
    EXPECT_GE(diff.steps.size(), 1u);

    // diff 適用後、inputs[0] が effect になっている
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Node>>(parent->inputs[0]));
    EXPECT_EQ(std::get<std::shared_ptr<Node>>(parent->inputs[0]), effect);

    // Undo
    op.revert_diff(diff);
    EXPECT_TRUE(is_isomorphic(parent, original));

    // Redo
    op.apply_diff(diff);
    EXPECT_EQ(std::get<std::shared_ptr<Node>>(parent->inputs[0]), effect);
}

TEST(Operations, RemoveNodeUndoRedo) {
    Operations op;
    auto       root  = std::make_shared<Node>();
    auto       child = std::make_shared<Node>();
    root->inputs.push_back(child);

    auto root_snapshot = std::make_shared<Node>(*root);

    // remove
    auto diff = op.remove_node(child);
    EXPECT_GE(diff.steps.size(), 1u);
    EXPECT_TRUE(root->inputs[0].valueless_by_exception() || std::holds_alternative<std::int64_t>(root->inputs[0]));

    // undo
    op.revert_diff(diff);
    EXPECT_TRUE(is_isomorphic(root, root_snapshot));
}
