#ifndef SERVER_HPP
#define SERVER_HPP

#include <counter_pn.hpp>
#include <list_rga.hpp>
#include <text_rga.hpp>

struct us_listen_socket_t;
namespace uWS { class Loop; }

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
    void run_loop();

    std::string _host;
    unsigned _port;
    counter_pn _master_counter;
    list_RGA _master_list;
    std::vector<list_change> _list_ops;
    text_RGA _master_text;
    std::vector<text_change> _text_ops;
    std::thread _io_thread;
    std::atomic<bool> _running{false};
    std::atomic<uWS::Loop*> _loop{nullptr};
    std::atomic<us_listen_socket_t*> _listen_socket{nullptr};
    std::uint64_t _next_id{1};
};

#endif
