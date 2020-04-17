// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/event_loop.h"

#include <utility>

#include "platform/api/logging.h"
#include "platform/api/udp_socket.h"

namespace openscreen {
namespace platform {

ReceivedData::ReceivedData() = default;
ReceivedData::ReceivedData(ReceivedData&&) = default;
ReceivedData::~ReceivedData() = default;

Error ReceiveDataFromEvent(const UdpSocketReadableEvent& read_event,
                           ReceivedData* data) {
  OSP_DCHECK(data);
  ErrorOr<size_t> len = read_event.socket->ReceiveMessage(
      &data->bytes[0], data->bytes.size(), &data->source,
      &data->original_destination);
  if (!len) {
    OSP_LOG_ERROR << "ReceiveMessage() on socket failed: "
                  << len.error().message();
    return len.error();
  }
  OSP_DCHECK_LE(len.value(), static_cast<size_t>(kUdpMaxPacketSize));
  data->length = len.value();
  data->socket = read_event.socket;
  return Error::None();
}

std::vector<ReceivedData> HandleUdpSocketReadEvents(const Events& events) {
  std::vector<ReceivedData> data;
  for (const auto& read_event : events.udp_readable_events) {
    ReceivedData next_data;
    if (ReceiveDataFromEvent(read_event, &next_data).ok())
      data.emplace_back(std::move(next_data));
  }
  return data;
}

std::vector<ReceivedData> OnePlatformLoopIteration(EventWaiterPtr waiter) {
  ErrorOr<Events> events = WaitForEvents(waiter);
  if (!events)
    return {};

  return HandleUdpSocketReadEvents(events.value());
}

}  // namespace platform
}  // namespace openscreen
