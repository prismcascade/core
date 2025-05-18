#!/usr/bin/env bash
set -euo pipefail

# -------- 共通設定 --------
BUILD_DIR=build
BUILD_TYPE=RelWithDebInfo       # 他に Debug / Release など

# -------- 準備 --------
mkdir -p "${BUILD_DIR}"
cd       "${BUILD_DIR}"

# -------- CMake 生成 --------
cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" ..

# -------- ビルド (並列) --------
cmake --build . --parallel

# -------- テスト実行 --------
echo "===== Running unit / integration tests ====="
# CTest は自動で gtest / RapidCheck 実行ファイルを検出する
ctest --output-on-failure --timeout 300

# -------- プラグインのインストール --------
mkdir -p "cli/plugins"
cp -v sample_plugins/*.so cli/plugins/

# -------- CLI 実行 (オプション) --------
if [[ -x cli/prismcascade_cli ]]; then
    echo "===== Running CLI ====="
        cli/prismcascade_cli
        else
            echo "CLI binary not found (cli/prismcascade_cli)."
            fi

