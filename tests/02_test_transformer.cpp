// tests/unit/test_transformer.cpp
//
// 範囲: ast::Transformer ― 低レベル 1 ステップ操作の検証
//
// 期待動作
//   1. insert_node   : 親 inputs[index] に追加され parent 参照が付く
//   2. delete_node   : 親‑子リンクが両方向とも解除される
//   3. replace_node  : variant が置換され前値が DiffStep.old に入る
//   4. connect_input / disconnect_input
//   5. reconnect_sub_edge : SubEdge 差し替え
//
// gtest_main と prismcascade ライブラリにリンクしてください
//
#include <gtest/gtest.h>

#include <prismcascade/ast/transformer.hpp>

using namespace prismcascade;
using ast::DiffStep;
using ast::Node;
using ast::Transformer;

TEST(Transformer, InsertAndDelete) {
    Transformer tr;

    auto parent = std::make_shared<Node>();
    auto child  = std::make_shared<Node>();

    /* insert */
    auto ins = tr.insert_node(parent, 0, child);
    EXPECT_EQ(ins.action, DiffStep::Type::InsertNode);
    ASSERT_EQ(parent->inputs.size(), 1u);
    EXPECT_TRUE(std::holds_alternative<std::shared_ptr<Node>>(parent->inputs[0]));
    EXPECT_EQ(std::get<std::shared_ptr<Node>>(parent->inputs[0]), child);
    EXPECT_FALSE(child->parent.expired());

    /* delete */
    auto del = tr.delete_node(child);
    EXPECT_EQ(del.action, DiffStep::Type::DeleteNode);
    EXPECT_TRUE(parent->inputs[0].valueless_by_exception() || std::holds_alternative<std::int64_t>(parent->inputs[0]));
    EXPECT_TRUE(child->parent.expired());
}

TEST(Transformer, ReplaceLiteral) {
    Transformer tr;

    auto n   = std::make_shared<Node>();
    auto old = std::make_shared<Node>();
    tr.insert_node(n, 0, old);

    Node::Input literal = std::int64_t{42};
    auto        rep     = tr.replace_node(n, 0, literal);
    EXPECT_EQ(rep.action, DiffStep::Type::ReplaceNode);
    ASSERT_TRUE(std::holds_alternative<std::int64_t>(n->inputs[0]));
    EXPECT_EQ(std::get<std::int64_t>(n->inputs[0]), 42);
}

TEST(Transformer, ConnectAndDisconnect) {
    Transformer tr;
    auto        p = std::make_shared<Node>();
    auto        c = std::make_shared<Node>();

    /* connect */
    auto st = tr.connect_input(p, 0, c);
    EXPECT_EQ(st.action, DiffStep::Type::ConnectInput);
    EXPECT_EQ(std::get<std::shared_ptr<Node>>(p->inputs[0]), c);

    /* disconnect */
    auto dc = tr.disconnect_input(p, 0);
    EXPECT_EQ(dc.action, DiffStep::Type::DisconnectInput);
    EXPECT_TRUE(p->inputs[0].valueless_by_exception() || std::holds_alternative<std::int64_t>(p->inputs[0]));
}

TEST(Transformer, ReconnectSubEdge) {
    Transformer tr;
    auto        n1 = std::make_shared<Node>();
    auto        n2 = std::make_shared<Node>();

    ast::SubEdge e1{VariableType::Int, n1, 0, n2, 1};
    auto         rs = tr.reconnect_sub_edge(n2, 0, e1);
    EXPECT_EQ(rs.action, DiffStep::Type::ReconnectSubEdge);
    ASSERT_TRUE(std::holds_alternative<ast::SubEdge>(n2->inputs[0]));
    auto got = std::get<ast::SubEdge>(n2->inputs[0]);
    EXPECT_EQ(got.from.lock(), n1);
    EXPECT_EQ(got.to.lock(), n2);
}
