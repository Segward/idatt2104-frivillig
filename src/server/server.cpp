#include <server.hpp>
#include <wire.hpp>

#include <algorithm>
#include <cstdio>
#include <utility>

network_server::network_server(const std::string& hostname, unsigned port)
  : master("server")
{
  sockpp::initialize();
  this->acc.open(sockpp::inet_address(hostname, port));
}

network_server::~network_server()
{
  this->stop();
  this->join();
}

void network_server::start()
{
  this->running = true;
  this->acceptor_thread = std::thread([this] { this->accept_loop(); });
}

void network_server::stop()
{
  if (!this->running.exchange(false)) return;
  this->acc.close();
  std::lock_guard<std::mutex> lock(this->mu);
  for (auto& c : this->conns) c->shutdown();
}

void network_server::join()
{
  if (this->acceptor_thread.joinable()) this->acceptor_thread.join();
  for (auto& t : this->client_threads) if (t.joinable()) t.join();
}

void network_server::accept_loop()
{
  while (this->running) {
    auto sock = this->acc.accept();
    if (!sock || !this->running) break;
    auto shared = std::make_shared<sockpp::tcp_socket>(std::move(sock));
    {
      std::lock_guard<std::mutex> lock(this->mu);
      this->conns.push_back(shared);
      std::string frame = serialize_state(this->master.get_state());
      shared->write(frame);
      this->client_threads.emplace_back([this, shared] { this->handle_connection(shared); });
      printf("[server] client connected (total=%zu)\n", this->conns.size());
    }
  }
}

void network_server::handle_connection(std::shared_ptr<sockpp::tcp_socket> sock)
{
  line_reader<sockpp::tcp_socket> reader(*sock);
  std::string line;
  while (reader.read_line(line)) {
    counter_state incoming = parse_state(line);
    std::lock_guard<std::mutex> lock(this->mu);
    this->master.merge(incoming);
    printf("[server] merged update; value=%lld\n", static_cast<long long>(this->master.value()));
    this->broadcast_locked();
  }
  std::lock_guard<std::mutex> lock(this->mu);
  this->conns.erase(std::remove(this->conns.begin(), this->conns.end(), sock), this->conns.end());
  printf("[server] client disconnected (total=%zu)\n", this->conns.size());
}

void network_server::broadcast_locked()
{
  std::string frame = serialize_state(this->master.get_state());
  for (auto& c : this->conns) {
    c->write(frame);
  }
}
