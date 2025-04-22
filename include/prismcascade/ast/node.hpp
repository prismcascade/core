#pragma once
#include <memory>
#include <optional>
#include <prismcascade/ast/edge.hpp>
#include <prismcascade/common/types.hpp>
#include <prismcascade/memory/types_internal.hpp>
#include <string>
#include <variant>
#include <vector>

namespace prismcascade::ast {

struct Node {
    // プラグイン実行用ハンドラ
    std::uint64_t plugin_handler{};
    std::uint64_t plugin_instance_handler{};

    // 変化しないメタデータ複製
    PluginMetaDataInternal meta;

    // 入力
    using Input = std::variant<std::monostate, std::shared_ptr<Node>,  // メイン出力を通じた AST 接続
                               SubEdge,                                // サブ出力参照
                               std::int64_t, bool, double, std::string, VectorParam, VideoFrame, AudioParam>;

    std::vector<Input> inputs;

    // 木構造
    std::weak_ptr<Node>                 parent;
    std::vector<std::weak_ptr<SubEdge>> sub_output_edges;

    // Clip として振る舞う場合
    std::optional<VideoMetaData> video_meta;
    std::optional<AudioMetaData> audio_meta;
};

}  // namespace prismcascade::ast
