#pragma once

#include <plugin/dynamic_library.hpp>
#include <core/project_data.hpp>
#include <memory/memory_manager.hpp>
#include <render/rendering_scheduler.hpp>
#include <string>
#include <optional>
#include <unordered_set>
#include <map>

namespace PrismCascade {


class HandleManager {
public:
    std::string fresh_id(){
        char buffer[15]{};
        snprintf(buffer, std::size(buffer), "%012ld", static_cast<long>(current_id++));
        return {buffer};
        // return std::format("{:012d}", current_id++);  // C++20
    }

    std::int64_t fresh_handler(){
        return current_id++;
    }

private:
    std::atomic_int64_t current_id{};
};

class PluginManager {
public:
    DllMemoryManager dll_memory_manager;
    HandleManager handle_manager;
    PluginManager();
    PluginManager(const std::optional<std::string>& path);
    ~PluginManager();

    std::shared_ptr<AstNode> make_node(std::string uuid, std::weak_ptr<AstNode> parent = std::weak_ptr<AstNode>());
    void assign_input(std::shared_ptr<AstNode> node, int index, AstNode::input_t input_value);

    bool invoke_start_rendering(std::shared_ptr<AstNode> node);
    bool invoke_render_frame(std::shared_ptr<AstNode> node, int frame);
    void invoke_finish_rendering(std::shared_ptr<AstNode> node);

    // TODO: この辺は，macroプラグインからのAST操作要求に対応した後で考える
    // effect_name -> effect_pointer
    // EffectHandler* getEffectHandler(const std::string& effectType);

private:
    std::map<std::uint64_t, DynamicLibrary> dynamic_libraries;

    std::vector<VariableType> infer_input_type(const AstNode::input_t& input_value);
    void remove_old_references(const AstNode::input_t& old_value, std::shared_ptr<AstNode> node);
    void add_new_references(const AstNode::input_t& new_value, std::shared_ptr<AstNode> node);
    void gather_subtree_nodes(std::shared_ptr<AstNode> start, std::unordered_set<std::shared_ptr<AstNode>>& visited);
    void remove_subtree_boundary_references(const std::unordered_set<std::shared_ptr<AstNode>>& subtree_set);

    // std::map<std::string, PluginHandle> plugins;
    // std::map<std::string, EffectHandler*> effectRegistry;
};

}
