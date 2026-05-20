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

Handler::Handler(counter_pn& target, std::string filter_id)
  : _target(target), _filter_id(std::move(filter_id)) {}

bool Handler::apply(const std::string& json_text) {
  auto incoming = decode_counter(json_text);
  if (!incoming) return false;
  if (_filter_id.empty()) {
    _target.merge(*incoming);
    return true;
  }
  counter_pn_state filtered;
  if (auto it = incoming->increments.find(_filter_id); it != incoming->increments.end()) {
    filtered.increments[_filter_id] = it->second;
  }
  if (auto it = incoming->decrements.find(_filter_id); it != incoming->decrements.end()) {
    filtered.decrements[_filter_id] = it->second;
  }
  _target.merge(filtered);
  return true;
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
    {"type", "counter"},
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
  if (type_it->get<std::string>() != "counter") return std::nullopt;

  auto inc_it = parsed.find("increments");
  auto dec_it = parsed.find("decrements");
  if (inc_it == parsed.end() || dec_it == parsed.end()) return std::nullopt;

  counter_pn_state out;
  if (!extract_counts(*inc_it, out.increments)) return std::nullopt;
  if (!extract_counts(*dec_it, out.decrements)) return std::nullopt;
  return out;
}

namespace {
  nlohmann::json list_change_to_json(const list_change& c) {
    return {
      {"op_type", c.type == list_operation_type::Insert ? "insert" : "delete"},
      {"operation_id", c.operation_id},
      {"element_id", c.element_id},
      {"previous_id", c.previous_id},
      {"value", c.value},
    };
  }

  std::optional<list_change> list_change_from_json(const nlohmann::json& j) {
    if (!j.is_object()) return std::nullopt;
    auto op_it = j.find("op_type");
    auto opid_it = j.find("operation_id");
    auto eid_it = j.find("element_id");
    auto pid_it = j.find("previous_id");
    auto val_it = j.find("value");
    if (op_it == j.end() || !op_it->is_string()) return std::nullopt;
    if (opid_it == j.end() || !opid_it->is_string()) return std::nullopt;
    if (eid_it == j.end() || !eid_it->is_string()) return std::nullopt;
    if (pid_it == j.end() || !pid_it->is_string()) return std::nullopt;
    if (val_it == j.end() || !val_it->is_string()) return std::nullopt;
    const std::string op = op_it->get<std::string>();
    if (op != "insert" && op != "delete") return std::nullopt;
    list_change out;
    out.type = (op == "insert") ? list_operation_type::Insert : list_operation_type::Delete;
    out.operation_id = opid_it->get<std::string>();
    out.element_id = eid_it->get<std::string>();
    out.previous_id = pid_it->get<std::string>();
    out.value = val_it->get<std::string>();
    if (out.operation_id.empty() || out.element_id.empty()) return std::nullopt;
    if (out.type == list_operation_type::Insert && out.value.empty()) return std::nullopt;
    return out;
  }
}

std::string Handler::encode_list_op(const list_change& change) {
  nlohmann::json msg = list_change_to_json(change);
  msg["type"] = "list_op";
  return msg.dump();
}

std::optional<list_change> Handler::decode_list_op(const std::string& json_text) {
  auto parsed = nlohmann::json::parse(json_text, nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) return std::nullopt;
  auto type_it = parsed.find("type");
  if (type_it == parsed.end() || !type_it->is_string()) return std::nullopt;
  if (type_it->get<std::string>() != "list_op") return std::nullopt;
  return list_change_from_json(parsed);
}

std::string Handler::encode_list_init(const std::vector<list_change>& ops) {
  nlohmann::json arr = nlohmann::json::array();
  for (const auto& c : ops) arr.push_back(list_change_to_json(c));
  nlohmann::json msg = {{"type", "list_init"}, {"ops", arr}};
  return msg.dump();
}

namespace {
  nlohmann::json text_change_to_json(const text_change& c) {
    return {
      {"op_type", c.type == text_operation_type::Insert ? "insert" : "delete"},
      {"operation_id", c.operation_id},
      {"element_id", c.element_id},
      {"previous_id", c.previous_id},
      {"value", std::string(1, c.value)},
    };
  }

  std::optional<text_change> text_change_from_json(const nlohmann::json& j) {
    if (!j.is_object()) return std::nullopt;
    auto op_it = j.find("op_type");
    auto opid_it = j.find("operation_id");
    auto eid_it = j.find("element_id");
    auto pid_it = j.find("previous_id");
    auto val_it = j.find("value");
    if (op_it == j.end() || !op_it->is_string()) return std::nullopt;
    if (opid_it == j.end() || !opid_it->is_string()) return std::nullopt;
    if (eid_it == j.end() || !eid_it->is_string()) return std::nullopt;
    if (pid_it == j.end() || !pid_it->is_string()) return std::nullopt;
    if (val_it == j.end() || !val_it->is_string()) return std::nullopt;
    const std::string op = op_it->get<std::string>();
    if (op != "insert" && op != "delete") return std::nullopt;
    text_change out;
    out.type = (op == "insert") ? text_operation_type::Insert : text_operation_type::Delete;
    out.operation_id = opid_it->get<std::string>();
    out.element_id = eid_it->get<std::string>();
    out.previous_id = pid_it->get<std::string>();
    const std::string v = val_it->get<std::string>();
    out.value = v.empty() ? '\0' : v[0];
    if (out.operation_id.empty() || out.element_id.empty()) return std::nullopt;
    if (out.type == text_operation_type::Insert && v.empty()) return std::nullopt;
    return out;
  }
}

std::string Handler::encode_text_op(const text_change& change) {
  nlohmann::json msg = text_change_to_json(change);
  msg["type"] = "text_op";
  return msg.dump();
}

std::optional<text_change> Handler::decode_text_op(const std::string& json_text) {
  auto parsed = nlohmann::json::parse(json_text, nullptr, false);
  if (parsed.is_discarded() || !parsed.is_object()) return std::nullopt;
  auto type_it = parsed.find("type");
  if (type_it == parsed.end() || !type_it->is_string()) return std::nullopt;
  if (type_it->get<std::string>() != "text_op") return std::nullopt;
  return text_change_from_json(parsed);
}

std::string Handler::encode_text_init(const std::vector<text_change>& ops) {
  nlohmann::json arr = nlohmann::json::array();
  for (const auto& c : ops) arr.push_back(text_change_to_json(c));
  nlohmann::json msg = {{"type", "text_init"}, {"ops", arr}};
  return msg.dump();
}
