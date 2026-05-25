#!/usr/bin/env sh
set -eu

VCPKG_PIN=05925b52317eb9a900a8583782e630aafd7a8929
VCPKG_TOOLCHAIN=vcpkg/scripts/buildsystems/vcpkg.cmake

if [ ! -f "$VCPKG_TOOLCHAIN" ] && [ -d .git ]; then
  git submodule update --init --recursive
fi

if [ ! -f "$VCPKG_TOOLCHAIN" ]; then
  rm -rf vcpkg
  git clone https://github.com/microsoft/vcpkg.git vcpkg
  git -C vcpkg checkout "$VCPKG_PIN"
fi

cmake --preset default
cmake --build --preset default
