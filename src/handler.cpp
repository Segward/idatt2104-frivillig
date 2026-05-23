// Handler implementation: JSON encode/decode for the WebSocket wire protocol.
// See include/handler.hpp for the public API.
//
// Notes:
//  - The wire uses "element_id" rather than "id" because the auth envelope
//    already owns "id" (the assigned replica ID).
//  - Each message has a "type" discriminator; decoders return nullopt on
//    mismatch, which is what makes the server's try-each-envelope dispatch
//    work without exception-driven control flow.
//  - encode_counter omits per-client zero entries — a freshly seeded
//    replica then ships an empty state that peers accept as a no-op merge.
//  - Decoders validate every field before constructing output, so a
//    malformed payload yields a clean nullopt rather than partial state.

#include <handler.hpp>

#include <nlohmann/json.hpp>

namespace {
  using json = nlohmann::json;

  bool extract_counts(const json& obj, std::unordered_map<std::string, std::uint64_t>& out) {
    if (!obj.is_object()) return false;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
      if (!it.value().is_number_unsigned()) return false;
      out[it.key()] = it.value().get<std::uint64_t>();
    }
    return true;
  }
}

std::string Handler::encode_auth(const std::string& id) {
  nlohmann::json msg = {{"type", "auth"}, {"id", id}};
  return msg.dump();
}

std::string Handler::encode_counter(const CounterPNState& state) {
  nlohmann::json increments = nlohmann::json::object();
  nlohmann::json decrements = nlohmann::json::object();
  for (const auto& [id, inc] : state.increments) {
    if (inc == 0) continue;
    increments[id] = inc;
  }
  for (const auto& [id, dec] : state.decrements) {
    if (dec == 0) continue;
    decrements[id] = dec;
  }
  nlohmann::json msg = {
    {"type", "counter_state"},
    {"increments", increments},
    {"decrements", decrements},
  };
  return msg.dump();
}

std::optional<CounterPNState> Handler::decode_counter(const std::string& json_text) {
  auto parsed = nlohmann::json::parse(json_text, nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) return std::nullopt;
  auto type_it = parsed.find("type");
  if (type_it == parsed.end() || !type_it->is_string()) return std::nullopt;
  if (type_it->get<std::string>() != "counter_state") return std::nullopt;

  auto inc_it = parsed.find("increments");
  auto dec_it = parsed.find("decrements");
  if (inc_it == parsed.end() || dec_it == parsed.end()) return std::nullopt;

  CounterPNState out;
  if (!extract_counts(*inc_it, out.increments)) return std::nullopt;
  if (!extract_counts(*dec_it, out.decrements)) return std::nullopt;
  return out;
}

namespace {
  nlohmann::json list_item_to_json(const ListItem& item) {
    return {
      {"element_id", item.id},
      {"previous_id", item.previous_id},
      {"value", item.value},
      {"deleted", item.deleted},
    };
  }

  std::optional<ListItem> list_item_from_json(const nlohmann::json& j) {
    if (!j.is_object()) return std::nullopt;
    auto eid_it = j.find("element_id");
    auto pid_it = j.find("previous_id");
    auto val_it = j.find("value");
    auto del_it = j.find("deleted");
    if (eid_it == j.end() || !eid_it->is_string()) return std::nullopt;
    if (pid_it == j.end() || !pid_it->is_string()) return std::nullopt;
    if (val_it == j.end() || !val_it->is_string()) return std::nullopt;
    if (del_it == j.end() || !del_it->is_boolean()) return std::nullopt;
    ListItem out;
    out.id = eid_it->get<std::string>();
    out.previous_id = pid_it->get<std::string>();
    out.value = val_it->get<std::string>();
    out.deleted = del_it->get<bool>();
    // Empty string is reserved for the list head sentinel.
    if (out.id.empty()) return std::nullopt;
    return out;
  }
}

std::string Handler::encode_list_state(const ListRGAState& state) {
  nlohmann::json nodes = nlohmann::json::array();
  for (const auto& item : state.nodes) nodes.push_back(list_item_to_json(item));
  nlohmann::json msg = {{"type", "list_state"}, {"nodes", nodes}};
  return msg.dump();
}

std::optional<ListRGAState> Handler::decode_list_state(const std::string& json_text) {
  auto parsed = nlohmann::json::parse(json_text, nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) return std::nullopt;
  auto type_it = parsed.find("type");
  if (type_it == parsed.end() || !type_it->is_string()) return std::nullopt;
  if (type_it->get<std::string>() != "list_state") return std::nullopt;
  auto nodes_it = parsed.find("nodes");
  if (nodes_it == parsed.end() || !nodes_it->is_array()) return std::nullopt;
  ListRGAState out;
  out.nodes.reserve(nodes_it->size());
  for (const auto& entry : *nodes_it) {
    auto item = list_item_from_json(entry);
    if (!item) return std::nullopt;
    out.nodes.push_back(*item);
  }
  return out;
}

namespace {
  nlohmann::json text_character_to_json(const TextCharacter& c) {
    return {
      {"element_id", c.id},
      {"previous_id", c.previous_id},
      {"value", c.value},
      {"deleted", c.deleted},
    };
  }

  std::optional<TextCharacter> text_character_from_json(const nlohmann::json& j) {
    if (!j.is_object()) return std::nullopt;
    auto eid_it = j.find("element_id");
    auto pid_it = j.find("previous_id");
    auto val_it = j.find("value");
    auto del_it = j.find("deleted");
    if (eid_it == j.end() || !eid_it->is_string()) return std::nullopt;
    if (pid_it == j.end() || !pid_it->is_string()) return std::nullopt;
    if (val_it == j.end() || !val_it->is_string()) return std::nullopt;
    if (del_it == j.end() || !del_it->is_boolean()) return std::nullopt;
    TextCharacter out;
    out.id = eid_it->get<std::string>();
    out.previous_id = pid_it->get<std::string>();
    out.value = val_it->get<std::string>();
    out.deleted = del_it->get<bool>();
    if (out.id.empty()) return std::nullopt;
    return out;
  }
}

std::string Handler::encode_text_state(const TextRGAState& state) {
  nlohmann::json nodes = nlohmann::json::array();
  for (const auto& c : state.nodes) nodes.push_back(text_character_to_json(c));
  nlohmann::json msg = {{"type", "text_state"}, {"nodes", nodes}};
  return msg.dump();
}

std::optional<TextRGAState> Handler::decode_text_state(const std::string& json_text) {
  auto parsed = nlohmann::json::parse(json_text, nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) return std::nullopt;
  auto type_it = parsed.find("type");
  if (type_it == parsed.end() || !type_it->is_string()) return std::nullopt;
  if (type_it->get<std::string>() != "text_state") return std::nullopt;
  auto nodes_it = parsed.find("nodes");
  if (nodes_it == parsed.end() || !nodes_it->is_array()) return std::nullopt;
  TextRGAState out;
  out.nodes.reserve(nodes_it->size());
  for (const auto& entry : *nodes_it) {
    auto c = text_character_from_json(entry);
    if (!c) return std::nullopt;
    out.nodes.push_back(*c);
  }
  return out;
}
