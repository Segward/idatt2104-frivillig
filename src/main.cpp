#include <server.hpp>
#include <messages.hpp>

int main() {
  try {
    Server server(default_host, default_port);
    server.start();
    server.join();
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
