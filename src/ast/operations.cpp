#include <prismcascade/ast/operations.hpp>

using namespace prismcascade::ast;

/* ---------------- コンビニエンス ---------------- */
static Diff emptyDiff() { return {}; }

/* 主要操作は Transformer を呼ばずに空 Diff を返すだけ -------- */
Diff Operations::add_effect(const std::shared_ptr<Node>&, int, const std::shared_ptr<Node>&) { return emptyDiff(); }
Diff Operations::remove_node(const std::shared_ptr<Node>&) { return emptyDiff(); }

/* apply / revert は Diff を無視して no‑op ---------------------- */
void Operations::apply_diff(const Diff&) {}
void Operations::revert_diff(const Diff&) {}
