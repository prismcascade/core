// include/prismcascade/util/dump.hpp
#pragma once
/**
 *  Prism Cascade – Debug Dump Helpers
 *  ----------------------------------
 *  * 依存先にソースを追加せず “ヘッダだけ” で手軽に診断できるよう、
 *    実装はすべて `inline` で提供する。
 *  * 出力は人間向け。フォーマットは後で変えても ABI に影響しない。
 */
#include <iostream>
#include <memory>
#include <prismcascade/ast/node.hpp>
#include <prismcascade/plugin/dll_memory_manager.hpp>
#include <string>
#include <unordered_set>

namespace prismcascade::util {

/* ───────────────────────────────────────────────────────────── */
/*  DLL メモリプールの概要                                      */
/* ───────────────────────────────────────────────────────────── */
inline void dump_memory_brief(const plugin::DllMemoryManager& mm) {
    std::cerr << "[Dump] ";
    mm.dump_memory_usage();
}

/* ───────────────────────────────────────────────────────────── */
/*  AST を再帰でダンプ                                          */
/* ───────────────────────────────────────────────────────────── */
inline void dump_ast(const std::shared_ptr<ast::Node>& n, int indent = 0) {
    if (!n) return;

    for (int i = 0; i < indent; ++i) std::cerr << "  ";
    std::cerr << "(" << n->plugin_instance_handler << ") " << n->meta.name << '\n';

    for (const auto& in : n->inputs) {
        if (std::holds_alternative<std::shared_ptr<ast::Node>>(in))
            dump_ast(std::get<std::shared_ptr<ast::Node>>(in), indent + 1);
    }
}

/* ───────────────────────────────────────────────────────────── */
/*  パラメータの値を簡易表示（Int / Float / Bool だけ）         */
/* ───────────────────────────────────────────────────────────── */
inline void dump_parameters(const plugin::DllMemoryManager& mm, const std::shared_ptr<ast::Node>& node) {
    using VT = VariableType;
    auto it  = mm.parameter_pack_instances.find(node->plugin_instance_handler);
    if (it == mm.parameter_pack_instances.end()) {
        std::cerr << "[Dump] no ParameterPack for instance " << node->plugin_instance_handler << '\n';
        return;
    }
    const auto& outPack = it->second.io_buffers[1].first;  // output side

    for (int i = 0; i < outPack.get()[0].type /*size placeholder*/; ++i) {
        const Parameter& p = outPack.get()[i];
        std::cerr << "  [" << i << "] ";
        switch (p.type) {
            case VT::Int:
                std::cerr << *reinterpret_cast<std::int64_t*>(p.value);
                break;
            case VT::Float:
                std::cerr << *reinterpret_cast<double*>(p.value);
                break;
            case VT::Bool:
                std::cerr << (*reinterpret_cast<bool*>(p.value) ? "true" : "false");
                break;
            default:
                std::cerr << "(unprintable)";
                break;
        }
        std::cerr << '\n';
    }
}

}  // namespace prismcascade::util
