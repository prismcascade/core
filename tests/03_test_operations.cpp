/* ── tests/16_test_operations.cpp ───────────────────────────────
   unit + property tests for ast::substitute / cut / detach_cross_edges
   gTest (+ RapidCheck for 1 case)
 *───────────────────────────────────────────────────────────── */

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <prismcascade/ast/operations.hpp>     // substitute / undo / redo
#include <prismcascade/ast/transform_ast.hpp>  // cut / detach

using namespace prismcascade;

/* 便利ヘルパ -------------------------------------------------- */
static std::shared_ptr<ast::AstNode> make_node(std::uint64_t id) {
    auto n                     = std::make_shared<ast::AstNode>();
    n->plugin_instance_handler = id;
    return n;
}
static bool has_parent(const std::shared_ptr<ast::AstNode>& n, const std::shared_ptr<ast::AstNode>& p) {
    return !n->parent.expired() && n->parent.lock() == p;
}
static bool is_subedge(const std::shared_ptr<ast::AstNode>& n, std::size_t slot,
                       const std::shared_ptr<ast::AstNode>& src, std::uint32_t idx) {
    if (slot >= n->inputs.size()) return false;
    auto se = std::get_if<ast::SubEdge>(&n->inputs[slot]);
    return se && se->output_index == idx && se->source.lock() == src;
}
static bool has_backref(const std::shared_ptr<ast::AstNode>& from, std::uint32_t outIdx,
                        const std::shared_ptr<ast::AstNode>& dst, std::uint32_t slot) {
    for (auto& r : from->sub_output_references)
        if (r.src_index == outIdx && !r.dst_node.expired() && r.dst_node.lock() == dst && r.dst_slot == slot)
            return true;
    return false;
}

/* 1. 孤立ノード → 空スロット --------------------------------- */
TEST(Substitute, Subst_IsolatedNodeToEmpty) {
    auto P = make_node(1);
    P->inputs.resize(1, std::monostate{});
    auto C = make_node(2);

    auto d = ast::substitute(P, 0, C);

    EXPECT_TRUE(has_parent(C, P));
    undo(d);
    EXPECT_FALSE(has_parent(C, P));
    redo(d);
    EXPECT_TRUE(has_parent(C, P));
}

/* 2. 孤立ノード → 埋まったスロット (旧値は即値) -------------- */
TEST(Substitute, Subst_IsolatedNodeToFilled) {
    auto P    = make_node(10);
    P->inputs = {123LL};
    auto C    = make_node(11);

    auto d = ast::substitute(P, 0, C);

    EXPECT_TRUE(has_parent(C, P));
    EXPECT_TRUE(std::holds_alternative<std::monostate>(P->inputs[0]) == false);

    undo(d);
    EXPECT_TRUE(std::holds_alternative<std::int64_t>(P->inputs[0]));
}

/* 3. ツリー内ノードを別場所へ移動 ----------------------------- */
TEST(Substitute, Subst_MoveExistingNode) {
    auto P1 = make_node(20);
    auto P2 = make_node(20);
    P1->inputs.resize(2, std::monostate{});
    P2->inputs.resize(2, std::monostate{});
    auto C = make_node(21);
    ast::substitute(P1, 0, C);  // P[0] = C

    auto d = ast::substitute(P2, 1, C);  // move to slot1

    EXPECT_TRUE(std::holds_alternative<std::monostate>(P2->inputs[0]));
    EXPECT_TRUE(has_parent(C, P2));
    undo(d);
    EXPECT_TRUE(std::holds_alternative<std::shared_ptr<ast::AstNode>>(P1->inputs[0]));
    EXPECT_TRUE(has_parent(C, P1));
}

/* 4. 自分→子孫 で cycle 検出 ---------------------------------- */
TEST(SubstituteError, Error_Subst_MainTreeCycle) {
    auto P = make_node(30);
    P->inputs.resize(1, std::monostate{});
    auto C = make_node(31);
    ast::substitute(P, 0, C);  // P→C

    EXPECT_THROW(ast::substitute(C, 0, P), std::domain_error);
}

/* 5. SubEdge 付け替え ----------------------------------------- */
TEST(Substitute, Subst_SubEdgeSwitch) {
    auto A = make_node(40);
    auto B = make_node(41);
    auto X = make_node(42);
    X->inputs.resize(1, std::monostate{});

    ast::substitute(X, 0, ast::SubEdge{A, 0});
    EXPECT_TRUE(has_backref(A, 0, X, 0));

    ast::substitute(X, 0, ast::SubEdge{B, 0});
    EXPECT_FALSE(has_backref(A, 0, X, 0));
    EXPECT_TRUE(has_backref(B, 0, X, 0));
}

/* 6. SubEdge → 即値 ------------------------------------------- */
TEST(Substitute, Subst_SubEdgeToLiteral) {
    auto A = make_node(50);
    auto X = make_node(51);
    X->inputs.resize(1, std::monostate{});

    ast::substitute(X, 0, ast::SubEdge{A, 0});
    EXPECT_TRUE(has_backref(A, 0, X, 0));

    ast::substitute(X, 0, 777LL);
    EXPECT_FALSE(has_backref(A, 0, X, 0));
}

/* 7. cut 子ノード --------------------------------------------- */
TEST(Cut, Cut_Node_UndoRedo) {
    auto P = make_node(60);
    auto C = make_node(61);
    P->inputs.resize(1, std::monostate{});
    ast::substitute(P, 0, C);

    auto d = ast::cut(P, 0);  // remove C
    EXPECT_FALSE(has_parent(C, P));
    undo(d);
    EXPECT_TRUE(has_parent(C, P));
    redo(d);
    EXPECT_FALSE(has_parent(C, P));
}

/* 8. cut SubEdge ---------------------------------------------- */
TEST(Cut, Cut_SubEdge_UndoRedo) {
    auto A = make_node(70);
    auto X = make_node(71);
    X->inputs.resize(1, std::monostate{});
    ast::substitute(X, 0, ast::SubEdge{A, 0});

    auto d = ast::cut(X, 0);
    EXPECT_FALSE(has_backref(A, 0, X, 0));
    undo(d);
    EXPECT_TRUE(has_backref(A, 0, X, 0));
}

/* 9. detach cross 内外両方向 ---------------------------------- */
TEST(Detach, Detach_CrossEdges) {
    auto A = make_node(80);
    auto X = make_node(81);
    X->inputs.resize(1, std::monostate{});
    auto Y = make_node(82);
    Y->inputs.resize(1, std::monostate{});

    ast::substitute(X, 0, ast::SubEdge{A, 0});  // A→X
    ast::substitute(Y, 0, ast::SubEdge{A, 0});  // A→Y
    ASSERT_TRUE(has_backref(A, 0, X, 0));
    ASSERT_TRUE(has_backref(A, 0, Y, 0));

    auto steps = ast::detach_cross_edges(A);
    EXPECT_FALSE(has_backref(A, 0, X, 0));
    EXPECT_FALSE(has_backref(A, 0, Y, 0));

    undo(steps);
    ASSERT_TRUE(has_backref(A, 0, X, 0));
    ASSERT_TRUE(has_backref(A, 0, Y, 0));
}

/* 10. detach keep inner-inner ---------------------------------- */
TEST(Detach, Detach_KeepInnerInner) {
    auto A = make_node(90);
    auto B = make_node(91);
    auto C = make_node(92);
    A->inputs.resize(1, std::monostate{});
    B->inputs.resize(1, std::monostate{});
    C->inputs.resize(1, std::monostate{});
    ast::substitute(A, 0, B);
    ast::substitute(B, 0, C);
    ast::substitute(C, 0, ast::SubEdge{A, 0});  // A→C (両者 subtree)

    auto steps = ast::detach_cross_edges(A);
    EXPECT_TRUE(has_backref(A, 0, C, 0));  // 残る
}

/* 11. detach keep outer-outer ---------------------------------- */
TEST(Detach, Detach_KeepOuterOuter) {
    auto X = make_node(100);
    auto Y = make_node(101);
    Y->inputs.resize(1, std::monostate{});
    ast::substitute(Y, 0, ast::SubEdge{X, 0});

    auto Z     = make_node(102);  // unrelated root
    auto steps = ast::detach_cross_edges(Z);
    EXPECT_TRUE(has_backref(X, 0, Y, 0));
}

/* 12. detach undo/redo ----------------------------------------- */
TEST(Detach, Detach_UndoRedo) {
    auto A = make_node(110);
    auto X = make_node(111);
    X->inputs.resize(1, std::monostate{});
    ast::substitute(X, 0, ast::SubEdge{A, 0});

    auto steps = ast::detach_cross_edges(A);
    undo(steps);
    EXPECT_TRUE(has_backref(A, 0, X, 0));
    redo(steps);
    EXPECT_FALSE(has_backref(A, 0, X, 0));
}

/* 13. Slot OOB エラー ----------------------------------------- */
TEST(Errors, Error_SlotOOB) {
    auto N = make_node(120);
    EXPECT_THROW(ast::substitute(N, 3, 42LL), std::domain_error);
}

/* 14. Cut 空 slot エラー -------------------------------------- */
TEST(Errors, Error_CutEmpty) {
    auto N = make_node(130);
    N->inputs.resize(1, std::monostate{});
    EXPECT_THROW(ast::cut(N, 0), std::domain_error);
}

/* 15. Detach nullptr ------------------------------------------ */
TEST(Errors, Error_DetachNull) {
    std::shared_ptr<ast::AstNode> np;
    EXPECT_THROW(ast::detach_cross_edges(np), std::domain_error);
}

/* 16. property: substitute は involution ----------------------- */
RC_GTEST_PROP(Property, RC_Subst_Involution, ()) {
    auto P = make_node(2000);
    P->inputs.resize(1, std::monostate{});

    auto v1 = *rc::gen::arbitrary<std::int64_t>();
    auto v2 = *rc::gen::arbitrary<std::int64_t>();

    auto d1 = ast::substitute(P, 0, v1);
    auto d2 = ast::substitute(P, 0, v2);

    undo(d2);
    undo(d1);

    RC_ASSERT(P->inputs[0].index() == 0);  // monostate に戻る
}

// -------------------------------- //

/* ------------------------------------------------------------------
   helper
 *------------------------------------------------------------------ */
std::shared_ptr<ast::AstNode> mk(std::uint64_t id) {
    auto n                     = std::make_shared<ast::AstNode>();
    n->plugin_instance_handler = id;
    return n;
}

bool inputIsNode(const std::shared_ptr<ast::AstNode>& p, std::size_t idx, const std::shared_ptr<ast::AstNode>& c) {
    if (idx >= p->inputs.size()) return false;
    auto ptr = std::get_if<std::shared_ptr<ast::AstNode>>(&p->inputs[idx]);
    return ptr && *ptr == c;
}

/* ------------------------------------------------------------------
   1. allow_cross = false で外部 SubEdge が強制切断
 *------------------------------------------------------------------ */
TEST(OperationAllowCross, CrossEdgesPrunedIfFalse) {
    auto A = mk(1);  // 移動元
    auto P = mk(2);
    P->inputs.resize(1, std::monostate{});
    auto Q = mk(3);
    Q->inputs.resize(1, std::monostate{});

    /* A を P[0] に接続しておく */
    ast::substitute(P, 0, A, /*allow_cross=*/false);

    /* 外部ノード R が A の副出力を掴んでいる状態 */
    auto R = mk(4);
    R->inputs.resize(1, std::monostate{});
    ast::substitute(R, 0, ast::SubEdge{A, 0}, true /*外部からは温存*/);
    ASSERT_TRUE(R->inputs[0].index() != 0);  // SubEdge であること確認

    /* A を Q[0] へ move（allow_cross=false）*/
    auto diff = ast::substitute(Q, 0, A, /*allow_cross=*/false);

    /* R の SubEdge は切られているはず */
    EXPECT_TRUE(std::holds_alternative<std::monostate>(R->inputs[0]));

    /* undo → R の接続が戻るか？ */
    for (auto it = diff.rbegin(); it != diff.rend(); ++it) ast::assign(it->node, it->index, it->old_value);

    EXPECT_FALSE(std::holds_alternative<std::monostate>(R->inputs[0]));
}

/* ------------------------------------------------------------------
   2. allow_cross = true で外部 SubEdge を残す
 *------------------------------------------------------------------ */
TEST(OperationAllowCross, CrossEdgesKeptIfTrue) {
    auto A = mk(10);
    auto P = mk(11);
    P->inputs.resize(1, std::monostate{});
    auto S = mk(12);
    S->inputs.resize(1, std::monostate{});

    ast::substitute(S, 0, ast::SubEdge{A, 0}, true);

    /* move A under P だが cross を維持 */
    ast::substitute(P, 0, A, /*allow_cross=*/true);

    EXPECT_FALSE(std::holds_alternative<std::monostate>(S->inputs[0]));  // まだ SubEdge
}

/* ------------------------------------------------------------------
   3. 3 段ネストツリーで付替え + undo/redo
 *------------------------------------------------------------------ */
TEST(OperationMoveDeep, TripleNestMoveUndoRedo) {
    auto R = mk(20);
    auto A = mk(21);
    auto B = mk(22);
    auto C = mk(23);

    R->inputs.resize(1, std::monostate{});
    A->inputs.resize(1, std::monostate{});
    B->inputs.resize(1, std::monostate{});

    ast::substitute(R, 0, A);
    ast::substitute(A, 0, B);
    ast::substitute(B, 0, C);  // R→A→B→C となる

    /* C を R 直下に移動 */
    auto diff = ast::substitute(R, 0, C, false);

    EXPECT_TRUE(inputIsNode(R, 0, C));
    EXPECT_TRUE(B->inputs[0].index() == 0);  // B のスロットは空

    /* undo → もとに戻る */
    for (auto it = diff.rbegin(); it != diff.rend(); ++it) ast::assign(it->node, it->index, it->old_value);

    EXPECT_TRUE(inputIsNode(B, 0, C));

    /* redo → また R 直下へ */
    for (auto& st : diff) ast::assign(st.node, st.index, st.new_value);
    EXPECT_TRUE(inputIsNode(R, 0, C));
}

/* ------------------------------------------------------------------
   4. 置換先が SubEdge のとき上書きされるか？
 *------------------------------------------------------------------ */
TEST(OperationOverwriteSubEdge, ReplaceSubEdge) {
    auto A = mk(30);
    auto B = mk(31);
    auto P = mk(32);
    P->inputs.resize(1, std::monostate{});
    ast::substitute(P, 0, ast::SubEdge{A, 0}, true);

    /* B を差し込む ⇒ SubEdge 消滅 */
    ast::substitute(P, 0, B);

    EXPECT_TRUE(inputIsNode(P, 0, B));
}

/* ------------------------------------------------------------------
   5. 例外: 自己ループを引き起こす substitute
 *------------------------------------------------------------------ */
TEST(OperationError, SelfLoopBySubstitute) {
    auto P = mk(40);
    P->inputs.resize(1, std::monostate{});
    EXPECT_THROW(ast::substitute(P, 0, P), std::domain_error);
}

/* ------------------------------------------------------------------
   6. 例外：子孫ノードを親へ直接差し込む
 *------------------------------------------------------------------ */
TEST(OperationError, DescendantMoveCreatesCycle) {
    auto R = mk(50);
    auto A = mk(51);
    R->inputs.resize(1, std::monostate{});
    A->inputs.resize(1, std::monostate{});
    ast::substitute(R, 0, A);

    /* R を A の子にしようとする → cycle */
    EXPECT_THROW(ast::substitute(A, 0, R), std::domain_error);
}
