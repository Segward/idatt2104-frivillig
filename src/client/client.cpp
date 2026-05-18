#include <client.hpp>
#include <handler.hpp>

Client::Client(const std::string& host, unsigned port) {
  sockpp::initialize();
  if (!_conn.connect(sockpp::inet_address(host, port))) {
    throw std::runtime_error("client connect failed: " + _conn.last_error_str());
  }
}

Client::~Client() {
  stop();
  join();
}

void Client::start() {
  if (_running.exchange(true)) return;
  _pkt.emplace(_conn);

  Packet::Type type;
  std::vector<std::uint8_t> payload;
  if (!_pkt->read(type, payload) || type != Packet::Type::Auth) {
    _running = false;
    _pkt.reset();
    throw std::runtime_error("auth handshake failed");
  }
  _client_id.assign(payload.begin(), payload.end());
  _local.emplace(_client_id);

  _worker = std::thread([this] { receive_loop(); });
}

void Client::stop() {
  if (!_running.exchange(false)) return;
  _conn.shutdown();
}

void Client::join() {
  if (_worker.joinable()) _worker.join();
}

void Client::increment(std::uint64_t amount) {
  std::lock_guard<std::mutex> lock(_mu);
  _local->increment(amount);
  send_state_locked();
}

void Client::decrement(std::uint64_t amount) {
  std::lock_guard<std::mutex> lock(_mu);
  _local->decrement(amount);
  send_state_locked();
}

std::int64_t Client::value() {
  std::lock_guard<std::mutex> lock(_mu);
  return _local->value();
}

void Client::send_state_locked() {
  Packet writer(_conn);
  if (!writer.write(Packet::Type::Counter, Handler::encode_counter(_local->state()))) {
    fprintf(stderr, "[%s] write failed: %s\n", _client_id.c_str(),
            _conn.last_error_str().c_str());
    _running = false;
    _conn.shutdown();
  }
}

void Client::receive_loop() {
  try {
    Handler handler(*_local);
    Packet::Type type;
    std::vector<std::uint8_t> payload;
    while (_pkt->read(type, payload)) {
      std::lock_guard<std::mutex> lock(_mu);
      if (!handler.apply(type, payload)) continue;
      printf("[%s] merged; value=%lld\n", _client_id.c_str(),
             static_cast<long long>(_local->value()));
    }
  } catch (const std::exception& e) {
    fprintf(stderr, "[%s] receive_loop terminated: %s\n", _client_id.c_str(), e.what());
  }
}
