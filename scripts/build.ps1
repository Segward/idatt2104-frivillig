#!/usr/bin/env pwsh
$ErrorActionPreference = 'Stop'

cmake --preset default
cmake --build --preset default
