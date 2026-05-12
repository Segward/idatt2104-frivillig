# idatt2104-frivillig

## Prerequisites

- CMake 3.25+
- A C++23 compiler
- Git (with submodules)

This project uses [vcpkg](https://github.com/microsoft/vcpkg) as a git submodule for dependencies. After cloning, initialize it:

```sh
git submodule update --init --recursive
```

## Build

Configure and build using the provided CMake preset:

```sh
cmake --preset default
cmake --build --preset default
```

The resulting executable is written to `build/idatt2104`.

## Run

```sh
./build/idatt2104
```

## Run unit tests

```sh
ctest --preset default
```
