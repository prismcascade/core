#include "dynamic_library.hpp"
#include <stdexcept>

#if defined(_WIN32) || defined(_WIN64)
#define _IS_WINDOWS_BUILD
#endif

#ifdef _IS_WINDOWS_BUILD
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

DynamicLibrary::DynamicLibrary(const std::string& path){
    void* handle = nullptr;;
#ifdef _IS_WINDOWS_BUILD
    // Windows
    HMODULE mod = ::LoadLibraryA(path.c_str());
    if (!mod)
        throw std::runtime_error("failed to load plugin");
    handle = mod;
#else
    // Linux / Macintosh
    void* so = ::dlopen(path.c_str(), RTLD_NOW);
    if (!so)
        throw std::runtime_error("failed to load plugin");
    handle = so;
#endif

    // shared_ptr に deleter 付きでセットする
    handle_ = {handle, [](void* handle){
        if (handle) {
#ifdef _IS_WINDOWS_BUILD
            ::FreeLibrary((HMODULE)handle);
#else
            ::dlclose(handle);
#endif
        }
    }};
}

void* DynamicLibrary::operator[] (const std::string& symbolName) {
    void* function_pointer;
#ifdef _IS_WINDOWS_BUILD
    function_pointer = (void*)::GetProcAddress((HMODULE)handle_.get(), symbolName.c_str());
#else
    function_pointer = ::dlsym(handle_.get(), symbolName.c_str());
#endif
    if(function_pointer)
        return function_pointer;
    else
        throw std::runtime_error("failed to resolve function pointer (the plugin may be corrupted)");
}
