#ifndef MESSAGES_HPP
#define MESSAGES_HPP

inline constexpr const char* default_host = "0.0.0.0";
inline constexpr unsigned default_port = 12345;

inline constexpr const char* default_cert_path =
    "/etc/letsencrypt/live/segward.com/fullchain.pem";
inline constexpr const char* default_key_path =
    "/etc/letsencrypt/live/segward.com/privkey.pem";

#endif
