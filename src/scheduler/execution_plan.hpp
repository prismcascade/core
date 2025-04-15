#pragma once

#include <ast/ast_node.hpp>
#include <cstdint>
#include <vector>
#include <memory>
#include <map>

namespace PrismCascade {

struct ExecutionUnit {
    std::shared_ptr<AstNode> node;
    std::vector<std::shared_ptr<AstNode>> dependencies;
};


// (plugin_instance_id, output_index) -> 寿命差分 (+: 値の生成, -: 値の消費)
using LifetimeMap = std::map<std::pair<std::int64_t, std::int32_t>, std::int64_t>;

class RenderingScheduler {
public:
    // NOTE: macro によるノード展開で得られたASTサブツリーについては，個別に topological sort する必要がある
    // Ast -> (トポロジカルソートしたもの（循環部分を除く）, 循環があったノード一覧（plugin_instance_handler順）， 寿命管理用配列 [(plugin_instance_id, output_index) -> 変化量] )
    // 寿命管理用配列： 第一返却値と同じ順番で「ある副出力を要求するところで+n/消費するところで-1」として，0になった時点以降では（そのフレームの描画では）使われない，という性質のもの
    static std::tuple<std::vector<std::shared_ptr<AstNode>>, std::vector<std::shared_ptr<AstNode>>, LifetimeMap> topological_sort(const std::shared_ptr<AstNode>& ast_root);

private:
};

}
