/* ── tests/12_test_transform.cpp ───────────────────────────────
   unit + property tests for ast::assign / cut / detach_cross_edges
   (gTest + RapidCheck)
 *───────────────────────────────────────────────────────────── */

#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <prismcascade/ast/node.hpp>
#include <prismcascade/ast/transform_ast.hpp>
#include <queue>

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

    ast::detach_cross_edges(A);

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

    auto Z = makeNode(313);  // unrelated subtree
    ast::detach_cross_edges(Z);

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
    C->inputs.resize(1, std::monostate{});
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

namespace {

/*  全ノードが “親ポインタを辿ったときに循環を持たない” かを検証 ───────── */
bool forestIsCycleFree(const std::vector<std::shared_ptr<ast::AstNode>>& nodes) {
    for (auto& n : nodes) {
        std::unordered_set<ast::AstNode*> seen;
        auto                              cur = n;
        while (cur) {
            if (!seen.insert(cur.get()).second) return false;  // 2 度目 ⇒ cycle
            cur = cur->parent.lock();
        }
    }
    return true;
}

/* 親リンクスナップショット (テスト用) */
std::vector<ast::AstNode*> snapshotParents(const std::vector<std::shared_ptr<ast::AstNode>>& nodes) {
    std::vector<ast::AstNode*> v;
    v.reserve(nodes.size());
    for (auto& n : nodes) v.push_back(n ? n->parent.lock().get() : nullptr);
    return v;
}

}  // namespace

/* ================================================================= */
/* 1. 多スロット独立性                                               */
/* ================================================================= */
TEST(TransformExtended, AssignDifferentSlotDoesNotAffectOthers) {
    auto P = makeNode(1);
    P->inputs.resize(3, std::monostate{});

    auto C1 = makeNode(2);
    auto C2 = makeNode(3);

    ASSERT_NO_THROW(ast::assign(P, 1, C1));
    ASSERT_NO_THROW(ast::assign(P, 2, 42));

    ASSERT_NO_THROW(ast::assign(P, 1, C2));  // slot-1 を差し替え

    // slot-2 はそのまま
    EXPECT_TRUE(std::holds_alternative<std::int64_t>(P->inputs[2]));
    EXPECT_EQ(std::get<std::int64_t>(P->inputs[2]), 42);
}

/* ================================================================= */
/* 2. Variant 全型 round-trip                                        */
/* ================================================================= */
TEST(TransformExtended, VariantAssignRoundTripForAllTypes) {
    auto N = makeNode(10);
    N->inputs.resize(1, std::monostate{});

    auto roundTrip = [&](auto value) {
        auto diff = ast::assign(N, 0, value);
        // undo → monostate に戻るはず
        for (auto it = diff.rbegin(); it != diff.rend(); ++it) ast::assign(it->node, it->index, it->old_value);
        EXPECT_TRUE(std::holds_alternative<std::monostate>(N->inputs[0]));
    };

    roundTrip(std::int64_t{123});
    roundTrip(true);
    roundTrip(3.14);
    roundTrip(std::string{"abc"});
}

/* ================================================================= */
/* 3. ランダム操作列 + 例外安全プロパティ                             */
/* ================================================================= */
RC_GTEST_PROP(TransformExtendedProp, RandomSequenceKeepsForestInvariant, ()) {
    constexpr int kNodes = 20;
    constexpr int kSlots = 3;
    constexpr int kSteps = 60;

    /* ノード集合と初期化 */
    std::vector<std::shared_ptr<ast::AstNode>> nodes;
    for (int i = 0; i < kNodes; ++i) {
        auto n = makeNode(i + 1000);
        n->inputs.resize(kSlots, std::monostate{});
        nodes.push_back(n);
    }

    /* 乱択操作列 */
    for (int step = 0; step < kSteps; ++step) {
        int op    = *rc::gen::inRange(0, 2);  // 0:assign 1:cut
        int n_idx = *rc::gen::inRange(0, kNodes);
        int slot  = *rc::gen::inRange(0, kSlots);

        auto parents_before = snapshotParents(nodes);

        try {
            if (op == 0) {
                int kind = *rc::gen::inRange(0, 3);  // 0:child 1:subedge 2:int
                if (kind == 0) {
                    ast::assign(nodes[n_idx], slot, nodes[*rc::gen::inRange(0, kNodes)]);
                } else if (kind == 1) {
                    auto from = nodes[*rc::gen::inRange(0, kNodes)];
                    ast::assign(nodes[n_idx], slot, ast::SubEdge{from, 0});
                } else {
                    ast::assign(nodes[n_idx], slot, *rc::gen::arbitrary<std::int64_t>());
                }
            } else {
                ast::cut(nodes[n_idx], slot);
            }
        } catch (const std::domain_error&) {
            /* 例外が飛んでも親リンクが壊れていないか比較 */
            RC_ASSERT(parents_before == snapshotParents(nodes));
        }
    }

    RC_ASSERT(forestIsCycleFree(nodes));
}
