// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_udp_socket_message_filter.h"

#include <cstring>
#include <utility>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"
#include "content/browser/renderer_host/pepper/browser_ppapi_host_impl.h"
#include "content/browser/renderer_host/pepper/pepper_socket_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/process_type.h"
#include "content/public/common/socket_permission_request.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/socket/udp_socket.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_net_address_private.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/error_conversion.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/udp_socket_resource_constants.h"
#include "ppapi/shared_impl/private/net_address_private_impl.h"
#include "ppapi/shared_impl/socket_option_data.h"

#if defined(OS_CHROMEOS)
#include "chromeos/network/firewall_hole.h"
#endif  // defined(OS_CHROMEOS)

using ppapi::NetAddressPrivateImpl;
using ppapi::host::NetErrorToPepperError;
using ppapi::proxy::UDPSocketResourceConstants;

namespace {

size_t g_num_udp_filter_instances = 0;

}  // namespace

namespace content {

PepperUDPSocketMessageFilter::PendingSend::PendingSend(
    const net::IPAddress& address,
    int port,
    const scoped_refptr<net::IOBufferWithSize>& buffer,
    const ppapi::host::ReplyMessageContext& context)
    : address(address), port(port), buffer(buffer), context(context) {}

PepperUDPSocketMessageFilter::PendingSend::PendingSend(
    const PendingSend& other) = default;

PepperUDPSocketMessageFilter::PendingSend::~PendingSend() {
}

PepperUDPSocketMessageFilter::PepperUDPSocketMessageFilter(
    BrowserPpapiHostImpl* host,
    PP_Instance instance,
    bool private_api)
    : socket_options_(0),
      rcvbuf_size_(0),
      sndbuf_size_(0),
      multicast_ttl_(0),
      can_use_multicast_(PP_ERROR_FAILED),
      closed_(false),
      remaining_recv_slots_(
          UDPSocketResourceConstants::kPluginReceiveBufferSlots),
      external_plugin_(host->external_plugin()),
      private_api_(private_api),
      render_process_id_(0),
      render_frame_id_(0),
      is_potentially_secure_plugin_context_(
          host->IsPotentiallySecurePluginContext(instance)) {
  ++g_num_udp_filter_instances;
  DCHECK(host);

  if (!host->GetRenderFrameIDsForInstance(
          instance, &render_process_id_, &render_frame_id_)) {
    NOTREACHED();
  }
}

PepperUDPSocketMessageFilter::~PepperUDPSocketMessageFilter() {
  Close();
  --g_num_udp_filter_instances;
}

// static
size_t PepperUDPSocketMessageFilter::GetNumInstances() {
  return g_num_udp_filter_instances;
}

scoped_refptr<base::TaskRunner>
PepperUDPSocketMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  switch (message.type()) {
    case PpapiHostMsg_UDPSocket_SetOption::ID:
    case PpapiHostMsg_UDPSocket_Close::ID:
    case PpapiHostMsg_UDPSocket_RecvSlotAvailable::ID:
      return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
    case PpapiHostMsg_UDPSocket_Bind::ID:
    case PpapiHostMsg_UDPSocket_SendTo::ID:
    case PpapiHostMsg_UDPSocket_JoinGroup::ID:
    case PpapiHostMsg_UDPSocket_LeaveGroup::ID:
      return BrowserThread::GetTaskRunnerForThread(BrowserThread::UI);
  }
  return nullptr;
}

int32_t PepperUDPSocketMessageFilter::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperUDPSocketMessageFilter, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UDPSocket_SetOption,
                                      OnMsgSetOption)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UDPSocket_Bind, OnMsgBind)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UDPSocket_SendTo,
                                      OnMsgSendTo)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_UDPSocket_Close,
                                        OnMsgClose)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
        PpapiHostMsg_UDPSocket_RecvSlotAvailable, OnMsgRecvSlotAvailable)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UDPSocket_JoinGroup,
                                      OnMsgJoinGroup)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_UDPSocket_LeaveGroup,
                                      OnMsgLeaveGroup)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperUDPSocketMessageFilter::OnMsgSetOption(
    const ppapi::host::HostMessageContext* context,
    PP_UDPSocket_Option name,
    const ppapi::SocketOptionData& value) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (closed_)
    return PP_ERROR_FAILED;

  switch (name) {
    case PP_UDPSOCKET_OPTION_ADDRESS_REUSE: {
      if (socket_.get()) {
        // AllowReuseAddress is only effective before Bind().
        // Note that this limitation originally comes from Windows, but
        // PPAPI tries to provide platform independent APIs.
        return PP_ERROR_FAILED;
      }

      bool boolean_value = false;
      if (!value.GetBool(&boolean_value))
        return PP_ERROR_BADARGUMENT;

      if (boolean_value) {
        socket_options_ |= SOCKET_OPTION_ADDRESS_REUSE;
      } else {
        socket_options_ &= ~SOCKET_OPTION_ADDRESS_REUSE;
      }
      return PP_OK;
    }
    case PP_UDPSOCKET_OPTION_BROADCAST: {
      bool boolean_value = false;
      if (!value.GetBool(&boolean_value))
        return PP_ERROR_BADARGUMENT;

      // If the socket is already bound, proxy the value to UDPSocket.
      if (socket_.get())
        return NetErrorToPepperError(socket_->SetBroadcast(boolean_value));

      // UDPSocket instance is not yet created, so remember the value here.
      if (boolean_value) {
        socket_options_ |= SOCKET_OPTION_BROADCAST;
      } else {
        socket_options_ &= ~SOCKET_OPTION_BROADCAST;
      }
      return PP_OK;
    }
    case PP_UDPSOCKET_OPTION_SEND_BUFFER_SIZE: {
      int32_t integer_value = 0;
      if (!value.GetInt32(&integer_value) || integer_value <= 0 ||
          integer_value > UDPSocketResourceConstants::kMaxSendBufferSize)
        return PP_ERROR_BADARGUMENT;

      // If the socket is already bound, proxy the value to UDPSocket.
      if (socket_.get()) {
        return NetErrorToPepperError(
            socket_->SetSendBufferSize(integer_value));
      }

      // UDPSocket instance is not yet created, so remember the value here.
      socket_options_ |= SOCKET_OPTION_SNDBUF_SIZE;
      sndbuf_size_ = integer_value;
      return PP_OK;
    }
    case PP_UDPSOCKET_OPTION_RECV_BUFFER_SIZE: {
      int32_t integer_value = 0;
      if (!value.GetInt32(&integer_value) || integer_value <= 0 ||
          integer_value > UDPSocketResourceConstants::kMaxReceiveBufferSize)
        return PP_ERROR_BADARGUMENT;

      // If the socket is already bound, proxy the value to UDPSocket.
      if (socket_.get()) {
        return NetErrorToPepperError(
            socket_->SetReceiveBufferSize(integer_value));
      }

      // UDPSocket instance is not yet created, so remember the value here.
      socket_options_ |= SOCKET_OPTION_RCVBUF_SIZE;
      rcvbuf_size_ = integer_value;
      return PP_OK;
    }
    case PP_UDPSOCKET_OPTION_MULTICAST_LOOP: {
      bool boolean_value = false;
      if (!value.GetBool(&boolean_value))
        return PP_ERROR_BADARGUMENT;

      // If the socket is already bound, proxy the value to UDPSocket.
      if (socket_) {
        if (can_use_multicast_ != PP_OK)
          return can_use_multicast_;

        return NetErrorToPepperError(
            socket_->SetMulticastLoopbackMode(boolean_value));
      }

      // UDPSocket instance is not yet created, so remember the value here.
      if (boolean_value) {
        socket_options_ |= SOCKET_OPTION_MULTICAST_LOOP;
      } else {
        socket_options_ &= ~SOCKET_OPTION_MULTICAST_LOOP;
      }
      return PP_OK;
    }
    case PP_UDPSOCKET_OPTION_MULTICAST_TTL: {
      int32_t integer_value = 0;
      if (!value.GetInt32(&integer_value) ||
          integer_value < 0 || integer_value > 255)
        return PP_ERROR_BADARGUMENT;

      // If the socket is already bound, proxy the value to UDPSocket.
      if (socket_) {
        if (can_use_multicast_ != PP_OK)
          return can_use_multicast_;

        return NetErrorToPepperError(
            socket_->SetMulticastTimeToLive(integer_value));
      }

      // UDPSocket instance is not yet created, so remember the value here.
      socket_options_ |= SOCKET_OPTION_MULTICAST_TTL;
      multicast_ttl_ = integer_value;
      return PP_OK;
    }
    default: {
      NOTREACHED();
      return PP_ERROR_BADARGUMENT;
    }
  }
}

int32_t PepperUDPSocketMessageFilter::OnMsgBind(
    const ppapi::host::HostMessageContext* context,
    const PP_NetAddress_Private& addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context);

  // Check for permissions to use multicast APIS. This check must be done while
  // on the UI thread, so we cache the value here to be used later on.
  PP_NetAddress_Private any_addr;
  NetAddressPrivateImpl::GetAnyAddress(PP_FALSE, &any_addr);
  can_use_multicast_ = CanUseMulticastAPI(any_addr);

  SocketPermissionRequest request =
      pepper_socket_utils::CreateSocketPermissionRequest(
          SocketPermissionRequest::UDP_BIND, addr);
  if (!pepper_socket_utils::CanUseSocketAPIs(external_plugin_,
                                             private_api_,
                                             &request,
                                             render_process_id_,
                                             render_frame_id_)) {
    return PP_ERROR_NOACCESS;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PepperUDPSocketMessageFilter::DoBind, this,
                     context->MakeReplyMessageContext(), addr));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperUDPSocketMessageFilter::OnMsgSendTo(
    const ppapi::host::HostMessageContext* context,
    const std::string& data,
    const PP_NetAddress_Private& addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context);

  SocketPermissionRequest request =
      pepper_socket_utils::CreateSocketPermissionRequest(
          SocketPermissionRequest::UDP_SEND_TO, addr);
  if (!pepper_socket_utils::CanUseSocketAPIs(external_plugin_,
                                             private_api_,
                                             &request,
                                             render_process_id_,
                                             render_frame_id_)) {
    return PP_ERROR_NOACCESS;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PepperUDPSocketMessageFilter::DoSendTo, this,
                     context->MakeReplyMessageContext(), data, addr));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperUDPSocketMessageFilter::OnMsgClose(
    const ppapi::host::HostMessageContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  Close();
  return PP_OK;
}

int32_t PepperUDPSocketMessageFilter::OnMsgRecvSlotAvailable(
    const ppapi::host::HostMessageContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (remaining_recv_slots_ <
      UDPSocketResourceConstants::kPluginReceiveBufferSlots) {
    remaining_recv_slots_++;
  }

  if (!recvfrom_buffer_.get() && !closed_ && socket_.get()) {
    DCHECK_EQ(1u, remaining_recv_slots_);
    DoRecvFrom();
  }

  return PP_OK;
}

int32_t PepperUDPSocketMessageFilter::OnMsgJoinGroup(
    const ppapi::host::HostMessageContext* context,
    const PP_NetAddress_Private& addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  int32_t ret = CanUseMulticastAPI(addr);
  if (ret != PP_OK)
    return ret;

  if (!socket_)
    return PP_ERROR_FAILED;

  net::IPAddressBytes group;
  uint16_t port;

  if (!NetAddressPrivateImpl::NetAddressToIPEndPoint(addr, &group, &port))
    return PP_ERROR_ADDRESS_INVALID;

  return NetErrorToPepperError(socket_->JoinGroup(net::IPAddress(group)));
}

int32_t PepperUDPSocketMessageFilter::OnMsgLeaveGroup(
    const ppapi::host::HostMessageContext* context,
    const PP_NetAddress_Private& addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  int32_t ret = CanUseMulticastAPI(addr);
  if (ret != PP_OK)
    return ret;

  if (!socket_)
    return PP_ERROR_FAILED;

  net::IPAddressBytes group;
  uint16_t port;

  if (!NetAddressPrivateImpl::NetAddressToIPEndPoint(addr, &group, &port))
    return PP_ERROR_ADDRESS_INVALID;

  return NetErrorToPepperError(socket_->LeaveGroup(net::IPAddress(group)));
}

void PepperUDPSocketMessageFilter::DoBind(
    const ppapi::host::ReplyMessageContext& context,
    const PP_NetAddress_Private& addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (closed_ || socket_.get()) {
    SendBindError(context, PP_ERROR_FAILED);
    return;
  }

  auto socket = std::make_unique<net::UDPSocket>(
      net::DatagramSocket::DEFAULT_BIND, nullptr, net::NetLogSource());

  net::IPAddressBytes address;
  uint16_t port;
  if (!NetAddressPrivateImpl::NetAddressToIPEndPoint(addr, &address, &port)) {
    SendBindError(context, PP_ERROR_ADDRESS_INVALID);
    return;
  }
  net::IPEndPoint end_point(net::IPAddress(address), port);
  {
    int net_result = socket->Open(end_point.GetFamily());
    if (net_result != net::OK) {
      SendBindError(context, NetErrorToPepperError(net_result));
      return;
    }
  }

  if (socket_options_ & SOCKET_OPTION_ADDRESS_REUSE) {
    int net_result = socket->AllowAddressReuse();
    if (net_result != net::OK) {
      SendBindError(context, NetErrorToPepperError(net_result));
      return;
    }
  }
  if (socket_options_ & SOCKET_OPTION_BROADCAST) {
    int net_result = socket->SetBroadcast(true);
    if (net_result != net::OK) {
      SendBindError(context, NetErrorToPepperError(net_result));
      return;
    }
  }
  if (socket_options_ & SOCKET_OPTION_SNDBUF_SIZE) {
    int net_result = socket->SetSendBufferSize(sndbuf_size_);
    if (net_result != net::OK) {
      SendBindError(context, NetErrorToPepperError(net_result));
      return;
    }
  }
  if (socket_options_ & SOCKET_OPTION_RCVBUF_SIZE) {
    int net_result = socket->SetReceiveBufferSize(rcvbuf_size_);
    if (net_result != net::OK) {
      SendBindError(context, NetErrorToPepperError(net_result));
      return;
    }
  }
  if (socket_options_ & SOCKET_OPTION_MULTICAST_LOOP) {
    if (can_use_multicast_ != PP_OK) {
      SendBindError(context, NetErrorToPepperError(can_use_multicast_));
      return;
    }

    int net_result = socket->SetMulticastLoopbackMode(true);
    if (net_result != net::OK) {
      SendBindError(context, NetErrorToPepperError(net_result));
      return;
    }
  }
  if (socket_options_ & SOCKET_OPTION_MULTICAST_TTL) {
    if (can_use_multicast_ != PP_OK) {
      SendBindError(context, NetErrorToPepperError(can_use_multicast_));
      return;
    }

    int net_result = socket->SetMulticastTimeToLive(multicast_ttl_);
    if (net_result != net::OK) {
      SendBindError(context, NetErrorToPepperError(net_result));
      return;
    }
  }

  {
    int net_result = socket->Bind(end_point);
    if (net_result != net::OK) {
      SendBindError(context, NetErrorToPepperError(net_result));
      return;
    }
  }

  net::IPEndPoint bound_address;
  {
    int net_result = socket->GetLocalAddress(&bound_address);
    if (net_result != net::OK) {
      SendBindError(context, NetErrorToPepperError(net_result));
      return;
    }
  }

  PP_NetAddress_Private net_address = NetAddressPrivateImpl::kInvalidNetAddress;
  if (!NetAddressPrivateImpl::IPEndPointToNetAddress(
          bound_address.address().bytes(), bound_address.port(),
          &net_address)) {
    SendBindError(context, PP_ERROR_ADDRESS_INVALID);
    return;
  }

#if defined(OS_CHROMEOS)
  OpenFirewallHole(
      bound_address,
      base::Bind(&PepperUDPSocketMessageFilter::OnBindComplete, this,
                 base::Passed(&socket), context, net_address));
#else
  OnBindComplete(std::move(socket), context, net_address);
#endif
}

void PepperUDPSocketMessageFilter::OnBindComplete(
    std::unique_ptr<net::UDPSocket> socket,
    const ppapi::host::ReplyMessageContext& context,
    const PP_NetAddress_Private& net_address) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  socket_.swap(socket);
  SendBindReply(context, PP_OK, net_address);

  DoRecvFrom();
}

#if defined(OS_CHROMEOS)
void PepperUDPSocketMessageFilter::OpenFirewallHole(
    const net::IPEndPoint& local_address,
    base::Closure bind_complete) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  pepper_socket_utils::FirewallHoleOpenCallback callback = base::Bind(
      &PepperUDPSocketMessageFilter::OnFirewallHoleOpened, this, bind_complete);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&pepper_socket_utils::OpenUDPFirewallHole, local_address,
                     callback));
}

void PepperUDPSocketMessageFilter::OnFirewallHoleOpened(
    base::Closure bind_complete,
    std::unique_ptr<chromeos::FirewallHole> hole) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  LOG_IF(WARNING, !hole.get()) << "Firewall hole could not be opened.";
  firewall_hole_.reset(hole.release());

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE, bind_complete);
}
#endif  // defined(OS_CHROMEOS)

void PepperUDPSocketMessageFilter::DoRecvFrom() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!closed_);
  DCHECK(socket_.get());
  DCHECK(!recvfrom_buffer_.get());
  DCHECK_GT(remaining_recv_slots_, 0u);

  recvfrom_buffer_ =
      new net::IOBuffer(UDPSocketResourceConstants::kMaxReadSize);

  // Use base::Unretained(this), so that the lifespan of this object doesn't
  // have to last until the callback is called.
  // It is safe to do so because |socket_| is owned by this object. If this
  // object gets destroyed (and so does |socket_|), the callback won't be
  // called.
  int net_result = socket_->RecvFrom(
      recvfrom_buffer_.get(), UDPSocketResourceConstants::kMaxReadSize,
      &recvfrom_address_,
      base::Bind(&PepperUDPSocketMessageFilter::OnRecvFromCompleted,
                 base::Unretained(this)));
  if (net_result != net::ERR_IO_PENDING)
    OnRecvFromCompleted(net_result);
}

void PepperUDPSocketMessageFilter::DoSendTo(
    const ppapi::host::ReplyMessageContext& context,
    const std::string& data,
    const PP_NetAddress_Private& addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(socket_.get());

  if (closed_ || !socket_.get()) {
    SendSendToError(context, PP_ERROR_FAILED);
    return;
  }

  size_t num_bytes = data.size();
  if (num_bytes == 0 ||
      num_bytes >
          static_cast<size_t>(UDPSocketResourceConstants::kMaxWriteSize)) {
    // Size of |data| is checked on the plugin side.
    NOTREACHED();
    SendSendToError(context, PP_ERROR_BADARGUMENT);
    return;
  }

  net::IPAddressBytes address;
  uint16_t port;
  if (!NetAddressPrivateImpl::NetAddressToIPEndPoint(addr, &address, &port)) {
    SendSendToError(context, PP_ERROR_ADDRESS_INVALID);
    return;
  }

  scoped_refptr<net::IOBufferWithSize> buffer(
      new net::IOBufferWithSize(num_bytes));
  memcpy(buffer->data(), data.data(), num_bytes);

  // Make sure a malicious plugin can't queue up an unlimited number of buffers.
  size_t num_pending_sends = pending_sends_.size();
  if (num_pending_sends == UDPSocketResourceConstants::kPluginSendBufferSlots) {
    SendSendToError(context, PP_ERROR_FAILED);
    return;
  }

  pending_sends_.push(
      PendingSend(net::IPAddress(address), port, buffer, context));
  // If there are other sends pending, we can't start yet.
  if (num_pending_sends)
    return;
  int net_result = StartPendingSend();
  if (net_result != net::ERR_IO_PENDING)
    FinishPendingSend(net_result);
}

int PepperUDPSocketMessageFilter::StartPendingSend() {
  DCHECK(!pending_sends_.empty());
  const PendingSend& pending_send = pending_sends_.front();
  // See OnMsgRecvFrom() for the reason why we use base::Unretained(this)
  // when calling |socket_| methods.
  int net_result = socket_->SendTo(
      pending_send.buffer.get(),
      pending_send.buffer->size(),
      net::IPEndPoint(pending_send.address, pending_send.port),
      base::Bind(&PepperUDPSocketMessageFilter::OnSendToCompleted,
                 base::Unretained(this)));
  return net_result;
}

void PepperUDPSocketMessageFilter::Close() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (socket_.get() && !closed_)
    socket_->Close();
  closed_ = true;
}

void PepperUDPSocketMessageFilter::OnRecvFromCompleted(int net_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(recvfrom_buffer_.get());

  int32_t pp_result = NetErrorToPepperError(net_result);

  // Convert IPEndPoint we get back from RecvFrom to a PP_NetAddress_Private
  // to send back.
  PP_NetAddress_Private addr = NetAddressPrivateImpl::kInvalidNetAddress;
  if (pp_result >= 0 &&
      !NetAddressPrivateImpl::IPEndPointToNetAddress(
          recvfrom_address_.address().bytes(), recvfrom_address_.port(),
          &addr)) {
    pp_result = PP_ERROR_ADDRESS_INVALID;
  }

  if (pp_result >= 0) {
    SendRecvFromResult(PP_OK, std::string(recvfrom_buffer_->data(), pp_result),
                       addr);
  } else {
    SendRecvFromError(pp_result);
  }

  recvfrom_buffer_ = nullptr;

  DCHECK_GT(remaining_recv_slots_, 0u);
  remaining_recv_slots_--;

  if (remaining_recv_slots_ > 0 && !closed_ && socket_.get())
    DoRecvFrom();
}

void PepperUDPSocketMessageFilter::OnSendToCompleted(int net_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FinishPendingSend(net_result);

  // Start pending sends until none are left or a send doesn't complete.
  while (!pending_sends_.empty()) {
    net_result = StartPendingSend();
    if (net_result == net::ERR_IO_PENDING)
      break;
    FinishPendingSend(net_result);
  }
}

void PepperUDPSocketMessageFilter::FinishPendingSend(int net_result) {
  DCHECK(!pending_sends_.empty());
  const PendingSend& pending_send = pending_sends_.front();
  int32_t pp_result = NetErrorToPepperError(net_result);
  if (pp_result < 0)
    SendSendToError(pending_send.context, pp_result);
  else
    SendSendToReply(pending_send.context, PP_OK, pp_result);

  pending_sends_.pop();
}

void PepperUDPSocketMessageFilter::SendBindReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t result,
    const PP_NetAddress_Private& addr) {
  UMA_HISTOGRAM_BOOLEAN("Pepper.PluginContextSecurity.UDPBind",
                        is_potentially_secure_plugin_context_);

  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(result);
  SendReply(reply_context, PpapiPluginMsg_UDPSocket_BindReply(addr));
}

void PepperUDPSocketMessageFilter::SendRecvFromResult(
    int32_t result,
    const std::string& data,
    const PP_NetAddress_Private& addr) {
  if (resource_host()) {
    resource_host()->host()->SendUnsolicitedReply(
        resource_host()->pp_resource(),
        PpapiPluginMsg_UDPSocket_PushRecvResult(result, data, addr));
  }
}

void PepperUDPSocketMessageFilter::SendSendToReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t result,
    int32_t bytes_written) {
  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(result);
  SendReply(reply_context, PpapiPluginMsg_UDPSocket_SendToReply(bytes_written));
}

void PepperUDPSocketMessageFilter::SendBindError(
    const ppapi::host::ReplyMessageContext& context,
    int32_t result) {
  SendBindReply(context, result, NetAddressPrivateImpl::kInvalidNetAddress);
}

void PepperUDPSocketMessageFilter::SendRecvFromError(
    int32_t result) {
  SendRecvFromResult(result, std::string(),
                     NetAddressPrivateImpl::kInvalidNetAddress);
}

void PepperUDPSocketMessageFilter::SendSendToError(
    const ppapi::host::ReplyMessageContext& context,
    int32_t result) {
  SendSendToReply(context, result, 0);
}

int32_t PepperUDPSocketMessageFilter::CanUseMulticastAPI(
    const PP_NetAddress_Private& addr) {
  // Check for plugin permissions.
  SocketPermissionRequest request =
      pepper_socket_utils::CreateSocketPermissionRequest(
          SocketPermissionRequest::UDP_MULTICAST_MEMBERSHIP, addr);
  if (!pepper_socket_utils::CanUseSocketAPIs(external_plugin_,
                                             private_api_,
                                             &request,
                                             render_process_id_,
                                             render_frame_id_)) {
    return PP_ERROR_NOACCESS;
  }

  return PP_OK;
}

}  // namespace content
