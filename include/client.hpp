#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <thread>
#include <sockpp/tcp_connector.h>

class client
{
  public:
    client(const std::string& hostname, unsigned port);
    ~client();
    void start();
    void join();
    bool send(const std::string& data);
  private:
    sockpp::tcp_connector conn;
    std::thread worker;
};

#endif
