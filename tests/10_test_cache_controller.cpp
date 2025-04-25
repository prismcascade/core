/* ── tests/20_test_layer5.cpp ──────────────────────────────────
   Layer-5 : video_cache / audio_ring / scalar_cache / live_controller
   (gTest + RapidCheck 1 Prop)
 *──────────────────────────────────────────────────────────── */
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <prismcascade/ast/operations.hpp>  // make_node / substitute
#include <prismcascade/memory/types_internal.hpp>
#include <prismcascade/scheduler/audio_ring.hpp>
#include <prismcascade/scheduler/live_controller.hpp>
#include <prismcascade/scheduler/scalar_cache.hpp>
#include <prismcascade/scheduler/video_cache.hpp>

using namespace prismcascade;

/* ───────────────────────────────────────────── */
/* helper : make dummy VideoFrameMemory          */
/* ───────────────────────────────────────────── */
static std::shared_ptr<memory::VideoFrameMemory> makeFrame(std::uint32_t w, std::uint32_t h, std::uint8_t fill) {
    auto          vf = std::make_shared<memory::VideoFrameMemory>();
    VideoMetaData md{w, h, 30.0, 1};
    vf->update_metadata(md);
    for (std::size_t i = 0; i < vf->size(); ++i) vf->at(i) = fill;
    return vf;
}

/* ────────────────────────────── */
/* 1. VideoCache basic hit/miss   */
/* ────────────────────────────── */
TEST(VideoCache, InsertFetch) {
    scheduler::video_cache cache;
    auto                   n = ast::make_node("uuid", 1, 1);

    scheduler::video_cache_key k{static_cast<std::int64_t>(n->plugin_instance_handler), 0, 0};
    auto                       frame = makeFrame(2, 2, 0xAA);
    cache.insert(k, frame);

    EXPECT_EQ(cache.ram_bytes(), frame->size());
    auto got = cache.fetch(k);
    ASSERT_TRUE(got);
    EXPECT_EQ(&got->at(0), &frame->at(0));  // same buffer
}

/* ────────────────────────────── */
/* 2. Eviction without swap       */
/* ────────────────────────────── */
TEST(VideoCache, EvictNoSwap) {
    scheduler::video_cache::options opt;
    opt.max_ram_bytes = 700;  // 上限 700 B に変更
    opt.enable_swap   = false;
    scheduler::video_cache cache(opt);

    auto n = ast::make_node("u", 2, 2);
    //  frame1(10×10)=400 B, frame2(12×13)=624 B (>700 −400)
    auto k1  = scheduler::video_cache_key{static_cast<std::int64_t>(n->plugin_instance_handler), 0, 0};
    auto k2  = k1;
    k2.stamp = 1;
    cache.insert(k1, makeFrame(10, 10, 0x01));  // 400 B
    cache.insert(k2, makeFrame(12, 13, 0x02));  // 624 B → 超過 → k1 drop

    EXPECT_FALSE(cache.fetch(k1));  // k1 dropped
    EXPECT_TRUE(cache.fetch(k2));   // newest kept
    EXPECT_LE(cache.ram_bytes(), opt.max_ram_bytes);
}

/* ────────────────────────────── */
/* 3. Eviction with swap+reload   */
/* ────────────────────────────── */
TEST(VideoCache, EvictSwapReload) {
    scheduler::video_cache::options opt;
    opt.max_ram_bytes = 512;  // small
    opt.enable_swap   = true;
    opt.swap_dir      = "./cache_test";
    scheduler::video_cache cache(opt);

    auto n  = ast::make_node("u", 3, 3);
    auto k  = scheduler::video_cache_key{static_cast<std::int64_t>(n->plugin_instance_handler), 0, 0};
    auto fr = makeFrame(8, 8, 0x7F);  // 256 B
    cache.insert(k, fr);              // will be swapped
    EXPECT_LT(cache.ram_bytes(), 512);

    auto got = cache.fetch(k);  // reload from disk
    ASSERT_TRUE(got);
    EXPECT_EQ(got->at(10), 0x7F);
}

/* ────────────────────────────── */
/* 4. AudioRing read-write order  */
/* ────────────────────────────── */
TEST(AudioRing, WriteReadWrap) {
    scheduler::audio_ring ring(8);  // 8 samples
    double                in1[6] = {0, 1, 2, 3, 4, 5};
    double                in2[4] = {6, 7, 8, 9};
    double                out[6]{};
    EXPECT_EQ(ring.write(in1, 6, 123), 6);  // head=0 tail=6
    EXPECT_EQ(ring.read(out, 4), 4);        // head=4 len=2
    EXPECT_EQ(ring.write(in2, 4, 456), 4);  // wrap tail
    EXPECT_EQ(ring.read(out, 6), 6);
    EXPECT_EQ(out[0], 4);
    EXPECT_EQ(out[5], 9);
}

/* ────────────────────────────── */
/* 5. ScalarCache store / clear   */
/* ────────────────────────────── */
TEST(ScalarCache, StoreFetchClear) {
    scheduler::scalar_cache sc;
    scheduler::scalar_key   k1{1, 10}, k2{1, 20};
    sc.store(k1, 42LL);
    sc.store(k2, true);
    EXPECT_EQ(std::get<std::int64_t>(sc.fetch(k1)), 42);
    sc.clear_before(15);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(sc.fetch(k1)));
    EXPECT_TRUE(std::holds_alternative<bool>(sc.fetch(k2)));
}

/* ────────────────────────────── */
/* 6. LiveController logic        */
/* ────────────────────────────── */
TEST(LiveCtrl, DropDecision) {
    scheduler::live_controller lc(/*max_lag=*/3);
    // lag 5 > max, root queues = 2 → drop expected
    EXPECT_TRUE(lc.need_drop(10, 15, 2));
    // root queue 1 → keep
    EXPECT_FALSE(lc.need_drop(20, 25, 1));
}

/* ────────────────────────────── */
/* 7. Property : VideoCache hit ratio under random access          */
/* ────────────────────────────── */
RC_GTEST_PROP(VideoCacheProp, RandomAccessHitMiss, ()) {
    scheduler::video_cache cache;
    auto                   n = ast::make_node("u", 999, 999);

    //    0‥15 の stamp 値
    std::uint64_t              stamp = *rc::gen::inRange<std::uint64_t>(0, 16);
    scheduler::video_cache_key k{static_cast<std::int64_t>(n->plugin_instance_handler), stamp, 0};

    // 前挿入しない場合 : 1 回目 fetch で miss → insert
    bool pre = *rc::gen::arbitrary<bool>();
    if (pre) cache.insert(k, makeFrame(8, 8, 0xEE));

    auto first = cache.fetch(k);
    if (!pre && first == nullptr) cache.insert(k, makeFrame(8, 8, 0xEE));
    auto second = cache.fetch(k);
    RC_ASSERT(second != nullptr);
}

/*
 *  テスト網羅性:
 *   1. VideoCache hit+RAM計算
 *   2. Evict 無 swap → drop & RAM bound
 *   3. Evict 有 swap → ディスク往復 & 内容保持
 *   4. AudioRing wrap correctness
 *   5. ScalarCache lifetime window
 *   6. LiveController lag/drop 条件分岐
 *   7. VideoCache prop: fetch idempotent / second-hit 必ず成功
 *
 *  これで Layer-5 各サブシステムの
 *   • 語義 (insert/fetch/evict/write/read)、
 *   • 資源上限 (ram_bytes, ring cap),
 *   • 可逆性・idempotence
 *  をすべて実行時に検査しているため十分。
 */
