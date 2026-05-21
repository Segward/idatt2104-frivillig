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

std::string Handler::encode_counter(const counter_pn_state& state) {
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

std::optional<counter_pn_state> Handler::decode_counter(const std::string& json_text) {
  auto parsed = nlohmann::json::parse(json_text, nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) return std::nullopt;
  auto type_it = parsed.find("type");
  if (type_it == parsed.end() || !type_it->is_string()) return std::nullopt;
  if (type_it->get<std::string>() != "counter_state") return std::nullopt;

  auto inc_it = parsed.find("increments");
  auto dec_it = parsed.find("decrements");
  if (inc_it == parsed.end() || dec_it == parsed.end()) return std::nullopt;

  counter_pn_state out;
  if (!extract_counts(*inc_it, out.increments)) return std::nullopt;
  if (!extract_counts(*dec_it, out.decrements)) return std::nullopt;
  return out;
}

namespace {
  nlohmann::json list_item_to_json(const list_item& item) {
    return {
      {"element_id", item.id},
      {"previous_id", item.previous_id},
      {"value", item.value},
      {"deleted", item.deleted},
    };
  }

  std::optional<list_item> list_item_from_json(const nlohmann::json& j) {
    if (!j.is_object()) return std::nullopt;
    auto eid_it = j.find("element_id");
    auto pid_it = j.find("previous_id");
    auto val_it = j.find("value");
    auto del_it = j.find("deleted");
    if (eid_it == j.end() || !eid_it->is_string()) return std::nullopt;
    if (pid_it == j.end() || !pid_it->is_string()) return std::nullopt;
    if (val_it == j.end() || !val_it->is_string()) return std::nullopt;
    if (del_it == j.end() || !del_it->is_boolean()) return std::nullopt;
    list_item out;
    out.id = eid_it->get<std::string>();
    out.previous_id = pid_it->get<std::string>();
    out.value = val_it->get<std::string>();
    out.deleted = del_it->get<bool>();
    if (out.id.empty()) return std::nullopt;
    return out;
  }
}

std::string Handler::encode_list_state(const list_RGA_state& state) {
  nlohmann::json nodes = nlohmann::json::array();
  for (const auto& item : state.nodes) nodes.push_back(list_item_to_json(item));
  nlohmann::json msg = {{"type", "list_state"}, {"nodes", nodes}};
  return msg.dump();
}

std::optional<list_RGA_state> Handler::decode_list_state(const std::string& json_text) {
  auto parsed = nlohmann::json::parse(json_text, nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) return std::nullopt;
  auto type_it = parsed.find("type");
  if (type_it == parsed.end() || !type_it->is_string()) return std::nullopt;
  if (type_it->get<std::string>() != "list_state") return std::nullopt;
  auto nodes_it = parsed.find("nodes");
  if (nodes_it == parsed.end() || !nodes_it->is_array()) return std::nullopt;
  list_RGA_state out;
  out.nodes.reserve(nodes_it->size());
  for (const auto& entry : *nodes_it) {
    auto item = list_item_from_json(entry);
    if (!item) return std::nullopt;
    out.nodes.push_back(*item);
  }
  return out;
}

namespace {
  nlohmann::json text_character_to_json(const text_character& c) {
    return {
      {"element_id", c.id},
      {"previous_id", c.previous_id},
      {"value", c.value},
      {"deleted", c.deleted},
    };
  }

  std::optional<text_character> text_character_from_json(const nlohmann::json& j) {
    if (!j.is_object()) return std::nullopt;
    auto eid_it = j.find("element_id");
    auto pid_it = j.find("previous_id");
    auto val_it = j.find("value");
    auto del_it = j.find("deleted");
    if (eid_it == j.end() || !eid_it->is_string()) return std::nullopt;
    if (pid_it == j.end() || !pid_it->is_string()) return std::nullopt;
    if (val_it == j.end() || !val_it->is_string()) return std::nullopt;
    if (del_it == j.end() || !del_it->is_boolean()) return std::nullopt;
    text_character out;
    out.id = eid_it->get<std::string>();
    out.previous_id = pid_it->get<std::string>();
    out.value = val_it->get<std::string>();
    out.deleted = del_it->get<bool>();
    if (out.id.empty()) return std::nullopt;
    return out;
  }
}

std::string Handler::encode_text_state(const text_RGA_state& state) {
  nlohmann::json nodes = nlohmann::json::array();
  for (const auto& c : state.nodes) nodes.push_back(text_character_to_json(c));
  nlohmann::json msg = {{"type", "text_state"}, {"nodes", nodes}};
  return msg.dump();
}

std::optional<text_RGA_state> Handler::decode_text_state(const std::string& json_text) {
  auto parsed = nlohmann::json::parse(json_text, nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) return std::nullopt;
  auto type_it = parsed.find("type");
  if (type_it == parsed.end() || !type_it->is_string()) return std::nullopt;
  if (type_it->get<std::string>() != "text_state") return std::nullopt;
  auto nodes_it = parsed.find("nodes");
  if (nodes_it == parsed.end() || !nodes_it->is_array()) return std::nullopt;
  text_RGA_state out;
  out.nodes.reserve(nodes_it->size());
  for (const auto& entry : *nodes_it) {
    auto c = text_character_from_json(entry);
    if (!c) return std::nullopt;
    out.nodes.push_back(*c);
  }
  return out;
}
