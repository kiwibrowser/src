// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_ICE_CONFIG_H_
#define REMOTING_PROTOCOL_ICE_CONFIG_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "third_party/webrtc/p2p/base/portallocator.h"
#include "third_party/webrtc/rtc_base/socketaddress.h"

namespace base {
class DictionaryValue;
}  // namespace base

namespace remoting {
namespace protocol {

struct IceConfig {
  IceConfig();
  IceConfig(const IceConfig& other);
  ~IceConfig();

  bool is_null() const { return expiration_time.is_null(); }

  // Parses JSON representation of the config. Returns null config if parsing
  // fails.
  static IceConfig Parse(const base::DictionaryValue& dictionary);
  static IceConfig Parse(const std::string& config_json);

  // Time when the config will stop being valid and need to be refreshed.
  base::Time expiration_time;

  std::vector<rtc::SocketAddress> stun_servers;

  // Legacy GTURN relay servers.
  std::vector<std::string> relay_servers;
  std::string relay_token;

  // Standard TURN servers
  std::vector<cricket::RelayServerConfig> turn_servers;
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_ICE_CONFIG_H_
