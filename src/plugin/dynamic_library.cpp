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

namespace fs   = std::filesystem;
namespace pcpl = prismcascade::plugin;

// ───────────────────────── internal
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

// ───────────────────────── ctor / dtor
pcpl::DynamicLibrary::DynamicLibrary(const std::string& path) {
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

// ───────────────────────── symbol resolve
void* pcpl::DynamicLibrary::operator[](const std::string& sym) {
    void* ptr = nullptr;
#ifdef PC_WINDOWS
    ptr = reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle_.get()), sym.c_str()));
#else
    ptr = ::dlsym(handle_.get(), sym.c_str());
#endif
    if (!ptr) throw std::runtime_error("symbol not found: " + sym);
    return ptr;
}

// ───────────────────────── plugin list
std::vector<std::string> pcpl::DynamicLibrary::list_plugins(const std::optional<std::string>& dir) {
    std::vector<std::string> out;

    fs::path base = dir ? fs::path(*dir) : fs::path(exe_path()).parent_path() / "plugins";

#ifdef PC_WINDOWS
    constexpr const char* ext = ".dll";
#else
    constexpr const char* ext = ".so";
#endif

    if (fs::exists(base) && fs::is_directory(base))
        for (auto& e : fs::directory_iterator(base))
            if (e.is_regular_file() && e.path().extension() == ext) out.push_back(e.path().string());

    return out;
}
