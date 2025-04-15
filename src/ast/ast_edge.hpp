#pragma once

#include <cstdint>
#include <memory>
#include <common/types.hpp>

namespace PrismCascade {

struct AstNode;

struct SubEdge {
    VariableType type{};

    std::weak_ptr<AstNode> from_node;
    std::int32_t input_index{};

    std::weak_ptr<AstNode> to_node;
    std::int32_t output_index{};
};

}
