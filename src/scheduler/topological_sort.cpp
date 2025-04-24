/* ── prismcascade/schedule/toposort.cpp ─────────────────────────
   DFS でトポロジカルソート。
   Visiting → Visiting の back-edge を検出した瞬間、再帰スタックから閉路パスを抽出。
   ─────────────────────────────────────────────────────────── */
#include <algorithm>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/scheduler/topological_sort.hpp>
#include <stack>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace prismcascade::schedule {
namespace {

enum class Mark : uint8_t { Initial, Visiting, Done };

/*---------------------------------------------------------------
   再帰 DFS 。
   成功: true
   閉路: false & cycle_path にパスを設定
 ----------------------------------------------------------------*/
bool dfs_visit(const std::shared_ptr<ast::AstNode>& v, std::unordered_map<std::int64_t, Mark>& mark,
               std::vector<std::shared_ptr<ast::AstNode>>& stack,
               std::vector<std::shared_ptr<ast::AstNode>>& post_order,
               std::vector<std::shared_ptr<ast::AstNode>>& cycle_path) {
    if (!v) return true;
    std::int64_t id = v->plugin_instance_handler;
    auto&        m  = mark[id];

    if (m == Mark::Visiting) {  // 後退辺: stack上に閉路が存在
        auto it = std::find_if(stack.begin(), stack.end(), [&](const std::shared_ptr<ast::AstNode>& p) {
            return p->plugin_instance_handler == id;
        });
        cycle_path.assign(it, stack.end());
        cycle_path.push_back(v);  // 戻ってきて閉路を閉じる
        return false;
    }
    if (m == Mark::Done) return true;

    m = Mark::Visiting;
    stack.push_back(v);

    auto push_child = [&](const std::shared_ptr<ast::AstNode>& n) -> bool {
        return dfs_visit(n, mark, stack, post_order, cycle_path);
    };

    // main-child ＋ SubEdge-source
    for (const auto& in : v->inputs) {
        if (auto p = std::get_if<std::shared_ptr<ast::AstNode>>(&in)) {
            if (*p && !push_child(*p)) return false;
        } else if (auto se = std::get_if<ast::SubEdge>(&in)) {
            if (auto src = se->source.lock())
                if (!push_child(src)) return false;
        }
    }

    stack.pop_back();
    m = Mark::Done;
    post_order.push_back(v);
    return true;
}

}  // namespace

/*================================================================
   try_topo_sort : 成功/失敗に応じ RankTable or cycle_path を返す
 =================================================================*/
TopoResult topological_sort(const std::shared_ptr<ast::AstNode>& root) {
    TopoResult res;
    if (!root) return res;  // empty → failure (cycle_path == {})

    std::unordered_map<std::int64_t, Mark>     mark;
    std::vector<std::shared_ptr<ast::AstNode>> stack;
    std::vector<std::shared_ptr<ast::AstNode>> post;

    if (!dfs_visit(root, mark, stack, post, res.cycle_path)) return res;  // cycle_path が埋まっている

    std::reverse(post.begin(), post.end());
    for (std::uint32_t r = 0; r < post.size(); ++r) {
        RankInfo info;
        info.rank = r;
        res.ranks.emplace(post[r]->plugin_instance_handler, info);
    }
    return res;
}

}  // namespace prismcascade::schedule
