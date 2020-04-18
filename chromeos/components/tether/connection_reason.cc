// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/connection_reason.h"

#include "base/logging.h"

namespace chromeos {

namespace tether {

ConnectionReason MessageTypeToConnectionReason(const MessageType& reason) {
  switch (reason) {
    case MessageType::TETHER_AVAILABILITY_REQUEST:
      return ConnectionReason::TETHER_AVAILABILITY_REQUEST;
    case MessageType::CONNECT_TETHERING_REQUEST:
      return ConnectionReason::CONNECT_TETHERING_REQUEST;
    case MessageType::KEEP_ALIVE_TICKLE:
      return ConnectionReason::KEEP_ALIVE_TICKLE;
    case MessageType::DISCONNECT_TETHERING_REQUEST:
      return ConnectionReason::DISCONNECT_TETHERING_REQUEST;
    default:
      // A MessageType that doesn't correspond to a valid request message was
      // passed.
      NOTREACHED();
      return ConnectionReason::TETHER_AVAILABILITY_REQUEST;
  }
}

std::string ConnectionReasonToString(const ConnectionReason& reason) {
  switch (reason) {
    case ConnectionReason::TETHER_AVAILABILITY_REQUEST:
      return "[TetherAvailabilityRequest]";
    case ConnectionReason::CONNECT_TETHERING_REQUEST:
      return "[ConnectTetheringRequest]";
    case ConnectionReason::KEEP_ALIVE_TICKLE:
      return "[KeepAliveTickle]";
    case ConnectionReason::DISCONNECT_TETHERING_REQUEST:
      return "[DisconnectTetheringRequest]";
    case ConnectionReason::PRESERVE_CONNECTION:
      return "[PreserveConnection]";
    default:
      return "[invalid ConnectionReason]";
  }
}

}  // namespace tether

}  // namespace chromeos
