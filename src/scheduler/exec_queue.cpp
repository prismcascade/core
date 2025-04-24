#include <prismcascade/scheduler/exec_queue.hpp>

namespace prismcascade::scheduler {

ExecutionQueue::ExecutionQueue(compare_t cmp) : pq_(cmp) {}

void ExecutionQueue::push(Item it) { pq_.push(std::move(it)); }

bool ExecutionQueue::pop(Item& out) {
    if (pq_.empty()) return false;
    out = pq_.top();
    pq_.pop();
    return true;
}

std::size_t ExecutionQueue::size() const noexcept { return pq_.size(); }

}  // namespace prismcascade::scheduler
