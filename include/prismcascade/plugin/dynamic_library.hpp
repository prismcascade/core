#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace prismcascade::plugin {

class DynamicLibrary {
   public:
    DynamicLibrary() = delete;  // デフォルト生成禁止
    explicit DynamicLibrary(const std::string& path);

    // シンボル解決。失敗すると runtime_error
    void* operator[](const std::string& symbol_name);

    // 指定ディレクトリ（未指定なら実行ファイル/plugins）から 拡張子 .dll / .so を列挙してフルパスを返す
    static std::vector<std::string> list_plugins(const std::optional<std::string>& dir = std::nullopt);

   private:
    std::shared_ptr<void> handle_;  // Windows: HMODULE, Linux: void*
};

}  // namespace prismcascade::plugin
