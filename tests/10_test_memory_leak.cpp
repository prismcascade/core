//
// ビルド条件
//   * ASan 有効ビルドでは実行してリークが 0 であることを確認
//   * ASan が無効なときはテストをスキップ
//
#include <gtest/gtest.h>

#include <prismcascade/plugin/dll_memory_manager.hpp>

using namespace prismcascade;
using plugin::DllMemoryManager;

/* --- コンパイラ別 ASan 検知マクロ --------------------------- */
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define PC_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define PC_ASAN 1
#endif
/* ------------------------------------------------------------- */

#if PC_ASAN
TEST(MemoryLeak, DllMemoryManagerStress) {
    DllMemoryManager mm;

    for (int iter = 0; iter < 1000; ++iter) {
        // TextParam allocate / free
        TextParam txt{};
        ASSERT_TRUE(mm.assign_text(&txt, "stress test string"));
        mm.free_text(&txt);

        // Vector<Int> allocate / free
        VectorParam vec{};
        vec.type = VariableType::Int;
        ASSERT_TRUE(mm.allocate_vector(&vec, 128));
        mm.free_vector(&vec);

        // VideoFrame / AudioParam allocate / free
        VideoMetaData md{.width = 16, .height = 16, .fps = 30.0, .total_frames = 1};
        VideoFrame    vf{};
        ASSERT_TRUE(mm.allocate_video(&vf, md));
        mm.free_video(&vf);

        AudioParam ap{};
        ASSERT_TRUE(mm.allocate_audio(&ap));
        mm.free_audio(&ap);
    }

    // dump_memory_usage() で distinct=0 を期待
    mm.dump_memory_usage();
    SUCCEED();  // 実際のリーク検知は ASan が行う
}
#else
TEST(MemoryLeak, DISABLED_AcceptWhenNoASan) { GTEST_SKIP() << "AddressSanitizer OFF – leak test skipped."; }
#endif
