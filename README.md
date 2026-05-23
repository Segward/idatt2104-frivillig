# CRDT Proof Of Concept Application

A small demo for poking at state-based CRDTs across browser tabs over a
TLS WebSocket. The server holds the master state in memory; each client
keeps a local copy and merges on sync, so edits made while offline still
converge once you reconnect.

> [!NOTE]
> On Windows, use the `.ps1` equivalents of the scripts below in PowerShell
> (e.g. `./scripts/cert.ps1`, `./scripts/build.ps1`).

## Implemented functionality
- PN counter, per-client increment and decrement maps merged by max-per-id.
- RGA list of strings with tombstoned deletes.
- RGA text for character-level editing in a textarea, with a diff against
  the rendered view so local typing turns into RGA inserts and deletes.
- TLS WebSocket server on uWebSockets that also serves the static client.
- Server-issued client ids, baked into element ids; auto-reconnect with
  exponential backoff on the client.
- Manual sync button that ships the full local state for merge and
  rebroadcast.

## Requirements
- CMake 3.25+
- A C++23 compiler
- Git
- OpenSSL
- make, pkg-config, curl, zip, unzip, tar
- certbot (optional, for a real domain cert)

## Install
> [!IMPORTANT]
> Clone with `--recurse-submodules` — vcpkg is vendored as a submodule.

```sh
git clone --recurse-submodules https://github.com/Segward/idatt2104-frivillig.git
cd idatt2104-frivillig
```

## Building the application
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

Then build:

```sh
./scripts/build.sh
```
> [!NOTE]
> The build copies the cert into the release folder. To pick up a new
> cert, either rerun `cert.sh` and rebuild from a clean `build/`, or
> drop the new files into `build/release/certs`.

## Run the application
> [!IMPORTANT]
> Complete [Building the application](#building-the-application) first.

Run the server binary produced by the build:

```sh
./build/release/server
```

Then open `https://localhost:12345` in your browser. 
With a deployment open `https://[your domain]` instead.

## Run the tests
> [!IMPORTANT]
> Complete [Building the application](#building-the-application) first.

The build also produces the GoogleTest suite. Run it with:

```sh
ctest --preset default
```

Or invoke the binary directly to filter by name:

```sh
./build/tests/tests --gtest_filter='text_rga_test.*'
```

## Future improvements
- State lives in memory. A server restart drops everything. Snapshotting
  or an external store would fix that.
- One shared document. Adding rooms would allow independent sessions on
  the same server.
- Sync is button-driven. Periodic or change-triggered sync, or op-based
  deltas, would remove the button and trim bandwidth.
- RGA tombstones are never garbage collected, so the document grows
  unboundedly with deletions. A causal-stability GC pass would bound it.
- Peer state is trusted. The counter saturates instead of overflowing,
  but a hostile peer can still pin it at `INT64_MAX` (merge is monotonic).
  Real auth and per-peer validation are needed before public exposure.
- `merge()` silently drops incoming nodes whose `previous_id` chain never
  resolves. Safe, but should be logged so malformed state is visible.
- Tests cover the CRDT cores and the wire handler. The WebSocket layer
  is untested.

## Third party
- GoogleTest: tests under `tests/`.
- uWebSockets: HTTP + WebSocket server with built-in TLS.
- OpenSSL: TLS backend for uWebSockets, and the CLI used by the cert
  scripts.
- nlohmann/json: wire format between server and client.
