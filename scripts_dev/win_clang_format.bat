
rem clang-format -i $(git ls-files *.cpp *.hpp *.h *.cxx)


@echo off
rem ──────────────────────────────────────────────────────────────
rem  clang-format-all.bat  ―  Git 管理下の C/C++ ソースを一括整形
rem  1. 事前に clang-format.exe を PATH に通しておく
rem  2. .clang-format がリポジトリ直下にあること
rem ──────────────────────────────────────────────────────────────

chcp 65001

setlocal enabledelayedexpansion

echo ===  Clang‑Format  run  ====================================
for /f "usebackq delims=" %%F in (`
        git ls-files *.c *.cc *.cpp *.cxx *.h *.hpp *.hxx
`) do (
    echo   formatting %%F
    clang-format -i "%%F"
)

echo ===  Done  ================================================
endlocal

