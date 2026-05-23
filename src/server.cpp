// Server implementation: WebSocket endpoint, static asset serving, and CRDT
// merge fan-out. See include/server.hpp for the public API.
//
// Notes:
//  - All uWS state (loop, routes, connection set, master CRDTs) lives on a
//    single IO thread; no mutexes needed inside run_loop's lambdas.
//  - stop() may be called from any thread (notably ~Server on main). It
//    trampolines the listen-socket close back onto the IO thread via
//    loop->defer(), because uWS sockets aren't safe to close from outside
//    their event loop.
//  - Messages dispatch try-each-envelope: feed the payload to each
//    Handler::decode_* until one accepts (the others return nullopt).
//  - Connection IDs are 128 random bits, not a counter, so a recycled
//    "client-N" can't OR a stale tombstone over a fresh node after restart.

#include <server.hpp>
#include <handler.hpp>

#include <uwebsockets/App.h>

namespace {
  struct PerSocketData {
    std::string id;
  };

  // WEBSITE_INDEX is baked in by CMake and points to the copy of website/
  // installed next to the server binary at build time. Sibling assets live
  // in the same directory.
  std::string load_text_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
  }

  std::string website_sibling(const std::string& filename) {
    std::string index = WEBSITE_INDEX;
    auto slash = index.find_last_of('/');
    if (slash == std::string::npos) return filename;
    return index.substr(0, slash + 1) + filename;
  }

  // 128-bit random hex id. Collision-resistant across server restarts so a
  // recycled "client-N" cannot OR a stale tombstone over a fresh local node.
  std::string make_client_id() {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<std::uint64_t> dist;
    std::uint64_t hi = dist(rng);
    std::uint64_t lo = dist(rng);
    std::ostringstream stream;
    stream << "client-" << std::hex << std::setfill('0')
           << std::setw(16) << hi
           << std::setw(16) << lo;
    return stream.str();
  }

  struct Assets {
    std::string index_html;
    std::string style_css;
    std::string app_js;
  };

  Assets load_assets() {
    return {
      load_text_file(WEBSITE_INDEX),
      load_text_file(website_sibling("style.css")),
      load_text_file(website_sibling("app.js")),
    };
  }

  // Wires the three static-asset routes. `assets` is captured by reference; it
  // must outlive `app`. run_loop owns both on the IO thread's stack.
  template<typename App>
  void wire_http_routes(App& app, const Assets& assets) {
    app.get("/", [&assets](auto* res, auto* /*req*/) {
      if (assets.index_html.empty()) {
        res->writeStatus("500 Internal Server Error")
           ->writeHeader("Content-Type", "text/plain")
           ->end("website/index.html missing from build output.");
        return;
      }
      res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(assets.index_html);
    });

    app.get("/style.css", [&assets](auto* res, auto* /*req*/) {
      res->writeHeader("Content-Type", "text/css; charset=utf-8")->end(assets.style_css);
    });

    app.get("/app.js", [&assets](auto* res, auto* /*req*/) {
      res->writeHeader("Content-Type", "application/javascript; charset=utf-8")->end(assets.app_js);
    });
  }
}

Server::Server(const std::string& host, unsigned port)
  : _host(host), _port(port),
    // The master CRDTs share a fixed replica id "server" — there's only ever
    // one of them per process, so collision with a real client_id is avoided
    // by the make_client_id() prefix scheme.
    _master_counter("server"),
    _master_list("server"),
    _master_text("server") {}

Server::~Server() {
  stop();
  join();
}

void Server::start() {
  if (_running.exchange(true)) return;
  _io_thread = std::thread([this] { run_loop(); });
}

void Server::stop() {
  if (!_running.exchange(false)) return;
  uWS::Loop* loop = _loop.load();
  if (!loop) return;
  // Defer the close onto the IO thread: uWS internals are not safe to touch
  // from outside the loop. Also swap the listen socket out atomically so a
  // racing stop() never closes it twice.
  loop->defer([this] {
    us_listen_socket_t* sock = _listen_socket.exchange(nullptr);
    if (sock) us_listen_socket_close(0, sock);
  });
}

void Server::join() {
  if (_io_thread.joinable()) _io_thread.join();
}

std::optional<std::string> Server::process_message(const std::string& client_id,
                                                   std::string_view payload) {
  // Try each envelope in turn. Whichever decoder accepts the payload owns
  // the dispatch; the others return nullopt and we fall through. The whole
  // block is try/catch'd because any unexpected exception from the CRDT
  // merges should kill the message, not the server.
  const std::string str(payload);
  try {
    if (auto incoming = Handler::decode_counter(str)) {
      _master_counter.merge(*incoming);
      printf("[server] counter merged from %s; value=%lld\n",
             client_id.c_str(), static_cast<long long>(_master_counter.value()));
      return Handler::encode_counter(_master_counter.state());
    }
    if (auto incoming = Handler::decode_list_state(str)) {
      _master_list.merge(*incoming);
      printf("[server] list merged from %s; size=%zu\n",
             client_id.c_str(), _master_list.value().size());
      return Handler::encode_list_state(_master_list.state());
    }
    if (auto incoming = Handler::decode_text_state(str)) {
      _master_text.merge(*incoming);
      printf("[server] text merged from %s; len=%zu\n",
             client_id.c_str(), _master_text.value().size());
      return Handler::encode_text_state(_master_text.state());
    }
  } catch (const std::exception& e) {
    fprintf(stderr, "[server] message handler error from %s: %s\n",
            client_id.c_str(), e.what());
  }
  return std::nullopt;
}

template<typename App>
void Server::start_listening(App& app) {
  app.listen(_host, static_cast<int>(_port), [this](auto* listen_socket) {
    if (!listen_socket) {
      fprintf(stderr, "[server] listen on %s:%u failed\n", _host.c_str(), _port);
      _running = false;
      return;
    }
    _listen_socket.store(listen_socket);
    _loop.store(uWS::Loop::get());
    printf("[server] open https://localhost:%u (Ctrl-C to quit)\n", _port);
  });
}

void Server::run_loop() {
  // All loop-thread-only state lives here. Captured by [&] in the lambdas
  // below, which is safe because every lambda runs on this same thread.
  using WS = uWS::WebSocket<true, true, PerSocketData>;
  std::set<WS*> conns;

  const Assets assets = load_assets();

  auto app = uWS::SSLApp({
    .key_file_name = KEY_PATH,
    .cert_file_name = CERT_PATH,
  });

  wire_http_routes(app, assets);

  app.ws<PerSocketData>("/*", {
    .compression = uWS::DISABLED,
    // Default uWS limits (16 KB inbound, 64 KB outbound backpressure) trip a
    // 1009 close as soon as a full text_state crosses the wire. Raise both.
    .maxPayloadLength = 16 * 1024 * 1024,
    .maxBackpressure = 16 * 1024 * 1024,
    .open = [&](auto* ws) {
      // On connect: mint an id, register the connection, then push the
      // assigned id + a full snapshot of all three masters so the client can
      // bootstrap without waiting for a peer change.
      auto* data = ws->getUserData();
      data->id = make_client_id();
      conns.insert(ws);
      ws->send(Handler::encode_auth(data->id), uWS::OpCode::TEXT);
      ws->send(Handler::encode_counter(_master_counter.state()), uWS::OpCode::TEXT);
      ws->send(Handler::encode_list_state(_master_list.state()), uWS::OpCode::TEXT);
      ws->send(Handler::encode_text_state(_master_text.state()), uWS::OpCode::TEXT);
      printf("[server] %s connected (total=%zu)\n", data->id.c_str(), conns.size());
    },
    .message = [this](auto* ws, std::string_view msg, uWS::OpCode /*op*/) {
      auto* data = ws->getUserData();
      if (auto reply = process_message(data->id, msg)) {
        ws->send(*reply, uWS::OpCode::TEXT);
      }
    },
    .close = [&](auto* ws, int /*code*/, std::string_view /*msg*/) {
      auto* data = ws->getUserData();
      conns.erase(ws);
      printf("[server] %s disconnected (total=%zu)\n", data->id.c_str(), conns.size());
    },
  });

  start_listening(app);
  app.run();
  _running = false;
}
