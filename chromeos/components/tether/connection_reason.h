// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_CONNECTION_REASON_H_
#define CHROMEOS_COMPONENTS_TETHER_CONNECTION_REASON_H_

#include <set>

#include "chromeos/components/tether/proto/tether.pb.h"

namespace chromeos {

namespace tether {

// Describes why a connection should be made to remote device. Most directly
// correspond to which MessageType should be sent to that same device.
enum class ConnectionReason {
  TETHER_AVAILABILITY_REQUEST,
  CONNECT_TETHERING_REQUEST,
  KEEP_ALIVE_TICKLE,
  DISCONNECT_TETHERING_REQUEST,
  // Indicates a connection should live beyond its immediately useful lifetime;
  // does not indicate that a particular message should be sent.
  PRESERVE_CONNECTION
};

ConnectionReason MessageTypeToConnectionReason(const MessageType& reason);
std::string ConnectionReasonToString(const ConnectionReason& reason);

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_CONNECTION_REASON_H_
