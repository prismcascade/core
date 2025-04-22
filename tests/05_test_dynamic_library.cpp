// // tests/unit/test_dynamic_library.cpp
// //
// // 範囲: plugin::DynamicLibrary
// //   1) 存在しないライブラリをロードすると例外
// //   2) list_plugins() が例外を投げず vector を返す
// //   3) 空ディレクトリを渡すと結果が空
// //
// #include <gtest/gtest.h>

// #include <filesystem>
// #include <prismcascade/plugin/dynamic_library.hpp>

// using prismcascade::plugin::DynamicLibrary;
// namespace fs = std::filesystem;

// /* ───────────────────────────────────────────────────────────── */
// /*  1. 失敗ロードは std::runtime_error                          */
// /* ───────────────────────────────────────────────────────────── */
// TEST(DynamicLibrary, LoadFailThrows) {
// #ifdef _WIN32
//     const char* dummy = "definitely_not_exist.dll";
// #else
//     const char* dummy = "definitely_not_exist.so";
// #endif
//     EXPECT_THROW({ DynamicLibrary lib(dummy); }, std::runtime_error);
// }

// /* ───────────────────────────────────────────────────────────── */
// /*  2. list_plugins() は常に安全に実行できる                     */
// /* ───────────────────────────────────────────────────────────── */
// TEST(DynamicLibrary, ListPluginsNoThrow) {
//     EXPECT_NO_THROW({
//         auto list = DynamicLibrary::list_plugins(std::nullopt);
//         // 返り値は空でもよい（CI 環境にプラグインが無い可能性あり）
//         EXPECT_GE(list.size(), 0u);
//     });
// }

// /* ───────────────────────────────────────────────────────────── */
// /*  3. 空ディレクトリ → 結果ベクタが空                          */
// /* ───────────────────────────────────────────────────────────── */
// TEST(DynamicLibrary, ListPluginsEmptyDir) {
//     fs::path tmp = fs::temp_directory_path() / "pc_empty_plugins";
//     fs::create_directories(tmp);
//     auto list = DynamicLibrary::list_plugins(tmp.string());
//     EXPECT_TRUE(list.empty());
//     fs::remove_all(tmp);
// }
