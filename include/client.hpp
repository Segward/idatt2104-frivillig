#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <counter.hpp>
#include <packet.hpp>
#include <sockpp/tcp_connector.h>

class Client {
  public:
    Client(const std::string& host, unsigned port);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;

    void start();
    void stop();
    void join();

    void increment(std::uint64_t amount = 1);
    void decrement(std::uint64_t amount = 1);
    std::int64_t value();

    const std::string& id() const { return _client_id; }

  private:
    void receive_loop();
    void send_state_locked();

    std::string _client_id;
    std::optional<Counter> _local;
    std::optional<Packet> _pkt;
    std::mutex _mu;
    sockpp::tcp_connector _conn;
    std::thread _worker;
    std::atomic<bool> _running{false};
};

#endif
