#ifndef SERVER_HPP
#define SERVER_HPP

#include <counter.hpp>
#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_socket.h>

class Server {
  public:
    Server(const std::string& host, unsigned port);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    void start();
    void stop();
    void join();

  private:
    struct Connection {
      std::shared_ptr<sockpp::tcp_socket> sock;
      std::string id;
    };

    void accept_loop();
    void handle_connection(std::shared_ptr<Connection> conn);
    void broadcast_locked();

    sockpp::tcp_acceptor _acc;
    Counter _master;
    std::mutex _mu;
    std::vector<std::shared_ptr<Connection>> _conns;
    std::thread _acceptor_thread;
    std::vector<std::thread> _client_threads;
    std::atomic<bool> _running{false};
    std::uint64_t _next_id{1};
};

#endif
