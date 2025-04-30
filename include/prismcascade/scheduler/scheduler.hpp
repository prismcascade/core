#pragma once
#include <cstdint>
#include <memory>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/scheduler/delay.hpp>
#include <prismcascade/scheduler/exec_queue.hpp>
#include <prismcascade/scheduler/live_controller.hpp>
#include <prismcascade/scheduler/split_dag.hpp>
#include <prismcascade/scheduler/topological_sort.hpp>
#include <prismcascade/scheduler/video_cache.hpp>
#include <vector>

namespace prismcascade::scheduler {

class Scheduler {
   public:
    struct Options {
        std::int64_t startup_buffer = 0;  // frame
    };

    Scheduler();
    explicit Scheduler(const Options& opt);

    /* compile  : AST → Rank / Delay / SubDAG / ExecutionQueue 初期化   */
    void compile(const std::shared_ptr<ast::AstNode>& root);

    /* tick : １アイテム実行（今回は run_cb に渡すだけ）         */
    bool tick(const std::function<void(const std::shared_ptr<ast::AstNode>&)>& run_cb);

    void setVideoCache(std::shared_ptr<video_cache> vc) { video_cache_ = std::move(vc); }
    void setLiveController(std::shared_ptr<live_controller> c) { live_ctl_ = std::move(c); }

   private:
    Options                     opt_;
    std::vector<subdag_info>    subdags_;  // queue_id 付き
    std::vector<ExecutionQueue> queues_;   // runtime PQ

    // optional DI objects
    std::shared_ptr<video_cache>     video_cache_;
    std::shared_ptr<live_controller> live_ctl_;
};

}  // namespace prismcascade::scheduler
