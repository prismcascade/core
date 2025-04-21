#include <prismcascade/scheduler/rendering_scheduler.hpp>

using namespace prismcascade;
namespace sched = prismcascade::scheduler;

std::tuple<std::vector<std::shared_ptr<ast::Node>>, std::vector<std::shared_ptr<ast::Node>>, sched::LifetimeTable>
sched::topological_sort(const std::shared_ptr<ast::Node>& root) {
    /* ルートのみを返す暫定実装 */
    std::vector<std::shared_ptr<ast::Node>> sorted;
    if (root) sorted.push_back(root);

    LifetimeTable lifetime(sorted.size());  // 空テーブル
    return {sorted, {}, lifetime};
}
