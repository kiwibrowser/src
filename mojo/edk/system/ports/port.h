// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_PORTS_PORT_H_
#define MOJO_EDK_SYSTEM_PORTS_PORT_H_

#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "mojo/edk/system/ports/event.h"
#include "mojo/edk/system/ports/message_queue.h"
#include "mojo/edk/system/ports/user_data.h"

namespace mojo {
namespace edk {
namespace ports {

class PortLocker;

// A Port is essentially a node in a circular list of addresses. For the sake of
// this documentation such a list will henceforth be referred to as a "route."
// Routes are the fundamental medium upon which all Node event circulation takes
// place and are thus the backbone of all Mojo message passing.
//
// Each Port is identified by a 128-bit address within a Node (see node.h). A
// Port doesn't really *do* anything per se: it's a named collection of state,
// and its owning Node manages all event production, transmission, routing, and
// processing logic. See Node for more details on how Ports may be used to
// transmit arbitrary user messages as well as other Ports.
//
// Ports may be in any of a handful of states (see State below) which dictate
// how they react to system events targeting them. In the simplest and most
// common case, Ports are initially created as an entangled pair (i.e. a simple
// cycle consisting of two Ports) both in the |kReceiving| State. Consider Ports
// we'll label |A| and |B| here, which may be created using
// Node::CreatePortPair():
//
//     +-----+          +-----+
//     |     |--------->|     |
//     |  A  |          |  B  |
//     |     |<---------|     |
//     +-----+          +-----+
//
// |A| references |B| via |peer_node_name| and |peer_port_name|, while |B| in
// turn references |A|. Note that a Node is NEVER aware of who is sending events
// to a given Port; it is only aware of where it must route events FROM a given
// Port.
//
// For the sake of documentation, we refer to one receiving port in a route as
// the "conjugate" of the other. A receiving port's conjugate is also its peer
// upon initial creation, but because of proxying this may not be the case at a
// later time.
//
// ALL access to this data structure must be guarded by |lock_| acquisition,
// which is only possible using a PortLocker. PortLocker ensures that
// overlapping Port lock acquisitions on a single thread are always acquired in
// a globally consistent order.
class Port : public base::RefCountedThreadSafe<Port> {
 public:
  // The state of a given Port. A Port may only exist in one of these states at
  // any given time.
  enum State {
    // The Port is not yet paired with a peer and is therefore unusable. See
    // Node::CreateUninitializedPort and Node::InitializePort for motivation.
    kUninitialized,

    // The Port is publicly visible outside of its Node and may be used to send
    // and receive user messages. There are always AT MOST two |kReceiving|
    // Ports along any given route. A user message event sent from a receiving
    // port is always circulated along the Port's route until it reaches either
    // a dead-end -- in which case the route is broken -- or it reaches the
    // other receiving Port in the route -- in which case it lands in that
    // Port's incoming message queue which can by read by user code.
    kReceiving,

    // The Port has been taken out of the |kReceiving| state in preparation for
    // proxying to a new destination. A Port enters this state immediately when
    // it's attached to a user message and may only leave this state when
    // transitioning to |kProxying|. See Node for more details.
    kBuffering,

    // The Port is forwarding all user messages (and most other events) to its
    // peer without discretion. Ports in the |kProxying| state may never leave
    // this state and only exist temporarily until their owning Node has
    // established that no more events will target them. See Node for more
    // details.
    kProxying,

    // The Port has been closed and is now permanently unusable. Only
    // |kReceiving| ports can be closed.
    kClosed
  };

  // The current State of the Port.
  State state;

  // The Node and Port address to which events should be routed FROM this Port.
  // Note that this is NOT necessarily the address of the Port currently sending
  // events TO this Port.
  NodeName peer_node_name;
  PortName peer_port_name;

  // The next available sequence number to use for outgoing user message events
  // originating from this port.
  uint64_t next_sequence_num_to_send;

  // The sequence number of the last message this Port should ever expect to
  // receive in its lifetime. May be used to determine that a proxying port is
  // ready to be destroyed or that a receiving port's conjugate has been closed
  // and we know the sequence number of the last message it sent.
  uint64_t last_sequence_num_to_receive;

  // The queue of incoming user messages received by this Port. Only non-empty
  // for buffering or receiving Ports. When a buffering port enters the proxying
  // state, it flushes its queue and the proxy then bypasses the queue
  // indefinitely.
  //
  // A receiving port's queue only has elements removed by user code reading
  // messages from the port.
  //
  // Note that this is a priority queue which only exposes messages to consumers
  // in strict sequential order.
  MessageQueue message_queue;

  // In some edge cases, a Node may need to remember to route a single special
  // event upon destruction of this (proxying) Port. That event is stashed here
  // in the interim.
  std::unique_ptr<std::pair<NodeName, ScopedEvent>> send_on_proxy_removal;

  // Arbitrary user data attached to the Port. In practice, Mojo uses this to
  // stash an observer interface which can be notified about various Port state
  // changes.
  scoped_refptr<UserData> user_data;

  // Indicates that this (proxying) Port has received acknowledgement that no
  // new user messages will be routed to it. If |true|, the proxy will be
  // removed once it has received and forwarded all sequenced messages up to and
  // including the one numbered |last_sequence_num_to_receive|.
  bool remove_proxy_on_last_message;

  // Indicates that this Port is aware that its nearest (in terms of forward,
  // non-zero cyclic routing distance) receiving Port has been closed.
  bool peer_closed;

  Port(uint64_t next_sequence_num_to_send,
       uint64_t next_sequence_num_to_receive);

  void AssertLockAcquired() {
#if DCHECK_IS_ON()
    lock_.AssertAcquired();
#endif
  }

 private:
  friend class base::RefCountedThreadSafe<Port>;
  friend class PortLocker;

  ~Port();

  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(Port);
};

}  // namespace ports
}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_PORTS_PORT_H_
