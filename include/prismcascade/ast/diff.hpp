#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <prismcascade/ast/node.hpp>
#include <vector>

namespace prismcascade::ast {

struct DiffStep {
    enum class Type { InsertNode, DeleteNode, ReplaceNode, ConnectInput, DisconnectInput, ReconnectSubEdge };

    Type                       action{};
    std::shared_ptr<Node>      target{};
    std::optional<Node::Input> old_value;
    std::optional<Node::Input> new_value;
    std::int32_t               index{};
};

struct Diff {
    std::vector<DiffStep> steps;
};

}  // namespace prismcascade::ast
