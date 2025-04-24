#pragma once
/*───────────────────────────────────────────────────────────────
   トポロジカルソート (Kahn 法) ―――
   入力 : AST のルートノード
   出力 : handler 値をキーとする RankTable
   ─────────────────────────────────────────────────────────────*/
#include <cstdint>
#include <memory>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/scheduler/time.hpp>
#include <unordered_map>

namespace prismcascade::schedule {

struct RankInfo {
    std::uint32_t rank    = 0;  //!< 0-origin
    timestamp_t   pre_ts  = 0;  //!< DFS pre 訪問順
    timestamp_t   post_ts = 0;  //!< DFS post 訪問順
};

struct TopoResult {
    std::unordered_map<std::int64_t, RankInfo> ranks;       // 成功時のみ非空
    std::vector<std::shared_ptr<ast::AstNode>> cycle_path;  // 閉路(失敗時)
};

/// root を含む DAG をトポロジカルに並べ、RankTable を返す。
/// 循環を検出した場合は std::domain_error を投げる。
TopoResult topological_sort(const std::shared_ptr<ast::AstNode>& root);

}  // namespace prismcascade::schedule
