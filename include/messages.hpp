#ifndef MESSAGES_HPP
#define MESSAGES_HPP

// Fallback listen address used by main.cpp when no override is supplied.
// Centralised so config, scripts, and tests stay in sync.
inline constexpr const char* default_host = "0.0.0.0";
inline constexpr unsigned default_port = 12345;

#endif
