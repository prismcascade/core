// ── src/transform/transform_ast.cpp ───────────────────────────────
#include <prismcascade/ast/transform_ast.hpp>
#include <stdexcept>
#include <unordered_set>

namespace prismcascade::ast {

namespace {

// --------------- ヘルパ ------------------

// 親 chain 内に target が居るか (O(depth))
bool is_ancestor(const std::shared_ptr<AstNode>& target, const std::shared_ptr<AstNode>& node) {
    for (auto p = node->parent.lock(); p; p = p->parent.lock())
        if (p == target) return true;
    return false;
}

// remove one SubOutputRef ; return true if erased
bool erase_backref(AstNode& from, std::shared_ptr<AstNode> dst, std::uint32_t dst_slot, std::uint32_t from_idx) {
    auto& v  = from.sub_output_references;
    auto  it = std::find_if(v.begin(), v.end(), [&](const AstNode::SubOutputRef& r) {
        return r.src_index == from_idx && !r.dst_node.expired() && r.dst_node.lock() == dst && r.dst_slot == dst_slot;
    });
    if (it != v.end()) {
        v.erase(it);
        return true;
    }
    return false;
}

}  // namespace

// ─────────────────────────────────────────
//  CUT
// ─────────────────────────────────────────
std::vector<AstDiffStep> cut(const std::shared_ptr<AstNode>& parent, std::int32_t slot) {
    if (!parent) throw std::domain_error("cut: parent=nullptr");
    if (slot < 0 || static_cast<std::size_t>(slot) >= parent->inputs.size())
        throw std::domain_error("cut: slot out of range");

    AstNode::input_t old = parent->inputs[slot];
    if (std::holds_alternative<std::monostate>(old)) throw std::domain_error("cut: slot already empty");

    std::vector<AstDiffStep> out;

    // 1) RemoveInput step
    out.push_back(AstDiffStep{
        StepKind::RemoveInput, parent, static_cast<std::uint32_t>(slot),
        old,              // old_value
        std::monostate{}  // new_value
    });
    parent->inputs[slot] = std::monostate{};

    // 2) back‑ref handling
    if (auto child = std::get_if<std::shared_ptr<AstNode>>(&old)) {
        if (*child) (*child)->parent.reset();
        // no SubEdge refs to remove
    } else if (auto sub_edge = std::get_if<SubEdge>(&old)) {
        if (auto from = sub_edge->source.lock()) {
            bool removed = erase_backref(*from, parent, slot, sub_edge->output_index);
            if (removed) {
                out.push_back(AstDiffStep{
                    StepKind::RemoveSubEdgeBackRef, from, sub_edge->output_index, {}, {},  // old/new unused
                });
            }
        }
    }
    return out;
}

// ─────────────────────────────────────────
//  ASSIGN
// ─────────────────────────────────────────
std::vector<AstDiffStep> assign(const std::shared_ptr<AstNode>& parent, std::uint64_t slot, AstNode::input_t value) {
    if (!parent) throw std::domain_error("assign: parent=nullptr");
    if (slot < 0 || static_cast<std::size_t>(slot) >= parent->inputs.size())
        throw std::domain_error("assign: slot out of range");

    // ↑ 型チェックはプラグイン側メタに依存→ここでは省略
    // ○ 閉路チェック (main‑tree)
    if (auto child = std::get_if<std::shared_ptr<AstNode>>(&value)) {
        if (*child && is_ancestor(*child, parent)) throw std::domain_error("assign: would create main‑tree cycle");
    }

    // ○ 閉路チェック (簡易 SubEdge)  : from が親 chainに含まれれば閉路
    if (auto se = std::get_if<SubEdge>(&value)) {
        auto from = se->source.lock();
        if (from && is_ancestor(parent, from)) throw std::domain_error("assign: would create sub‑edge cycle");
    }

    std::vector<AstDiffStep> out;
    AstNode::input_t         old = parent->inputs[slot];

    // a) 旧値が SubEdge だった場合、逆参照を外す
    if (auto seOld = std::get_if<SubEdge>(&old)) {
        if (auto oldFrom = seOld->source.lock()) {
            bool removed = erase_backref(*oldFrom, parent, slot, seOld->output_index);
            if (removed) { out.push_back({StepKind::RemoveSubEdgeBackRef, oldFrom, seOld->output_index}); }
        }
    }

    // b) 書き込み
    parent->inputs[slot] = value;
    out.push_back(AstDiffStep{StepKind::AssignInput, parent, slot, old, value});

    // c) 新値が SubEdge なら逆参照を追加
    if (auto seNew = std::get_if<SubEdge>(&value)) {
        if (auto from = seNew->source.lock()) {
            from->sub_output_references.push_back(AstNode::SubOutputRef{seNew->output_index, parent, slot});

            out.push_back({StepKind::AddSubEdgeBackRef, from, seNew->output_index});
        }
    }

    // d) 新値が shared_ptr child ⇒ 親リンク更新
    if (auto child = std::get_if<std::shared_ptr<AstNode>>(&value))
        if (*child) (*child)->parent = parent;

    return out;
}

}  // namespace prismcascade::ast
