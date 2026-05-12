#include <client.hpp>
#include <server.hpp>
#include <chrono>
#include <cstdio>
#include <thread>
#include <sockpp/version.h>

int main()
{
  printf("sockpp %s\n", sockpp::SOCKPP_VERSION.c_str());
  sockpp::initialize();

  const unsigned port = 12345;
  server srv("127.0.0.1", port);
  client user("127.0.0.1", port);

  srv.start();
  user.start();

  user.send("hello from client");
  srv.send("hello from server");

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  return 0;
}
