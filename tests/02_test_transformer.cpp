/* ── tests/12_test_transform.cpp ───────────────────────────────
   unit + property tests for ast::assign / cut / detach_cross_edges
   (gTest + RapidCheck)
 *───────────────────────────────────────────────────────────── */

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <prismcascade/ast/node.hpp>
#include <prismcascade/ast/transform_ast.hpp>

using namespace prismcascade;

/* ────────────────────────────── */
/* helpers                        */
/* ────────────────────────────── */
namespace {

std::shared_ptr<ast::AstNode> makeNode(std::uint64_t id) {
    auto n                     = std::make_shared<ast::AstNode>();
    n->plugin_instance_handler = id;
    return n;
}

bool inputIsChild(const std::shared_ptr<ast::AstNode>& parent, std::size_t slot,
                  const std::shared_ptr<ast::AstNode>& child) {
    if (slot >= parent->inputs.size()) return false;
    auto p = std::get_if<std::shared_ptr<ast::AstNode>>(&parent->inputs[slot]);
    return p && *p == child;
}

bool inputIsSubEdge(const std::shared_ptr<ast::AstNode>& n, std::size_t slot, const ast::SubEdge& e) {
    if (slot >= n->inputs.size()) return false;
    auto se = std::get_if<ast::SubEdge>(&n->inputs[slot]);
    if (!se) return false;
    return se->output_index == e.output_index && se->source.lock() == e.source.lock();
}

bool hasBackRef(const std::shared_ptr<ast::AstNode>& from, std::size_t outIdx, const std::shared_ptr<ast::AstNode>& dst,
                std::size_t slot) {
    for (auto& ref : from->sub_output_references)
        if (ref.src_index == outIdx && !ref.dst_node.expired() && ref.dst_node.lock() == dst && ref.dst_slot == slot)
            return true;
    return false;
}

}  // namespace

/* ────────────────────────────── */
/* 1. assign: 親リンク・back-ref  */
/* ────────────────────────────── */
TEST(TransformAssign, ParentAndBackrefsUpdate) {
    auto P = makeNode(1);
    auto C = makeNode(2);
    P->inputs.resize(1, std::monostate{});

    auto diff = ast::assign(P, 0, C);

    ASSERT_TRUE(inputIsChild(P, 0, C));
    EXPECT_EQ(C->parent.lock(), P);

    /* undo */
    for (auto it = diff.rbegin(); it != diff.rend(); ++it) ast::assign(it->node, it->index, it->old_value);
    EXPECT_FALSE(inputIsChild(P, 0, C));
    EXPECT_TRUE(C->parent.expired());
}

TEST(TransformAssign, SubEdgeBackrefUpdate) {
    auto A = makeNode(10);
    auto B = makeNode(11);
    B->inputs.resize(1, std::monostate{});

    auto diff = ast::assign(B, 0, ast::SubEdge{A, 0});
    EXPECT_TRUE(hasBackRef(A, 0, B, 0));

    /* undo */
    for (auto it = diff.rbegin(); it != diff.rend(); ++it) ast::assign(it->node, it->index, it->old_value);
    EXPECT_FALSE(hasBackRef(A, 0, B, 0));
}

/* ────────────────────────────── */
/* 2. cut                          */
/* ────────────────────────────── */
TEST(TransformCut, CutChildRemovesParent) {
    auto P    = makeNode(20);
    auto C    = makeNode(21);
    P->inputs = {C};
    C->parent = P;

    auto diff = ast::cut(P, 0);
    EXPECT_FALSE(inputIsChild(P, 0, C));
    EXPECT_TRUE(C->parent.expired());

    /* redo */
    for (auto& st : diff) ast::assign(st.node, st.index, st.old_value);
    EXPECT_TRUE(inputIsChild(P, 0, C));
    EXPECT_EQ(C->parent.lock(), P);
}

TEST(TransformCut, CutSubEdgeRemovesBackref) {
    auto A = makeNode(30);
    auto B = makeNode(31);
    B->inputs.resize(1, std::monostate{});
    ast::assign(B, 0, ast::SubEdge{A, 0});  // back-ref auto-created

    auto diff = ast::cut(B, 0);
    EXPECT_FALSE(hasBackRef(A, 0, B, 0));
    EXPECT_FALSE(inputIsSubEdge(B, 0, ast::SubEdge{A, 0}));

    /* undo */
    for (auto& st : diff) ast::assign(st.node, st.index, st.old_value);
    EXPECT_TRUE(hasBackRef(A, 0, B, 0));
    EXPECT_TRUE(inputIsSubEdge(B, 0, ast::SubEdge{A, 0}));
}

/* ────────────────────────────── */
/* 3. detach_cross_edges           */
/* ────────────────────────────── */
TEST(TransformDetach, InnerOuterEdgesBothDirections) {
    auto A = makeNode(40);
    auto X = makeNode(41);

    X->inputs.resize(1, std::monostate{});
    ast::assign(X, 0, ast::SubEdge{A, 0});  // A → X

    auto steps = ast::detach_cross_edges(A);
    EXPECT_FALSE(hasBackRef(A, 0, X, 0));
    EXPECT_FALSE(inputIsSubEdge(X, 0, ast::SubEdge{A, 0}));

    /* undo */
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) ast::assign(it->node, it->index, it->old_value);
    EXPECT_TRUE(hasBackRef(A, 0, X, 0));
    EXPECT_TRUE(inputIsSubEdge(X, 0, ast::SubEdge{A, 0}));
}

/* ────────────────────────────── */
/* 4. property: assign involution  */
/* ────────────────────────────── */
RC_GTEST_PROP(TransformProp, AssignIsInvolution, ()) {
    auto P = makeNode(1000);
    P->inputs.resize(1, std::monostate{});

    auto v1 = *rc::gen::arbitrary<std::int64_t>();
    auto v2 = *rc::gen::arbitrary<std::int64_t>();

    auto d1 = ast::assign(P, 0, v1);
    auto d2 = ast::assign(P, 0, v2);

    /* undo d2 then d1 */
    for (auto it = d2.rbegin(); it != d2.rend(); ++it) ast::assign(it->node, it->index, it->old_value);
    for (auto it = d1.rbegin(); it != d1.rend(); ++it) ast::assign(it->node, it->index, it->old_value);

    RC_ASSERT(P->inputs[0].index() == 0);  // monostate
}

/* ────────────────────────────── */
/* 5. subtree detach demo         */
/* ────────────────────────────── */
TEST(TransformCut, CrossEdgeSubtreeDetach) {
    auto R = makeNode(200);
    auto A = makeNode(201);
    auto Y = makeNode(202);  // outside → inside
    auto Z = makeNode(203);  // inside → outside

    R->inputs = {A};
    A->parent = R;

    /* outer → inner */
    Y->inputs.resize(1, std::monostate{});
    ast::assign(Y, 0, ast::SubEdge{A, 0});

    /* inner → outer */
    Z->inputs.resize(1, std::monostate{});
    ast::assign(Z, 0, ast::SubEdge{A, 0});

    auto steps = ast::detach_cross_edges(A);
    for (const auto& st : steps) ast::assign(st.node, st.index, st.new_value);

    EXPECT_FALSE(hasBackRef(A, 0, Z, 0));
    EXPECT_FALSE(hasBackRef(A, 0, Y, 0));
    EXPECT_FALSE(inputIsSubEdge(Y, 0, ast::SubEdge{A, 0}));
    EXPECT_FALSE(inputIsSubEdge(Z, 0, ast::SubEdge{A, 0}));
}

/* ────────────────────────────── */
/* 6. detach should not touch in-in / out-out edges                */
/* ────────────────────────────── */
/* ────────────────────────────────────────────── */
/* 6-a. 内→内 back-ref は detach で保持される   */
/* ────────────────────────────────────────────── */
TEST(DetachNoTouch, InnerToInnerKept) {
    auto A = makeNode(300);  // subtree root
    auto B = makeNode(301);  // 内部ノード
    auto C = makeNode(302);  // 内部ノード (AのSubEdgeを参照)

    /* ── main-tree:  A → B → C ───────────── */
    A->inputs.resize(1, std::monostate{});
    ast::assign(A, 0, B);  // parent(B)=A
    B->inputs.resize(1, std::monostate{});
    ast::assign(B, 0, C);  // parent(C)=B

    /* ── 内→内 SubEdge:  A[0] → C.slot0 ─── */
    C->inputs.resize(1, std::monostate{});
    ast::assign(C, 0, ast::SubEdge{A, 0});  // back-ref 自動生成

    ASSERT_TRUE(hasBackRef(A, 0, C, 0));  // sanity
    ASSERT_TRUE(inputIsSubEdge(C, 0, ast::SubEdge{A, 0}));

    /* detach A-subtree */
    auto steps = ast::detach_cross_edges(A);
    for (auto& s : steps) ast::assign(s.node, s.index, s.new_value);

    /* 内→内 の back-ref は保持されるはず */
    EXPECT_TRUE(hasBackRef(A, 0, C, 0));
    EXPECT_TRUE(inputIsSubEdge(C, 0, ast::SubEdge{A, 0}));

    /* undo も念のため動くか確認 */
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) ast::assign(it->node, it->index, it->old_value);

    EXPECT_TRUE(hasBackRef(A, 0, C, 0));
    EXPECT_TRUE(inputIsSubEdge(C, 0, ast::SubEdge{A, 0}));
}

TEST(DetachNoTouch, OuterToOuterKept) {
    auto X = makeNode(311);
    auto Y = makeNode(312);

    Y->inputs.resize(1, std::monostate{});
    ast::assign(Y, 0, ast::SubEdge{X, 0});  // X → Y

    auto Z     = makeNode(313);  // unrelated subtree
    auto steps = ast::detach_cross_edges(Z);
    for (auto& s : steps) ast::assign(s.node, s.index, s.new_value);

    EXPECT_TRUE(hasBackRef(X, 0, Y, 0));
    EXPECT_TRUE(inputIsSubEdge(Y, 0, ast::SubEdge{X, 0}));
}

/* ────────────────────────────── */
/* 7. detach undo round-trip      */
/* ────────────────────────────── */
TEST(DetachUndo, FullRoundTrip) {
    /* ── 構造 ─────────────────
          A        (subtree root)
          │
          B        (inner child)

          X        (outer consumer; SubEdge A[0])
       ───────────────────────── */

    auto A = makeNode(320);
    auto B = makeNode(321);
    auto X = makeNode(322);

    /* A → B  (main-tree) */
    A->inputs.resize(1, std::monostate{});
    ast::assign(A, 0, B);  // parent linkは assign が設定

    /* X → A  (cross SubEdge) */
    X->inputs.resize(1, std::monostate{});
    ast::assign(X, 0, ast::SubEdge{A, 0});
    ASSERT_TRUE(hasBackRef(A, 0, X, 0));  // sanity

    /* detach A-subtree */
    auto steps = ast::detach_cross_edges(A);
    for (auto& st : steps) ast::assign(st.node, st.index, st.new_value);

    EXPECT_FALSE(hasBackRef(A, 0, X, 0));
    EXPECT_FALSE(inputIsSubEdge(X, 0, ast::SubEdge{A, 0}));

    /* undo 全適用 */
    for (auto it = steps.rbegin(); it != steps.rend(); ++it) ast::assign(it->node, it->index, it->old_value);

    EXPECT_TRUE(hasBackRef(A, 0, X, 0));
    EXPECT_TRUE(inputIsSubEdge(X, 0, ast::SubEdge{A, 0}));
}

/* ────────────────────────────── */
/* 8. assign: SubEdge→別 SubEdge  */
/* ────────────────────────────── */
TEST(AssignSubEdgeToSubEdge, BackrefSwitch) {
    auto A = makeNode(330);
    auto B = makeNode(331);
    auto C = makeNode(332);

    C->inputs.resize(1, std::monostate{});
    ast::assign(C, 0, ast::SubEdge{A, 0});
    EXPECT_TRUE(hasBackRef(A, 0, C, 0));
    EXPECT_FALSE(hasBackRef(B, 0, C, 0));

    ast::assign(C, 0, ast::SubEdge{B, 0});
    EXPECT_FALSE(hasBackRef(A, 0, C, 0));
    EXPECT_TRUE(hasBackRef(B, 0, C, 0));
}

/* ────────────────────────────── */
/* 9. error handling              */
/* ────────────────────────────── */
TEST(AssignError, SlotOutOfRange) {
    auto N = makeNode(400);
    EXPECT_THROW(ast::assign(N, 3, 42), std::domain_error);
}

TEST(AssignError, MainTreeCycle) {
    auto P = makeNode(401);
    auto C = makeNode(402);
    P->inputs.resize(1, std::monostate{});
    ast::assign(P, 0, C);                                   // P → C
    EXPECT_THROW(ast::assign(C, 0, P), std::domain_error);  // would create cycle
}

TEST(CutError, EmptySlot) {
    auto N = makeNode(410);
    N->inputs.resize(1, std::monostate{});
    EXPECT_THROW(ast::cut(N, 0), std::domain_error);
}

TEST(DetachError, Nullptr) {
    std::shared_ptr<ast::AstNode> nullNode;
    EXPECT_THROW(ast::detach_cross_edges(nullNode), std::domain_error);
}

// // tests/unit/test_transformer.cpp
// //
// // 範囲: ast::Transformer ― 低レベル 1 ステップ操作の検証
// //
// // 期待動作
// //   1. insert_node   : 親 inputs[index] に追加され parent 参照が付く
// //   2. delete_node   : 親‑子リンクが両方向とも解除される
// //   3. replace_node  : variant が置換され前値が DiffStep.old に入る
// //   4. connect_input / disconnect_input
// //   5. reconnect_sub_edge : SubEdge 差し替え
// //
// // gtest_main と prismcascade ライブラリにリンクしてください
// //
// #include <gtest/gtest.h>

// #include <prismcascade/ast/transformer.hpp>

// using namespace prismcascade;
// using ast::DiffStep;
// using ast::Node;
// using ast::Transformer;

// TEST(Transformer, InsertAndDelete) {
//     Transformer tr;

//     auto parent = std::make_shared<Node>();
//     auto child  = std::make_shared<Node>();

//     /* insert */
//     auto ins = tr.insert_node(parent, 0, child);
//     EXPECT_EQ(ins.action, DiffStep::Type::InsertNode);
//     ASSERT_EQ(parent->inputs.size(), 1u);
//     EXPECT_TRUE(std::holds_alternative<std::shared_ptr<Node>>(parent->inputs[0]));
//     EXPECT_EQ(std::get<std::shared_ptr<Node>>(parent->inputs[0]), child);
//     EXPECT_FALSE(child->parent.expired());

//     /* delete */
//     auto del = tr.delete_node(child);
//     EXPECT_EQ(del.action, DiffStep::Type::DeleteNode);
//     EXPECT_TRUE(parent->inputs[0].valueless_by_exception() ||
//     std::holds_alternative<std::int64_t>(parent->inputs[0])); EXPECT_TRUE(child->parent.expired());
// }

// TEST(Transformer, ReplaceLiteral) {
//     Transformer tr;

//     auto n   = std::make_shared<Node>();
//     auto old = std::make_shared<Node>();
//     tr.insert_node(n, 0, old);

//     Node::Input literal = std::int64_t{42};
//     auto        rep     = tr.replace_node(n, 0, literal);
//     EXPECT_EQ(rep.action, DiffStep::Type::ReplaceNode);
//     ASSERT_TRUE(std::holds_alternative<std::int64_t>(n->inputs[0]));
//     EXPECT_EQ(std::get<std::int64_t>(n->inputs[0]), 42);
// }

// TEST(Transformer, ConnectAndDisconnect) {
//     Transformer tr;
//     auto        p = std::make_shared<Node>();
//     auto        c = std::make_shared<Node>();

//     /* connect */
//     auto st = tr.connect_input(p, 0, c);
//     EXPECT_EQ(st.action, DiffStep::Type::ConnectInput);
//     EXPECT_EQ(std::get<std::shared_ptr<Node>>(p->inputs[0]), c);

//     /* disconnect */
//     auto dc = tr.disconnect_input(p, 0);
//     EXPECT_EQ(dc.action, DiffStep::Type::DisconnectInput);
//     EXPECT_TRUE(p->inputs[0].valueless_by_exception() || std::holds_alternative<std::int64_t>(p->inputs[0]));
// }

// TEST(Transformer, ReconnectSubEdge) {
//     Transformer tr;
//     auto        n1 = std::make_shared<Node>();
//     auto        n2 = std::make_shared<Node>();

//     ast::SubEdge e1{VariableType::Int, n1, 0, n2, 1};
//     auto         rs = tr.reconnect_sub_edge(n2, 0, e1);
//     EXPECT_EQ(rs.action, DiffStep::Type::ReconnectSubEdge);
//     ASSERT_TRUE(std::holds_alternative<ast::SubEdge>(n2->inputs[0]));
//     auto got = std::get<ast::SubEdge>(n2->inputs[0]);
//     EXPECT_EQ(got.from.lock(), n1);
//     EXPECT_EQ(got.to.lock(), n2);
// }
