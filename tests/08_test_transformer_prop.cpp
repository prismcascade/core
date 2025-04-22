// // tests/property/test_transformer_prop.cpp
// #include <gtest/gtest.h>
// #include <rapidcheck/Gen.h>  // map / container / resize
// #include <rapidcheck/gtest.h>

// #include <prismcascade/ast/transformer.hpp>

// using namespace prismcascade;
// using ast::Node;
// using ast::Transformer;

// /* ---------- 自前ジェネレータ（深さ 0‑3） -------------------- */
// namespace {

// rc::Gen<std::shared_ptr<Node>> smallTree(int depth = 0) {
//     using GenNode = rc::Gen<std::shared_ptr<Node>>;

//     // 子どもは最大 2 個、深さが 3 を超えたら葉ノード
//     if (depth >= 3) return rc::gen::map(rc::gen::arbitrary<int>(), [](int) { return std::make_shared<Node>(); });

//     // children vector を生成
//     GenNode childGen = rc::gen::map(rc::gen::arbitrary<int>(), [depth](int) { return *smallTree(depth + 1); });

//     auto vecGen = rc::gen::container<std::vector<std::shared_ptr<Node>>>(childGen);
//     vecGen      = rc::gen::resize(2, vecGen);  // サイズ制限 0‑2

//     return rc::gen::map(vecGen, [depth](auto children) {
//         auto n = std::make_shared<Node>();
//         for (auto& ch : children) {
//             n->inputs.push_back(ch);
//             ch->parent = n;
//         }
//         return n;
//     });
// }

// }  // unnamed namespace

// /* RapidCheck Arbitrary 特化 ------------------------------------ */
// namespace rc {
// template <>
// struct Arbitrary<std::shared_ptr<Node>> {
//     static Gen<std::shared_ptr<Node>> arbitrary() { return smallTree(); }
// };
// }  // namespace rc

// /* 簡易同型判定 -------------------------------------------------- */
// static bool iso(const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b) {
//     if (!a || !b) return !a && !b;
//     if (a->inputs.size() != b->inputs.size()) return false;
//     for (std::size_t i = 0; i < a->inputs.size(); ++i) {
//         if (a->inputs[i].index() != b->inputs[i].index()) return false;
//         if (std::holds_alternative<std::shared_ptr<Node>>(a->inputs[i])) {
//             if (!iso(std::get<std::shared_ptr<Node>>(a->inputs[i]), std::get<std::shared_ptr<Node>>(b->inputs[i])))
//                 return false;
//         }
//     }
//     return true;
// }

// /* ---------- Property 1: insert→delete = identity -------------- */
// RC_GTEST_PROP(TransformerProp, InsertDeleteIdentity, (const std::shared_ptr<Node>& root)) {
//     RC_PRE(root);
//     Transformer tr;

//     auto child    = std::make_shared<Node>();
//     auto snapshot = std::make_shared<Node>(*root);

//     tr.insert_node(root, 0, child);
//     tr.delete_node(child);

//     RC_ASSERT(iso(root, snapshot));
// }

// /* ---------- Property 2: replace literal reversible ------------ */
// RC_GTEST_PROP(TransformerProp, ReplaceLiteralIdentity, (const std::shared_ptr<Node>& root, int v)) {
//     RC_PRE(root);
//     Transformer tr;

//     root->inputs.resize(1);
//     auto before = root->inputs[0];
//     tr.replace_node(root, 0, std::int64_t{v});
//     tr.replace_node(root, 0, before);

//     RC_ASSERT(iso(root->inputs[0].index() == before.index() ? root : nullptr, root));
// }
