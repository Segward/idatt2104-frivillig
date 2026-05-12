#include <gtest/gtest.h>
#include <client.hpp>
#include <string>
#include <thread>
#include <sockpp/tcp_acceptor.h>

using namespace std::chrono_literals;

TEST(client_test, send_writes_to_server) {
  std::string received;
  std::thread server([&]() {
    sockpp::tcp_acceptor acc(sockpp::inet_address("127.0.0.1", 13101));
    auto sock = acc.accept();
    char buf[64];
    ssize_t n = sock.read(buf, sizeof(buf));
    received.assign(buf, n);
  });
  std::this_thread::sleep_for(50ms);

  client("127.0.0.1", 13101).send("ping");
  server.join();

  EXPECT_EQ(received, "ping");
}

TEST(client_test, start_reads_from_server) {
  std::thread server([]() {
    sockpp::tcp_acceptor acc(sockpp::inet_address("127.0.0.1", 13102));
    auto sock = acc.accept();
    sock.write(std::string("hello"));
  });
  std::this_thread::sleep_for(50ms);

  testing::internal::CaptureStdout();
  client user("127.0.0.1", 13102);
  user.start();
  user.join();
  std::string out = testing::internal::GetCapturedStdout();
  server.join();

  EXPECT_NE(out.find("hello"), std::string::npos);
}
