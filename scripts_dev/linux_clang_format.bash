#!/bin/bash
set -eu

clang-format -i $(git ls-files *.cpp *.hpp *.h *.cxx)

