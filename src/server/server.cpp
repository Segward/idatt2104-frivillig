#include <server.hpp>
#include <handler.hpp>
#include <packet.hpp>

Server::Server(const std::string& host, unsigned port)
  : _master("server") {
  sockpp::initialize();
  if (!_acc.open(sockpp::inet_address(host, port))) {
    throw std::runtime_error("server bind failed: " + _acc.last_error_str());
  }
}

Server::~Server() {
  stop();
  join();
}

void Server::start() {
  if (_running.exchange(true)) return;
  _acceptor_thread = std::thread([this] { accept_loop(); });
}

void Server::stop() {
  if (!_running.exchange(false)) return;
  _acc.close();
  std::lock_guard<std::mutex> lock(_mu);
  for (auto& c : _conns) c->sock->shutdown();
}

void Server::join() {
  if (_acceptor_thread.joinable()) _acceptor_thread.join();
  for (auto& t : _client_threads) if (t.joinable()) t.join();
}

void Server::accept_loop() {
  try {
    while (_running) {
      auto sock = _acc.accept();
      if (!sock || !_running) break;
      auto shared = std::make_shared<sockpp::tcp_socket>(std::move(sock));

      std::string id;
      {
        std::lock_guard<std::mutex> lock(_mu);
        id = "client-" + std::to_string(_next_id++);
      }

      // Send the handshake before publishing the connection: nobody else holds
      // this socket yet, so the write cannot race with a broadcast.
      Packet writer(*shared);
      std::vector<std::uint8_t> id_bytes(id.begin(), id.end());
      if (!writer.write(Packet::Type::Auth, id_bytes)) continue;
      if (!writer.write(Packet::Type::Counter, Handler::encode_counter(_master.state()))) continue;

      auto conn = std::make_shared<Connection>(Connection{shared, id});
      {
        std::lock_guard<std::mutex> lock(_mu);
        _conns.push_back(conn);
        _client_threads.emplace_back([this, conn] { handle_connection(conn); });
        printf("[server] %s connected (total=%zu)\n", id.c_str(), _conns.size());
      }
    }
  } catch (const std::exception& e) {
    fprintf(stderr, "[server] accept_loop terminated: %s\n", e.what());
  }
}

void Server::handle_connection(std::shared_ptr<Connection> conn) {
  try {
    Packet pkt(*conn->sock);
    Handler handler(_master, conn->id);
    Packet::Type type;
    std::vector<std::uint8_t> payload;
    while (pkt.read(type, payload)) {
      std::lock_guard<std::mutex> lock(_mu);
      if (!handler.apply(type, payload)) continue;
      printf("[server] merged update from %s; value=%lld\n", conn->id.c_str(),
             static_cast<long long>(_master.value()));
      broadcast_locked();
    }
  } catch (const std::exception& e) {
    fprintf(stderr, "[server] handler for %s terminated: %s\n", conn->id.c_str(), e.what());
  }
  std::lock_guard<std::mutex> lock(_mu);
  _conns.erase(std::remove(_conns.begin(), _conns.end(), conn), _conns.end());
  printf("[server] %s disconnected (total=%zu)\n", conn->id.c_str(), _conns.size());
}

void Server::broadcast_locked() {
  // _mu is held: writes to each socket are serialized across broadcasters,
  // which prevents interleaved frames at the cost of head-of-line blocking
  // on slow clients. Acceptable for this project's scale.
  auto payload = Handler::encode_counter(_master.state());
  for (auto& c : _conns) {
    Packet writer(*c->sock);
    if (!writer.write(Packet::Type::Counter, payload)) {
      fprintf(stderr, "[server] broadcast to %s failed\n", c->id.c_str());
    }
  }
}
