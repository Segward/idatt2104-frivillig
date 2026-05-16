#ifndef SERVER_HPP
#define SERVER_HPP

#include <atomic>
#include <counter.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_socket.h>

class network_server {
public:
  network_server(const std::string& hostname, unsigned port);
  ~network_server();
  void start();
  void stop();
  void join();

private:
  void accept_loop();
  void handle_connection(std::shared_ptr<sockpp::tcp_socket> sock);
  void broadcast_locked();

  sockpp::tcp_acceptor acc;
  counter master;
  std::mutex mu;
  std::vector<std::shared_ptr<sockpp::tcp_socket>> conns;
  std::thread acceptor_thread;
  std::vector<std::thread> client_threads;
  std::atomic<bool> running{false};
};

#endif
