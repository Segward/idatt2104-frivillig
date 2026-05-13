#include <client.hpp>
#include <cstdio>
#include <sockpp/version.h>

client::client(const std::string& hostname, unsigned port)
{
  sockpp::initialize();
  this->conn.connect(sockpp::inet_address(hostname, port));
}

client::~client()
{
  this->conn.shutdown();
  this->join();
}

bool client::send(const std::string& data)
{
  ssize_t n = this->conn.write(data);
  if (n != static_cast<ssize_t>(data.size())) {
    fprintf(stderr, "write failed: %s\n", this->conn.last_error_str().c_str());
    return false;
  }
  return true;
}

void client::start()
{
  this->worker = std::thread([this]() {
    char buf[512];
    while (true) {
      ssize_t n = this->conn.read(buf, sizeof(buf) - 1);
      if (n > 0) {
        buf[n] = '\0';
        printf("received %zd bytes:\n%s\n", n, buf);
      } else if (n == 0) {
        break;
      } else {
        fprintf(stderr, "read failed: %s\n", this->conn.last_error_str().c_str());
        break;
      }
    }
  });
}

void client::join()
{
  if (this->worker.joinable()) this->worker.join();
}
