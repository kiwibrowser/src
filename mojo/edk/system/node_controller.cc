// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/node_controller.h"

#include <algorithm>
#include <limits>
#include <vector>

#include "base/bind.h"
#include "base/containers/queue.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process_handle.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "mojo/edk/embedder/named_platform_channel_pair.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/system/broker.h"
#include "mojo/edk/system/broker_host.h"
#include "mojo/edk/system/configuration.h"
#include "mojo/edk/system/core.h"
#include "mojo/edk/system/request_context.h"
#include "mojo/edk/system/user_message_impl.h"

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include "mojo/edk/system/mach_port_relay.h"
#endif

#if !defined(OS_NACL)
#include "crypto/random.h"
#endif

namespace mojo {
namespace edk {

namespace {

#if defined(OS_NACL)
template <typename T>
void GenerateRandomName(T* out) { base::RandBytes(out, sizeof(T)); }
#else
template <typename T>
void GenerateRandomName(T* out) { crypto::RandBytes(out, sizeof(T)); }
#endif

ports::NodeName GetRandomNodeName() {
  ports::NodeName name;
  GenerateRandomName(&name);
  return name;
}

Channel::MessagePtr SerializeEventMessage(ports::ScopedEvent event) {
  if (event->type() == ports::Event::Type::kUserMessage) {
    // User message events must already be partially serialized.
    return UserMessageImpl::FinalizeEventMessage(
        ports::Event::Cast<ports::UserMessageEvent>(&event));
  }

  void* data;
  size_t size = event->GetSerializedSize();
  auto message = NodeChannel::CreateEventMessage(size, size, &data, 0);
  event->Serialize(data);
  return message;
}

ports::ScopedEvent DeserializeEventMessage(
    const ports::NodeName& from_node,
    Channel::MessagePtr channel_message) {
  void* data;
  size_t size;
  NodeChannel::GetEventMessageData(channel_message.get(), &data, &size);
  auto event = ports::Event::Deserialize(data, size);
  if (!event)
    return nullptr;

  if (event->type() != ports::Event::Type::kUserMessage)
    return event;

  // User messages require extra parsing.
  const size_t event_size = event->GetSerializedSize();

  // Note that if this weren't true, the event couldn't have been deserialized
  // in the first place.
  DCHECK_LE(event_size, size);

  auto message_event = ports::Event::Cast<ports::UserMessageEvent>(&event);
  auto message = UserMessageImpl::CreateFromChannelMessage(
      message_event.get(), std::move(channel_message),
      static_cast<uint8_t*>(data) + event_size, size - event_size);
  message->set_source_node(from_node);

  message_event->AttachMessage(std::move(message));
  return std::move(message_event);
}

// Used by NodeController to watch for shutdown. Since no IO can happen once
// the IO thread is killed, the NodeController can cleanly drop all its peers
// at that time.
class ThreadDestructionObserver
    : public base::MessageLoopCurrent::DestructionObserver {
 public:
  static void Create(scoped_refptr<base::TaskRunner> task_runner,
                     const base::Closure& callback) {
    if (task_runner->RunsTasksInCurrentSequence()) {
      // Owns itself.
      new ThreadDestructionObserver(callback);
    } else {
      task_runner->PostTask(FROM_HERE,
                            base::Bind(&Create, task_runner, callback));
    }
  }

 private:
  explicit ThreadDestructionObserver(const base::Closure& callback)
      : callback_(callback) {
    base::MessageLoopCurrent::Get()->AddDestructionObserver(this);
  }

  ~ThreadDestructionObserver() override {
    base::MessageLoopCurrent::Get()->RemoveDestructionObserver(this);
  }

  // base::MessageLoopCurrent::DestructionObserver:
  void WillDestroyCurrentMessageLoop() override {
    callback_.Run();
    delete this;
  }

  const base::Closure callback_;

  DISALLOW_COPY_AND_ASSIGN(ThreadDestructionObserver);
};

}  // namespace

NodeController::~NodeController() {}

NodeController::NodeController(Core* core)
    : core_(core),
      name_(GetRandomNodeName()),
      node_(new ports::Node(name_, this)) {
  DVLOG(1) << "Initializing node " << name_;
}

#if defined(OS_MACOSX) && !defined(OS_IOS)
void NodeController::CreateMachPortRelay(
    base::PortProvider* port_provider) {
  base::AutoLock lock(mach_port_relay_lock_);
  DCHECK(!mach_port_relay_);
  mach_port_relay_.reset(new MachPortRelay(port_provider));
}
#endif

void NodeController::SetIOTaskRunner(
    scoped_refptr<base::TaskRunner> task_runner) {
  io_task_runner_ = task_runner;
  ThreadDestructionObserver::Create(
      io_task_runner_,
      base::Bind(&NodeController::DropAllPeers, base::Unretained(this)));
}

void NodeController::SendBrokerClientInvitation(
    base::ProcessHandle target_process,
    ConnectionParams connection_params,
    const std::vector<std::pair<std::string, ports::PortRef>>& attached_ports,
    const ProcessErrorCallback& process_error_callback) {
  // Generate the temporary remote node name here so that it can be associated
  // with the ports "attached" to this invitation.
  ports::NodeName temporary_node_name;
  GenerateRandomName(&temporary_node_name);

  {
    base::AutoLock lock(reserved_ports_lock_);
    PortMap& port_map = reserved_ports_[temporary_node_name];
    for (auto& entry : attached_ports) {
      auto result = port_map.emplace(entry.first, entry.second);
      DCHECK(result.second) << "Duplicate attachment: " << entry.first;
    }
  }

  ScopedProcessHandle scoped_target_process =
      ScopedProcessHandle::CloneFrom(target_process);
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&NodeController::SendBrokerClientInvitationOnIOThread,
                     base::Unretained(this), std::move(scoped_target_process),
                     std::move(connection_params), temporary_node_name,
                     process_error_callback));
}

void NodeController::AcceptBrokerClientInvitation(
    ConnectionParams connection_params) {
  DCHECK(!GetConfiguration().is_broker_process);
#if !defined(OS_MACOSX) && !defined(OS_NACL_SFI) && !defined(OS_FUCHSIA)
  // Use the bootstrap channel for the broker and receive the node's channel
  // synchronously as the first message from the broker.
  base::ElapsedTimer timer;
  broker_.reset(new Broker(connection_params.TakeChannelHandle()));
  ScopedInternalPlatformHandle platform_handle =
      broker_->GetInviterInternalPlatformHandle();

  if (!platform_handle.is_valid()) {
    // Most likely the inviter's side of the channel has already been closed and
    // the broker was unable to negotiate a NodeChannel pipe. In this case we
    // can cancel our connection to our inviter.
    DVLOG(1) << "Cannot connect to invalid inviter channel.";
    CancelPendingPortMerges();
    return;
  }
  connection_params = ConnectionParams(connection_params.protocol(),
                                       std::move(platform_handle));
#endif

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&NodeController::AcceptBrokerClientInvitationOnIOThread,
                     base::Unretained(this), std::move(connection_params)));
}

uint64_t NodeController::ConnectToPeer(ConnectionParams connection_params,
                                       const ports::PortRef& port) {
  uint64_t id = 0;
  {
    base::AutoLock lock(peers_lock_);
    id = next_peer_connection_id_++;
  }
  io_task_runner_->PostTask(FROM_HERE,
                            base::Bind(&NodeController::ConnectToPeerOnIOThread,
                                       base::Unretained(this), id,
                                       base::Passed(&connection_params), port));
  return id;
}

void NodeController::ClosePeerConnection(uint64_t peer_connection_id) {
  io_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NodeController::ClosePeerConnectionOnIOThread,
                            base::Unretained(this), peer_connection_id));
}

void NodeController::SetPortObserver(const ports::PortRef& port,
                                     scoped_refptr<PortObserver> observer) {
  node_->SetUserData(port, std::move(observer));
}

void NodeController::ClosePort(const ports::PortRef& port) {
  SetPortObserver(port, nullptr);
  int rv = node_->ClosePort(port);
  DCHECK_EQ(rv, ports::OK) << " Failed to close port: " << port.name();
}

int NodeController::SendUserMessage(
    const ports::PortRef& port,
    std::unique_ptr<ports::UserMessageEvent> message) {
  return node_->SendUserMessage(port, std::move(message));
}

void NodeController::MergePortIntoInviter(const std::string& name,
                                          const ports::PortRef& port) {
  scoped_refptr<NodeChannel> inviter;
  bool reject_merge = false;
  {
    // Hold |pending_port_merges_lock_| while getting |inviter|. Otherwise,
    // there is a race where the inviter can be set, and |pending_port_merges_|
    // be processed between retrieving |inviter| and adding the merge to
    // |pending_port_merges_|.
    base::AutoLock lock(pending_port_merges_lock_);
    inviter = GetInviterChannel();
    if (reject_pending_merges_) {
      reject_merge = true;
    } else if (!inviter) {
      pending_port_merges_.push_back(std::make_pair(name, port));
      return;
    }
  }
  if (reject_merge) {
    node_->ClosePort(port);
    DVLOG(2) << "Rejecting port merge for name " << name
             << " due to closed inviter channel.";
    return;
  }

  inviter->RequestPortMerge(port.name(), name);
}

int NodeController::MergeLocalPorts(const ports::PortRef& port0,
                                    const ports::PortRef& port1) {
  return node_->MergeLocalPorts(port0, port1);
}

base::WritableSharedMemoryRegion NodeController::CreateSharedBuffer(
    size_t num_bytes) {
#if !defined(OS_MACOSX) && !defined(OS_NACL_SFI) && !defined(OS_FUCHSIA)
  // Shared buffer creation failure is fatal, so always use the broker when we
  // have one; unless of course the embedder forces us not to.
  if (!GetConfiguration().force_direct_shared_memory_allocation && broker_)
    return broker_->GetWritableSharedMemoryRegion(num_bytes);
#endif
  return base::WritableSharedMemoryRegion::Create(num_bytes);
}

void NodeController::RequestShutdown(const base::Closure& callback) {
  {
    base::AutoLock lock(shutdown_lock_);
    shutdown_callback_ = callback;
    shutdown_callback_flag_.Set(true);
  }

  AttemptShutdownIfRequested();
}

void NodeController::NotifyBadMessageFrom(const ports::NodeName& source_node,
                                          const std::string& error) {
  scoped_refptr<NodeChannel> peer = GetPeerChannel(source_node);
  if (peer)
    peer->NotifyBadMessage(error);
}

void NodeController::SendBrokerClientInvitationOnIOThread(
    ScopedProcessHandle target_process,
    ConnectionParams connection_params,
    ports::NodeName temporary_node_name,
    const ProcessErrorCallback& process_error_callback) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

#if !defined(OS_MACOSX) && !defined(OS_NACL) && !defined(OS_FUCHSIA)
  PlatformChannelPair node_channel;
  ScopedInternalPlatformHandle server_handle = node_channel.PassServerHandle();
  // BrokerHost owns itself.
  BrokerHost* broker_host = new BrokerHost(
      target_process.get(), connection_params.TakeChannelHandle(),
      process_error_callback);
  bool channel_ok = broker_host->SendChannel(node_channel.PassClientHandle());

#if defined(OS_WIN)
  if (!channel_ok) {
    // On Windows the above operation may fail if the channel is crossing a
    // session boundary. In that case we fall back to a named pipe.
    NamedPlatformChannelPair named_channel;
    server_handle = named_channel.PassServerHandle();
    broker_host->SendNamedChannel(named_channel.handle().name);
  }
#else
  CHECK(channel_ok);
#endif  // defined(OS_WIN)

  scoped_refptr<NodeChannel> channel = NodeChannel::Create(
      this,
      ConnectionParams(connection_params.protocol(), std::move(server_handle)),
      io_task_runner_, process_error_callback);

#else  // !defined(OS_MACOSX) && !defined(OS_NACL)
  scoped_refptr<NodeChannel> channel =
      NodeChannel::Create(this, std::move(connection_params), io_task_runner_,
                          process_error_callback);
#endif  // !defined(OS_MACOSX) && !defined(OS_NACL)

  // We set up the invitee channel with a temporary name so it can be identified
  // as a pending invitee if it writes any messages to the channel. We may start
  // receiving messages from it (though we shouldn't) as soon as Start() is
  // called below.

  pending_invitations_.insert(std::make_pair(temporary_node_name, channel));

  channel->SetRemoteNodeName(temporary_node_name);
  channel->SetRemoteProcessHandle(std::move(target_process));
  channel->Start();

  channel->AcceptInvitee(name_, temporary_node_name);
}

void NodeController::AcceptBrokerClientInvitationOnIOThread(
    ConnectionParams connection_params) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  {
    base::AutoLock lock(inviter_lock_);
    DCHECK(inviter_name_ == ports::kInvalidNodeName);

    // At this point we don't know the inviter's name, so we can't yet insert it
    // into our |peers_| map. That will happen as soon as we receive an
    // AcceptInvitee message from them.
    bootstrap_inviter_channel_ =
        NodeChannel::Create(this, std::move(connection_params), io_task_runner_,
                            ProcessErrorCallback());
    // Prevent the inviter pipe handle from being closed on shutdown. Pipe
    // closure may be used by the inviter to detect the invitee process has
    // exited.
    bootstrap_inviter_channel_->LeakHandleOnShutdown();
  }
  bootstrap_inviter_channel_->Start();
}

void NodeController::ConnectToPeerOnIOThread(uint64_t peer_connection_id,
                                             ConnectionParams connection_params,
                                             ports::PortRef port) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  scoped_refptr<NodeChannel> channel = NodeChannel::Create(
      this, std::move(connection_params), io_task_runner_, {});

  ports::NodeName token;
  GenerateRandomName(&token);
  peer_connections_.emplace(token,
                            PeerConnection{channel, port, peer_connection_id});
  peer_connections_by_id_.emplace(peer_connection_id, token);

  channel->SetRemoteNodeName(token);
  channel->Start();

  channel->AcceptPeer(name_, token, port.name());
}

void NodeController::ClosePeerConnectionOnIOThread(
    uint64_t peer_connection_id) {
  RequestContext request_context(RequestContext::Source::SYSTEM);
  auto peer = peer_connections_by_id_.find(peer_connection_id);
  // The connection may already be closed.
  if (peer == peer_connections_by_id_.end())
    return;

  // |peer| may be removed so make a copy of |name|.
  ports::NodeName name = peer->second;
  DropPeer(name, nullptr);
}

scoped_refptr<NodeChannel> NodeController::GetPeerChannel(
    const ports::NodeName& name) {
  base::AutoLock lock(peers_lock_);
  auto it = peers_.find(name);
  if (it == peers_.end())
    return nullptr;
  return it->second;
}

scoped_refptr<NodeChannel> NodeController::GetInviterChannel() {
  ports::NodeName inviter_name;
  {
    base::AutoLock lock(inviter_lock_);
    inviter_name = inviter_name_;
  }
  return GetPeerChannel(inviter_name);
}

scoped_refptr<NodeChannel> NodeController::GetBrokerChannel() {
  if (GetConfiguration().is_broker_process)
    return nullptr;

  ports::NodeName broker_name;
  {
    base::AutoLock lock(broker_lock_);
    broker_name = broker_name_;
  }
  return GetPeerChannel(broker_name);
}

void NodeController::AddPeer(const ports::NodeName& name,
                             scoped_refptr<NodeChannel> channel,
                             bool start_channel) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  DCHECK(name != ports::kInvalidNodeName);
  DCHECK(channel);

  channel->SetRemoteNodeName(name);

  OutgoingMessageQueue pending_messages;
  {
    base::AutoLock lock(peers_lock_);
    if (peers_.find(name) != peers_.end()) {
      // This can happen normally if two nodes race to be introduced to each
      // other. The losing pipe will be silently closed and introduction should
      // not be affected.
      DVLOG(1) << "Ignoring duplicate peer name " << name;
      return;
    }

    auto result = peers_.insert(std::make_pair(name, channel));
    DCHECK(result.second);

    DVLOG(2) << "Accepting new peer " << name << " on node " << name_;

    auto it = pending_peer_messages_.find(name);
    if (it != pending_peer_messages_.end()) {
      std::swap(pending_messages, it->second);
      pending_peer_messages_.erase(it);
    }
  }

  if (start_channel)
    channel->Start();

  // Flush any queued message we need to deliver to this node.
  while (!pending_messages.empty()) {
    channel->SendChannelMessage(std::move(pending_messages.front()));
    pending_messages.pop();
  }
}

void NodeController::DropPeer(const ports::NodeName& name,
                              NodeChannel* channel) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  {
    base::AutoLock lock(peers_lock_);
    auto it = peers_.find(name);

    if (it != peers_.end()) {
      ports::NodeName peer = it->first;
      peers_.erase(it);
      DVLOG(1) << "Dropped peer " << peer;
    }

    pending_peer_messages_.erase(name);
    pending_invitations_.erase(name);
  }

  std::vector<ports::PortRef> ports_to_close;
  {
    // Clean up any reserved ports.
    base::AutoLock lock(reserved_ports_lock_);
    auto it = reserved_ports_.find(name);
    if (it != reserved_ports_.end()) {
      for (auto& entry : it->second)
        ports_to_close.emplace_back(entry.second);
      reserved_ports_.erase(it);
    }
  }

  bool is_inviter;
  {
    base::AutoLock lock(inviter_lock_);
    is_inviter =
        (name == inviter_name_ || channel == bootstrap_inviter_channel_);
  }

  // If the error comes from the inviter channel, we also need to cancel any
  // port merge requests, so that errors can be propagated to the message
  // pipes.
  if (is_inviter)
    CancelPendingPortMerges();

  auto peer = peer_connections_.find(name);
  if (peer != peer_connections_.end()) {
    peer_connections_by_id_.erase(peer->second.connection_id);
    ports_to_close.push_back(peer->second.local_port);
    peer_connections_.erase(peer);
  }

  for (const auto& port : ports_to_close)
    node_->ClosePort(port);

  node_->LostConnectionToNode(name);
  AttemptShutdownIfRequested();
}

void NodeController::SendPeerEvent(const ports::NodeName& name,
                                   ports::ScopedEvent event) {
  Channel::MessagePtr event_message = SerializeEventMessage(std::move(event));
  if (!event_message)
    return;
  scoped_refptr<NodeChannel> peer = GetPeerChannel(name);
#if defined(OS_WIN)
  if (event_message->has_handles()) {
    // If we're sending a message with handles we aren't the destination
    // node's inviter or broker (i.e. we don't know its process handle), ask
    // the broker to relay for us.
    scoped_refptr<NodeChannel> broker = GetBrokerChannel();
    if (!peer || !peer->HasRemoteProcessHandle()) {
      if (!GetConfiguration().is_broker_process && broker) {
        broker->RelayEventMessage(name, std::move(event_message));
      } else {
        base::AutoLock lock(broker_lock_);
        pending_relay_messages_[name].emplace(std::move(event_message));
      }
      return;
    }
  }
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  if (event_message->has_mach_ports()) {
    // Messages containing Mach ports are always routed through the broker, even
    // if the broker process is the intended recipient.
    bool use_broker = false;
    if (!GetConfiguration().is_broker_process) {
      base::AutoLock lock(inviter_lock_);
      use_broker = (bootstrap_inviter_channel_ ||
                    inviter_name_ != ports::kInvalidNodeName);
    }

    if (use_broker) {
      scoped_refptr<NodeChannel> broker = GetBrokerChannel();
      if (broker) {
        broker->RelayEventMessage(name, std::move(event_message));
      } else {
        base::AutoLock lock(broker_lock_);
        pending_relay_messages_[name].emplace(std::move(event_message));
      }
      return;
    }
  }
#endif  // defined(OS_WIN)

  if (peer) {
    peer->SendChannelMessage(std::move(event_message));
    return;
  }

  // If we don't know who the peer is and we are the broker, we can only assume
  // the peer is invalid, i.e., it's either a junk name or has already been
  // disconnected.
  scoped_refptr<NodeChannel> broker = GetBrokerChannel();
  if (!broker) {
    DVLOG(1) << "Dropping message for unknown peer: " << name;
    return;
  }

  // If we aren't the broker, assume we just need to be introduced and queue
  // until that can be either confirmed or denied by the broker.
  bool needs_introduction = false;
  {
    base::AutoLock lock(peers_lock_);
    // We may have been introduced on another thread by the time we get here.
    // Double-check to be safe.
    auto it = peers_.find(name);
    if (it == peers_.end()) {
      auto& queue = pending_peer_messages_[name];
      needs_introduction = queue.empty();
      queue.emplace(std::move(event_message));
    } else {
      peer = it->second;
    }
  }
  if (needs_introduction)
    broker->RequestIntroduction(name);
  else if (peer)
    peer->SendChannelMessage(std::move(event_message));
}

void NodeController::DropAllPeers() {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  std::vector<scoped_refptr<NodeChannel>> all_peers;
  {
    base::AutoLock lock(inviter_lock_);
    if (bootstrap_inviter_channel_) {
      // |bootstrap_inviter_channel_| isn't null'd here becuase we rely on its
      // existence to determine whether or not this is the root node. Once
      // bootstrap_inviter_channel_->ShutDown() has been called,
      // |bootstrap_inviter_channel_| is essentially a dead object and it
      // doesn't matter if it's deleted now or when |this| is deleted. Note:
      // |bootstrap_inviter_channel_| is only modified on the IO thread.
      all_peers.push_back(bootstrap_inviter_channel_);
    }
  }

  {
    base::AutoLock lock(peers_lock_);
    for (const auto& peer : peers_)
      all_peers.push_back(peer.second);
    for (const auto& peer : pending_invitations_)
      all_peers.push_back(peer.second);
    peers_.clear();
    pending_invitations_.clear();
    pending_peer_messages_.clear();
    peer_connections_.clear();
  }

  for (const auto& peer : all_peers)
    peer->ShutDown();

  if (destroy_on_io_thread_shutdown_)
    delete this;
}

void NodeController::ForwardEvent(const ports::NodeName& node,
                                  ports::ScopedEvent event) {
  DCHECK(event);
  if (node == name_)
    node_->AcceptEvent(std::move(event));
  else
    SendPeerEvent(node, std::move(event));

  AttemptShutdownIfRequested();
}

void NodeController::BroadcastEvent(ports::ScopedEvent event) {
  Channel::MessagePtr channel_message = SerializeEventMessage(std::move(event));
  DCHECK(channel_message && !channel_message->has_handles());

  scoped_refptr<NodeChannel> broker = GetBrokerChannel();
  if (broker)
    broker->Broadcast(std::move(channel_message));
  else
    OnBroadcast(name_, std::move(channel_message));
}

void NodeController::PortStatusChanged(const ports::PortRef& port) {
  scoped_refptr<ports::UserData> user_data;
  node_->GetUserData(port, &user_data);

  PortObserver* observer = static_cast<PortObserver*>(user_data.get());
  if (observer) {
    observer->OnPortStatusChanged();
  } else {
    DVLOG(2) << "Ignoring status change for " << port.name() << " because it "
             << "doesn't have an observer.";
  }
}

void NodeController::OnAcceptInvitee(const ports::NodeName& from_node,
                                     const ports::NodeName& inviter_name,
                                     const ports::NodeName& token) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  scoped_refptr<NodeChannel> inviter;
  {
    base::AutoLock lock(inviter_lock_);
    if (bootstrap_inviter_channel_ &&
        inviter_name_ == ports::kInvalidNodeName) {
      inviter_name_ = inviter_name;
      inviter = bootstrap_inviter_channel_;
    }
  }

  if (!inviter) {
    DLOG(ERROR) << "Unexpected AcceptInvitee message from " << from_node;
    DropPeer(from_node, nullptr);
    return;
  }

  inviter->SetRemoteNodeName(inviter_name);
  inviter->AcceptInvitation(token, name_);

  // NOTE: The invitee does not actually add its inviter as a peer until
  // receiving an AcceptBrokerClient message from the broker. The inviter will
  // request that said message be sent upon receiving AcceptInvitation.

  DVLOG(1) << "Broker client " << name_ << " accepting invitation from "
           << inviter_name;
}

void NodeController::OnAcceptInvitation(const ports::NodeName& from_node,
                                        const ports::NodeName& token,
                                        const ports::NodeName& invitee_name) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  auto it = pending_invitations_.find(from_node);
  if (it == pending_invitations_.end() || token != from_node) {
    DLOG(ERROR) << "Received unexpected AcceptInvitation message from "
                << from_node;
    DropPeer(from_node, nullptr);
    return;
  }

  {
    base::AutoLock lock(reserved_ports_lock_);
    auto it = reserved_ports_.find(from_node);
    if (it != reserved_ports_.end()) {
      // Swap the temporary node name's reserved ports into an entry keyed by
      // the real node name.
      auto result =
          reserved_ports_.emplace(invitee_name, std::move(it->second));
      DCHECK(result.second);
      reserved_ports_.erase(it);
    }
  }

  scoped_refptr<NodeChannel> channel = it->second;
  pending_invitations_.erase(it);

  DCHECK(channel);

  DVLOG(1) << "Node " << name_ << " accepted invitee " << invitee_name;

  AddPeer(invitee_name, channel, false /* start_channel */);

  // TODO(rockot): We could simplify invitee initialization if we could
  // synchronously get a new async broker channel from the broker. For now we do
  // it asynchronously since it's only used to facilitate handle passing, not
  // handle creation.
  scoped_refptr<NodeChannel> broker = GetBrokerChannel();
  if (broker) {
    // Inform the broker of this new client.
    broker->AddBrokerClient(invitee_name, channel->CloneRemoteProcessHandle());
  } else {
    // If we have no broker, either we need to wait for one, or we *are* the
    // broker.
    scoped_refptr<NodeChannel> inviter = GetInviterChannel();
    if (!inviter) {
      base::AutoLock lock(inviter_lock_);
      inviter = bootstrap_inviter_channel_;
    }

    if (!inviter) {
      // Yes, we're the broker. We can initialize the client directly.
      channel->AcceptBrokerClient(name_, ScopedInternalPlatformHandle());
    } else {
      // We aren't the broker, so wait for a broker connection.
      base::AutoLock lock(broker_lock_);
      pending_broker_clients_.push(invitee_name);
    }
  }
}

void NodeController::OnAddBrokerClient(const ports::NodeName& from_node,
                                       const ports::NodeName& client_name,
                                       base::ProcessHandle process_handle) {
  ScopedProcessHandle scoped_process_handle(process_handle);

  scoped_refptr<NodeChannel> sender = GetPeerChannel(from_node);
  if (!sender) {
    DLOG(ERROR) << "Ignoring AddBrokerClient from unknown sender.";
    return;
  }

  if (GetPeerChannel(client_name)) {
    DLOG(ERROR) << "Ignoring AddBrokerClient for known client.";
    DropPeer(from_node, nullptr);
    return;
  }

  PlatformChannelPair broker_channel;
  ConnectionParams connection_params(TransportProtocol::kLegacy,
                                     broker_channel.PassServerHandle());
  scoped_refptr<NodeChannel> client =
      NodeChannel::Create(this, std::move(connection_params), io_task_runner_,
                          ProcessErrorCallback());

#if defined(OS_WIN)
  // The broker must have a working handle to the client process in order to
  // properly copy other handles to and from the client.
  if (!scoped_process_handle.is_valid()) {
    DLOG(ERROR) << "Broker rejecting client with invalid process handle.";
    return;
  }
#endif
  client->SetRemoteProcessHandle(std::move(scoped_process_handle));

  AddPeer(client_name, client, true /* start_channel */);

  DVLOG(1) << "Broker " << name_ << " accepting client " << client_name
           << " from peer " << from_node;

  sender->BrokerClientAdded(client_name, broker_channel.PassClientHandle());
}

void NodeController::OnBrokerClientAdded(
    const ports::NodeName& from_node,
    const ports::NodeName& client_name,
    ScopedInternalPlatformHandle broker_channel) {
  scoped_refptr<NodeChannel> client = GetPeerChannel(client_name);
  if (!client) {
    DLOG(ERROR) << "BrokerClientAdded for unknown client " << client_name;
    return;
  }

  // This should have come from our own broker.
  if (GetBrokerChannel() != GetPeerChannel(from_node)) {
    DLOG(ERROR) << "BrokerClientAdded from non-broker node " << from_node;
    return;
  }

  DVLOG(1) << "Client " << client_name << " accepted by broker " << from_node;

  client->AcceptBrokerClient(from_node, std::move(broker_channel));
}

void NodeController::OnAcceptBrokerClient(
    const ports::NodeName& from_node,
    const ports::NodeName& broker_name,
    ScopedInternalPlatformHandle broker_channel) {
  DCHECK(!GetConfiguration().is_broker_process);

  // This node should already have an inviter in bootstrap mode.
  ports::NodeName inviter_name;
  scoped_refptr<NodeChannel> inviter;
  {
    base::AutoLock lock(inviter_lock_);
    inviter_name = inviter_name_;
    inviter = bootstrap_inviter_channel_;
    bootstrap_inviter_channel_ = nullptr;
  }
  DCHECK(inviter_name == from_node);
  DCHECK(inviter);

  base::queue<ports::NodeName> pending_broker_clients;
  std::unordered_map<ports::NodeName, OutgoingMessageQueue>
      pending_relay_messages;
  {
    base::AutoLock lock(broker_lock_);
    broker_name_ = broker_name;
    std::swap(pending_broker_clients, pending_broker_clients_);
    std::swap(pending_relay_messages, pending_relay_messages_);
  }
  DCHECK(broker_name != ports::kInvalidNodeName);

  // It's now possible to add both the broker and the inviter as peers.
  // Note that the broker and inviter may be the same node.
  scoped_refptr<NodeChannel> broker;
  if (broker_name == inviter_name) {
    DCHECK(!broker_channel.is_valid());
    broker = inviter;
  } else {
    DCHECK(broker_channel.is_valid());
    broker = NodeChannel::Create(
        this,
        ConnectionParams(TransportProtocol::kLegacy, std::move(broker_channel)),
        io_task_runner_, ProcessErrorCallback());
    AddPeer(broker_name, broker, true /* start_channel */);
  }

  AddPeer(inviter_name, inviter, false /* start_channel */);

  {
    // Complete any port merge requests we have waiting for the inviter.
    base::AutoLock lock(pending_port_merges_lock_);
    for (const auto& request : pending_port_merges_)
      inviter->RequestPortMerge(request.second.name(), request.first);
    pending_port_merges_.clear();
  }

  // Feed the broker any pending invitees of our own.
  while (!pending_broker_clients.empty()) {
    const ports::NodeName& invitee_name = pending_broker_clients.front();
    auto it = pending_invitations_.find(invitee_name);
    // If for any reason we don't have a pending invitation for the invitee,
    // there's nothing left to do: we've already swapped the relevant state into
    // the stack.
    if (it != pending_invitations_.end()) {
      broker->AddBrokerClient(invitee_name,
                              it->second->CloneRemoteProcessHandle());
    }
    pending_broker_clients.pop();
  }

#if defined(OS_WIN) || (defined(OS_MACOSX) && !defined(OS_IOS))
  // Have the broker relay any messages we have waiting.
  for (auto& entry : pending_relay_messages) {
    const ports::NodeName& destination = entry.first;
    auto& message_queue = entry.second;
    while (!message_queue.empty()) {
      broker->RelayEventMessage(destination, std::move(message_queue.front()));
      message_queue.pop();
    }
  }
#endif

  DVLOG(1) << "Client " << name_ << " accepted by broker " << broker_name;
}

void NodeController::OnEventMessage(const ports::NodeName& from_node,
                                    Channel::MessagePtr channel_message) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  auto event = DeserializeEventMessage(from_node, std::move(channel_message));
  if (!event) {
    // We silently ignore unparseable events, as they may come from a process
    // running a newer version of Mojo.
    DVLOG(1) << "Ignoring invalid or unknown event from " << from_node;
    return;
  }

  node_->AcceptEvent(std::move(event));

  AttemptShutdownIfRequested();
}

void NodeController::OnRequestPortMerge(
    const ports::NodeName& from_node,
    const ports::PortName& connector_port_name,
    const std::string& name) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  DVLOG(2) << "Node " << name_ << " received RequestPortMerge for name " << name
           << " and port " << connector_port_name << "@" << from_node;

  ports::PortRef local_port;
  {
    base::AutoLock lock(reserved_ports_lock_);
    auto it = reserved_ports_.find(from_node);
    // TODO(https://crbug.com/822034): We should send a notification back to the
    // requestor so they can clean up their dangling port in this failure case.
    // This requires changes to the internal protocol, which can't be made yet.
    // Until this is done, pipes from |MojoExtractMessagePipeFromInvitation()|
    // will never break if the given name was invalid.
    if (it == reserved_ports_.end()) {
      DVLOG(1) << "Ignoring port merge request from node " << from_node << ". "
               << "No ports reserved for that node.";
      return;
    }

    PortMap& port_map = it->second;
    auto port_it = port_map.find(name);
    if (port_it == port_map.end()) {
      DVLOG(1) << "Ignoring request to connect to port for unknown name "
               << name << " from node " << from_node;
      return;
    }
    local_port = port_it->second;
    port_map.erase(port_it);
    if (port_map.empty())
      reserved_ports_.erase(it);
  }

  int rv = node_->MergePorts(local_port, from_node, connector_port_name);
  if (rv != ports::OK)
    DLOG(ERROR) << "MergePorts failed: " << rv;
}

void NodeController::OnRequestIntroduction(const ports::NodeName& from_node,
                                           const ports::NodeName& name) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  scoped_refptr<NodeChannel> requestor = GetPeerChannel(from_node);
  if (from_node == name || name == ports::kInvalidNodeName || !requestor) {
    DLOG(ERROR) << "Rejecting invalid OnRequestIntroduction message from "
                << from_node;
    DropPeer(from_node, nullptr);
    return;
  }

  scoped_refptr<NodeChannel> new_friend = GetPeerChannel(name);
  if (!new_friend) {
    // We don't know who they're talking about!
    requestor->Introduce(name, ScopedInternalPlatformHandle());
  } else {
    PlatformChannelPair new_channel;
    requestor->Introduce(name, new_channel.PassServerHandle());
    new_friend->Introduce(from_node, new_channel.PassClientHandle());
  }
}

void NodeController::OnIntroduce(const ports::NodeName& from_node,
                                 const ports::NodeName& name,
                                 ScopedInternalPlatformHandle channel_handle) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  if (!channel_handle.is_valid()) {
    node_->LostConnectionToNode(name);

    DVLOG(1) << "Could not be introduced to peer " << name;
    base::AutoLock lock(peers_lock_);
    pending_peer_messages_.erase(name);
    return;
  }

  scoped_refptr<NodeChannel> channel = NodeChannel::Create(
      this,
      ConnectionParams(TransportProtocol::kLegacy, std::move(channel_handle)),
      io_task_runner_, ProcessErrorCallback());

  DVLOG(1) << "Adding new peer " << name << " via broker introduction.";
  AddPeer(name, channel, true /* start_channel */);
}

void NodeController::OnBroadcast(const ports::NodeName& from_node,
                                 Channel::MessagePtr message) {
  DCHECK(!message->has_handles());

  auto event = DeserializeEventMessage(from_node, std::move(message));
  if (!event) {
    // We silently ignore unparseable events, as they may come from a process
    // running a newer version of Mojo.
    DVLOG(1) << "Ignoring request to broadcast invalid or unknown event from "
             << from_node;
    return;
  }

  base::AutoLock lock(peers_lock_);
  for (auto& iter : peers_) {
    // Clone and send the event to each known peer. Events which cannot be
    // cloned cannot be broadcast.
    ports::ScopedEvent clone = event->Clone();
    if (!clone) {
      DVLOG(1) << "Ignoring request to broadcast invalid event from "
               << from_node << " [type=" << static_cast<uint32_t>(event->type())
               << "]";
      return;
    }

    iter.second->SendChannelMessage(SerializeEventMessage(std::move(clone)));
  }
}

#if defined(OS_WIN) || (defined(OS_MACOSX) && !defined(OS_IOS))
void NodeController::OnRelayEventMessage(const ports::NodeName& from_node,
                                         base::ProcessHandle from_process,
                                         const ports::NodeName& destination,
                                         Channel::MessagePtr message) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  if (GetBrokerChannel()) {
    // Only the broker should be asked to relay a message.
    LOG(ERROR) << "Non-broker refusing to relay message.";
    DropPeer(from_node, nullptr);
    return;
  }

  // The broker should always know which process this came from.
  DCHECK(from_process != base::kNullProcessHandle);

#if defined(OS_WIN)
  // Rewrite the handles to this (the broker) process. If the message is
  // destined for another client process, the handles will be rewritten to that
  // process before going out (see NodeChannel::WriteChannelMessage).
  //
  // TODO: We could avoid double-duplication.
  //
  // Note that we explicitly mark the handles as being owned by the sending
  // process before rewriting them, in order to accommodate RewriteHandles'
  // internal sanity checks.
  std::vector<ScopedInternalPlatformHandle> handles = message->TakeHandles();
  for (auto& handle : handles)
    handle.get().owning_process = from_process;
  if (!Channel::Message::RewriteHandles(
          from_process, base::GetCurrentProcessHandle(), &handles)) {
    DLOG(ERROR) << "Failed to relay one or more handles.";
  }
  message->SetHandles(std::move(handles));
#else
  std::vector<ScopedInternalPlatformHandle> handles = message->TakeHandles();
  for (auto& handle : handles) {
    if (handle.get().type == InternalPlatformHandle::Type::MACH_NAME) {
      MachPortRelay* relay = GetMachPortRelay();
      if (!relay) {
        handle.get().type = InternalPlatformHandle::Type::MACH;
        handle.get().port = MACH_PORT_NULL;
        DLOG(ERROR) << "Receiving Mach ports without a port relay from "
                    << from_node << ".";
        continue;
      }
      relay->ExtractPort(&handle, from_process);
    }
  }
  message->SetHandles(std::move(handles));
#endif  // defined(OS_WIN)

  if (destination == name_) {
    // Great, we can deliver this message locally.
    OnEventMessage(from_node, std::move(message));
    return;
  }

  scoped_refptr<NodeChannel> peer = GetPeerChannel(destination);
  if (peer)
    peer->EventMessageFromRelay(from_node, std::move(message));
  else
    DLOG(ERROR) << "Dropping relay message for unknown node " << destination;
}

void NodeController::OnEventMessageFromRelay(const ports::NodeName& from_node,
                                             const ports::NodeName& source_node,
                                             Channel::MessagePtr message) {
  if (GetPeerChannel(from_node) != GetBrokerChannel()) {
    LOG(ERROR) << "Refusing relayed message from non-broker node.";
    DropPeer(from_node, nullptr);
    return;
  }

  OnEventMessage(source_node, std::move(message));
}
#endif

void NodeController::OnAcceptPeer(const ports::NodeName& from_node,
                                  const ports::NodeName& token,
                                  const ports::NodeName& peer_name,
                                  const ports::PortName& port_name) {
  DCHECK(io_task_runner_->RunsTasksInCurrentSequence());

  auto it = peer_connections_.find(from_node);
  if (it == peer_connections_.end()) {
    DLOG(ERROR) << "Received unexpected AcceptPeer message from " << from_node;
    DropPeer(from_node, nullptr);
    return;
  }

  scoped_refptr<NodeChannel> channel = std::move(it->second.channel);
  ports::PortRef local_port = it->second.local_port;
  uint64_t peer_connection_id = it->second.connection_id;
  peer_connections_.erase(it);
  DCHECK(channel);

  if (name_ == peer_name) {
    // If the peer connection is a self connection (which is used in tests),
    // drop the channel to it and skip straight to merging the ports.
    peer_connections_by_id_.erase(peer_connection_id);
  } else {
    peer_connections_by_id_[peer_connection_id] = peer_name;
    peer_connections_.emplace(
        peer_name, PeerConnection{nullptr, local_port, peer_connection_id});
    DVLOG(1) << "Node " << name_ << " accepted peer " << peer_name;

    AddPeer(peer_name, channel, false /* start_channel */);
  }

  // We need to choose one side to initiate the port merge. It doesn't matter
  // who does it as long as they don't both try. Simple solution: pick the one
  // with the "smaller" port name.
  if (local_port.name() < port_name)
    node()->MergePorts(local_port, peer_name, port_name);
}

void NodeController::OnChannelError(const ports::NodeName& from_node,
                                    NodeChannel* channel) {
  if (io_task_runner_->RunsTasksInCurrentSequence()) {
    RequestContext request_context(RequestContext::Source::SYSTEM);
    DropPeer(from_node, channel);
  } else {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&NodeController::OnChannelError, base::Unretained(this),
                   from_node, base::RetainedRef(channel)));
  }
}

#if defined(OS_MACOSX) && !defined(OS_IOS)
MachPortRelay* NodeController::GetMachPortRelay() {
  {
    base::AutoLock lock(inviter_lock_);
    // Return null if we're not the root.
    if (bootstrap_inviter_channel_ || inviter_name_ != ports::kInvalidNodeName)
      return nullptr;
  }

  base::AutoLock lock(mach_port_relay_lock_);
  return mach_port_relay_.get();
}
#endif

void NodeController::CancelPendingPortMerges() {
  std::vector<ports::PortRef> ports_to_close;

  {
    base::AutoLock lock(pending_port_merges_lock_);
    reject_pending_merges_ = true;
    for (const auto& port : pending_port_merges_)
      ports_to_close.push_back(port.second);
    pending_port_merges_.clear();
  }

  for (const auto& port : ports_to_close)
    node_->ClosePort(port);
}

void NodeController::DestroyOnIOThreadShutdown() {
  destroy_on_io_thread_shutdown_ = true;
}

void NodeController::AttemptShutdownIfRequested() {
  if (!shutdown_callback_flag_)
    return;

  base::Closure callback;
  {
    base::AutoLock lock(shutdown_lock_);
    if (shutdown_callback_.is_null())
      return;
    if (!node_->CanShutdownCleanly(
          ports::Node::ShutdownPolicy::ALLOW_LOCAL_PORTS)) {
      DVLOG(2) << "Unable to cleanly shut down node " << name_;
      return;
    }

    callback = shutdown_callback_;
    shutdown_callback_.Reset();
    shutdown_callback_flag_.Set(false);
  }

  DCHECK(!callback.is_null());

  callback.Run();
}

NodeController::PeerConnection::PeerConnection() = default;

NodeController::PeerConnection::PeerConnection(
    const PeerConnection& other) = default;

NodeController::PeerConnection::PeerConnection(
    PeerConnection&& other) = default;

NodeController::PeerConnection::PeerConnection(
    scoped_refptr<NodeChannel> channel,
    const ports::PortRef& local_port,
    uint64_t connection_id)
    : channel(std::move(channel)),
      local_port(local_port),
      connection_id(connection_id) {}

NodeController::PeerConnection::~PeerConnection() = default;

NodeController::PeerConnection& NodeController::PeerConnection::
operator=(const PeerConnection& other) = default;

NodeController::PeerConnection& NodeController::PeerConnection::
operator=(PeerConnection&& other) = default;

}  // namespace edk
}  // namespace mojo
