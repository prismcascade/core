// tests/unit/test_types.cpp
//
// ビルドに必要なもの:
//   target_link_libraries(<exe> PRIVATE prismcascade gtest_main)
//
// 検証項目
//   1. 列挙型 VariableType が 7 要素あること
//   2. VideoMetaData / AudioMetaData のデフォルト値
//   3. TextParam など構造体サイズが非ゼロ
//   4. ParameterPack の 0 初期化
//
// GoogleTest を使用
//
#include <gtest/gtest.h>

#include <prismcascade/common/types.hpp>

using namespace prismcascade;

TEST(Types, EnumCount) {
    // C++17 では std::to_underlying が無いので static_cast
    EXPECT_EQ(static_cast<int>(VariableType::Audio) + 1, 7);
}

TEST(Types, VideoMetaDefault) {
    VideoMetaData v{};
    EXPECT_EQ(v.width, 0u);
    EXPECT_EQ(v.height, 0u);
    EXPECT_EQ(v.fps, 0.0);
    EXPECT_EQ(v.total_frames, 0u);
}

TEST(Types, AudioMetaDefault) {
    AudioMetaData a{};
    EXPECT_EQ(a.total_samples, 0u);
}

TEST(Types, StructSizeNonZero) {
    EXPECT_GT(sizeof(TextParam), 0u);
    EXPECT_GT(sizeof(VectorParam), 0u);
    EXPECT_GT(sizeof(VideoFrame), 0u);
    EXPECT_GT(sizeof(AudioParam), 0u);
}

TEST(Types, ParameterPackInit) {
    ParameterPack p{};
    EXPECT_EQ(p.size, 0);
    EXPECT_EQ(p.parameters, nullptr);
}
