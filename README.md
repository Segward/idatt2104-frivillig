# idatt2104-frivillig

## Requirements
- CMake 3.25+
- A C++23 compiler
- OpenSSL
- Git
- vcpkg build tools: `make`, `pkg-config`, `curl`, `zip`, `unzip`, `tar`

## Install
> [!IMPORTANT]
> Clone with `--recurse-submodules` — vcpkg is vendored as a submodule.

```sh
git clone --recurse-submodules https://github.com/Segward/idatt2104-frivillig.git
cd idatt2104-frivillig
```

## Setup
Generate a self-signed placeholder cert into `./certs/`:

```sh
./scripts/cert.sh
```

For a real cert, pass a domain:

```sh
sudo ./scripts/cert.sh [your domain]
```

> [!NOTE]
> Point the domain at this machine, forward port 80 to 80 (for cert
> issuance), and 443 to 12345 (the port the server listens on).

## Build and run
> [!IMPORTANT]
> Complete the [Setup](#setup) first — the server won't start without certs.

```sh
./scripts/build.sh
./build/release/server
```

## Third party
- GoogleTest: C++ unit testing framework
- uWebSockets: HTTP and WebSocket server with built-in TLS
- nlohmann/json: JSON parsing and serialization
