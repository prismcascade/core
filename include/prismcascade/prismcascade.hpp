// include/prismcascade.hpp
#pragma once

//
// 基本型・C‑ABI
//
#include <prismcascade/common/abi.hpp>
#include <prismcascade/common/types.hpp>

//
// AST レイヤ
//
#include <prismcascade/ast/diff.hpp>
#include <prismcascade/ast/edge.hpp>
#include <prismcascade/ast/macro.hpp>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/ast/operations.hpp>
#include <prismcascade/ast/transformer.hpp>

//
// プラグイン & メモリ
//
#include <prismcascade/plugin/dll_memory_manager.hpp>
#include <prismcascade/plugin/dynamic_library.hpp>
#include <prismcascade/plugin/plugin_manager.hpp>

//
// スケジューラ
//
#include <prismcascade/scheduler/rendering_scheduler.hpp>

//
// ユーティリティ
//
#include <prismcascade/util/dump.hpp>
