#include <handler.hpp>

namespace {
  void write_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(value >> 8);
    out.push_back(value & 0xff);
  }

  void write_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.push_back(value >> 24);
    out.push_back(value >> 16);
    out.push_back(value >> 8);
    out.push_back(value & 0xff);
  }

  void write_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (int s = 56; s >= 0; s -= 8) out.push_back((value >> s) & 0xff);
  }

  std::uint16_t read_u16(const std::uint8_t* p) {
    return (std::uint16_t(p[0]) << 8) | p[1];
  }

  std::uint32_t read_u32(const std::uint8_t* p) {
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
           (std::uint32_t(p[2]) << 8)  |  std::uint32_t(p[3]);
  }

  std::uint64_t read_u64(const std::uint8_t* p) {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    return v;
  }

  void write_entry(std::vector<std::uint8_t>& out, const std::string& id,
                   std::uint64_t inc, std::uint64_t dec) {
    write_u16(out, static_cast<std::uint16_t>(id.size()));
    out.insert(out.end(), id.begin(), id.end());
    write_u64(out, inc);
    write_u64(out, dec);
  }
}

Handler::Handler(Counter& target, std::string filter_id)
  : _target(target), _filter_id(std::move(filter_id)) {}

bool Handler::apply(Packet::Type type, const std::vector<std::uint8_t>& payload) {
  switch (type) {
    case Packet::Type::Counter: {
      auto incoming = decode_counter(payload.data(), payload.size());
      if (!incoming) return false;
      if (_filter_id.empty()) {
        _target.merge(*incoming);
        return true;
      }
      CounterState filtered;
      if (auto it = incoming->increments.find(_filter_id); it != incoming->increments.end()) {
        filtered.increments[_filter_id] = it->second;
      }
      if (auto it = incoming->decrements.find(_filter_id); it != incoming->decrements.end()) {
        filtered.decrements[_filter_id] = it->second;
      }
      _target.merge(filtered);
      return true;
    }
    case Packet::Type::Auth:
      return false;
  }
  return false;
}

std::vector<std::uint8_t> Handler::encode_counter(const CounterState& state) {
  std::vector<std::uint8_t> payload;
  std::uint32_t count = 0;

  for (const auto& [id, inc] : state.increments) {
    std::uint64_t dec = 0;
    auto it = state.decrements.find(id);
    if (it != state.decrements.end()) dec = it->second;
    if (id.size() > std::numeric_limits<std::uint16_t>::max()) continue;
    write_entry(payload, id, inc, dec);
    ++count;
  }
  for (const auto& [id, dec] : state.decrements) {
    if (state.increments.find(id) != state.increments.end()) continue;
    if (id.size() > std::numeric_limits<std::uint16_t>::max()) continue;
    write_entry(payload, id, 0, dec);
    ++count;
  }

  std::vector<std::uint8_t> out;
  out.reserve(4 + payload.size());
  write_u32(out, count);
  out.insert(out.end(), payload.begin(), payload.end());
  return out;
}

std::optional<CounterState> Handler::decode_counter(const std::uint8_t* data, std::size_t len) {
  const std::uint8_t* p = data;
  const std::uint8_t* end = data + len;
  if (end - p < 4) return std::nullopt;
  std::uint32_t count = read_u32(p);
  p += 4;

  CounterState out;
  for (std::uint32_t i = 0; i < count; ++i) {
    if (end - p < 2) return std::nullopt;
    std::uint16_t id_len = read_u16(p);
    p += 2;
    if (std::size_t(end - p) < std::size_t(id_len) + 16) return std::nullopt;
    // ids are treated as opaque bytes — no UTF-8 validation.
    std::string id(reinterpret_cast<const char*>(p), id_len);
    p += id_len;
    if (out.increments.count(id) || out.decrements.count(id)) return std::nullopt;
    out.increments[id] = read_u64(p); p += 8;
    out.decrements[id] = read_u64(p); p += 8;
  }
  if (p != end) return std::nullopt;
  return out;
}
