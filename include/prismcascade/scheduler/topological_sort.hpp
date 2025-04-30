#pragma once
#include <cstdint>
#include <memory>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/scheduler/time.hpp>
#include <unordered_map>
#include <vector>

namespace prismcascade::scheduler {

struct RankInfo {
    std::int64_t                rank               = 0;  // 葉 0  → 根へ昇順
    std::int64_t                last_consumer_rank = 0;  // 変数寿命 (Δtimeと両方の検査が必要)
    timestamp_t                 pre_ts             = 0;  // DFS pre  順
    timestamp_t                 post_ts            = 0;  // DFS post 順
    std::optional<std::int64_t> delay              = 0;
};

struct TopoResult {
    std::unordered_map<std::int64_t, RankInfo> ranks;       // 成功時のみ非空
    std::vector<std::shared_ptr<ast::AstNode>> cycle_path;  // 閉路パス (失敗時)
};

/// DAG をトポロジカルソートして Rank 情報を返す。
/// 失敗（閉路検出）時は cycle_path にパスを格納し ranks は空。
TopoResult topological_sort(const std::shared_ptr<ast::AstNode>& root);

}  // namespace prismcascade::scheduler
