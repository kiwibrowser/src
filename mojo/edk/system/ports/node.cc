// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/ports/node.h"

#include <string.h>

#include <algorithm>
#include <utility>

#include "base/atomicops.h"
#include "base/containers/stack_container.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_local.h"
#include "build/build_config.h"
#include "mojo/edk/system/ports/event.h"
#include "mojo/edk/system/ports/node_delegate.h"
#include "mojo/edk/system/ports/port_locker.h"

#if !defined(OS_NACL)
#include "crypto/random.h"
#else
#include "base/rand_util.h"
#endif

namespace mojo {
namespace edk {
namespace ports {

namespace {

constexpr size_t kRandomNameCacheSize = 256;

// Random port name generator which maintains a cache of random bytes to draw
// from. This amortizes the cost of random name generation on platforms where
// RandBytes may have significant per-call overhead.
//
// Note that the use of this cache means one has to be careful about fork()ing
// a process once any port names have been generated, as that behavior can lead
// to collisions between independently generated names in different processes.
class RandomNameGenerator {
 public:
  RandomNameGenerator() = default;
  ~RandomNameGenerator() = default;

  PortName GenerateRandomPortName() {
    base::AutoLock lock(lock_);
    if (cache_index_ == kRandomNameCacheSize) {
#if defined(OS_NACL)
      base::RandBytes(cache_, sizeof(PortName) * kRandomNameCacheSize);
#else
      crypto::RandBytes(cache_, sizeof(PortName) * kRandomNameCacheSize);
#endif
      cache_index_ = 0;
    }
    return cache_[cache_index_++];
  }

 private:
  base::Lock lock_;
  PortName cache_[kRandomNameCacheSize];
  size_t cache_index_ = kRandomNameCacheSize;

  DISALLOW_COPY_AND_ASSIGN(RandomNameGenerator);
};

base::LazyInstance<RandomNameGenerator>::Leaky g_name_generator =
    LAZY_INSTANCE_INITIALIZER;

int DebugError(const char* message, int error_code) {
  NOTREACHED() << "Oops: " << message;
  return error_code;
}

#define OOPS(x) DebugError(#x, x)

bool CanAcceptMoreMessages(const Port* port) {
  // Have we already doled out the last message (i.e., do we expect to NOT
  // receive further messages)?
  uint64_t next_sequence_num = port->message_queue.next_sequence_num();
  if (port->state == Port::kClosed)
    return false;
  if (port->peer_closed || port->remove_proxy_on_last_message) {
    if (port->last_sequence_num_to_receive == next_sequence_num - 1)
      return false;
  }
  return true;
}

void GenerateRandomPortName(PortName* name) {
  *name = g_name_generator.Get().GenerateRandomPortName();
}

}  // namespace

Node::Node(const NodeName& name, NodeDelegate* delegate)
    : name_(name), delegate_(this, delegate) {}

Node::~Node() {
  if (!ports_.empty())
    DLOG(WARNING) << "Unclean shutdown for node " << name_;
}

bool Node::CanShutdownCleanly(ShutdownPolicy policy) {
  PortLocker::AssertNoPortsLockedOnCurrentThread();
  base::AutoLock ports_lock(ports_lock_);

  if (policy == ShutdownPolicy::DONT_ALLOW_LOCAL_PORTS) {
#if DCHECK_IS_ON()
    for (auto& entry : ports_) {
      DVLOG(2) << "Port " << entry.first << " referencing node "
               << entry.second->peer_node_name << " is blocking shutdown of "
               << "node " << name_ << " (state=" << entry.second->state << ")";
    }
#endif
    return ports_.empty();
  }

  DCHECK_EQ(policy, ShutdownPolicy::ALLOW_LOCAL_PORTS);

  // NOTE: This is not efficient, though it probably doesn't need to be since
  // relatively few ports should be open during shutdown and shutdown doesn't
  // need to be blazingly fast.
  bool can_shutdown = true;
  for (auto& entry : ports_) {
    PortRef port_ref(entry.first, entry.second);
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();
    if (port->peer_node_name != name_ && port->state != Port::kReceiving) {
      can_shutdown = false;
#if DCHECK_IS_ON()
      DVLOG(2) << "Port " << entry.first << " referencing node "
               << port->peer_node_name << " is blocking shutdown of "
               << "node " << name_ << " (state=" << port->state << ")";
#else
      // Exit early when not debugging.
      break;
#endif
    }
  }

  return can_shutdown;
}

int Node::GetPort(const PortName& port_name, PortRef* port_ref) {
  PortLocker::AssertNoPortsLockedOnCurrentThread();
  base::AutoLock lock(ports_lock_);
  auto iter = ports_.find(port_name);
  if (iter == ports_.end())
    return ERROR_PORT_UNKNOWN;

#if defined(OS_ANDROID) && defined(ARCH_CPU_ARM64)
  // Workaround for https://crbug.com/665869.
  base::subtle::MemoryBarrier();
#endif

  *port_ref = PortRef(port_name, iter->second);
  return OK;
}

int Node::CreateUninitializedPort(PortRef* port_ref) {
  PortName port_name;
  GenerateRandomPortName(&port_name);

  scoped_refptr<Port> port(new Port(kInitialSequenceNum, kInitialSequenceNum));
  int rv = AddPortWithName(port_name, port);
  if (rv != OK)
    return rv;

  *port_ref = PortRef(port_name, std::move(port));
  return OK;
}

int Node::InitializePort(const PortRef& port_ref,
                         const NodeName& peer_node_name,
                         const PortName& peer_port_name) {
  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();
    if (port->state != Port::kUninitialized)
      return ERROR_PORT_STATE_UNEXPECTED;

    port->state = Port::kReceiving;
    port->peer_node_name = peer_node_name;
    port->peer_port_name = peer_port_name;
  }

  delegate_->PortStatusChanged(port_ref);

  return OK;
}

int Node::CreatePortPair(PortRef* port0_ref, PortRef* port1_ref) {
  int rv;

  rv = CreateUninitializedPort(port0_ref);
  if (rv != OK)
    return rv;

  rv = CreateUninitializedPort(port1_ref);
  if (rv != OK)
    return rv;

  rv = InitializePort(*port0_ref, name_, port1_ref->name());
  if (rv != OK)
    return rv;

  rv = InitializePort(*port1_ref, name_, port0_ref->name());
  if (rv != OK)
    return rv;

  return OK;
}

int Node::SetUserData(const PortRef& port_ref,
                      scoped_refptr<UserData> user_data) {
  SinglePortLocker locker(&port_ref);
  auto* port = locker.port();
  if (port->state == Port::kClosed)
    return ERROR_PORT_STATE_UNEXPECTED;

  port->user_data = std::move(user_data);

  return OK;
}

int Node::GetUserData(const PortRef& port_ref,
                      scoped_refptr<UserData>* user_data) {
  SinglePortLocker locker(&port_ref);
  auto* port = locker.port();
  if (port->state == Port::kClosed)
    return ERROR_PORT_STATE_UNEXPECTED;

  *user_data = port->user_data;

  return OK;
}

int Node::ClosePort(const PortRef& port_ref) {
  std::vector<std::unique_ptr<UserMessageEvent>> undelivered_messages;
  NodeName peer_node_name;
  PortName peer_port_name;
  uint64_t last_sequence_num = 0;
  bool was_initialized = false;
  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();
    switch (port->state) {
      case Port::kUninitialized:
        break;

      case Port::kReceiving:
        was_initialized = true;
        port->state = Port::kClosed;

        // We pass along the sequence number of the last message sent from this
        // port to allow the peer to have the opportunity to consume all inbound
        // messages before notifying the embedder that this port is closed.
        last_sequence_num = port->next_sequence_num_to_send - 1;

        peer_node_name = port->peer_node_name;
        peer_port_name = port->peer_port_name;

        // If the port being closed still has unread messages, then we need to
        // take care to close those ports so as to avoid leaking memory.
        port->message_queue.TakeAllMessages(&undelivered_messages);
        break;

      default:
        return ERROR_PORT_STATE_UNEXPECTED;
    }
  }

  ErasePort(port_ref.name());

  if (was_initialized) {
    DVLOG(2) << "Sending ObserveClosure from " << port_ref.name() << "@"
             << name_ << " to " << peer_port_name << "@" << peer_node_name;
    delegate_->ForwardEvent(peer_node_name,
                            std::make_unique<ObserveClosureEvent>(
                                peer_port_name, last_sequence_num));
    for (const auto& message : undelivered_messages) {
      for (size_t i = 0; i < message->num_ports(); ++i) {
        PortRef ref;
        if (GetPort(message->ports()[i], &ref) == OK)
          ClosePort(ref);
      }
    }
  }
  return OK;
}

int Node::GetStatus(const PortRef& port_ref, PortStatus* port_status) {
  SinglePortLocker locker(&port_ref);
  auto* port = locker.port();
  if (port->state != Port::kReceiving)
    return ERROR_PORT_STATE_UNEXPECTED;

  port_status->has_messages = port->message_queue.HasNextMessage();
  port_status->receiving_messages = CanAcceptMoreMessages(port);
  port_status->peer_closed = port->peer_closed;
  port_status->peer_remote = port->peer_node_name != name_;
  return OK;
}

int Node::GetMessage(const PortRef& port_ref,
                     std::unique_ptr<UserMessageEvent>* message,
                     MessageFilter* filter) {
  *message = nullptr;

  DVLOG(4) << "GetMessage for " << port_ref.name() << "@" << name_;

  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();

    // This could also be treated like the port being unknown since the
    // embedder should no longer be referring to a port that has been sent.
    if (port->state != Port::kReceiving)
      return ERROR_PORT_STATE_UNEXPECTED;

    // Let the embedder get messages until there are no more before reporting
    // that the peer closed its end.
    if (!CanAcceptMoreMessages(port))
      return ERROR_PORT_PEER_CLOSED;

    port->message_queue.GetNextMessage(message, filter);
  }

  // Allow referenced ports to trigger PortStatusChanged calls.
  if (*message) {
    for (size_t i = 0; i < (*message)->num_ports(); ++i) {
      PortRef new_port_ref;
      int rv = GetPort((*message)->ports()[i], &new_port_ref);

      DCHECK_EQ(OK, rv) << "Port " << new_port_ref.name() << "@" << name_
                        << " does not exist!";

      SinglePortLocker locker(&new_port_ref);
      DCHECK(locker.port()->state == Port::kReceiving);
      locker.port()->message_queue.set_signalable(true);
    }

    // The user may retransmit this message from another port. We reset the
    // sequence number so that the message will get a new one if that happens.
    (*message)->set_sequence_num(0);
  }

  return OK;
}

int Node::SendUserMessage(const PortRef& port_ref,
                          std::unique_ptr<UserMessageEvent> message) {
  int rv = SendUserMessageInternal(port_ref, &message);
  if (rv != OK) {
    // If send failed, close all carried ports. Note that we're careful not to
    // close the sending port itself if it happened to be one of the encoded
    // ports (an invalid but possible condition.)
    for (size_t i = 0; i < message->num_ports(); ++i) {
      if (message->ports()[i] == port_ref.name())
        continue;

      PortRef port;
      if (GetPort(message->ports()[i], &port) == OK)
        ClosePort(port);
    }
  }
  return rv;
}

int Node::AcceptEvent(ScopedEvent event) {
  switch (event->type()) {
    case Event::Type::kUserMessage:
      return OnUserMessage(Event::Cast<UserMessageEvent>(&event));
    case Event::Type::kPortAccepted:
      return OnPortAccepted(Event::Cast<PortAcceptedEvent>(&event));
    case Event::Type::kObserveProxy:
      return OnObserveProxy(Event::Cast<ObserveProxyEvent>(&event));
    case Event::Type::kObserveProxyAck:
      return OnObserveProxyAck(Event::Cast<ObserveProxyAckEvent>(&event));
    case Event::Type::kObserveClosure:
      return OnObserveClosure(Event::Cast<ObserveClosureEvent>(&event));
    case Event::Type::kMergePort:
      return OnMergePort(Event::Cast<MergePortEvent>(&event));
  }
  return OOPS(ERROR_NOT_IMPLEMENTED);
}

int Node::MergePorts(const PortRef& port_ref,
                     const NodeName& destination_node_name,
                     const PortName& destination_port_name) {
  PortName new_port_name;
  Event::PortDescriptor new_port_descriptor;
  {
    SinglePortLocker locker(&port_ref);

    DVLOG(1) << "Sending MergePort from " << port_ref.name() << "@" << name_
             << " to " << destination_port_name << "@" << destination_node_name;

    // Send the port-to-merge over to the destination node so it can be merged
    // into the port cycle atomically there.
    new_port_name = port_ref.name();
    ConvertToProxy(locker.port(), destination_node_name, &new_port_name,
                   &new_port_descriptor);
  }

  if (new_port_descriptor.peer_node_name == name_ &&
      destination_node_name != name_) {
    // Ensure that the locally retained peer of the new proxy gets a status
    // update so it notices that its peer is now remote.
    PortRef local_peer;
    if (GetPort(new_port_descriptor.peer_port_name, &local_peer) == OK)
      delegate_->PortStatusChanged(local_peer);
  }

  delegate_->ForwardEvent(
      destination_node_name,
      std::make_unique<MergePortEvent>(destination_port_name, new_port_name,
                                       new_port_descriptor));
  return OK;
}

int Node::MergeLocalPorts(const PortRef& port0_ref, const PortRef& port1_ref) {
  DVLOG(1) << "Merging local ports " << port0_ref.name() << "@" << name_
           << " and " << port1_ref.name() << "@" << name_;
  return MergePortsInternal(port0_ref, port1_ref,
                            true /* allow_close_on_bad_state */);
}

int Node::LostConnectionToNode(const NodeName& node_name) {
  // We can no longer send events to the given node. We also can't expect any
  // PortAccepted events.

  DVLOG(1) << "Observing lost connection from node " << name_ << " to node "
           << node_name;

  DestroyAllPortsWithPeer(node_name, kInvalidPortName);
  return OK;
}

int Node::OnUserMessage(std::unique_ptr<UserMessageEvent> message) {
  PortName port_name = message->port_name();

#if DCHECK_IS_ON()
  std::ostringstream ports_buf;
  for (size_t i = 0; i < message->num_ports(); ++i) {
    if (i > 0)
      ports_buf << ",";
    ports_buf << message->ports()[i];
  }

  DVLOG(4) << "OnUserMessage " << message->sequence_num()
           << " [ports=" << ports_buf.str() << "] at " << port_name << "@"
           << name_;
#endif

  // Even if this port does not exist, cannot receive anymore messages or is
  // buffering or proxying messages, we still need these ports to be bound to
  // this node. When the message is forwarded, these ports will get transferred
  // following the usual method. If the message cannot be accepted, then the
  // newly bound ports will simply be closed.
  for (size_t i = 0; i < message->num_ports(); ++i) {
    Event::PortDescriptor& descriptor = message->port_descriptors()[i];
    if (descriptor.referring_node_name == kInvalidNodeName) {
      // If the referring node name is invalid, this descriptor can be ignored
      // and the port should already exist locally.
      PortRef port_ref;
      if (GetPort(message->ports()[i], &port_ref) != OK)
        return ERROR_PORT_UNKNOWN;
    } else {
      int rv = AcceptPort(message->ports()[i], descriptor);
      if (rv != OK)
        return rv;

      // Ensure that the referring node is wiped out of this descriptor. This
      // allows the event to be forwarded across multiple local hops without
      // attempting to accept the port more than once.
      descriptor.referring_node_name = kInvalidNodeName;
    }
  }

  PortRef port_ref;
  GetPort(port_name, &port_ref);
  bool has_next_message = false;
  bool message_accepted = false;
  bool should_forward_messages = false;
  if (port_ref.is_valid()) {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();

    // Reject spurious messages if we've already received the last expected
    // message.
    if (CanAcceptMoreMessages(port)) {
      message_accepted = true;
      port->message_queue.AcceptMessage(std::move(message), &has_next_message);

      if (port->state == Port::kBuffering) {
        has_next_message = false;
      } else if (port->state == Port::kProxying) {
        has_next_message = false;
        should_forward_messages = true;
      }
    }
  }

  if (should_forward_messages) {
    int rv = ForwardUserMessagesFromProxy(port_ref);
    if (rv != OK)
      return rv;
    TryRemoveProxy(port_ref);
  }

  if (!message_accepted) {
    DVLOG(2) << "Message not accepted!\n";
    // Close all newly accepted ports as they are effectively orphaned.
    for (size_t i = 0; i < message->num_ports(); ++i) {
      PortRef attached_port_ref;
      if (GetPort(message->ports()[i], &attached_port_ref) == OK) {
        ClosePort(attached_port_ref);
      } else {
        DLOG(WARNING) << "Cannot close non-existent port!\n";
      }
    }
  } else if (has_next_message) {
    delegate_->PortStatusChanged(port_ref);
  }

  return OK;
}

int Node::OnPortAccepted(std::unique_ptr<PortAcceptedEvent> event) {
  PortRef port_ref;
  if (GetPort(event->port_name(), &port_ref) != OK)
    return ERROR_PORT_UNKNOWN;

#if DCHECK_IS_ON()
  {
    SinglePortLocker locker(&port_ref);
    DVLOG(2) << "PortAccepted at " << port_ref.name() << "@" << name_
             << " pointing to " << locker.port()->peer_port_name << "@"
             << locker.port()->peer_node_name;
  }
#endif

  return BeginProxying(port_ref);
}

int Node::OnObserveProxy(std::unique_ptr<ObserveProxyEvent> event) {
  if (event->port_name() == kInvalidPortName) {
    // An ObserveProxy with an invalid target port name is a broadcast used to
    // inform ports when their peer (which was itself a proxy) has become
    // defunct due to unexpected node disconnection.
    //
    // Receiving ports affected by this treat it as equivalent to peer closure.
    // Proxies affected by this can be removed and will in turn broadcast their
    // own death with a similar message.
    DCHECK_EQ(event->proxy_target_node_name(), kInvalidNodeName);
    DCHECK_EQ(event->proxy_target_port_name(), kInvalidPortName);
    DestroyAllPortsWithPeer(event->proxy_node_name(), event->proxy_port_name());
    return OK;
  }

  // The port may have already been closed locally, in which case the
  // ObserveClosure message will contain the last_sequence_num field.
  // We can then silently ignore this message.
  PortRef port_ref;
  if (GetPort(event->port_name(), &port_ref) != OK) {
    DVLOG(1) << "ObserveProxy: " << event->port_name() << "@" << name_
             << " not found";
    return OK;
  }

  DVLOG(2) << "ObserveProxy at " << port_ref.name() << "@" << name_
           << ", proxy at " << event->proxy_port_name() << "@"
           << event->proxy_node_name() << " pointing to "
           << event->proxy_target_port_name() << "@"
           << event->proxy_target_node_name();

  bool update_status = false;
  ScopedEvent event_to_forward;
  NodeName event_target_node;
  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();

    if (port->peer_node_name == event->proxy_node_name() &&
        port->peer_port_name == event->proxy_port_name()) {
      if (port->state == Port::kReceiving) {
        port->peer_node_name = event->proxy_target_node_name();
        port->peer_port_name = event->proxy_target_port_name();
        event_target_node = event->proxy_node_name();
        event_to_forward = std::make_unique<ObserveProxyAckEvent>(
            event->proxy_port_name(), port->next_sequence_num_to_send - 1);
        update_status = true;
        DVLOG(2) << "Forwarding ObserveProxyAck from " << event->port_name()
                 << "@" << name_ << " to " << event->proxy_port_name() << "@"
                 << event_target_node;
      } else {
        // As a proxy ourselves, we don't know how to honor the ObserveProxy
        // event or to populate the last_sequence_num field of ObserveProxyAck.
        // Afterall, another port could be sending messages to our peer now
        // that we've sent out our own ObserveProxy event.  Instead, we will
        // send an ObserveProxyAck indicating that the ObserveProxy event
        // should be re-sent (last_sequence_num set to kInvalidSequenceNum).
        // However, this has to be done after we are removed as a proxy.
        // Otherwise, we might just find ourselves back here again, which
        // would be akin to a busy loop.

        DVLOG(2) << "Delaying ObserveProxyAck to " << event->proxy_port_name()
                 << "@" << event->proxy_node_name();

        port->send_on_proxy_removal.reset(new std::pair<NodeName, ScopedEvent>(
            event->proxy_node_name(),
            std::make_unique<ObserveProxyAckEvent>(event->proxy_port_name(),
                                                   kInvalidSequenceNum)));
      }
    } else {
      // Forward this event along to our peer. Eventually, it should find the
      // port referring to the proxy.
      event_target_node = port->peer_node_name;
      event->set_port_name(port->peer_port_name);
      event_to_forward = std::move(event);
    }
  }

  if (event_to_forward)
    delegate_->ForwardEvent(event_target_node, std::move(event_to_forward));

  if (update_status)
    delegate_->PortStatusChanged(port_ref);

  return OK;
}

int Node::OnObserveProxyAck(std::unique_ptr<ObserveProxyAckEvent> event) {
  DVLOG(2) << "ObserveProxyAck at " << event->port_name() << "@" << name_
           << " (last_sequence_num=" << event->last_sequence_num() << ")";

  PortRef port_ref;
  if (GetPort(event->port_name(), &port_ref) != OK)
    return ERROR_PORT_UNKNOWN;  // The port may have observed closure first.

  bool try_remove_proxy_immediately;
  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();
    if (port->state != Port::kProxying)
      return OOPS(ERROR_PORT_STATE_UNEXPECTED);

    // If the last sequence number is invalid, this is a signal that we need to
    // retransmit the ObserveProxy event for this port rather than flagging the
    // the proxy for removal ASAP.
    try_remove_proxy_immediately =
        event->last_sequence_num() != kInvalidSequenceNum;
    if (try_remove_proxy_immediately) {
      // We can now remove this port once we have received and forwarded the
      // last message addressed to this port.
      port->remove_proxy_on_last_message = true;
      port->last_sequence_num_to_receive = event->last_sequence_num();
    }
  }

  if (try_remove_proxy_immediately)
    TryRemoveProxy(port_ref);
  else
    InitiateProxyRemoval(port_ref);

  return OK;
}

int Node::OnObserveClosure(std::unique_ptr<ObserveClosureEvent> event) {
  // OK if the port doesn't exist, as it may have been closed already.
  PortRef port_ref;
  if (GetPort(event->port_name(), &port_ref) != OK)
    return OK;

  // This message tells the port that it should no longer expect more messages
  // beyond last_sequence_num. This message is forwarded along until we reach
  // the receiving end, and this message serves as an equivalent to
  // ObserveProxyAck.

  bool notify_delegate = false;
  NodeName peer_node_name;
  PortName peer_port_name;
  bool try_remove_proxy = false;
  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();

    port->peer_closed = true;
    port->last_sequence_num_to_receive = event->last_sequence_num();

    DVLOG(2) << "ObserveClosure at " << port_ref.name() << "@" << name_
             << " (state=" << port->state << ") pointing to "
             << port->peer_port_name << "@" << port->peer_node_name
             << " (last_sequence_num=" << event->last_sequence_num() << ")";

    // We always forward ObserveClosure, even beyond the receiving port which
    // cares about it. This ensures that any dead-end proxies beyond that port
    // are notified to remove themselves.

    if (port->state == Port::kReceiving) {
      notify_delegate = true;

      // When forwarding along the other half of the port cycle, this will only
      // reach dead-end proxies. Tell them we've sent our last message so they
      // can go away.
      //
      // TODO: Repurposing ObserveClosure for this has the desired result but
      // may be semantically confusing since the forwarding port is not actually
      // closed. Consider replacing this with a new event type.
      event->set_last_sequence_num(port->next_sequence_num_to_send - 1);
    } else {
      // We haven't yet reached the receiving peer of the closed port, so we'll
      // forward the message along as-is.
      // See about removing the port if it is a proxy as our peer won't be able
      // to participate in proxy removal.
      port->remove_proxy_on_last_message = true;
      if (port->state == Port::kProxying)
        try_remove_proxy = true;
    }

    DVLOG(2) << "Forwarding ObserveClosure from " << port_ref.name() << "@"
             << name_ << " to peer " << port->peer_port_name << "@"
             << port->peer_node_name
             << " (last_sequence_num=" << event->last_sequence_num() << ")";

    peer_node_name = port->peer_node_name;
    peer_port_name = port->peer_port_name;
  }

  if (try_remove_proxy)
    TryRemoveProxy(port_ref);

  event->set_port_name(peer_port_name);
  delegate_->ForwardEvent(peer_node_name, std::move(event));

  if (notify_delegate)
    delegate_->PortStatusChanged(port_ref);

  return OK;
}

int Node::OnMergePort(std::unique_ptr<MergePortEvent> event) {
  PortRef port_ref;
  GetPort(event->port_name(), &port_ref);

  DVLOG(1) << "MergePort at " << port_ref.name() << "@" << name_
           << " merging with proxy " << event->new_port_name() << "@" << name_
           << " pointing to " << event->new_port_descriptor().peer_port_name
           << "@" << event->new_port_descriptor().peer_node_name
           << " referred by "
           << event->new_port_descriptor().referring_port_name << "@"
           << event->new_port_descriptor().referring_node_name;

  // Accept the new port. This is now the receiving end of the other port cycle
  // to be merged with ours. Note that we always attempt to accept the new port
  // first as otherwise its peer receiving port could be left stranded
  // indefinitely.
  if (AcceptPort(event->new_port_name(), event->new_port_descriptor()) != OK) {
    if (port_ref.is_valid())
      ClosePort(port_ref);
    return ERROR_PORT_STATE_UNEXPECTED;
  }

  PortRef new_port_ref;
  GetPort(event->new_port_name(), &new_port_ref);
  if (!port_ref.is_valid() && new_port_ref.is_valid()) {
    ClosePort(new_port_ref);
    return ERROR_PORT_UNKNOWN;
  } else if (port_ref.is_valid() && !new_port_ref.is_valid()) {
    ClosePort(port_ref);
    return ERROR_PORT_UNKNOWN;
  }

  return MergePortsInternal(port_ref, new_port_ref,
                            false /* allow_close_on_bad_state */);
}

int Node::AddPortWithName(const PortName& port_name, scoped_refptr<Port> port) {
  PortLocker::AssertNoPortsLockedOnCurrentThread();
  base::AutoLock lock(ports_lock_);
  if (!ports_.emplace(port_name, std::move(port)).second)
    return OOPS(ERROR_PORT_EXISTS);  // Suggests a bad UUID generator.
  DVLOG(2) << "Created port " << port_name << "@" << name_;
  return OK;
}

void Node::ErasePort(const PortName& port_name) {
  PortLocker::AssertNoPortsLockedOnCurrentThread();
  scoped_refptr<Port> port;
  {
    base::AutoLock lock(ports_lock_);
    auto it = ports_.find(port_name);
    if (it == ports_.end())
      return;
    port = std::move(it->second);
    ports_.erase(it);
  }
  // NOTE: We are careful not to release the port's messages while holding any
  // locks, since they may run arbitrary user code upon destruction.
  std::vector<std::unique_ptr<UserMessageEvent>> messages;
  {
    PortRef port_ref(port_name, std::move(port));
    SinglePortLocker locker(&port_ref);
    locker.port()->message_queue.TakeAllMessages(&messages);
  }
  DVLOG(2) << "Deleted port " << port_name << "@" << name_;
}

int Node::SendUserMessageInternal(const PortRef& port_ref,
                                  std::unique_ptr<UserMessageEvent>* message) {
  std::unique_ptr<UserMessageEvent>& m = *message;
  for (size_t i = 0; i < m->num_ports(); ++i) {
    if (m->ports()[i] == port_ref.name())
      return ERROR_PORT_CANNOT_SEND_SELF;
  }

  NodeName target_node;
  int rv = PrepareToForwardUserMessage(port_ref, Port::kReceiving,
                                       false /* ignore_closed_peer */, m.get(),
                                       &target_node);
  if (rv != OK)
    return rv;

  // Beyond this point there's no sense in returning anything but OK. Even if
  // message forwarding or acceptance fails, there's nothing the embedder can
  // do to recover. Assume that failure beyond this point must be treated as a
  // transport failure.

  DCHECK_NE(kInvalidNodeName, target_node);
  if (target_node != name_) {
    delegate_->ForwardEvent(target_node, std::move(m));
    return OK;
  }

  int accept_result = AcceptEvent(std::move(m));
  if (accept_result != OK) {
    // See comment above for why we don't return an error in this case.
    DVLOG(2) << "AcceptEvent failed: " << accept_result;
  }

  return OK;
}

int Node::MergePortsInternal(const PortRef& port0_ref,
                             const PortRef& port1_ref,
                             bool allow_close_on_bad_state) {
  const PortRef* port_refs[2] = {&port0_ref, &port1_ref};
  {
    base::Optional<PortLocker> locker(base::in_place, port_refs, 2);
    auto* port0 = locker->GetPort(port0_ref);
    auto* port1 = locker->GetPort(port1_ref);

    // There are several conditions which must be met before we'll consider
    // merging two ports:
    //
    // - They must both be in the kReceiving state
    // - They must not be each other's peer
    // - They must have never sent a user message
    //
    // If any of these criteria are not met, we fail early.
    if (port0->state != Port::kReceiving || port1->state != Port::kReceiving ||
        (port0->peer_node_name == name_ &&
         port0->peer_port_name == port1_ref.name()) ||
        (port1->peer_node_name == name_ &&
         port1->peer_port_name == port0_ref.name()) ||
        port0->next_sequence_num_to_send != kInitialSequenceNum ||
        port1->next_sequence_num_to_send != kInitialSequenceNum) {
      // On failure, we only close a port if it was at least properly in the
      // |kReceiving| state. This avoids getting the system in an inconsistent
      // state by e.g. closing a proxy abruptly.
      //
      // Note that we must release the port locks before closing ports.
      const bool close_port0 =
          port0->state == Port::kReceiving || allow_close_on_bad_state;
      const bool close_port1 =
          port1->state == Port::kReceiving || allow_close_on_bad_state;
      locker.reset();
      if (close_port0)
        ClosePort(port0_ref);
      if (close_port1)
        ClosePort(port1_ref);
      return ERROR_PORT_STATE_UNEXPECTED;
    }

    // Swap the ports' peer information and switch them both to proxying mode.
    std::swap(port0->peer_node_name, port1->peer_node_name);
    std::swap(port0->peer_port_name, port1->peer_port_name);
    port0->state = Port::kProxying;
    port1->state = Port::kProxying;
    if (port0->peer_closed)
      port0->remove_proxy_on_last_message = true;
    if (port1->peer_closed)
      port1->remove_proxy_on_last_message = true;
  }

  // Flush any queued messages from the new proxies and, if successful, complete
  // the merge by initiating proxy removals.
  if (ForwardUserMessagesFromProxy(port0_ref) == OK &&
      ForwardUserMessagesFromProxy(port1_ref) == OK) {
    for (size_t i = 0; i < 2; ++i) {
      bool try_remove_proxy_immediately = false;
      ScopedEvent closure_event;
      NodeName closure_event_target_node;
      {
        SinglePortLocker locker(port_refs[i]);
        auto* port = locker.port();
        DCHECK(port->state == Port::kProxying);
        try_remove_proxy_immediately = port->remove_proxy_on_last_message;
        if (try_remove_proxy_immediately || port->peer_closed) {
          // If either end of the port cycle is closed, we propagate an
          // ObserveClosure event.
          closure_event_target_node = port->peer_node_name;
          closure_event = std::make_unique<ObserveClosureEvent>(
              port->peer_port_name, port->last_sequence_num_to_receive);
        }
      }
      if (try_remove_proxy_immediately)
        TryRemoveProxy(*port_refs[i]);
      else
        InitiateProxyRemoval(*port_refs[i]);

      if (closure_event) {
        delegate_->ForwardEvent(closure_event_target_node,
                                std::move(closure_event));
      }
    }

    return OK;
  }

  // If we failed to forward proxied messages, we keep the system in a
  // consistent state by undoing the peer swap and closing the ports.
  {
    PortLocker locker(port_refs, 2);
    auto* port0 = locker.GetPort(port0_ref);
    auto* port1 = locker.GetPort(port1_ref);
    std::swap(port0->peer_node_name, port1->peer_node_name);
    std::swap(port0->peer_port_name, port1->peer_port_name);
    port0->remove_proxy_on_last_message = false;
    port1->remove_proxy_on_last_message = false;
    DCHECK_EQ(Port::kProxying, port0->state);
    DCHECK_EQ(Port::kProxying, port1->state);
    port0->state = Port::kReceiving;
    port1->state = Port::kReceiving;
  }

  ClosePort(port0_ref);
  ClosePort(port1_ref);
  return ERROR_PORT_STATE_UNEXPECTED;
}

void Node::ConvertToProxy(Port* port,
                          const NodeName& to_node_name,
                          PortName* port_name,
                          Event::PortDescriptor* port_descriptor) {
  port->AssertLockAcquired();
  PortName local_port_name = *port_name;

  PortName new_port_name;
  GenerateRandomPortName(&new_port_name);

  // Make sure we don't send messages to the new peer until after we know it
  // exists. In the meantime, just buffer messages locally.
  DCHECK(port->state == Port::kReceiving);
  port->state = Port::kBuffering;

  // If we already know our peer is closed, we already know this proxy can
  // be removed once it receives and forwards its last expected message.
  if (port->peer_closed)
    port->remove_proxy_on_last_message = true;

  *port_name = new_port_name;

  port_descriptor->peer_node_name = port->peer_node_name;
  port_descriptor->peer_port_name = port->peer_port_name;
  port_descriptor->referring_node_name = name_;
  port_descriptor->referring_port_name = local_port_name;
  port_descriptor->next_sequence_num_to_send = port->next_sequence_num_to_send;
  port_descriptor->next_sequence_num_to_receive =
      port->message_queue.next_sequence_num();
  port_descriptor->last_sequence_num_to_receive =
      port->last_sequence_num_to_receive;
  port_descriptor->peer_closed = port->peer_closed;
  memset(port_descriptor->padding, 0, sizeof(port_descriptor->padding));

  // Configure the local port to point to the new port.
  port->peer_node_name = to_node_name;
  port->peer_port_name = new_port_name;
}

int Node::AcceptPort(const PortName& port_name,
                     const Event::PortDescriptor& port_descriptor) {
  scoped_refptr<Port> port =
      base::MakeRefCounted<Port>(port_descriptor.next_sequence_num_to_send,
                                 port_descriptor.next_sequence_num_to_receive);
  port->state = Port::kReceiving;
  port->peer_node_name = port_descriptor.peer_node_name;
  port->peer_port_name = port_descriptor.peer_port_name;
  port->last_sequence_num_to_receive =
      port_descriptor.last_sequence_num_to_receive;
  port->peer_closed = port_descriptor.peer_closed;

  DVLOG(2) << "Accepting port " << port_name
           << " [peer_closed=" << port->peer_closed
           << "; last_sequence_num_to_receive="
           << port->last_sequence_num_to_receive << "]";

  // A newly accepted port is not signalable until the message referencing the
  // new port finds its way to the consumer (see GetMessage).
  port->message_queue.set_signalable(false);

  int rv = AddPortWithName(port_name, std::move(port));
  if (rv != OK)
    return rv;

  // Allow referring port to forward messages.
  delegate_->ForwardEvent(
      port_descriptor.referring_node_name,
      std::make_unique<PortAcceptedEvent>(port_descriptor.referring_port_name));
  return OK;
}

int Node::PrepareToForwardUserMessage(const PortRef& forwarding_port_ref,
                                      Port::State expected_port_state,
                                      bool ignore_closed_peer,
                                      UserMessageEvent* message,
                                      NodeName* forward_to_node) {
  bool target_is_remote = false;
  for (;;) {
    NodeName target_node_name;
    {
      SinglePortLocker locker(&forwarding_port_ref);
      target_node_name = locker.port()->peer_node_name;
    }

    // NOTE: This may call out to arbitrary user code, so it's important to call
    // it only while no port locks are held on the calling thread.
    if (target_node_name != name_) {
      if (!message->NotifyWillBeRoutedExternally()) {
        LOG(ERROR) << "NotifyWillBeRoutedExternally failed unexpectedly.";
        return ERROR_PORT_STATE_UNEXPECTED;
      }
    }

    // Simultaneously lock the forwarding port as well as all attached ports.
    base::StackVector<PortRef, 4> attached_port_refs;
    base::StackVector<const PortRef*, 5> ports_to_lock;
    attached_port_refs.container().resize(message->num_ports());
    ports_to_lock.container().resize(message->num_ports() + 1);
    ports_to_lock[0] = &forwarding_port_ref;
    for (size_t i = 0; i < message->num_ports(); ++i) {
      GetPort(message->ports()[i], &attached_port_refs[i]);
      DCHECK(attached_port_refs[i].is_valid());
      ports_to_lock[i + 1] = &attached_port_refs[i];
    }
    PortLocker locker(ports_to_lock.container().data(),
                      ports_to_lock.container().size());
    auto* forwarding_port = locker.GetPort(forwarding_port_ref);

    if (forwarding_port->peer_node_name != target_node_name) {
      // The target node has already changed since we last held the lock.
      if (target_node_name == name_) {
        // If the target node was previously this local node, we need to restart
        // the loop, since that means we may now route the message externally.
        continue;
      }

      target_node_name = forwarding_port->peer_node_name;
    }
    target_is_remote = target_node_name != name_;

    if (forwarding_port->state != expected_port_state)
      return ERROR_PORT_STATE_UNEXPECTED;
    if (forwarding_port->peer_closed && !ignore_closed_peer)
      return ERROR_PORT_PEER_CLOSED;

    // Messages may already have a sequence number if they're being forwarded by
    // a proxy. Otherwise, use the next outgoing sequence number.
    if (message->sequence_num() == 0)
      message->set_sequence_num(forwarding_port->next_sequence_num_to_send++);
#if DCHECK_IS_ON()
    std::ostringstream ports_buf;
    for (size_t i = 0; i < message->num_ports(); ++i) {
      if (i > 0)
        ports_buf << ",";
      ports_buf << message->ports()[i];
    }
#endif

    if (message->num_ports() > 0) {
      // Sanity check to make sure we can actually send all the attached ports.
      // They must all be in the |kReceiving| state and must not be the sender's
      // own peer.
      DCHECK_EQ(message->num_ports(), attached_port_refs.container().size());
      for (size_t i = 0; i < message->num_ports(); ++i) {
        auto* attached_port = locker.GetPort(attached_port_refs[i]);
        int error = OK;
        if (attached_port->state != Port::kReceiving) {
          error = ERROR_PORT_STATE_UNEXPECTED;
        } else if (attached_port_refs[i].name() ==
                   forwarding_port->peer_port_name) {
          error = ERROR_PORT_CANNOT_SEND_PEER;
        }

        if (error != OK) {
          // Not going to send. Backpedal on the sequence number.
          forwarding_port->next_sequence_num_to_send--;
          return error;
        }
      }

      if (target_is_remote) {
        // We only bother to proxy and rewrite ports in the event if it's
        // going to be routed to an external node. This substantially reduces
        // the amount of port churn in the system, as many port-carrying
        // events are routed at least 1 or 2 intra-node hops before (if ever)
        // being routed externally.
        Event::PortDescriptor* port_descriptors = message->port_descriptors();
        for (size_t i = 0; i < message->num_ports(); ++i) {
          ConvertToProxy(locker.GetPort(attached_port_refs[i]),
                         target_node_name, message->ports() + i,
                         port_descriptors + i);
        }
      }
    }

#if DCHECK_IS_ON()
    DVLOG(4) << "Sending message " << message->sequence_num()
             << " [ports=" << ports_buf.str() << "]"
             << " from " << forwarding_port_ref.name() << "@" << name_ << " to "
             << forwarding_port->peer_port_name << "@" << target_node_name;
#endif

    *forward_to_node = target_node_name;
    message->set_port_name(forwarding_port->peer_port_name);
    break;
  }

  if (target_is_remote) {
    for (size_t i = 0; i < message->num_ports(); ++i) {
      // For any ports that were converted to proxies above, make sure their
      // prior local peer (if applicable) receives a status update so it can be
      // made aware of its peer's location.
      const Event::PortDescriptor& descriptor = message->port_descriptors()[i];
      if (descriptor.peer_node_name == name_) {
        PortRef local_peer;
        if (GetPort(descriptor.peer_port_name, &local_peer) == OK)
          delegate_->PortStatusChanged(local_peer);
      }
    }
  }

  return OK;
}

int Node::BeginProxying(const PortRef& port_ref) {
  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();
    if (port->state != Port::kBuffering)
      return OOPS(ERROR_PORT_STATE_UNEXPECTED);
    port->state = Port::kProxying;
  }

  int rv = ForwardUserMessagesFromProxy(port_ref);
  if (rv != OK)
    return rv;

  bool try_remove_proxy_immediately;
  ScopedEvent closure_event;
  NodeName closure_target_node;
  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();
    if (port->state != Port::kProxying)
      return OOPS(ERROR_PORT_STATE_UNEXPECTED);

    try_remove_proxy_immediately = port->remove_proxy_on_last_message;
    if (try_remove_proxy_immediately) {
      // Make sure we propagate closure to our current peer.
      closure_target_node = port->peer_node_name;
      closure_event = std::make_unique<ObserveClosureEvent>(
          port->peer_port_name, port->last_sequence_num_to_receive);
    }
  }

  if (try_remove_proxy_immediately) {
    TryRemoveProxy(port_ref);
    delegate_->ForwardEvent(closure_target_node, std::move(closure_event));
  } else {
    InitiateProxyRemoval(port_ref);
  }

  return OK;
}

int Node::ForwardUserMessagesFromProxy(const PortRef& port_ref) {
  for (;;) {
    // NOTE: We forward messages in sequential order here so that we maintain
    // the message queue's notion of next sequence number. That's useful for the
    // proxy removal process as we can tell when this port has seen all of the
    // messages it is expected to see.
    std::unique_ptr<UserMessageEvent> message;
    {
      SinglePortLocker locker(&port_ref);
      locker.port()->message_queue.GetNextMessage(&message, nullptr);
      if (!message)
        break;
    }

    NodeName target_node;
    int rv = PrepareToForwardUserMessage(port_ref, Port::kProxying,
                                         true /* ignore_closed_peer */,
                                         message.get(), &target_node);
    if (rv != OK)
      return rv;

    delegate_->ForwardEvent(target_node, std::move(message));
  }
  return OK;
}

void Node::InitiateProxyRemoval(const PortRef& port_ref) {
  NodeName peer_node_name;
  PortName peer_port_name;
  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();
    peer_node_name = port->peer_node_name;
    peer_port_name = port->peer_port_name;
  }

  // To remove this node, we start by notifying the connected graph that we are
  // a proxy. This allows whatever port is referencing this node to skip it.
  // Eventually, this node will receive ObserveProxyAck (or ObserveClosure if
  // the peer was closed in the meantime).
  delegate_->ForwardEvent(peer_node_name,
                          std::make_unique<ObserveProxyEvent>(
                              peer_port_name, name_, port_ref.name(),
                              peer_node_name, peer_port_name));
}

void Node::TryRemoveProxy(const PortRef& port_ref) {
  bool should_erase = false;
  NodeName removal_target_node;
  ScopedEvent removal_event;

  {
    SinglePortLocker locker(&port_ref);
    auto* port = locker.port();
    DCHECK(port->state == Port::kProxying);

    // Make sure we have seen ObserveProxyAck before removing the port.
    if (!port->remove_proxy_on_last_message)
      return;

    if (!CanAcceptMoreMessages(port)) {
      should_erase = true;
      if (port->send_on_proxy_removal) {
        removal_target_node = port->send_on_proxy_removal->first;
        removal_event = std::move(port->send_on_proxy_removal->second);
      }
    } else {
      DVLOG(2) << "Cannot remove port " << port_ref.name() << "@" << name_
               << " now; waiting for more messages";
    }
  }

  if (should_erase)
    ErasePort(port_ref.name());

  if (removal_event)
    delegate_->ForwardEvent(removal_target_node, std::move(removal_event));
}

void Node::DestroyAllPortsWithPeer(const NodeName& node_name,
                                   const PortName& port_name) {
  // Wipes out all ports whose peer node matches |node_name| and whose peer port
  // matches |port_name|. If |port_name| is |kInvalidPortName|, only the peer
  // node is matched.

  std::vector<PortRef> ports_to_notify;
  std::vector<PortName> dead_proxies_to_broadcast;
  std::vector<std::unique_ptr<UserMessageEvent>> undelivered_messages;

  {
    PortLocker::AssertNoPortsLockedOnCurrentThread();
    base::AutoLock ports_lock(ports_lock_);

    for (auto iter = ports_.begin(); iter != ports_.end(); ++iter) {
      PortRef port_ref(iter->first, iter->second);
      {
        SinglePortLocker locker(&port_ref);
        auto* port = locker.port();

        if (port->peer_node_name == node_name &&
            (port_name == kInvalidPortName ||
             port->peer_port_name == port_name)) {
          if (!port->peer_closed) {
            // Treat this as immediate peer closure. It's an exceptional
            // condition akin to a broken pipe, so we don't care about losing
            // messages.

            port->peer_closed = true;
            port->last_sequence_num_to_receive =
                port->message_queue.next_sequence_num() - 1;

            if (port->state == Port::kReceiving)
              ports_to_notify.push_back(PortRef(iter->first, port));
          }

          // We don't expect to forward any further messages, and we don't
          // expect to receive a Port{Accepted,Rejected} event. Because we're
          // a proxy with no active peer, we cannot use the normal proxy removal
          // procedure of forward-propagating an ObserveProxy. Instead we
          // broadcast our own death so it can be back-propagated. This is
          // inefficient but rare.
          if (port->state != Port::kReceiving) {
            dead_proxies_to_broadcast.push_back(iter->first);
            std::vector<std::unique_ptr<UserMessageEvent>> messages;
            iter->second->message_queue.TakeAllMessages(&messages);
            for (auto& message : messages)
              undelivered_messages.emplace_back(std::move(message));
          }
        }
      }
    }
  }

  for (const auto& proxy_name : dead_proxies_to_broadcast) {
    ErasePort(proxy_name);
    DVLOG(2) << "Forcibly deleted port " << proxy_name << "@" << name_;
  }

  // Wake up any receiving ports who have just observed simulated peer closure.
  for (const auto& port : ports_to_notify)
    delegate_->PortStatusChanged(port);

  for (const auto& proxy_name : dead_proxies_to_broadcast) {
    // Broadcast an event signifying that this proxy is no longer functioning.
    delegate_->BroadcastEvent(std::make_unique<ObserveProxyEvent>(
        kInvalidPortName, name_, proxy_name, kInvalidNodeName,
        kInvalidPortName));

    // Also process death locally since the port that points this closed one
    // could be on the current node.
    // Note: Although this is recursive, only a single port is involved which
    // limits the expected branching to 1.
    DestroyAllPortsWithPeer(name_, proxy_name);
  }

  // Close any ports referenced by undelivered messages.
  for (const auto& message : undelivered_messages) {
    for (size_t i = 0; i < message->num_ports(); ++i) {
      PortRef ref;
      if (GetPort(message->ports()[i], &ref) == OK)
        ClosePort(ref);
    }
  }
}

Node::DelegateHolder::DelegateHolder(Node* node, NodeDelegate* delegate)
    : node_(node), delegate_(delegate) {
  DCHECK(node_);
}

Node::DelegateHolder::~DelegateHolder() {}

#if DCHECK_IS_ON()
void Node::DelegateHolder::EnsureSafeDelegateAccess() const {
  PortLocker::AssertNoPortsLockedOnCurrentThread();
  base::AutoLock lock(node_->ports_lock_);
}
#endif

}  // namespace ports
}  // namespace edk
}  // namespace mojo
