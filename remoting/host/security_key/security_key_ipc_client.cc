// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/security_key/security_key_ipc_client.h"

#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_macros.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "remoting/host/chromoting_messages.h"
#include "remoting/host/ipc_constants.h"
#include "remoting/host/security_key/security_key_ipc_constants.h"

namespace remoting {

SecurityKeyIpcClient::SecurityKeyIpcClient()
    : named_channel_handle_(remoting::GetSecurityKeyIpcChannel()),
      weak_factory_(this) {}

SecurityKeyIpcClient::~SecurityKeyIpcClient() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

bool SecurityKeyIpcClient::CheckForSecurityKeyIpcServerChannel() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!channel_handle_.is_valid()) {
    channel_handle_ = mojo::edk::CreateClientHandle(named_channel_handle_);
  }
  return channel_handle_.is_valid();
}

void SecurityKeyIpcClient::EstablishIpcConnection(
    const ConnectedCallback& connected_callback,
    const base::Closure& connection_error_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(connected_callback);
  DCHECK(connection_error_callback);
  DCHECK(!ipc_channel_);

  connected_callback_ = connected_callback;
  connection_error_callback_ = connection_error_callback;

  ConnectToIpcChannel();
}

bool SecurityKeyIpcClient::SendSecurityKeyRequest(
    const std::string& request_payload,
    const ResponseCallback& response_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!request_payload.empty());
  DCHECK(response_callback);

  if (!ipc_channel_) {
    LOG(ERROR) << "Request made before IPC connection was established.";
    return false;
  }

  if (response_callback_) {
    LOG(ERROR)
        << "Request made while waiting for a response to a previous request.";
    return false;
  }

  response_callback_ = response_callback;
  return ipc_channel_->Send(
      new ChromotingRemoteSecurityKeyToNetworkMsg_Request(request_payload));
}

void SecurityKeyIpcClient::CloseIpcConnection() {
  DCHECK(thread_checker_.CalledOnValidThread());
  ipc_channel_.reset();
}

void SecurityKeyIpcClient::SetIpcChannelHandleForTest(
    const mojo::edk::NamedPlatformHandle& channel_handle) {
  named_channel_handle_ = channel_handle;
}

void SecurityKeyIpcClient::SetExpectedIpcServerSessionIdForTest(
    uint32_t expected_session_id) {
  expected_ipc_server_session_id_ = expected_session_id;
}

bool SecurityKeyIpcClient::OnMessageReceived(const IPC::Message& message) {
  DCHECK(thread_checker_.CalledOnValidThread());

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(SecurityKeyIpcClient, message)
    IPC_MESSAGE_HANDLER(ChromotingNetworkToRemoteSecurityKeyMsg_Response,
                        OnSecurityKeyResponse)
    IPC_MESSAGE_HANDLER(ChromotingNetworkToRemoteSecurityKeyMsg_ConnectionReady,
                        OnConnectionReady)
    IPC_MESSAGE_HANDLER(ChromotingNetworkToRemoteSecurityKeyMsg_InvalidSession,
                        OnInvalidSession)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  CHECK(handled) << "Received unexpected IPC type: " << message.type();
  return handled;
}

void SecurityKeyIpcClient::OnChannelConnected(int32_t peer_pid) {
  DCHECK(thread_checker_.CalledOnValidThread());

#if defined(OS_WIN)
  DWORD peer_session_id;
  if (!ProcessIdToSessionId(peer_pid, &peer_session_id)) {
    PLOG(ERROR) << "ProcessIdToSessionId failed";
    base::ResetAndReturn(&connection_error_callback_).Run();
    return;
  }

  if (peer_session_id != expected_ipc_server_session_id_) {
    LOG(ERROR)
        << "Cannot establish connection with IPC server running in session: "
        << peer_session_id;
    base::ResetAndReturn(&connection_error_callback_).Run();
    return;
  }
#endif  // defined(OS_WIN)
}

void SecurityKeyIpcClient::OnChannelError() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (connection_error_callback_) {
    base::ResetAndReturn(&connection_error_callback_).Run();
  }
}

void SecurityKeyIpcClient::OnSecurityKeyResponse(
    const std::string& response_data) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!response_data.empty()) {
    base::ResetAndReturn(&response_callback_).Run(response_data);
  } else {
    LOG(ERROR) << "Invalid response received";
    if (connection_error_callback_) {
      base::ResetAndReturn(&connection_error_callback_).Run();
    }
  }
}

void SecurityKeyIpcClient::OnConnectionReady() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!connected_callback_) {
    LOG(ERROR) << "Unexpected ConnectionReady message received.";
    if (connection_error_callback_) {
      base::ResetAndReturn(&connection_error_callback_).Run();
    }
    return;
  }

  base::ResetAndReturn(&connected_callback_).Run(/*connection_usable=*/true);
}

void SecurityKeyIpcClient::OnInvalidSession() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!connected_callback_) {
    LOG(ERROR) << "Unexpected InvalidSession message received.";
    if (connection_error_callback_) {
      base::ResetAndReturn(&connection_error_callback_).Run();
    }
    return;
  }

  base::ResetAndReturn(&connected_callback_).Run(/*connection_usable=*/false);
}

void SecurityKeyIpcClient::ConnectToIpcChannel() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Verify that any existing IPC connection has been closed.
  CloseIpcConnection();

  if (!channel_handle_.is_valid() && !CheckForSecurityKeyIpcServerChannel()) {
    if (connection_error_callback_) {
      base::ResetAndReturn(&connection_error_callback_).Run();
    }
    return;
  }

  ipc_channel_ =
      IPC::Channel::CreateClient(peer_connection_
                                     .Connect(mojo::edk::ConnectionParams(
                                         mojo::edk::TransportProtocol::kLegacy,
                                         std::move(channel_handle_)))
                                     .release(),
                                 this, base::ThreadTaskRunnerHandle::Get());
  if (ipc_channel_->Connect()) {
    return;
  }
  ipc_channel_.reset();

  if (connection_error_callback_) {
    base::ResetAndReturn(&connection_error_callback_).Run();
  }
}

}  // namespace remoting
