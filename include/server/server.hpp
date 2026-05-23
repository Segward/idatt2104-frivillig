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
    void close_listen_socket();

    // Called from the listen callback once uWS reports the bind result.
    void on_listen_result(us_listen_socket_t* listen_socket);

    // Three WebSocket handlers, declared as templates so server.hpp doesn't
    // need to pull in the uWS WebSocket type. Defined in server.cpp where the
    // uWS headers are visible; the only callers (lambdas in run_loop) live
    // there too, so the compiler can instantiate them in-place.
    template<typename Ws> void on_open(Ws* ws, std::set<Ws*>& connections);
    template<typename Ws> void on_message(Ws* ws, std::string_view payload);
    template<typename Ws> void on_close(Ws* ws, std::set<Ws*>& connections);

    // Binds the listen socket and publishes _listen_socket / _loop on success.
    template<typename App> void start_listening(App& app);
    std::string _host;
    unsigned _port;
    CounterPN _master_counter;
    ListRGA _master_list;
    TextRGA _master_text;
    std::thread _io_thread;
    std::atomic<bool> _running{false};

    // Flipped to true once the IO thread has either published _loop or
    // failed to bind. stop() waits on this before reading _loop so a fast
    // stop() after start() can never miss the publish and leave the loop
    // orphaned. atomic::wait/notify is idempotent — safe even if both
    // listen branches end up writing it.
    std::atomic<bool> _started{false};

    // Published from the IO thread once the loop starts, so stop() can defer
    // work back onto the loop from any thread.
    std::atomic<uWS::Loop*> _loop{nullptr};

    // Atomic so stop() can swap it out from outside the loop without racing
    // the close handler.
    std::atomic<us_listen_socket_t*> _listen_socket{nullptr};
};

#endif
