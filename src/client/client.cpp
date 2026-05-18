#include <client.hpp>
#include <packet.hpp>

network_client::network_client(std::string id, const std::string& hostname, unsigned port)
  : client_id(std::move(id)), local(client_id)
{
  sockpp::initialize();
  this->conn.connect(sockpp::inet_address(hostname, port));
}

network_client::~network_client()
{
  this->stop();
  this->join();
}

void network_client::start()
{
  this->running = true;
  this->worker = std::thread([this] { this->receive_loop(); });
}

void network_client::stop()
{
  if (!this->running.exchange(false)) return;
  this->conn.shutdown();
}

void network_client::join()
{
  if (this->worker.joinable()) this->worker.join();
}

void network_client::increment(std::uint64_t amount)
{
  std::lock_guard<std::mutex> lock(this->mu);
  this->local.increment(amount);
  this->send_state_locked();
}

void network_client::decrement(std::uint64_t amount)
{
  std::lock_guard<std::mutex> lock(this->mu);
  this->local.decrement(amount);
  this->send_state_locked();
}

std::int64_t network_client::value()
{
  std::lock_guard<std::mutex> lock(this->mu);
  return this->local.value();
}

void network_client::send_state_locked()
{
  auto frame = serialize_counter(this->local.get_state());
  ssize_t n = this->conn.write(frame.data(), frame.size());
  if (n != static_cast<ssize_t>(frame.size())) {
    fprintf(stderr, "[%s] write failed: %s\n", this->client_id.c_str(),
            this->conn.last_error_str().c_str());
  }
}

void network_client::receive_loop()
{
  packet_reader<sockpp::tcp_connector> reader(this->conn);
  std::vector<std::uint8_t> pkt;
  while (reader.read_packet(pkt)) {
    switch (static_cast<packet_type>(pkt[0])) {
      case packet_type::COUNTER: {
        counter_state incoming = parse_counter(pkt.data(), pkt.size());
        std::lock_guard<std::mutex> lock(this->mu);
        this->local.merge(incoming);
        printf("[%s] merged; value=%lld\n", this->client_id.c_str(),
               static_cast<long long>(this->local.value()));
        break;
      }
    }
  }
}
