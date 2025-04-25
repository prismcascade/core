// include/prismcascade/scheduler/rendering_scheduler.hpp
#pragma once
/**
 *  Prism Cascade – RenderingScheduler (interface)
 *  ------------------------------------------------
 *  AST ルートを受け取り、トポロジカルソートと
 *  変数寿命解析を行う純ヘッダ API。
 *
 *  実装は  src/scheduler/rendering_scheduler.cpp
 */
#include <map>
#include <memory>
#include <prismcascade/ast/node.hpp>
#include <tuple>
#include <vector>

namespace prismcascade::scheduler {

/**
 * topological_sort
 * ------------------------------------------------------------
 * @param ast_root  AST のルートノード（主出力ツリー）
 *
 * @return  (A, B, C)
 *    A : 循環部を除いたトポロジカル順ノード列
 *    B : 循環を構成するノード列（plugin_instance_handler 順）
 *    C : 変数寿命差分テーブル
 *        C[i] は A[i] の直前で
 *          (child_instance_id, output_index) -> +1 / -1
 *        を表し、+n は生成、-n は消費を示す。
 */
using LifetimeTable = std::vector<std::map<std::pair<std::int64_t, std::int32_t>, std::int64_t>>;

std::tuple<std::vector<std::shared_ptr<ast::AstNode>>,  // sorted
           std::vector<std::shared_ptr<ast::AstNode>>,  // cycles
           LifetimeTable> topological_sort(const std::shared_ptr<ast::AstNode>& ast_root);

}  // namespace prismcascade::scheduler
