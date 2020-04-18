// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_PORTS_NODE_DELEGATE_H_
#define MOJO_EDK_SYSTEM_PORTS_NODE_DELEGATE_H_

#include <stddef.h>

#include "mojo/edk/system/ports/event.h"
#include "mojo/edk/system/ports/name.h"
#include "mojo/edk/system/ports/port_ref.h"

namespace mojo {
namespace edk {
namespace ports {

class NodeDelegate {
 public:
  virtual ~NodeDelegate() {}

  // Forward an event (possibly asynchronously) to the specified node.
  virtual void ForwardEvent(const NodeName& node, ScopedEvent event) = 0;

  // Broadcast an event to all nodes.
  virtual void BroadcastEvent(ScopedEvent event) = 0;

  // Indicates that the port's status has changed recently. Use Node::GetStatus
  // to query the latest status of the port. Note, this event could be spurious
  // if another thread is simultaneously modifying the status of the port.
  virtual void PortStatusChanged(const PortRef& port_ref) = 0;
};

}  // namespace ports
}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_PORTS_NODE_DELEGATE_H_
