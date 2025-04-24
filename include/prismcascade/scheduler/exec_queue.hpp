#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/scheduler/time.hpp>
#include <queue>
#include <vector>

namespace prismcascade::scheduler {

class ExecutionQueue {
   public:
    struct Item {
        timestamp_t                   stamp;  // pop 優先キー
        std::int64_t                  rank;   // tie-break
        std::shared_ptr<ast::AstNode> node;   // 実行ターゲット
    };
    using compare_t = std::function<bool(const Item&, const Item&)>;

    explicit ExecutionQueue(compare_t cmp);  // 比較器 DI
    void        push(Item it);
    bool        pop(Item& out);  // empty→false
    std::size_t size() const noexcept;

   private:
    std::priority_queue<Item, std::vector<Item>, compare_t> pq_;
};
}  // namespace prismcascade::scheduler
