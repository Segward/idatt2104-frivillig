# idatt2104-frivillig

## Requirements
- CMake 3.25+
- A C++23 compiler
- Git (with submodules)

## Install
To install this project you need to clone this repository:

```sh
git clone https://github.com/Segward/idatt2104-frivillig.git
```

This project uses [vcpkg](https://github.com/microsoft/vcpkg) as a git submodule.
After cloning the project you need to initialize it:

```sh
git submodule update --init --recursive
```

## Build
Configure and build using the provided CMake preset:

```sh
cmake --preset default
cmake --build --preset default
```

The executable is written to `build/idatt2104`.

## Run

```sh
./build/idatt2104
```

## Unit tests

```sh
ctest --preset default
```

## Third party dependencies
- GoogleTest; Google C++ testing and mocking framework.
- Sockpp; Modern C++ socket library.
