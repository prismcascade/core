#pragma once
#include <cstdint>
#include <memory>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/scheduler/delay.hpp>
#include <prismcascade/scheduler/exec_queue.hpp>
#include <prismcascade/scheduler/split_dag.hpp>
#include <prismcascade/scheduler/topological_sort.hpp>
#include <vector>

namespace prismcascade::scheduler {

class Scheduler {
   public:
    struct Options {
        std::int64_t startup_buffer = 0;  // frame
    };

    explicit Scheduler(const Options& opt = {});

    /* compile  : AST → Rank / Delay / SubDAG / ExecutionQueue 初期化   */
    void compile(const std::shared_ptr<ast::AstNode>& root);

    /* tick : １アイテム実行（今回は run_cb に渡すだけ）         */
    bool tick(const std::function<void(const std::shared_ptr<ast::AstNode>&)>& run_cb);

   private:
    Options                     opt_;
    std::vector<subdag_info>    subdags_;  // queue_id 付き
    std::vector<ExecutionQueue> queues_;   // runtime PQ
};

}  // namespace prismcascade::scheduler
