#pragma once

#include <string>
#include <memory>

class DynamicLibrary {
public:
    DynamicLibrary() = delete;  // デフォルト構築禁止
    DynamicLibrary(const std::string& path);

    // resolve symbol name
    void* operator[] (const std::string& symbolName);

private:
    std::shared_ptr<void> handle_;  // Windows: HMODULE, Linux: void*
    bool load(const std::string& path);
};
