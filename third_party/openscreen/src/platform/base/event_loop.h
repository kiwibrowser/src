// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_EVENT_LOOP_H_
#define PLATFORM_BASE_EVENT_LOOP_H_

#include <sys/types.h>

#include <array>
#include <cstdint>
#include <vector>

#include "osp_base/error.h"
#include "osp_base/ip_address.h"
#include "platform/api/event_waiter.h"

namespace openscreen {
namespace platform {

static constexpr int kUdpMaxPacketSize = 1 << 16;

struct ReceivedData {
  ReceivedData();
  ReceivedData(ReceivedData&&) MAYBE_NOEXCEPT;
  ~ReceivedData();

  IPEndpoint source;
  IPEndpoint original_destination;
  std::array<uint8_t, kUdpMaxPacketSize> bytes;
  ssize_t length;
  UdpSocket* socket;
};

Error ReceiveDataFromEvent(const UdpSocketReadableEvent& read_event,
                           ReceivedData* data);
std::vector<ReceivedData> HandleUdpSocketReadEvents(const Events& events);
std::vector<ReceivedData> OnePlatformLoopIteration(EventWaiterPtr waiter);

}  // namespace platform
}  // namespace openscreen

#endif  // PLATFORM_BASE_EVENT_LOOP_H_
