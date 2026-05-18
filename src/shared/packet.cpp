#include <packet.hpp>

namespace {
  constexpr std::size_t read_chunk = 512;

  void write_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(value >> 24);
    out.push_back(value >> 16);
    out.push_back(value >> 8);
    out.push_back(value & 0xff);
  }

  std::uint32_t read_u32(const std::uint8_t* p) {
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
           (std::uint32_t(p[2]) << 8)  |  std::uint32_t(p[3]);
  }
}

Packet::Packet(sockpp::stream_socket& sock)
  : _sock(sock) {}

bool Packet::read(Type& out_type, std::vector<std::uint8_t>& out_payload) {
  if (!fill(header_size)) return false;
  std::uint32_t payload_len = read_u32(_buffer.data() + 1);
  if (payload_len > max_payload) return false;
  std::size_t total = header_size + payload_len;
  if (!fill(total)) return false;

  out_type = static_cast<Type>(_buffer[0]);
  out_payload.assign(_buffer.begin() + header_size, _buffer.begin() + total);
  _buffer.erase(_buffer.begin(), _buffer.begin() + total);
  return true;
}

bool Packet::write(Type type, const std::vector<std::uint8_t>& payload) {
  if (payload.size() > max_payload) return false;
  std::vector<std::uint8_t> frame;
  frame.reserve(header_size + payload.size());
  frame.push_back(static_cast<std::uint8_t>(type));
  write_u32(frame, static_cast<std::uint32_t>(payload.size()));
  frame.insert(frame.end(), payload.begin(), payload.end());
  return _sock.write(frame.data(), frame.size()) == static_cast<ssize_t>(frame.size());
}

bool Packet::fill(std::size_t need) {
  while (_buffer.size() < need) {
    std::uint8_t tmp[read_chunk];
    auto n = _sock.read(tmp, sizeof(tmp));
    if (n <= 0) return false;
    _buffer.insert(_buffer.end(), tmp, tmp + n);
  }
  return true;
}
