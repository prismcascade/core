add_executable(prismcascade_cli cli.cpp)
target_link_libraries(prismcascade_cli PRIVATE prismcascade)

# UTF‑8（MSVC）
target_compile_options(prismcascade_cli PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
