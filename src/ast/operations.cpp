#include <prismcascade/ast/operations.hpp>
#include <prismcascade/ast/transform_ast.hpp>
#include <stdexcept>
#include <unordered_map>

namespace prismcascade::ast {

/* ------------------------------------------------------------------
 *  make_node
 *     – ユーザが与えたプラグイン UUID だけを手掛かりに「素の AstNode」を
 *       1 つ作って返す。
 *     – まだ AST には接続されていない（ parent／sub_output も空）。
 *     – ハンドラ番号は **実行ごとに一意** になるよう
 *       スレッド安全な単純インクリメントで振っておく。
 * ------------------------------------------------------------------ */
std::shared_ptr<AstNode> make_node(const std::string& plugin_uuid, std::uint64_t plugin_handler,
                                   std::uint64_t plugin_instance_handler) {
    auto n         = std::make_shared<AstNode>();
    n->plugin_uuid = plugin_uuid;

    /* --- 仮メタデータ（後で PluginManager が書き換える想定） --- */
    n->plugin_type      = PluginType::Effect;  // とりあえず Effect 扱い
    n->protocol_version = 1;
    n->plugin_name      = "(unresolved)";

    /* --- 一意ハンドラを割り振る --- */
    n->plugin_handler          = plugin_handler;
    n->plugin_instance_handler = plugin_instance_handler;

    /* --- 入出力スロットは未定。呼び出し側で resize してから assign する --- */
    n->inputs.clear();
    n->sub_output_references.clear();
    n->parent.reset();

    /* DLL 側実メモリはまだ不要なので空のまま */
    n->input_parameters  = std::make_shared<memory::ParameterPackMemory>();
    n->output_parameters = std::make_shared<memory::ParameterPackMemory>();

    return n;
}

// --------------------------------------------------------------
//   ヘルパ : subTreeContains(nodeSet , t)
// --------------------------------------------------------------
namespace {
void collectSubtree(const std::shared_ptr<ast::AstNode>& r, std::unordered_map<ast::AstNode*, bool>& seen) {
    if (!r || seen[r.get()]) return;
    seen[r.get()] = true;
    for (auto& in : r->inputs)
        if (auto ch = std::get_if<std::shared_ptr<ast::AstNode>>(&in)) collectSubtree(*ch, seen);
}
bool subtreeContains(const std::shared_ptr<ast::AstNode>& root, const std::shared_ptr<ast::AstNode>& n) {
    std::unordered_map<ast::AstNode*, bool> seen;
    collectSubtree(root, seen);
    return n && seen[n.get()];
}
}  // namespace

//  --------------------------------------------------------------
//    substitute
//  --------------------------------------------------------------
std::vector<AstDiffStep> substitute(const std::shared_ptr<ast::AstNode>& parent, std::uint32_t slot,
                                    ast::AstNode::input_t value, bool allow_cross) {
    if (!parent) throw std::domain_error("substitute: null parent");
    if (slot >= parent->inputs.size()) throw std::domain_error("substitute: slot OOB");

    std::vector<AstDiffStep> diff;  // 最終差分

    // ─────────────────────────────────────────
    // Step‑1  :  value が AST 内に既に居る？ → “抜き取り”
    // ─────────────────────────────────────────
    if (auto child = std::get_if<std::shared_ptr<ast::AstNode>>(&value)) {
        if (*child && !(*child)->parent.expired()) {
            auto          prevParent = (*child)->parent.lock();
            auto&         siblings   = prevParent->inputs;
            std::uint32_t prevSlot   = 0;
            for (std::uint32_t i = 0; i < siblings.size(); ++i)
                if (std::holds_alternative<std::shared_ptr<ast::AstNode>>(siblings[i])
                    && std::get<std::shared_ptr<ast::AstNode>>(siblings[i]) == *child) {
                    prevSlot = i;
                    break;
                }

            // 1‑a) CUT (child を孤立させる)
            auto steps = cut(prevParent, prevSlot);
            diff.insert(diff.end(), steps.begin(), steps.end());

            // 1‑b) 外界と cross-edge を残す？  allow_cross=false なら絶縁
            if (!allow_cross) {
                auto steps2 = detach_cross_edges(*child);
                diff.insert(diff.end(), steps2.begin(), steps2.end());
            }
        }
    }

    // ─────────────────────────────────────────
    // Step‑2  :  置換先スロットに元いた値 W を切り離す
    // ─────────────────────────────────────────
    {
        const auto& cur = parent->inputs[slot];
        /* いま入っている値が空で無ければ CUT して diff に積む */
        if (!std::holds_alternative<std::monostate>(cur)) {
            auto steps = cut(parent, slot);  // parent[slot] ← monostate
            diff.insert(diff.end(), steps.begin(), steps.end());

            /* detach 不要クロスエッジ */
            const ast::AstNode::input_t& W = steps.back().old_value;
            if (!allow_cross) {
                if (auto oldChild = std::get_if<std::shared_ptr<ast::AstNode>>(&W)) {
                    auto steps2 = detach_cross_edges(*oldChild);
                    diff.insert(diff.end(), steps2.begin(), steps2.end());
                }
            }
        }
    }

    // ─────────────────────────────────────────
    // Step‑3  :  目的値を代入
    // ─────────────────────────────────────────
    {
        auto steps = assign(parent, slot, value);
        diff.insert(diff.end(), steps.begin(), steps.end());
    }

    return diff;
}

//  --------------------------------------------------------------
//    Redo / Undo
//  --------------------------------------------------------------

namespace {

void assign_diff_step(const AstDiffStep& s, const AstNode::input_t& value) {
    if (!s.node) throw std::runtime_error("undo_redo: null node in diff step");
    if (s.index >= s.node->inputs.size()) throw std::runtime_error("undo_redo: slot OOB in diff step");

    // back-ref／parent 更新などは assign() に委ねる
    assign(s.node, s.index, value);
}

}  // namespace

void redo(const std::vector<AstDiffStep>& diff) {
    for (const auto& st : diff) assign_diff_step(st, st.new_value);  // forward 順に new_value
}

void undo(const std::vector<AstDiffStep>& diff) {
    for (auto it = diff.rbegin(); it != diff.rend(); ++it) assign_diff_step(*it, it->old_value);  // 逆順に old_value
}

}  // namespace prismcascade::ast
