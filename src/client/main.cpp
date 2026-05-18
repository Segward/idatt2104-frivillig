#include <client.hpp>
#include <packet.hpp>

int main() {
  try {
    Client client(default_host, default_port);
    client.start();

    printf("[%s] connected to %s:%u. commands: inc [n], dec [n], val, quit\n",
           client.id().c_str(), default_host, default_port);

    std::string line;
    while (std::getline(std::cin, line)) {
      std::istringstream is(line);
      std::string cmd;
      is >> cmd;
      if (cmd.empty()) continue;
      if (cmd == "quit" || cmd == "exit") break;
      if (cmd == "inc") {
        std::uint64_t n = 1;
        is >> n;
        client.increment(n);
      } else if (cmd == "dec") {
        std::uint64_t n = 1;
        is >> n;
        client.decrement(n);
      } else if (cmd == "val") {
        printf("[%s] value=%lld\n", client.id().c_str(),
               static_cast<long long>(client.value()));
      } else {
        printf("[%s] unknown command: %s\n", client.id().c_str(), cmd.c_str());
      }
    }
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
