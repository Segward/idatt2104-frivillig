#ifndef WIRE_HPP
#define WIRE_HPP

#include <counter.hpp>
#include <cstddef>
#include <string>

inline constexpr const char* default_host = "127.0.0.1";
inline constexpr unsigned default_port = 12345;

std::string serialize_state(const counter_state& s);
counter_state parse_state(const std::string& frame);

template <typename socket_type>
class line_reader {
public:
  explicit line_reader(socket_type& s) : sock(s) {}

  bool read_line(std::string& out)
  {
    out.clear();
    while (true) {
      auto nl = buf.find('\n');
      if (nl != std::string::npos) {
        out.assign(buf, 0, nl);
        buf.erase(0, nl + 1);
        return true;
      }
      char tmp[512];
      auto n = sock.read(tmp, sizeof(tmp));
      if (n <= 0) return false;
      buf.append(tmp, static_cast<std::size_t>(n));
    }
  }

private:
  socket_type& sock;
  std::string buf;
};

#endif
