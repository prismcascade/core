#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <map>
#include <memory>

// TODO: 修正
namespace PrismCascade {

class MemoryAllocator {
public:
    MemoryAllocator() = default;
    ~MemoryAllocator() = default;

    void* allocate(std::size_t size);
    void  deallocate(void* ptr);

    // TODO: alignmentなどを考慮

private:
    // 追跡したいなら何かしら記録
    std::mutex mutex_;
    std::map<void*, std::size_t> allocation_map_;
};

}
