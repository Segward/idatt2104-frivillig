#include <client.hpp>
#include <packet.hpp>

int main(int argc, char** argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s <client-id>\n", argv[0]);
    return 1;
  }
  std::string id = argv[1];

  network_client cli(id, default_host, default_port);
  cli.start();

  printf("[%s] connected to %s:%u. commands: inc [n], dec [n], val, quit\n",
         id.c_str(), default_host, default_port);

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
      cli.increment(n);
    } else if (cmd == "dec") {
      std::uint64_t n = 1;
      is >> n;
      cli.decrement(n);
    } else if (cmd == "val") {
      printf("[%s] value=%lld\n", id.c_str(), static_cast<long long>(cli.value()));
    } else {
      printf("[%s] unknown command: %s\n", id.c_str(), cmd.c_str());
    }
  }
  return 0;
}
