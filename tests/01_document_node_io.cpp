#include <gtest/gtest.h>

#include <iomanip>
#include <prismcascade/ast/operations.hpp>
#include <prismcascade/memory/types_internal.hpp>

using namespace prismcascade;

TEST(NodeInputOutputExample, SimpleInputOutput) {
    // ノード生成
    auto node = ast::make_node("node_uuid", 1, 1);

    // メタ情報を元に，ホスト側でメモリ確保 (入力)
    node->input_parameters->update_types(
        {{VariableType::Int}, {VariableType::Video}});  // 二重配列なのは，Vectorに対応するため

    ASSERT_EQ(node->input_parameters->types().at(1).at(0), VariableType::Video);  // 必ず通るはず
    if (auto video_ptr =
            std::dynamic_pointer_cast<memory::VideoFrameMemory>(node->input_parameters->buffer()[1])) {  // 必ず通るはず
        // channels は 4 固定 (RGBA)
        VideoMetaData metadata{1, 1, 30, 100};
        video_ptr->update_metadata(metadata);
    } else {
        throw std::runtime_error("impossible");
    }

    // メタ情報を元に，ホスト側でメモリ確保 (出力)
    node->output_parameters->update_types({{VariableType::Float}, {VariableType::Video}});

    if (auto video_ptr = std::dynamic_pointer_cast<memory::VideoFrameMemory>(
            node->output_parameters->buffer()[1])) {  // 必ず通るはず
        VideoMetaData metadata{1, 1, 24, 100};
        video_ptr->update_metadata(metadata);
    }

    // 入力をホスト側で詰める
    if (auto int_ptr =
            std::dynamic_pointer_cast<memory::IntParamMemory>(node->input_parameters->buffer()[0])) {  // 必ず通るはず
        int_ptr->buffer() = 42;
    }
    if (auto video_ptr =
            std::dynamic_pointer_cast<memory::VideoFrameMemory>(node->input_parameters->buffer()[1])) {  // 必ず通るはず
        video_ptr->at(0) = 10;                                                                           // 1px目 R
        video_ptr->at(1) = 20;                                                                           // 1px目 G
        video_ptr->at(2) = 30;                                                                           // 1px目 B
        video_ptr->at(3) = 40;                                                                           // 1px目 A
    }

    // プラグインに渡すポインタ
    ParameterPack input_parameter_pack  = node->input_parameters->get_paramter_struct();
    ParameterPack output_parameter_pack = node->output_parameters->get_paramter_struct();

    // この部分は本来プラグイン内で行う想定
    ASSERT_EQ(input_parameter_pack.size, 2);
    ASSERT_EQ(input_parameter_pack.parameters[0].type, VariableType::Int);
    const auto input_int   = reinterpret_cast<std::int64_t*>(input_parameter_pack.parameters[0].value);
    const auto input_video = reinterpret_cast<VideoFrame*>(input_parameter_pack.parameters[1].value);

    ASSERT_EQ(input_parameter_pack.parameters[1].type, VariableType::Video);

    ASSERT_EQ(output_parameter_pack.size, 2);
    ASSERT_EQ(output_parameter_pack.parameters[0].type, VariableType::Float);
    ASSERT_EQ(output_parameter_pack.parameters[1].type, VariableType::Video);
    const auto output_float = reinterpret_cast<double*>(output_parameter_pack.parameters[0].value);
    const auto output_video = reinterpret_cast<VideoFrame*>(output_parameter_pack.parameters[1].value);

    *output_float = static_cast<double>(*input_int) / 2;
    // TODO: output_video->current_frame は，ホスト側でセットする？ 要検討
    for (int i = 0; i < 4; ++i) { output_video->frame_buffer[i] = input_video->frame_buffer[i] * 2; }

    // ここからホストに戻った後の処理
    // 出力を取り出す
    if (auto float_ptr = std::dynamic_pointer_cast<memory::FloatParamMemory>(
            node->output_parameters->buffer()[0])) {  // 必ず通るはず
        EXPECT_EQ(float_ptr->buffer(), 21);
    }
    if (auto video_ptr = std::dynamic_pointer_cast<memory::VideoFrameMemory>(
            node->output_parameters->buffer()[1])) {  // 必ず通るはず
        EXPECT_EQ(video_ptr->at(0), 20);              // 1px目 R
        EXPECT_EQ(video_ptr->at(1), 40);              // 1px目 G
        EXPECT_EQ(video_ptr->at(2), 60);              // 1px目 B
        EXPECT_EQ(video_ptr->at(3), 80);              // 1px目 A
    }
}
