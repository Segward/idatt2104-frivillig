# CRDT Proof Of Concept Application

A small client/server demo that explores how state-based CRDTs behave when
multiple browser tabs edit the same data over a TLS WebSocket connection.
The server keeps the master state in memory and rebroadcasts merges; each
client also keeps its own copy and merges incoming state locally, so edits
made while offline still converge once the client reconnects and syncs.

> [!NOTE]
> On Windows, use the `.ps1` equivalents of the scripts below in PowerShell
> (e.g. `./scripts/cert.ps1`, `./scripts/build.ps1`).

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
> Complete the [Setup](#setup) first since the server won't start without certs.

```sh
./scripts/build.sh
./build/release/server
```

Then open `https://localhost:12345` in your browser. 
With a deployment open `https://[your domain]` instead.

## Run the tests
The build step also compiles the GoogleTest suite. After building, run it
from the build directory:

```sh
./scripts/build.sh
cd build && ctest --output-on-failure
```

Or invoke the test binary directly to filter by name:

```sh
./build/tests/tests --gtest_filter='TextRGA*'
```

## Implemented functionality
- PN counter CRDT with per-client increment and decrement maps, merged
  by max-per-id.
- RGA list CRDT for an ordered, append-and-remove list of strings, with
  tombstoned deletes.
- RGA text CRDT for character-level collaborative editing in a textarea,
  including diff-based local edits against the rendered view.
- TLS-terminated WebSocket server built on uWebSockets, serving the
  static client and broadcasting merges to all connected peers.
- Server-issued client ids used as the site identifier in CRDT element
  ids, with automatic client reconnect and exponential backoff.
- Manual sync button that ships the client's full local state to the
  server, where it is merged into the master and rebroadcast.

## Future improvements
- State is in-memory only; restarting the server drops all data. Adding
  a persistence layer (snapshot on disk, or an external store) would
  let sessions survive restarts.
- There is a single shared document. Adding rooms or document ids would
  allow multiple independent collaborations on the same server.
- Sync is manual via the button. Periodic or change-triggered sync, or
  moving to op-based deltas, would remove the need for an explicit
  action and reduce bandwidth on every merge.
- RGA tombstones are never garbage collected, so the document grows
  unboundedly with deletions. A causal-stability based GC pass would
  bound the state size.
- The only authentication is the server-issued client id; there is no
  access control. Adding real auth would be required before exposing
  the server publicly.
- Server-side coverage is limited to the CRDT cores and the wire
  handler. Adding integration tests around the WebSocket layer would
  catch regressions in connection handling.

## Third party
- GoogleTest: C++ unit testing framework, used for the test suite under
  `tests/`.
- uWebSockets: HTTP and WebSocket server with built-in TLS, used to
  serve the static client and run the realtime sync channel.
- OpenSSL: TLS backend used by uWebSockets, and used by the cert
  scripts to generate the self-signed development certificate.
- nlohmann/json: JSON parsing and serialization, used by the handler to
  encode and decode the messages exchanged between server and clients.
