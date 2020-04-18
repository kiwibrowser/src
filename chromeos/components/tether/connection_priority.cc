// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/connection_priority.h"

#include "base/logging.h"

namespace chromeos {

namespace tether {

ConnectionPriority PriorityForConnectionReason(
    ConnectionReason connection_reason) {
  switch (connection_reason) {
    case ConnectionReason::CONNECT_TETHERING_REQUEST:
    case ConnectionReason::DISCONNECT_TETHERING_REQUEST:
      return ConnectionPriority::CONNECTION_PRIORITY_HIGH;
    case ConnectionReason::KEEP_ALIVE_TICKLE:
      return ConnectionPriority::CONNECTION_PRIORITY_MEDIUM;
    default:
      return ConnectionPriority::CONNECTION_PRIORITY_LOW;
  }
}

ConnectionPriority HighestPriorityForConnectionReasons(
    std::set<ConnectionReason> connection_reasons) {
  DCHECK(!connection_reasons.empty());

  ConnectionPriority highest_priority =
      ConnectionPriority::CONNECTION_PRIORITY_LOW;
  for (const auto& connection_reason : connection_reasons) {
    ConnectionPriority priority_for_type =
        PriorityForConnectionReason(connection_reason);
    if (priority_for_type > highest_priority)
      highest_priority = priority_for_type;
  }

  return highest_priority;
}

}  // namespace tether

}  // namespace chromeos
