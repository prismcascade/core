#pragma once
#include <memory>
#include <utility>
#include <vector>

namespace prismcascade::ast {

struct Node;

struct MacroExpansion {
    // [深さ, ノード]
    std::vector<std::pair<int, std::shared_ptr<Node>>> generated_ast;

    // TODO: 展開ロジック
};

}  // namespace prismcascade::ast
