// ── prismcascade/schedule/delay.cpp ─────────────────────────────
// gdelay(child) = max_{path}(Σ lookAhead) を一回の BFS で求める。
//  ─ 親 → 子 方向へ単調に非減少の値が伝搬するため
//    幅優先でも正しい最終値が得られる。
//  ─ 連結性を保証するため、各ノードは「必ず一度」キューへ入れる。
//    （lookAhead=0 の鎖でも delay_table にキーが残るように）

#include <prismcascade/scheduler/delay.hpp>
#include <queue>
#include <unordered_set>

namespace prismcascade::scheduler {

DelayTable build_delay_table(const std::shared_ptr<ast::AstNode>& root) {
    DelayTable g;  // handler → gdelay
    if (!root) return g;

    std::queue<std::shared_ptr<ast::AstNode>> q;

    // 「まだ push していない」ことを判定
    std::unordered_set<std::int64_t> enqueued;

    // enqueue(): ノードを一度だけ Q に入れるヘルパ
    // 不変条件:  enqueued.contains(id) ⇒ id が Q に入った/処理済み
    auto enqueue = [&](const std::shared_ptr<ast::AstNode>& n) {
        if (!n) return;
        auto id = n->plugin_instance_handler;
        if (enqueued.insert(id).second)  // 初回だけ true を返す
            q.push(n);
    };

    enqueue(root);                         // BFS 出発点
    g[root->plugin_instance_handler] = 0;  // 必ずキーを生成 (初期 gdelay 0)

    while (!q.empty()) {
        auto n = q.front();
        q.pop();
        auto id = n->plugin_instance_handler;

        // 現在までに確定している遅延値
        std::uint32_t cur = g[id];

        // ノード自身の lookAhead を簡易的に (output_parameters の slot0 長さを流用) */
        std::uint32_t la = 0;
        if (n->output_parameters && !n->output_parameters->types().empty())
            la = static_cast<std::uint32_t>(n->output_parameters->types()[0].size());

        std::uint32_t total = cur + la;
        g[id]               = total;  // 上書き（常に >= 以前の値）

        // propagate(): 子ノードへ遅延値を伝搬し、最低 1 度は訪問させる
        // 単調性: total >= g[child] が成立 ⇒ g の更新は一方向
        auto propagate = [&](const std::shared_ptr<ast::AstNode>& child) {
            if (!child) return;
            auto  cid = child->plugin_instance_handler;
            auto& ref = g[cid];  // default 0 でキー生成
            if (total > ref)     // 「より長いパス」のみ値を更新
                ref = total;
            enqueue(child);  // 値が不変でも Q に一度は入れる
        };

        // すべての outgoing edge をたどる
        for (const auto& in : n->inputs) {
            if (auto p = std::get_if<std::shared_ptr<ast::AstNode>>(&in))
                propagate(*p);
            else if (auto se = std::get_if<ast::SubEdge>(&in))
                if (auto src = se->source.lock()) propagate(src);
        }
    }
    return g;
}

}  // namespace prismcascade::scheduler
