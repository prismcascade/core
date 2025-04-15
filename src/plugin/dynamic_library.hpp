#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>

namespace PrismCascade {

class DynamicLibrary {
public:
    DynamicLibrary() = delete;  // デフォルト構築禁止
    DynamicLibrary(const std::string& path);

    // resolve symbol name
    void* operator[] (const std::string& symbolName);

    static std::vector<std::string> list_plugin(const std::optional<std::string>& path);

private:
    std::shared_ptr<void> handle_;  // Windows: HMODULE, Linux: void*
};

}
