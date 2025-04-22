#pragma once
#include <cstdint>
#include <memory>

namespace prismcascade::ast {

struct AstNode;

struct SubEdge {
    std::weak_ptr<AstNode> source;
    std::uint64_t          output_index{};
};

}  // namespace prismcascade::ast
