#include <wire.hpp>

#include <sstream>
#include <string>

std::string serialize_state(const counter_state& s)
{
  std::ostringstream os;
  bool first = true;
  for (const auto& [id, inc_val] : s.increments) {
    std::uint64_t dec_val = 0;
    auto it = s.decrements.find(id);
    if (it != s.decrements.end()) dec_val = it->second;
    if (!first) os << ';';
    os << id << ':' << inc_val << ':' << dec_val;
    first = false;
  }
  for (const auto& [id, dec_val] : s.decrements) {
    if (s.increments.find(id) != s.increments.end()) continue;
    if (!first) os << ';';
    os << id << ':' << 0 << ':' << dec_val;
    first = false;
  }
  os << '\n';
  return os.str();
}

counter_state parse_state(const std::string& frame)
{
  counter_state s;
  std::size_t i = 0;
  while (i < frame.size()) {
    std::size_t semi = frame.find(';', i);
    if (semi == std::string::npos) semi = frame.size();
    std::string tok = frame.substr(i, semi - i);
    std::size_t c1 = tok.find(':');
    std::size_t c2 = c1 == std::string::npos ? std::string::npos : tok.find(':', c1 + 1);
    if (c1 != std::string::npos && c2 != std::string::npos) {
      std::string id = tok.substr(0, c1);
      if (!id.empty()) {
        try {
          std::uint64_t inc = std::stoull(tok.substr(c1 + 1, c2 - c1 - 1));
          std::uint64_t dec = std::stoull(tok.substr(c2 + 1));
          s.increments[id] = inc;
          s.decrements[id] = dec;
        } catch (...) {
        }
      }
    }
    i = semi + 1;
  }
  return s;
}
