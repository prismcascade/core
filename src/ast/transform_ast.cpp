#include <algorithm>
#include <prismcascade/ast/transform_ast.hpp>
#include <queue>
#include <stdexcept>
#include <unordered_set>

namespace prismcascade::ast {
namespace inner_transform {

namespace {

// ヘルパ

// 親 chain 内に target が居るか (O(depth))
bool is_ancestor(const std::shared_ptr<AstNode>& target, const std::shared_ptr<AstNode>& node) {
    for (auto p = node ? node->parent.lock() : nullptr; p; p = p->parent.lock())
        if (p == target) return true;
    return false;
}

// remove one SubOutputRef ; return true if erased
bool erase_backref(const std::shared_ptr<AstNode>& from, const std::shared_ptr<AstNode>& dst, std::uint64_t dst_slot,
                   std::uint32_t from_idx) {
    auto& vec = from->sub_output_references;
    auto  it  = std::find_if(vec.begin(), vec.end(), [&](const AstNode::SubOutputRef& r) {
        return r.src_index == from_idx && !r.dst_node.expired() && r.dst_node.lock() == dst && r.dst_slot == dst_slot;
    });
    if (it != vec.end()) {
        vec.erase(it);
        return true;
    }
    return false;
}

}  // namespace

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//                CUT                //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
std::vector<AstDiffStep> cut(const std::shared_ptr<AstNode>& parent, std::uint64_t slot) {
    if (!parent) throw std::domain_error("cut: parent=nullptr");
    if (slot >= parent->inputs.size()) throw std::domain_error("cut: slot OOB");

    AstNode::input_t old = parent->inputs[slot];
    // ここのエラーはなくても動作としては同じだが，無駄を検出するために一応例外を飛ばす
    if (std::holds_alternative<std::monostate>(old)) throw std::domain_error("cut: slot empty");

    std::vector<AstDiffStep> diff;

    /* 1) 親スロットを monostate */
    parent->inputs[slot] = std::monostate{};
    diff.push_back({parent, slot, old, std::monostate{}});

    /* 2) 旧値に応じてリンク解除 */
    if (auto child = std::get_if<std::shared_ptr<AstNode>>(&old)) {
        if (*child) (*child)->parent.reset();
    } else if (auto se = std::get_if<SubEdge>(&old)) {
        if (auto from = se->source.lock()) erase_backref(from, parent, slot, se->output_index);
    }
    return diff;
}

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//              ASSIGN               //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
std::vector<AstDiffStep> assign(const std::shared_ptr<AstNode>& parent, std::uint64_t slot, AstNode::input_t value) {
    if (!parent) throw std::domain_error("assign: parent=null");
    if (slot >= parent->inputs.size()) throw std::domain_error("assign: slot OOB");

    /* 木閉路チェック (child) */
    if (auto child = std::get_if<std::shared_ptr<AstNode>>(&value)) {
        if (*child) {
            if (*child == parent)  // 自己代入を明示的に拒否
                throw std::domain_error("assign: self-loop");
            if (is_ancestor(*child, parent))  // 子孫を祖先に付ける → cycle
                throw std::domain_error("assign: would create main-tree cycle");
        }
    }

    /* SubEdge 閉路: from が parent 系列なら NG */
    if (auto se = std::get_if<SubEdge>(&value)) {
        auto from = se->source.lock();
        if (from && is_ancestor(parent, from)) throw std::domain_error("assign: creates sub-edge cycle");
    }

    std::vector<AstDiffStep> diff;
    AstNode::input_t         old = parent->inputs[slot];

    /* (i) 旧値の解体 */
    if (auto oldChild = std::get_if<std::shared_ptr<AstNode>>(&old))
        if (*oldChild) (*oldChild)->parent.reset();
    if (auto oldSe = std::get_if<SubEdge>(&old))
        if (auto oldFrom = oldSe->source.lock()) erase_backref(oldFrom, parent, slot, oldSe->output_index);

    /* (ii) 書き込み & diff push */
    parent->inputs[slot] = value;
    diff.push_back({parent, slot, old, value});

    /* (iii) 新値リンク作成 */
    if (auto newSe = std::get_if<SubEdge>(&value))
        if (auto from = newSe->source.lock())
            from->sub_output_references.push_back({newSe->output_index, parent, slot});
    if (auto child = std::get_if<std::shared_ptr<AstNode>>(&value))
        if (*child) (*child)->parent = parent;

    return diff;
}

// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
//    サブツリー境界の sub_edge 切断   //
// -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- //
namespace {

std::unordered_set<std::shared_ptr<AstNode>> collect_subtree(const std::shared_ptr<AstNode>& root) {
    std::unordered_set<std::shared_ptr<AstNode>> set;
    std::queue<std::shared_ptr<AstNode>>         q;
    q.push(root);
    while (!q.empty()) {
        auto n = q.front();
        q.pop();
        if (!n || !set.insert(n).second) continue;
        for (const auto& in : n->inputs)
            if (auto ch = std::get_if<std::shared_ptr<AstNode>>(&in))
                if (*ch) q.push(*ch);
    }
    return set;
}

}  // namespace

std::vector<AstDiffStep> detach_cross_edges(const std::shared_ptr<AstNode>& subRoot) {
    if (!subRoot) throw std::domain_error("detach_cross_edges: null");

    auto inside    = collect_subtree(subRoot);
    auto is_inside = [&](const std::shared_ptr<AstNode>& n) { return inside.count(n); };

    std::vector<AstDiffStep> diff;

    /* --- 内 → 外 : back-ref ベクタ走査 ------------------------- */
    for (const auto& nInside : inside) {
        auto&                                   vec  = nInside->sub_output_references;
        std::vector<ast::AstNode::SubOutputRef> refs = vec;
        for (const auto& ref : refs) {
            auto dst = ref.dst_node.lock();
            if (!dst) continue;
            if (!is_inside(dst)) {
                /* dst の slot を空にする */
                auto add = assign(dst, ref.dst_slot, std::monostate{});
                diff.insert(diff.end(), add.begin(), add.end());
            }
        }
    }

    /* --- 外 → 内 : 入力スロット走査 --------------------------- */
    for (const auto& nInside : inside) {
        for (std::uint64_t i = 0; i < nInside->inputs.size(); ++i) {
            if (auto se = std::get_if<SubEdge>(&nInside->inputs[i])) {
                if (auto from = se->source.lock(); from && !is_inside(from)) {
                    auto d = assign(nInside, i, std::monostate{});
                    diff.insert(diff.end(), d.begin(), d.end());
                }
            }
        }
    }
    return diff;
}

}  // namespace inner_transform
}  // namespace prismcascade::ast
