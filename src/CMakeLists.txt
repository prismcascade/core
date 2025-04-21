# ──────────────────────────────────────────────
# PrismCascade Core ライブラリ
# ──────────────────────────────────────────────

# 1. すべての cpp を再帰的に集める
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/ast/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/plugin/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/memory/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/scheduler/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")  # その他

# 2. 静的ライブラリ
add_library(prismcascade STATIC ${LIB_SOURCES})
add_library(prismcascade::core ALIAS prismcascade)

# 3. インクルードディレクトリ
target_include_directories(prismcascade
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE              # 実装側から src/ の内部ヘッダを参照可
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include/prismcascade
)

# 4. コンパイラオプション
target_compile_features(prismcascade PUBLIC cxx_std_20)

# UTF‑8（MSVC）
target_compile_options(prismcascade PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
