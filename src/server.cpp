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
  };

  // WEBSITE_INDEX is baked in by CMake and points to the copy of website/
  // installed next to the server binary at build time.
  std::string load_index_html() {
    std::ifstream f(WEBSITE_INDEX);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
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
  // Track live sockets so stop() can drop them; only touched on the loop thread.
  using WS = uWS::WebSocket<false, true, PerSocketData>;
  std::set<WS*> conns;
  std::uint64_t next_id = _next_id;

  const std::string index_html = load_index_html();

  auto app = uWS::App();

  app.get("/", [&](auto* res, auto* /*req*/) {
    if (index_html.empty()) {
      res->writeStatus("500 Internal Server Error")
         ->writeHeader("Content-Type", "text/plain")
         ->end("website/index.html missing from build output.");
      return;
    }
    res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(index_html);
  });

  app.ws<PerSocketData>("/*", {
    .compression = uWS::DISABLED,
    .open = [&](auto* ws) {
      auto* data = ws->getUserData();
      data->id = "client-" + std::to_string(next_id++);
      conns.insert(ws);
      ws->subscribe(kBroadcastTopic);
      ws->send(Handler::encode_auth(data->id), uWS::OpCode::TEXT);
      ws->send(Handler::encode_counter(_master_counter.state()), uWS::OpCode::TEXT);
      ws->send(Handler::encode_list_init(_list_ops), uWS::OpCode::TEXT);
      ws->send(Handler::encode_text_init(_text_ops), uWS::OpCode::TEXT);
      printf("[server] %s connected (total=%zu)\n", data->id.c_str(), conns.size());
    },
    .message = [&](auto* ws, std::string_view msg, uWS::OpCode /*op*/) {
      auto* data = ws->getUserData();
      const std::string payload(msg);

      if (auto counter_in = Handler::decode_counter(payload)) {
        Handler handler(_master_counter, data->id);
        handler.apply(payload);
        const auto state_msg = Handler::encode_counter(_master_counter.state());
        ws->publish(kBroadcastTopic, state_msg, uWS::OpCode::TEXT);
        ws->send(state_msg, uWS::OpCode::TEXT);
        printf("[server] counter merged from %s; value=%lld\n",
               data->id.c_str(), static_cast<long long>(_master_counter.value()));
        return;
      }

      if (auto op = Handler::decode_list_op(payload)) {
        if (_master_list.has_applied(op->operation_id)) return;
        try {
          _master_list.apply(*op);
        } catch (const std::exception& e) {
          fprintf(stderr, "[server] list op from %s rejected: %s\n",
                  data->id.c_str(), e.what());
          return;
        }
        _list_ops.push_back(*op);
        const auto op_msg = Handler::encode_list_op(*op);
        ws->publish(kBroadcastTopic, op_msg, uWS::OpCode::TEXT);
        printf("[server] list op from %s; size=%zu\n",
               data->id.c_str(), _master_list.value().size());
        return;
      }

      if (auto op = Handler::decode_text_op(payload)) {
        if (_master_text.has_applied(op->operation_id)) return;
        try {
          _master_text.apply(*op);
        } catch (const std::exception& e) {
          fprintf(stderr, "[server] text op from %s rejected: %s\n",
                  data->id.c_str(), e.what());
          return;
        }
        _text_ops.push_back(*op);
        const auto op_msg = Handler::encode_text_op(*op);
        ws->publish(kBroadcastTopic, op_msg, uWS::OpCode::TEXT);
        printf("[server] text op from %s; len=%zu\n",
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
    printf("[server] listening on %s:%u\n", _host.c_str(), _port);
  });

  app.run();
  _next_id = next_id;
  _running = false;
}
