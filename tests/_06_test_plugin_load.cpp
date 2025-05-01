// tests/integration/test_plugin_load.cpp
//
// 目的:
//   * PluginManager が DLL / SO を動的ロードできること
//   * getMetaInfo が呼び出され UUID → ノード生成が成功すること
//   * start → frame → finish が true を返すこと
//
// 依存:
//   * plugins/twice/twice_plugin.{dll|so} が
//     ビルド生成ディレクトリ <exe>/plugins に配置されている
//
#include <gtest/gtest.h>

#include <prismcascade/plugin/plugin_manager.hpp>

// using namespace prismcascade;
// using plugin::PluginManager;

// TEST(PluginIntegration, TwicePluginLifecycle)
// {
//     // 「plugins」フォルダを自動探索するコンストラクタを使う
//     PluginManager pm;

//     // サンプル Twice プラグイン UUID
//     const std::string twice_uuid = "f0000000-0000-0000-0000-000000000000";

//     // ノード生成
//     auto node = pm.make_node(twice_uuid);
//     ASSERT_NE(node, nullptr);

//     // 呼び出しがエラーなく true を返す
//     ASSERT_TRUE(pm.invoke_start_rendering(node));
//     ASSERT_TRUE(pm.invoke_render_frame(node, 0));
//     pm.invoke_finish_rendering(node);

//     // plugin_manager がパラメータ領域を持っていることを確認
//     auto& mm = pm.dll_memory_manager;
//     ASSERT_TRUE(mm.parameter_pack_instances.count(node->plugin_instance_handler) > 0);
// }
