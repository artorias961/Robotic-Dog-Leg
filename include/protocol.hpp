#pragma once
// Wire protocol — byte-for-byte identical to the master's protocol.hpp
// (Conventions.md): little-endian packed structs, 14-byte header,
// CRC16-CCITT over header+payload, crc field last.
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace leg::proto {

inline constexpr std::uint16_t kMagic   = 0xC0DE;  // fixed start marker
inline constexpr std::uint8_t  kVersion = 1;

enum class Type : std::uint8_t {
  Setpoint  = 1,
  Command   = 2,
  Heartbeat = 3,
  Telemetry = 4,
  Ack       = 5,
};

enum class Cmd : std::uint8_t {
  Enable      = 1,  // arm (refused while faults are latched)
  Disable     = 2,  // disarm
  ClearFaults = 3,  // operator acknowledged a fault; only honored while disarmed
};

#pragma pack(push, 1)
struct Header {
  std::uint16_t magic;
  std::uint8_t  version;
  std::uint8_t  type;
  std::uint8_t  leg_id;
  std::uint8_t  flags;
  std::uint32_t seq;
  std::uint32_t t_ms;
};
static_assert(sizeof(Header) == 14);

struct SetpointPacket {
  Header header;
  float  target_rad[3];
  std::uint16_t crc;
};
struct CommandPacket {
  Header header;
  std::uint8_t cmd;
  float        param;
  std::uint16_t crc;
};
struct HeartbeatPacket {
  Header header;
  std::uint8_t fsm_state;
  std::uint16_t crc;
};
struct TelemetryPacket {
  Header header;
  float pos_rad[3];
  float current_a[3];
  std::uint16_t fault_flags;
  std::uint16_t lost_packets;
  std::uint16_t crc_errors;
  std::uint8_t  armed;
  std::uint8_t  state;
  std::uint16_t crc;
};
struct AckPacket {
  Header header;
  std::uint32_t acked_seq;
  std::uint16_t crc;
};
#pragma pack(pop)

constexpr std::uint16_t crc16(std::span<const std::uint8_t> data) {
  std::uint16_t crc = 0xFFFF;
  for (std::uint8_t b : data) {
    crc ^= static_cast<std::uint16_t>(b) << 8;
    for (int i = 0; i < 8; ++i)
      crc = (crc & 0x8000) ? static_cast<std::uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<std::uint16_t>(crc << 1);
  }
  return crc;
}

template <typename P>
void seal(P& pkt) {
  auto* bytes = reinterpret_cast<std::uint8_t*>(&pkt);
  pkt.crc = crc16({bytes, sizeof(P) - sizeof(std::uint16_t)});
}

template <typename P>
std::optional<P> parse(std::span<const std::uint8_t> raw, Type expected) {
  if (raw.size() != sizeof(P)) return std::nullopt;
  P pkt;
  std::memcpy(&pkt, raw.data(), sizeof(P));
  if (pkt.header.magic != kMagic) return std::nullopt;
  if (pkt.header.version != kVersion) return std::nullopt;
  if (pkt.header.type != static_cast<std::uint8_t>(expected)) return std::nullopt;
  if (crc16(raw.first(sizeof(P) - sizeof(std::uint16_t))) != pkt.crc) return std::nullopt;
  return pkt;
}

}  // namespace leg::proto
