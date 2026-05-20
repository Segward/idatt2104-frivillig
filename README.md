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

The server executable is written to `build/server`.

## Run
Start the server:

```sh
./build/server
```

Then open <http://127.0.0.1:12345/> in a browser. The server hosts the demo page
and the WebSocket endpoint on the same port. Open the URL in multiple tabs to see
the counter sync live.

## Unit tests

```sh
ctest --preset default
```

## Third party dependencies
- GoogleTest; Google C++ testing and mocking framework.
- uWebSockets; WebSocket server library.
- nlohmann/json; JSON for Modern C++.
