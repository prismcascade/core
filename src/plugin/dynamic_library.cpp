#include "dynamic_library.hpp"

#if defined(_WIN32) || defined(_WIN64)
#define _IS_WINDOWS_BUILD
#endif

#ifdef _IS_WINDOWS_BUILD
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

bool DynamicLibrary::load(const std::string& path) {
    unload();  // 多重ロード防止

#ifdef _IS_WINDOWS_BUILD
    // Windows
    HMODULE mod = ::LoadLibraryA(path.c_str());
    if (!mod) {
        return false;  // 失敗
    }
    handle_ = mod;
#else
    // Linux / Macintosh
    void* so = ::dlopen(path.c_str(), RTLD_NOW);
    if (!so) {
        return false;
    }
    handle_ = so;
#endif

    return true;  // 成功
}

void DynamicLibrary::unload() {
    if (handle_) {
#ifdef _IS_WINDOWS_BUILD
        ::FreeLibrary((HMODULE)handle_);
#else
        ::dlclose(handle_);
#endif
        handle_ = nullptr;
    }
}

void* DynamicLibrary::resolveSymbol(const std::string& symbolName) {
    if (!handle_) {
        return nullptr;
    }
#ifdef _IS_WINDOWS_BUILD
    return (void*)::GetProcAddress((HMODULE)handle_, symbolName.c_str());
#else
    return ::dlsym(handle_, symbolName.c_str());
#endif
}
