#ifndef SERVER_HPP
#define SERVER_HPP

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_socket.h>

class server
{
  public:
    server(const std::string& hostname, unsigned port);
    ~server();
    void start();
    void join();
    bool send(const std::string& data);
  private:
    void wait_for_accept();
    sockpp::tcp_acceptor acc;
    sockpp::tcp_socket conn;
    std::mutex m;
    std::condition_variable cv;
    bool accepted = false;
    std::thread worker;
};

#endif
