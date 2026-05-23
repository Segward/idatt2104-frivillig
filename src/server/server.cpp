// Server implementation. See include/server/server.hpp for the public API.

#include <server/server.hpp>
#include <server/assets.hpp>
#include <server/handler.hpp>
#include <server/messages.hpp>

#include <uwebsockets/App.h>

namespace {
    struct PerSocketData {
        std::string id;
    };
    using ServerWs = uWS::WebSocket<true, true, PerSocketData>;
    using HttpResponse = uWS::HttpResponse<true>;

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

    void serve_index(HttpResponse* res, const Assets& assets) {
        res->writeHeader("Content-Type", "text/html; charset=utf-8")->end(assets.index_html);
    }

    void serve_css(HttpResponse* res, const Assets& assets) {
        res->writeHeader("Content-Type", "text/css; charset=utf-8")->end(assets.style_css);
    }

    void serve_js(HttpResponse* res, const Assets& assets) {
        res->writeHeader("Content-Type", "application/javascript; charset=utf-8")->end(assets.app_js);
    }

    // `assets` is captured by reference and must outlive `app`. run_loop owns
    // both on the IO thread's stack.
    template<typename App>
    void wire_http_routes(App& app, const Assets& assets) {
        app.get("/", [&assets](auto* res, auto*) { serve_index(res, assets); });
        app.get("/style.css", [&assets](auto* res, auto*) { serve_css(res, assets); });
        app.get("/app.js", [&assets](auto* res, auto*) { serve_js(res, assets); });
    }
}

// The master CRDTs share a fixed replica id "server"; collisions with real
// client ids are avoided by make_client_id()'s "client-" prefix.
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

// Safe to call from any thread. _started.wait blocks until the IO thread has
// either published _loop or reported a listen failure, so a stop() racing
// the listen callback can never see _loop == nullptr and skip the deferred
// close. The close itself is deferred onto the IO thread because uWS
// internals are not safe to touch from outside their loop.
void Server::stop() {
    if (!_running.exchange(false)) return;
    _started.wait(false);
    uWS::Loop* loop = _loop.load();
    if (!loop) return;
    loop->defer([this] { close_listen_socket(); });
}

void Server::join() {
    if (_io_thread.joinable()) _io_thread.join();
}

// Atomic swap so a racing stop() can never close the socket twice.
void Server::close_listen_socket() {
    us_listen_socket_t* sock = _listen_socket.exchange(nullptr);
    if (sock) us_listen_socket_close(0, sock);
}

// Whichever branch runs, count the _started latch down before returning so a
// concurrent stop() can unblock.
void Server::on_listen_result(us_listen_socket_t* listen_socket) {
    if (!listen_socket) {
        fprintf(stderr, "[server] listen on %s:%u failed\n", _host.c_str(), _port);
        _running = false;
        _started.store(true);
        _started.notify_all();
        return;
    }
    _listen_socket.store(listen_socket);
    _loop.store(uWS::Loop::get());
    _started.store(true);
    _started.notify_all();
    printf("[server] open https://localhost:%u (Ctrl-C to quit)\n", _port);
}

// On connect: mint an id, register the connection, then push the assigned id
// + a full snapshot of all three masters so the client can bootstrap without
// waiting for a peer change.
template<typename Ws>
void Server::on_open(Ws* ws, std::set<Ws*>& connections) {
    auto* data = ws->getUserData();
    data->id = make_client_id();
    connections.insert(ws);
    ws->send(Handler::encode_auth(data->id), uWS::OpCode::TEXT);
    ws->send(Handler::encode_counter(_master_counter.state()), uWS::OpCode::TEXT);
    ws->send(Handler::encode_list_state(_master_list.state()), uWS::OpCode::TEXT);
    ws->send(Handler::encode_text_state(_master_text.state()), uWS::OpCode::TEXT);
    printf("[server] %s connected (total=%zu)\n", data->id.c_str(), connections.size());
}

template<typename Ws>
void Server::on_message(Ws* ws, std::string_view payload) {
    auto* data = ws->getUserData();
    if (auto reply = process_message(_master_counter, _master_list, _master_text, data->id, payload)) {
        ws->send(*reply, uWS::OpCode::TEXT);
    }
}

template<typename Ws>
void Server::on_close(Ws* ws, std::set<Ws*>& connections) {
    auto* data = ws->getUserData();
    connections.erase(ws);
    printf("[server] %s disconnected (total=%zu)\n", data->id.c_str(), connections.size());
}

template<typename App>
void Server::start_listening(App& app) {
    app.listen(_host, static_cast<int>(_port), [this](auto* sock) { on_listen_result(sock); });
}

// All loop-thread-only state (connections, app, assets) lives here on the IO
// thread's stack. The default uWS limits (16 KB inbound, 64 KB outbound
// backpressure) trip a 1009 close as soon as a full text_state crosses the
// wire, so both are raised to 16 MiB.
void Server::run_loop() {
    std::set<ServerWs*> connections;
    const Assets assets = Assets::load();
    auto app = uWS::SSLApp({
        .key_file_name = KEY_PATH,
        .cert_file_name = CERT_PATH,
    });
    wire_http_routes(app, assets);
    app.ws<PerSocketData>("/*", {
        .compression = uWS::DISABLED,
        .maxPayloadLength = 16 * 1024 * 1024,
        .maxBackpressure = 16 * 1024 * 1024,
        .open = [this, &connections](auto* ws) { on_open(ws, connections); },
        .message = [this](auto* ws, std::string_view msg, auto) { on_message(ws, msg); },
        .close = [this, &connections](auto* ws, int, std::string_view) { on_close(ws, connections); },
    });
    start_listening(app);
    app.run();
    _running = false;
}
