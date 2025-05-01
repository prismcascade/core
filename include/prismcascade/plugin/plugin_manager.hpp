// // include/prismcascade/plugin/plugin_manager.hpp
// #pragma once
// /**
//  *  Prism Cascade – PluginManager
//  *  -----------------------------
//  *  * 動的ライブラリ (.dll / .so) のロード
//  *  * プラグイン UUID → ノード生成
//  *  * 実行ライフサイクル (start / frame / finish)
//  *  * DllMemoryManager とのブリッジ
//  *
//  *  ※ 実装は src/plugin/plugin_manager.cpp
//  */
// #include <map>
// #include <memory>
// #include <optional>
// #include <prismcascade/ast/node.hpp>
// #include <prismcascade/common/types.hpp>
// #include <prismcascade/plugin/dll_memory_manager.hpp>
// #include <prismcascade/plugin/dynamic_library.hpp>
// #include <string>
// #include <unordered_set>
// #include <vector>

// namespace prismcascade::plugin {

// /* ───── PluginManager ───────────────────────────────────────── */
// class PluginManager {
//    public:
//     DllMemoryManager dll_memory_manager;
//     HandleManager    handle_manager;

//     explicit PluginManager(const std::optional<std::string>& plugin_dir = std::nullopt);

//     /* -------- ノード生成／入力設定 ----------------------------- */
//     std::shared_ptr<ast::AstNode> make_node(const std::string& uuid, std::weak_ptr<ast::AstNode> parent = {});

//     void assign_input(std::shared_ptr<ast::AstNode> node, int index, ast::AstNode::input_t value);

//     /* -------- プラグイン呼び出し ------------------------------- */
//     bool invoke_start_rendering(std::shared_ptr<ast::AstNode> node);
//     bool invoke_render_frame(std::shared_ptr<ast::AstNode> node, int frame);
//     void invoke_finish_rendering(std::shared_ptr<ast::AstNode> node);

//    private:
//     /* UUID → PluginHandlerID */
//     std::map<std::string, std::int64_t> uuid_to_handler_;

//     /* handler → 動的ライブラリ */
//     std::map<std::int64_t, plugin::DynamicLibrary> libraries_;

//     /* internal helpers */
//     std::vector<VariableType> infer_input_type(const ast::AstNode::input_t&);

//     void remove_old_refs(const ast::AstNode::input_t&, const std::shared_ptr<ast::AstNode>&);
//     void add_new_refs(const ast::AstNode::input_t&, const std::shared_ptr<ast::AstNode>&);

//     void gather_subtree_nodes(const std::shared_ptr<ast::AstNode>&               root,
//                               std::unordered_set<std::shared_ptr<ast::AstNode>>& out);
//     void detach_subtree_boundary(const std::unordered_set<std::shared_ptr<ast::AstNode>>&);
// };

// }  // namespace prismcascade::plugin
