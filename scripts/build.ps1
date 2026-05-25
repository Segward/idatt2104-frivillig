#!/usr/bin/env pwsh
$ErrorActionPreference = 'Stop'

$VcpkgPin = '05925b52317eb9a900a8583782e630aafd7a8929'
$VcpkgToolchain = 'vcpkg/scripts/buildsystems/vcpkg.cmake'

if (-not (Test-Path $VcpkgToolchain) -and (Test-Path .git)) {
  git submodule update --init --recursive
}

if (-not (Test-Path $VcpkgToolchain)) {
  if (Test-Path vcpkg) {
    Remove-Item -Recurse -Force vcpkg
  }
  git clone https://github.com/microsoft/vcpkg.git vcpkg
  git -C vcpkg checkout $VcpkgPin
}

cmake --preset default
cmake --build --preset default
