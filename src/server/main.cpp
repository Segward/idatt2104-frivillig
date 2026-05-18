#include <server.hpp>
#include <packet.hpp>

int main(int argc, char** argv) {
  try {
    std::string host = argc > 1 ? argv[1] : default_host;
    unsigned port = argc > 2 ? static_cast<unsigned>(std::atoi(argv[2])) : default_port;

    Server server(host, port);
    server.start();
    printf("[server] listening on %s:%u (Ctrl-C to quit)\n", host.c_str(), port);
    server.join();
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
