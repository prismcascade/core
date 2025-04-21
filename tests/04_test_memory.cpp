// tests/unit/test_memory.cpp
//
// 範囲: plugin::DllMemoryManager ― バッファ確保と共有ポインタの健全性
//
#include <gtest/gtest.h>

#include <prismcascade/plugin/dll_memory_manager.hpp>

using namespace prismcascade;
using plugin::DllMemoryManager;

/* ───────────────────────────────────────────────────────────── */
/*  TextParam: assign / copy / free                             */
/* ───────────────────────────────────────────────────────────── */
TEST(DllMemoryManager, TextAssignAndCopy) {
    DllMemoryManager mm;
    TextParam        a{}, b{};

    ASSERT_TRUE(mm.assign_text(&a, "hello"));
    mm.copy_text(&b, &a);

    EXPECT_EQ(a.buffer, b.buffer);  // 同じ共有文字列
    EXPECT_EQ(a.size, b.size);

    mm.free_text(&a);
    mm.free_text(&b);  // 重複 free でクラッシュしない
}

/* ───────────────────────────────────────────────────────────── */
/*  VectorParam (Int)                                           */
/* ───────────────────────────────────────────────────────────── */
TEST(DllMemoryManager, VectorAllocateInt) {
    DllMemoryManager mm;
    VectorParam      vec{};
    vec.type = VariableType::Int;

    ASSERT_TRUE(mm.allocate_vector(&vec, 4));
    EXPECT_EQ(vec.size, 4);
    auto* buf = static_cast<std::int64_t*>(vec.buffer);
    buf[0]    = 1;
    buf[1]    = 2;
    EXPECT_EQ(buf[0], 1);

    mm.free_vector(&vec);
}

/* ───────────────────────────────────────────────────────────── */
/*  VideoFrame allocate / copy / free                           */
/* ───────────────────────────────────────────────────────────── */
TEST(DllMemoryManager, VideoAllocateCopy) {
    DllMemoryManager mm;
    VideoFrame       v1{}, v2{};
    VideoMetaData    md{.width = 2, .height = 2, .fps = 30.0, .total_frames = 10};

    ASSERT_TRUE(mm.allocate_video(&v1, md));
    uint8_t* ptr1 = v1.frame_buffer;

    mm.copy_video(&v2, &v1);
    EXPECT_EQ(v2.frame_buffer, ptr1);  // 共有バッファ

    mm.free_video(&v1);
    mm.free_video(&v2);
}

/* ───────────────────────────────────────────────────────────── */
/*  AudioParam allocate / copy / free                           */
/* ───────────────────────────────────────────────────────────── */
TEST(DllMemoryManager, AudioAllocateCopy) {
    DllMemoryManager mm;
    AudioParam       a1{}, a2{};

    ASSERT_TRUE(mm.allocate_audio(&a1));
    double* p1 = a1.buffer;

    mm.copy_audio(&a2, &a1);
    EXPECT_EQ(a2.buffer, p1);

    mm.free_audio(&a1);
    mm.free_audio(&a2);
}
