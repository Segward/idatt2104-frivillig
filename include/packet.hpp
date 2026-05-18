#ifndef PACKET_HPP
#define PACKET_HPP

#include <counter.hpp>

inline constexpr const char* default_host = "127.0.0.1";
inline constexpr unsigned default_port = 12345;

enum class packet_type : std::uint8_t {
  COUNTER = 1,
};

inline constexpr std::size_t packet_header_size = 5;

std::vector<std::uint8_t> serialize_counter(const counter_state& s);
counter_state parse_counter(const std::uint8_t* data, std::size_t len);

template <typename socket_type>
class packet_reader {
public:
  explicit packet_reader(socket_type& s) : sock(s) {}

  bool read_packet(std::vector<std::uint8_t>& out)
  {
    if (!fill(packet_header_size)) return false;
    std::uint32_t payload_len = (std::uint32_t(buf[1]) << 24) |
                                (std::uint32_t(buf[2]) << 16) |
                                (std::uint32_t(buf[3]) << 8)  |
                                 std::uint32_t(buf[4]);
    std::size_t total = packet_header_size + payload_len;
    if (!fill(total)) return false;
    out.assign(buf.begin(), buf.begin() + total);
    buf.erase(buf.begin(), buf.begin() + total);
    return true;
  }

private:
  bool fill(std::size_t need)
  {
    while (buf.size() < need) {
      std::uint8_t tmp[512];
      auto n = sock.read(tmp, sizeof(tmp));
      if (n <= 0) return false;
      buf.insert(buf.end(), tmp, tmp + n);
    }
    return true;
  }

  socket_type& sock;
  std::vector<std::uint8_t> buf;
};

#endif
