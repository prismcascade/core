#include "dynamic_library.hpp"
#include <stdexcept>
#include <filesystem>
#include <vector>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#define _IS_WINDOWS_BUILD
#endif

#ifdef _IS_WINDOWS_BUILD
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace PrismCascade {

DynamicLibrary::DynamicLibrary(const std::string& path){
    void* handle = nullptr;
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

namespace {
    #ifdef _IS_WINDOWS_BUILD
        std::string get_executable_path() {
            char buf[MAX_PATH];
            DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
            if(len == 0 || len == MAX_PATH) throw std::runtime_error("GetModuleFileNameA failed");
            return std::string(buf, len);
        }
    #else
        std::string get_executable_path() {
            char buf[PATH_MAX];
            ssize_t len = readlink("/proc/self/exe", buf, PATH_MAX - 1);
            if(len <= 0) throw std::runtime_error("readlink failed");
            buf[len] = '\0';
            return std::string(buf, len);
        }
    #endif
}

std::vector<std::string> DynamicLibrary::list_plugin(){
    std::vector<std::string> result;
    auto exePath = get_executable_path();
    auto dir = std::filesystem::path(exePath).parent_path() / "plugins";
#ifdef _IS_WINDOWS_BUILD
    std::string ext = ".dll";
#else
    std::string ext = ".so";
#endif
    if(std::filesystem::exists(dir) && std::filesystem::is_directory(dir)){
        for(auto& entry : std::filesystem::directory_iterator(dir)){
            if(entry.is_regular_file()){
                auto p = entry.path();
                if(p.extension() == ext){
                    result.push_back(p.string());
                }
            }
        }
    }
    return result;
}

}
