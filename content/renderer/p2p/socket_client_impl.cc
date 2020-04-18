// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/p2p/socket_client_impl.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/common/p2p_messages.h"
#include "content/renderer/p2p/socket_client_delegate.h"
#include "content/renderer/p2p/socket_dispatcher.h"
#include "content/renderer/render_thread_impl.h"
#include "crypto/random.h"

namespace {

uint64_t GetUniqueId(uint32_t random_socket_id, uint32_t packet_id) {
  uint64_t uid = random_socket_id;
  uid <<= 32;
  uid |= packet_id;
  return uid;
}

}  // namespace

namespace content {

P2PSocketClientImpl::P2PSocketClientImpl(
    P2PSocketDispatcher* dispatcher,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : dispatcher_(dispatcher),
      ipc_task_runner_(dispatcher->task_runner()),
      delegate_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      socket_id_(0),
      delegate_(nullptr),
      state_(STATE_UNINITIALIZED),
      traffic_annotation_(traffic_annotation),
      random_socket_id_(0),
      next_packet_id_(0) {
  crypto::RandBytes(&random_socket_id_, sizeof(random_socket_id_));
}

P2PSocketClientImpl::~P2PSocketClientImpl() {
  CHECK(state_ == STATE_CLOSED || state_ == STATE_UNINITIALIZED);
}

void P2PSocketClientImpl::Init(P2PSocketType type,
                               const net::IPEndPoint& local_address,
                               uint16_t min_port,
                               uint16_t max_port,
                               const P2PHostAndIPEndPoint& remote_address,
                               P2PSocketClientDelegate* delegate) {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());
  DCHECK(delegate);
  // |delegate_| is only accessesed on |delegate_message_loop_|.
  delegate_ = delegate;

  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&P2PSocketClientImpl::DoInit, this, type, local_address,
                     min_port, max_port, remote_address));
}

void P2PSocketClientImpl::DoInit(P2PSocketType type,
                                 const net::IPEndPoint& local_address,
                                 uint16_t min_port,
                                 uint16_t max_port,
                                 const P2PHostAndIPEndPoint& remote_address) {
  DCHECK_EQ(state_, STATE_UNINITIALIZED);
  state_ = STATE_OPENING;
  socket_id_ = dispatcher_->RegisterClient(this);
  dispatcher_->SendP2PMessage(new P2PHostMsg_CreateSocket(
      type, socket_id_, local_address, P2PPortRange(min_port, max_port),
      remote_address));
}

uint64_t P2PSocketClientImpl::Send(const net::IPEndPoint& address,
                                   const std::vector<char>& data,
                                   const rtc::PacketOptions& options) {
  uint64_t unique_id = GetUniqueId(random_socket_id_, ++next_packet_id_);
  if (!ipc_task_runner_->BelongsToCurrentThread()) {
    ipc_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&P2PSocketClientImpl::SendWithPacketId, this,
                                  address, data, options, unique_id));
    return unique_id;
  }

  // Can send data only when the socket is open.
  DCHECK(state_ == STATE_OPEN || state_ == STATE_ERROR);
  if (state_ == STATE_OPEN) {
    SendWithPacketId(address, data, options, unique_id);
  }

  return unique_id;
}

void P2PSocketClientImpl::SendWithPacketId(const net::IPEndPoint& address,
                                           const std::vector<char>& data,
                                           const rtc::PacketOptions& options,
                                           uint64_t packet_id) {
  TRACE_EVENT_ASYNC_BEGIN0("p2p", "Send", packet_id);

  dispatcher_->SendP2PMessage(new P2PHostMsg_Send(
      socket_id_, data, P2PPacketInfo(address, options, packet_id),
      net::MutableNetworkTrafficAnnotationTag(traffic_annotation_)));
}

void P2PSocketClientImpl::SetOption(P2PSocketOption option,
                                    int value) {
  if (!ipc_task_runner_->BelongsToCurrentThread()) {
    ipc_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&P2PSocketClientImpl::SetOption, this, option, value));
    return;
  }

  DCHECK(state_ == STATE_OPEN || state_ == STATE_ERROR);
  if (state_ == STATE_OPEN) {
    dispatcher_->SendP2PMessage(new P2PHostMsg_SetOption(socket_id_,
                                                         option, value));
  }
}

void P2PSocketClientImpl::Close() {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());

  delegate_ = nullptr;

  ipc_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&P2PSocketClientImpl::DoClose, this));
}

void P2PSocketClientImpl::DoClose() {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  if (dispatcher_) {
    if (state_ == STATE_OPEN || state_ == STATE_OPENING ||
        state_ == STATE_ERROR) {
      dispatcher_->SendP2PMessage(new P2PHostMsg_DestroySocket(socket_id_));
    }
    dispatcher_->UnregisterClient(socket_id_);
  }

  state_ = STATE_CLOSED;
}

int P2PSocketClientImpl::GetSocketID() const {
  return socket_id_;
}

void P2PSocketClientImpl::SetDelegate(P2PSocketClientDelegate* delegate) {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());
  delegate_ = delegate;
}

void P2PSocketClientImpl::OnSocketCreated(
    const net::IPEndPoint& local_address,
    const net::IPEndPoint& remote_address) {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, STATE_OPENING);
  state_ = STATE_OPEN;

  delegate_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&P2PSocketClientImpl::DeliverOnSocketCreated,
                                this, local_address, remote_address));
}

void P2PSocketClientImpl::DeliverOnSocketCreated(
    const net::IPEndPoint& local_address,
    const net::IPEndPoint& remote_address) {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());
  if (delegate_)
    delegate_->OnOpen(local_address, remote_address);
}

void P2PSocketClientImpl::OnIncomingTcpConnection(
    const net::IPEndPoint& address) {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(state_, STATE_OPEN);

  scoped_refptr<P2PSocketClientImpl> new_client =
      new P2PSocketClientImpl(dispatcher_, traffic_annotation_);
  new_client->socket_id_ = dispatcher_->RegisterClient(new_client.get());
  new_client->state_ = STATE_OPEN;
  new_client->delegate_task_runner_ = delegate_task_runner_;

  dispatcher_->SendP2PMessage(new P2PHostMsg_AcceptIncomingTcpConnection(
      socket_id_, address, new_client->socket_id_));

  delegate_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&P2PSocketClientImpl::DeliverOnIncomingTcpConnection, this,
                     address, new_client));
}

void P2PSocketClientImpl::DeliverOnIncomingTcpConnection(
    const net::IPEndPoint& address,
    scoped_refptr<P2PSocketClient> new_client) {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());
  if (delegate_) {
    delegate_->OnIncomingTcpConnection(address, new_client.get());
  } else {
    // Just close the socket if there is no delegate to accept it.
    new_client->Close();
  }
}

void P2PSocketClientImpl::OnSendComplete(
    const P2PSendPacketMetrics& send_metrics) {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());

  delegate_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&P2PSocketClientImpl::DeliverOnSendComplete,
                                this, send_metrics));
}

void P2PSocketClientImpl::DeliverOnSendComplete(
    const P2PSendPacketMetrics& send_metrics) {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());
  if (delegate_)
    delegate_->OnSendComplete(send_metrics);
}

void P2PSocketClientImpl::OnError() {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  state_ = STATE_ERROR;

  delegate_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&P2PSocketClientImpl::DeliverOnError, this));
}

void P2PSocketClientImpl::DeliverOnError() {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());
  if (delegate_)
    delegate_->OnError();
}

void P2PSocketClientImpl::OnDataReceived(const net::IPEndPoint& address,
                                         const std::vector<char>& data,
                                         const base::TimeTicks& timestamp) {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(STATE_OPEN, state_);
  delegate_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&P2PSocketClientImpl::DeliverOnDataReceived,
                                this, address, data, timestamp));
}

void P2PSocketClientImpl::DeliverOnDataReceived(
  const net::IPEndPoint& address, const std::vector<char>& data,
  const base::TimeTicks& timestamp) {
  DCHECK(delegate_task_runner_->BelongsToCurrentThread());
  if (delegate_)
    delegate_->OnDataReceived(address, data, timestamp);
}

void P2PSocketClientImpl::Detach() {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  dispatcher_ = nullptr;
  OnError();
}

}  // namespace content
