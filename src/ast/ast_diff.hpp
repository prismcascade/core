#pragma once

#include <ast_node.hpp>
#include <cstdint>
#include <optional>
#include <memory>

namespace PrismCascade {
struct AstDiffStep {
    enum class Type {
        InsertNode,
        DeleteNode,
        ReplaceNode,
        ConnectInput,
        DisconnectInput,
        ReconnectSubEdge
    };

    Type action;
    std::shared_ptr<AstNode> target;
    std::optional<AstInput> old_value;
    std::optional<AstInput> new_value;
    int32_t index{};
};

struct AstDiff {
    std::vector<AstDiffStep> steps;
};

}
