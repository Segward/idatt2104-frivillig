#include <server.hpp>
#include <wire.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

int main(int argc, char** argv)
{
  std::string host = argc > 1 ? argv[1] : default_host;
  unsigned port = argc > 2 ? static_cast<unsigned>(std::atoi(argv[2])) : default_port;

  network_server srv(host, port);
  srv.start();
  printf("[server] listening on %s:%u (Ctrl-C to quit)\n", host.c_str(), port);
  srv.join();
  return 0;
}
