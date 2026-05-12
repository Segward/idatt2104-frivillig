#include <server.hpp>
#include <cstdio>
#include <sockpp/version.h>

server::server(const std::string& hostname, unsigned port)
{
  sockpp::initialize();
  this->acc.open(sockpp::inet_address(hostname, port));
}

server::~server()
{
  this->acc.close();
  this->conn.shutdown();
  this->join();
}

void server::wait_for_accept()
{
  std::unique_lock<std::mutex> lock(this->m);
  this->cv.wait(lock, [this] { return this->accepted; });
}

bool server::send(const std::string& data)
{
  this->wait_for_accept();
  ssize_t n = this->conn.write(data);
  if (n != static_cast<ssize_t>(data.size())) {
    fprintf(stderr, "write failed: %s\n", this->conn.last_error_str().c_str());
    return false;
  }
  return true;
}

void server::start()
{
  this->worker = std::thread([this]() {
    auto c = this->acc.accept();
    {
      std::lock_guard<std::mutex> lock(this->m);
      this->conn = std::move(c);
      this->accepted = true;
    }
    this->cv.notify_all();

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

void server::join()
{
  if (this->worker.joinable()) this->worker.join();
}
