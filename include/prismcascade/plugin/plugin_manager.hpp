// include/prismcascade/plugin/plugin_manager.hpp
#pragma once
/**
 *  Prism Cascade – PluginManager
 *  -----------------------------
 *  * 動的ライブラリ (.dll / .so) のロード
 *  * プラグイン UUID → ノード生成
 *  * 実行ライフサイクル (start / frame / finish)
 *  * DllMemoryManager とのブリッジ
 *
 *  ※ 実装は src/plugin/plugin_manager.cpp
 */
#include <map>
#include <memory>
#include <optional>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/common/types.hpp>
#include <prismcascade/plugin/dll_memory_manager.hpp>
#include <prismcascade/plugin/dynamic_library.hpp>
#include <string>
#include <unordered_set>
#include <vector>

namespace prismcascade::plugin {

/* ───── simple id generator ─────────────────────────────────── */
class HandleManager {
   public:
    /** 12 桁ゼロ詰めの文字列 ID */
    std::string fresh_id();

    /** 64‑bit 整数ハンドラ */
    std::int64_t fresh_handler();

   private:
    std::atomic<std::int64_t> current_{0};
};

/* ───── PluginManager ───────────────────────────────────────── */
class PluginManager {
   public:
    DllMemoryManager dll_memory_manager;
    HandleManager    handle_manager;

    explicit PluginManager(const std::optional<std::string>& plugin_dir = std::nullopt);

    /* -------- ノード生成／入力設定 ----------------------------- */
    std::shared_ptr<ast::Node> make_node(const std::string& uuid, std::weak_ptr<ast::Node> parent = {});

    void assign_input(std::shared_ptr<ast::Node> node, int index, ast::Node::Input value);

    /* -------- プラグイン呼び出し ------------------------------- */
    bool invoke_start_rendering(std::shared_ptr<ast::Node> node);
    bool invoke_render_frame(std::shared_ptr<ast::Node> node, int frame);
    void invoke_finish_rendering(std::shared_ptr<ast::Node> node);

   private:
    /* UUID → PluginHandlerID */
    std::map<std::string, std::int64_t> uuid_to_handler_;

    /* handler → 動的ライブラリ */
    std::map<std::int64_t, plugin::DynamicLibrary> libraries_;

    /* internal helpers */
    std::vector<VariableType> infer_input_type(const ast::Node::Input&);

    void remove_old_refs(const ast::Node::Input&, const std::shared_ptr<ast::Node>&);
    void add_new_refs(const ast::Node::Input&, const std::shared_ptr<ast::Node>&);

    void gather_subtree_nodes(const std::shared_ptr<ast::Node>&               root,
                              std::unordered_set<std::shared_ptr<ast::Node>>& out);
    void detach_subtree_boundary(const std::unordered_set<std::shared_ptr<ast::Node>>&);
};

}  // namespace prismcascade::plugin
