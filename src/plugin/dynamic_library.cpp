#include <filesystem>
#include <plugin/dynamic_library.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#define PC_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>
#endif

namespace {

#ifdef PC_WINDOWS
std::string exe_path() {
    char  buf[MAX_PATH];
    DWORD len = ::GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len == 0 || len == MAX_PATH) throw std::runtime_error("GetModuleFileNameA failed");
    return std::string(buf, len);
}
#else
std::string exe_path() {
    char    buf[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", buf, PATH_MAX - 1);
    if (len <= 0) throw std::runtime_error("readlink failed");
    buf[len] = '\0';
    return std::string(buf, len);
}
#endif

}  // namespace

namespace prismcascade::plugin {
DynamicLibrary::DynamicLibrary(const std::string& path) {
    void* raw = nullptr;

#ifdef PC_WINDOWS
    HMODULE h = ::LoadLibraryA(path.c_str());
    if (!h) throw std::runtime_error("LoadLibraryA failed: " + path);
    raw = h;
#else
    void* so = ::dlopen(path.c_str(), RTLD_NOW);
    if (!so) throw std::runtime_error("dlopen failed: " + path);
    raw = so;
#endif

    handle_.reset(raw, [](void* h) {
#ifdef PC_WINDOWS
        if (h) ::FreeLibrary(static_cast<HMODULE>(h));
#else
        if (h) ::dlclose(h);
#endif
    });
}

void* DynamicLibrary::operator[](const std::string& sym) {
    void* ptr = nullptr;
#ifdef PC_WINDOWS
    ptr = reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle_.get()), sym.c_str()));
#else
    ptr = ::dlsym(handle_.get(), sym.c_str());
#endif
    if (!ptr) throw std::runtime_error("prismcascade: dynamic_library: operator[]: symbol not found: " + sym);
    return ptr;
}

std::vector<std::string> DynamicLibrary::list_plugins(const std::optional<std::string>& dir) {
    std::vector<std::string> out;
    using namespace std::filesystem;

    path base = dir ? path(*dir) : path(exe_path()).parent_path() / "plugins";

#ifdef PC_WINDOWS
    constexpr const char* ext = ".dll";
#else
    constexpr const char* ext = ".so";
#endif

    if (exists(base) && is_directory(base))
        for (auto& e : directory_iterator(base))
            if (e.is_regular_file() && e.path().extension() == ext) out.push_back(e.path().string());

    return out;
}

}  // namespace prismcascade::plugin
