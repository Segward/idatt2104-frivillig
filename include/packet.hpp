#ifndef PACKET_HPP
#define PACKET_HPP

#include <sockpp/stream_socket.h>

inline constexpr const char* default_host = "127.0.0.1";
inline constexpr unsigned default_port = 12345;

class Packet {
  public:
    enum class Type : std::uint8_t {
      Auth = 1,
      Counter = 2,
    };

    static constexpr std::size_t header_size = 5;
    static constexpr std::size_t max_payload = 1u << 20;

    explicit Packet(sockpp::stream_socket& sock);
    ~Packet() = default;

    Packet(const Packet&) = delete;
    Packet& operator=(const Packet&) = delete;
    Packet(Packet&&) = delete;
    Packet& operator=(Packet&&) = delete;

    bool read(Type& out_type, std::vector<std::uint8_t>& out_payload);
    bool write(Type type, const std::vector<std::uint8_t>& payload);

  private:
    bool fill(std::size_t need);

    sockpp::stream_socket& _sock;
    std::vector<std::uint8_t> _buffer;
};

#endif
