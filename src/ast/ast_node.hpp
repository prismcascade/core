#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <variant>
#include <plugin/plugin_metadata.hpp>
#include <ast/ast_edge.hpp>
#include <ast/ast_macro.hpp>

namespace PrismCascade {

using AstInput = std::variant<
    std::shared_ptr<AstNode>,   // メイン出力を通じたAST接続
    SubEdge,                    // サブ出力の参照
    std::int64_t,
    bool,
    double,
    std::string,
    VectorParam,
    VideoFrame,
    AudioBuffer
>;

struct AstNode {
	// handler は実行ごとに変化する，勝手につけた通し番号
	std::uint64_t plugin_handler{};
	std::uint64_t plugin_instance_handler{};

	// uuidは，プラグイン側で決められる識別ID
	std::string plugin_uuid{};

    // メタデータからも辿れるが，処理の途中で途中で変化しない値なので，扱いやすさのためにもコピーをここに置く
	std::int32_t protocol_version{};
	PluginType plugin_type{};
	std::string plugin_name{};

    // 入力リスト（即値，サブ出力参照，他ノードなど）
    std::vector<AstInput> inputs;

    // 出力先
    std::weak_ptr<AstNode> parent;
    std::vector<std::weak_ptr<SubEdge>> sub_output_edges;

    // これ自体が video clip や audio clip としてふるまう場合，そのメタデータが入る
    std::optional<VideoMetaData> video_meta;
    std::optional<AudioMetaData> audio_meta;

    // TODO: macro実装時に考える
    // std::optional<MacroExpansion> macro;

	static AstInput make_empty_value(const std::vector<VariableType>& types);
};

}
