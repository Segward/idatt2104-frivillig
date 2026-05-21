#include <server.hpp>
#include <handler.hpp>

#include <fstream>
#include <set>
#include <sstream>
#include <uwebsockets/App.h>

namespace {
  constexpr const char* kBroadcastTopic = "broadcast";

  struct PerSocketData {
    std::string id;
    std::uint64_t id_number{0};
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
}

Server::Server(const std::string& host, unsigned port)
  : _host(host), _port(port),
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
  loop->defer([this] {
    us_listen_socket_t* sock = _listen_socket.exchange(nullptr);
    if (sock) us_listen_socket_close(0, sock);
  });
}

void Server::join() {
  if (_io_thread.joinable()) _io_thread.join();
}

void Server::run_loop() {
  // All loop-thread-only state lives here.
  using WS = uWS::WebSocket<true, true, PerSocketData>;
  std::set<WS*> conns;
  std::uint64_t next_id_number = 0;

  const std::string index_html = load_text_file(WEBSITE_INDEX);
  const std::string style_css = load_text_file(website_sibling("style.css"));
  const std::string app_js = load_text_file(website_sibling("app.js"));

  auto app = uWS::SSLApp({
    .key_file_name = KEY_PATH,
    .cert_file_name = CERT_PATH,
  });

  app.get("/", [&](auto* res, auto* /*req*/) {
    if (index_html.empty()) {
      res->writeStatus("500 Internal Server Error")
         ->writeHeader("Content-Type", "text/plain")
         ->end("website/index.html missing from build output.");
      return;
    }
    res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(index_html);
  });

  app.get("/style.css", [&](auto* res, auto* /*req*/) {
    res->writeHeader("Content-Type", "text/css; charset=utf-8")->end(style_css);
  });

  app.get("/app.js", [&](auto* res, auto* /*req*/) {
    res->writeHeader("Content-Type", "application/javascript; charset=utf-8")->end(app_js);
  });

  app.ws<PerSocketData>("/*", {
    .compression = uWS::DISABLED,
    .open = [&](auto* ws) {
      auto* data = ws->getUserData();
      data->id_number = ++next_id_number;
      data->id = "client-" + std::to_string(data->id_number);
      conns.insert(ws);
      ws->subscribe(kBroadcastTopic);
      ws->send(Handler::encode_auth(data->id), uWS::OpCode::TEXT);
      ws->send(Handler::encode_counter(_master_counter.state()), uWS::OpCode::TEXT);
      ws->send(Handler::encode_list_state(_master_list.state()), uWS::OpCode::TEXT);
      ws->send(Handler::encode_text_state(_master_text.state()), uWS::OpCode::TEXT);
      printf("[server] %s connected (total=%zu)\n", data->id.c_str(), conns.size());
    },
    .message = [&](auto* ws, std::string_view msg, uWS::OpCode /*op*/) {
      auto* data = ws->getUserData();
      const std::string payload(msg);

      if (auto incoming = Handler::decode_counter(payload)) {
        _master_counter.merge(*incoming);
        const auto state_msg = Handler::encode_counter(_master_counter.state());
        ws->publish(kBroadcastTopic, state_msg, uWS::OpCode::TEXT);
        ws->send(state_msg, uWS::OpCode::TEXT);
        printf("[server] counter merged from %s; value=%lld\n",
               data->id.c_str(), static_cast<long long>(_master_counter.value()));
        return;
      }

      if (auto incoming = Handler::decode_list_state(payload)) {
        _master_list.merge(*incoming);
        const auto state_msg = Handler::encode_list_state(_master_list.state());
        ws->publish(kBroadcastTopic, state_msg, uWS::OpCode::TEXT);
        ws->send(state_msg, uWS::OpCode::TEXT);
        printf("[server] list merged from %s; size=%zu\n",
               data->id.c_str(), _master_list.value().size());
        return;
      }

      if (auto incoming = Handler::decode_text_state(payload)) {
        _master_text.merge(*incoming);
        const auto state_msg = Handler::encode_text_state(_master_text.state());
        ws->publish(kBroadcastTopic, state_msg, uWS::OpCode::TEXT);
        ws->send(state_msg, uWS::OpCode::TEXT);
        printf("[server] text merged from %s; len=%zu\n",
               data->id.c_str(), _master_text.value().size());
        return;
      }
    },
    .close = [&](auto* ws, int /*code*/, std::string_view /*msg*/) {
      auto* data = ws->getUserData();
      conns.erase(ws);
      printf("[server] %s disconnected (total=%zu)\n", data->id.c_str(), conns.size());
    },
  });

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

  app.run();
  _running = false;
}
