#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <atomic>
#include <counter.hpp>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <sockpp/tcp_connector.h>

class network_client {
public:
  network_client(std::string id, const std::string& hostname, unsigned port);
  ~network_client();

  void start();
  void stop();
  void join();

  void increment(std::uint64_t amount = 1);
  void decrement(std::uint64_t amount = 1);
  std::int64_t value();
  const std::string& id() const { return client_id; }

private:
  void receive_loop();
  void send_state_locked();

  std::string client_id;
  counter local;
  std::mutex mu;
  sockpp::tcp_connector conn;
  std::thread worker;
  std::atomic<bool> running{false};
};

#endif
