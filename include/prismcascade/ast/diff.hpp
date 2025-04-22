#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <prismcascade/ast/node.hpp>
#include <vector>

namespace prismcascade::ast {

struct DiffStep {
    enum class Type { InsertNode, DeleteNode, ReplaceNode, ConnectInput, DisconnectInput, ReconnectSubEdge };

    Type                            action{};
    std::shared_ptr<AstNode>        target{};
    std::optional<AstNode::input_t> old_value;
    std::optional<AstNode::input_t> new_value;
    std::int32_t                    index{};
};

struct Diff {
    std::vector<DiffStep> steps;
};

}  // namespace prismcascade::ast
