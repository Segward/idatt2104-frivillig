#ifndef SERVER_HPP
#define SERVER_HPP

#include <crdt/counter_pn.hpp>
#include <crdt/list_rga.hpp>
#include <crdt/text_rga.hpp>

struct us_listen_socket_t;
namespace uWS { class Loop; }

// Owns the WebSocket endpoint and the authoritative master CRDTs. Each
// connecting browser gets an assigned replica ID and a snapshot of the
// current state; incoming peer state is merged back into the masters and
// echoed to the sender. The IO loop runs on its own thread so the main
// thread can block on join().
class Server {
  public:
    Server(const std::string& host, unsigned port);
    ~Server();

    // Owns OS resources (listen socket, thread) that cannot safely be
    // aliased or relocated.
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    void start();

    // Safe to call from any thread; trampolines onto the IO loop.
    void stop();

    void join();

  private:
    void run_loop();

    std::string _host;
    unsigned _port;

    CounterPN _master_counter;
    ListRGA _master_list;
    TextRGA _master_text;

    std::thread _io_thread;
    std::atomic<bool> _running{false};

    // Published from the IO thread once the loop starts, so stop() can defer
    // work back onto the loop from any thread.
    std::atomic<uWS::Loop*> _loop{nullptr};

    // Atomic so stop() can swap it out from outside the loop without racing
    // the close handler.
    std::atomic<us_listen_socket_t*> _listen_socket{nullptr};
};

#endif
