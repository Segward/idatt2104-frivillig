#include <packet.hpp>

namespace {

void write_u16(std::vector<std::uint8_t>& out, std::uint16_t v)
{
  out.push_back(v >> 8);
  out.push_back(v & 0xff);
}

void write_u32(std::vector<std::uint8_t>& out, std::uint32_t v)
{
  out.push_back(v >> 24);
  out.push_back(v >> 16);
  out.push_back(v >> 8);
  out.push_back(v & 0xff);
}

void write_u64(std::vector<std::uint8_t>& out, std::uint64_t v)
{
  for (int s = 56; s >= 0; s -= 8) out.push_back((v >> s) & 0xff);
}

std::uint16_t read_u16(const std::uint8_t* p)
{
  return (std::uint16_t(p[0]) << 8) | p[1];
}

std::uint32_t read_u32(const std::uint8_t* p)
{
  return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
         (std::uint32_t(p[2]) << 8)  |  std::uint32_t(p[3]);
}

std::uint64_t read_u64(const std::uint8_t* p)
{
  std::uint64_t v = 0;
  for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
  return v;
}

void write_entry(std::vector<std::uint8_t>& out, const std::string& id,
                 std::uint64_t inc, std::uint64_t dec)
{
  write_u16(out, id.size());
  out.insert(out.end(), id.begin(), id.end());
  write_u64(out, inc);
  write_u64(out, dec);
}

}

std::vector<std::uint8_t> serialize_counter(const counter_state& s)
{
  std::vector<std::uint8_t> payload;
  std::uint32_t count = 0;

  for (const auto& [id, inc] : s.increments) {
    std::uint64_t dec = 0;
    auto it = s.decrements.find(id);
    if (it != s.decrements.end()) dec = it->second;
    write_entry(payload, id, inc, dec);
    ++count;
  }
  for (const auto& [id, dec] : s.decrements) {
    if (s.increments.find(id) != s.increments.end()) continue;
    write_entry(payload, id, 0, dec);
    ++count;
  }

  std::vector<std::uint8_t> packet;
  packet.reserve(packet_header_size + 4 + payload.size());
  packet.push_back(std::uint8_t(packet_type::COUNTER));
  write_u32(packet, 4 + payload.size());
  write_u32(packet, count);
  packet.insert(packet.end(), payload.begin(), payload.end());
  return packet;
}

counter_state parse_counter(const std::uint8_t* data, std::size_t len)
{
  if (len < packet_header_size) return {};
  if (static_cast<packet_type>(data[0]) != packet_type::COUNTER) return {};
  if (read_u32(data + 1) != len - packet_header_size) return {};

  const std::uint8_t* p = data + packet_header_size;
  const std::uint8_t* end = data + len;
  if (end - p < 4) return {};
  std::uint32_t count = read_u32(p);
  p += 4;

  counter_state out;
  for (std::uint32_t i = 0; i < count; ++i) {
    if (end - p < 2) return {};
    std::uint16_t id_len = read_u16(p);
    p += 2;
    if (std::size_t(end - p) < std::size_t(id_len) + 16) return {};
    std::string id(reinterpret_cast<const char*>(p), id_len);
    p += id_len;
    out.increments[id] = read_u64(p); p += 8;
    out.decrements[id] = read_u64(p); p += 8;
  }
  if (p != end) return {};
  return out;
}
