#pragma once
#include <string>
#include <map>
#include <memory>
#include <atomic>
#include <vector>
#include <unordered_set>

#include <ast/ast_node.hpp>
#include <plugin/plugin_metadata.hpp>
#include <plugin/data_types.hpp>
#include "dynamic_library.hpp"

namespace PrismCascade {

class CacheManager;

//-----------------------------------------------------------
// ID発行
//-----------------------------------------------------------
class HandleManager {
public:
    HandleManager() : current_id_(0) {}

    std::string fresh_id(){
        char buffer[15]{};
        snprintf(buffer, std::size(buffer), "%012ld", static_cast<long>(current_id_++));
        return {buffer};
        // return std::format("{:012d}", current_id++);  // C++20
    }

    std::int64_t fresh_handler(){
        return current_id_++;
    }

private:
    std::atomic<std::int64_t> current_id_{};
};

// TODO: uuidとplugin_handlerの対応を確認

//-----------------------------------------------------------
// PluginManager: プラグイン読み込み & ASTノードへの紐付け
//-----------------------------------------------------------
class PluginManager {
public:
    PluginManager(std::shared_ptr<CacheManager> cache);
    ~PluginManager();

    // 動的ライブラリ読み込み
    bool load_plugin(const std::string& path);

    // プラグインメタデータを登録/取得
    bool register_plugin_meta(const PluginMetaDataInternal& meta);
    std::optional<PluginMetaDataInternal> get_plugin_meta(const std::string& uuid) const;

    // AST ノード生成
    std::shared_ptr<AstNode> create_node(const std::string& plugin_uuid);

    // 入出力設定など
    void set_input_value(std::shared_ptr<AstNode> node, int index, AstInput value);

    // レンダリング開始・終了・フレームごとの動き (簡易版)
    bool invoke_start_rendering(std::shared_ptr<AstNode> node);
    bool invoke_render_frame(std::shared_ptr<AstNode> node, std::uint64_t frame);
    void invoke_finish_rendering(std::shared_ptr<AstNode> node);

private:
    void gather_subtree_nodes(std::shared_ptr<AstNode> start, std::unordered_set<std::shared_ptr<AstNode>>& visited);
    void remove_subtree_boundary_references(const std::unordered_set<std::shared_ptr<AstNode>>& subtree_set);

private:
    std::shared_ptr<CacheManager> cache_;
    HandleManager handle_mgr_;

    // uuid -> PluginMetaDataInternal
    std::map<std::string, PluginMetaDataInternal> plugin_registry_;

    // path -> DynamicLibrary
    std::map<std::string, std::unique_ptr<DynamicLibrary>> libraries_;
};

}
