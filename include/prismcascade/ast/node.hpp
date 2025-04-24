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

struct AstNode {
    struct SubOutputRef {
        std::uint64_t          src_index;  // this‐node の副出力 index
        std::weak_ptr<AstNode> dst_node;   // 参照しているノード
        std::uint64_t          dst_slot;   // dst_node の入力スロット
    };

    // 入力
    // TODO: 将来的にはVectorParam に即値を入れられるようにする？
    using input_t = std::variant<std::monostate,
                                 std::shared_ptr<AstNode>,  // メイン出力を通じた AST 接続
                                 SubEdge,                   // サブ出力参照
                                 std::int64_t, bool, double, std::string>;

    // handler は実行ごとに変化する，勝手につけた通し番号
    std::uint64_t plugin_handler{};
    std::uint64_t plugin_instance_handler{};
    // uuidは，プラグイン側で決められる識別ID
    std::string plugin_uuid{};

    // メタデータからも辿れるが，処理の途中で途中で変化しない値なので，扱いやすさのためにもコピーをここに置く
    std::int32_t protocol_version{};
    PluginType   plugin_type{};
    std::string  plugin_name{};

    // 主出力先
    std::weak_ptr<AstNode> parent;
    // 副出力先 (src_index, dst_node, dst_index)
    std::vector<SubOutputRef> sub_output_references;

    // 入力
    std::vector<input_t> inputs;

    // DLL側のメモリ
    std::shared_ptr<memory::ParameterPackMemory> input_parameters;
    std::shared_ptr<memory::ParameterPackMemory> output_parameters;

    // Clip として振る舞う場合
    std::optional<VideoMetaData> video_meta;
    std::optional<AudioMetaData> audio_meta;
};

}  // namespace prismcascade::ast
