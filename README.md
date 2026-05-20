# idatt2104-frivillig

## Requirements
- CMake 3.25+
- A C++23 compiler
- OpenSSL
- Git
- vcpkg build tools: `make`, `pkg-config`, `curl`, `zip`, `unzip`, `tar`

## Install
```sh
git clone --recurse-submodules https://github.com/Segward/idatt2104-frivillig.git
cd idatt2104-frivillig
```

## Build
```sh
./scripts/cert.sh
./scripts/build.sh
```

## Run
```sh
./build/release/server
```

## Certificate
`cert.sh` with no args generates a self-signed placeholder. For a real cert,
pass a domain:

```sh
sudo ./scripts/cert.sh [your domain]
```

## Third party
- GoogleTest: C++ unit testing framework
- uWebSockets: HTTP and WebSocket server with built-in TLS
- nlohmann/json: JSON parsing and serialization
