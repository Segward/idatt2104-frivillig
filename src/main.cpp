#include <cstdio>
#include <string>
#include <sockpp/tcp_connector.h>
#include <sockpp/version.h>

int main() {
  printf("sockpp %s\n", sockpp::SOCKPP_VERSION.c_str());

  sockpp::initialize();

  sockpp::tcp_connector conn;
  if (!conn.connect(sockpp::inet_address("example.com", 80))) {
    fprintf(stderr, "connect failed: %s\n", conn.last_error_str().c_str());
    return 1;
  }

  printf("connected to %s\n", conn.peer_address().to_string().c_str());

  const std::string req = "GET / HTTP/1.0\r\nHost: example.com\r\nConnection: close\r\n\r\n";
  if (conn.write(req) != static_cast<ssize_t>(req.size())) {
    fprintf(stderr, "write failed: %s\n", conn.last_error_str().c_str());
    return 1;
  }

  char buf[512];
  ssize_t n = conn.read(buf, sizeof(buf) - 1);
  if (n < 0) {
    fprintf(stderr, "read failed: %s\n", conn.last_error_str().c_str());
    return 1;
  }

  buf[n] = '\0';
  printf("received %zd bytes:\n%s\n", n, buf);

  return 0;
}
