#include <gtest/gtest.h>
#include <server.hpp>
#include <string>
#include <sockpp/tcp_connector.h>

TEST(server_test, start_reads_from_client) {
  testing::internal::CaptureStdout();
  server srv("127.0.0.1", 13201);
  srv.start();

  {
    sockpp::tcp_connector conn(sockpp::inet_address("127.0.0.1", 13201));
    conn.write(std::string("ping"));
  }

  srv.join();
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("ping"), std::string::npos);
}

TEST(server_test, send_writes_to_client) {
  server srv("127.0.0.1", 13202);
  srv.start();

  sockpp::tcp_connector conn(sockpp::inet_address("127.0.0.1", 13202));
  srv.send("hello");

  char buf[64];
  ssize_t n = conn.read(buf, sizeof(buf));
  std::string received(buf, n > 0 ? n : 0);

  EXPECT_EQ(received, "hello");
}
