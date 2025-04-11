#pragma once

#include <string>
// #include <format>
#include <optional>
#include <set>
#include <mutex>
#include <atomic>
#include <core/project_data.hpp>
#include <plugin/plugin_manager.hpp>

namespace PrismCascade {

class RenderingScheduler {
public:
    // std::shared_ptr<AstNode> ast;
    // std::shared_ptr<PluginManager> manager;

    static std::tuple<std::vector<std::shared_ptr<AstNode>>, std::vector<std::shared_ptr<AstNode>>, std::vector<std::map<std::pair<std::int64_t, std::int32_t>, std::int64_t>>>
    topological_sort(const std::shared_ptr<AstNode>& ast_root);

private:

};

}
