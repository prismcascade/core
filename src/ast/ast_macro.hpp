#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace PrismCascade {
    struct AstNode;

    struct MacroExpansion {
        // 生成されたノード一覧
        // [depth, ast_node]
        // 外側のコンテナが vector で良いのかは要検討
        std::vector<std::pair<int, std::shared_ptr<AstNode>>> generated_ast;

        MacroExpansion expand_macro(std::shared_ptr<AstNode> macro_node,
            std::shared_ptr<AstNode> argument);

        // TODO: 必要なものを定義
    };
}
