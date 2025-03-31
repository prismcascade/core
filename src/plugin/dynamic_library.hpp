#pragma once

#include <string>

class DynamicLibrary {
public:
    DynamicLibrary() : handle_(nullptr) {}
    ~DynamicLibrary() { unload(); }

    // とりあえず Copy 禁止 （必要なら shared_ptr で持ちまわす）
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;

    bool load(const std::string& path);
    void unload();

    void* resolveSymbol(const std::string& symbolName);

    // Check if loaded
    operator bool() const { return handle_ != nullptr; }

private:
    void* handle_; // Windows: HMODULE, Linux: void*
};
