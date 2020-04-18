// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/ports/port.h"

namespace mojo {
namespace edk {
namespace ports {

Port::Port(uint64_t next_sequence_num_to_send,
           uint64_t next_sequence_num_to_receive)
    : state(kUninitialized),
      next_sequence_num_to_send(next_sequence_num_to_send),
      last_sequence_num_to_receive(0),
      message_queue(next_sequence_num_to_receive),
      remove_proxy_on_last_message(false),
      peer_closed(false) {}

Port::~Port() {}

}  // namespace ports
}  // namespace edk
}  // namespace mojo
