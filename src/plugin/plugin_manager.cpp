// // src/plugin/plugin_manager.cpp
// #include <cassert>
// #include <common/abi.hpp>  // C‑ABI typedef
// #include <iostream>
// #include <plugin/plugin_manager.hpp>

// namespace pc    = prismcascade;
// namespace pcpl  = prismcascade::plugin;
// namespace pcast = prismcascade::ast;
// using pc::VariableType;

// /* ───────────────────── HandleManager impl ───────────────────── */
// std::string pcpl::HandleManager::fresh_id() {
//     char buf[16]{};
//     std::snprintf(buf, sizeof(buf), "%012lld", static_cast<long long>(current_.fetch_add(1)));
//     return buf;
// }
// std::int64_t pcpl::HandleManager::fresh_handler() { return current_.fetch_add(1); }

// /* ───────────────────── ctor : load every plugin ─────────────── */
// pcpl::PluginManager::PluginManager(const std::optional<std::string>& plugin_dir) {
//     using pcpl::DynamicLibrary;

//     for (auto& path : DynamicLibrary::list_plugins(plugin_dir)) {
//         try {
//             DynamicLibrary lib(path);
//             auto           handler = handle_manager.fresh_handler();

//             /* resolve getMetaInfo */
//             auto getMeta = reinterpret_cast<fn_getMetaInfo>(lib["getMetaInfo"]);

//             pc::PluginMetaData meta{};
//             getMeta(
//                 /*host*/ &dll_memory_manager,
//                 /*self*/ handler,
//                 /*out*/ &meta, dll_memory_manager.allocate_param_static, dll_memory_manager.assign_text_static,
//                 dll_memory_manager.add_required_handler_static, dll_memory_manager.add_handleable_effect_static);

//             uuid_to_handler_[meta.uuid.buffer] = handler;
//             libraries_.emplace(handler, std::move(lib));

//             std::cerr << "[PluginManager] loaded " << meta.name.buffer << " (" << meta.uuid.buffer << ")\n";
//         } catch (const std::exception& e) { std::cerr << "[PluginManager] skip " << path << " : " << e.what() <<
//         "\n"; }
//     }
// }

// /* ───────────────────── make_node / assign_input ─────────────── */
// std::shared_ptr<pcast::Node> pcpl::PluginManager::make_node(const std::string&         uuid,
//                                                             std::weak_ptr<pcast::Node> parent) {
//     auto it = uuid_to_handler_.find(uuid);
//     if (it == uuid_to_handler_.end()) throw std::runtime_error("plugin uuid not loaded: " + uuid);

//     auto node                     = std::make_shared<pcast::Node>();
//     node->plugin_handler          = it->second;
//     node->plugin_instance_handler = handle_manager.fresh_handler();
//     node->parent                  = std::move(parent);
//     node->meta.uuid               = std::move(uuid);
//     // name 等は TODO: キャッシュからコピー
//     return node;
// }

// void pcpl::PluginManager::assign_input(std::shared_ptr<pcast::Node> node, int idx, pcast::Node::Input val) {
//     if (static_cast<std::size_t>(idx) >= node->inputs.size()) node->inputs.resize(idx + 1);
//     remove_old_refs(node->inputs[idx], node);
//     node->inputs[idx] = std::move(val);
//     add_new_refs(node->inputs[idx], node);
// }

// /* ───────────────────── lifecycle stubs ──────────────────────── */
// bool pcpl::PluginManager::invoke_start_rendering(std::shared_ptr<pcast::Node>) { return true; }

// bool pcpl::PluginManager::invoke_render_frame(std::shared_ptr<pcast::Node>, int) { return true; }

// void pcpl::PluginManager::invoke_finish_rendering(std::shared_ptr<pcast::Node>) {}

// /* ───────────────────── helper stubs (未実装) ─────────────────── */
// std::vector<VariableType> pcpl::PluginManager::infer_input_type(const pcast::Node::Input&) { return {}; }

// void pcpl::PluginManager::remove_old_refs(const pcast::Node::Input&, const std::shared_ptr<pcast::Node>&) {}
// void pcpl::PluginManager::add_new_refs(const pcast::Node::Input&, const std::shared_ptr<pcast::Node>&) {}

// void pcpl::PluginManager::gather_subtree_nodes(const std::shared_ptr<pcast::Node>&,
//                                                std::unordered_set<std::shared_ptr<pcast::Node>>&) {}
// void pcpl::PluginManager::detach_subtree_boundary(const std::unordered_set<std::shared_ptr<pcast::Node>>&) {}
