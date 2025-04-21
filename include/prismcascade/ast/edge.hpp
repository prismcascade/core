#pragma once
#include <cstdint>
#include <memory>
#include <prismcascade/common/types.hpp>

namespace prismcascade::ast {

struct Node;

struct SubEdge {
    VariableType        type{};
    std::weak_ptr<Node> from;
    std::int32_t        from_index{};
    std::weak_ptr<Node> to;
    std::int32_t        to_index{};
};

}  // namespace prismcascade::ast
