#pragma once
// Small zsock helpers shared by the RX and TX threads.
#include <zephyr/net/socket.h>

#include <cstdint>
#include <cstring>

#include "config.hpp"

namespace leg::net {

inline sockaddr_in master_endpoint() {
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(cfg::kPortTx);
  zsock_inet_pton(AF_INET, cfg::kMasterIp, &a.sin_addr);
  return a;
}

// Bound RX socket (we listen on :5005), or -1.
inline int make_rx_socket() {
  const int s = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s < 0) return -1;
  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = INADDR_ANY;
  local.sin_port = htons(cfg::kPortRx);
  if (zsock_bind(s, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0) {
    zsock_close(s);
    return -1;
  }
  return s;
}

// Unbound TX socket for telemetry/acks toward the master.
inline int make_tx_socket() {
  return zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

inline bool from_master(const sockaddr_in& from) {
  static const sockaddr_in master = master_endpoint();
  return std::memcmp(&from.sin_addr, &master.sin_addr, sizeof(in_addr)) == 0;
}

}  // namespace leg::net
