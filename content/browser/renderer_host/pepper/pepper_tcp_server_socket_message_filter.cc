// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_tcp_server_socket_message_filter.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "content/browser/renderer_host/pepper/browser_ppapi_host_impl.h"
#include "content/browser/renderer_host/pepper/content_browser_pepper_host_factory.h"
#include "content/browser/renderer_host/pepper/pepper_socket_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/socket_permission_request.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_net_address_private.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/error_conversion.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/api_id.h"
#include "ppapi/shared_impl/ppb_tcp_socket_shared.h"
#include "ppapi/shared_impl/private/net_address_private_impl.h"

#if defined(OS_CHROMEOS)
#include "chromeos/network/firewall_hole.h"
#endif  // defined(OS_CHROMEOS)

using ppapi::NetAddressPrivateImpl;
using ppapi::host::NetErrorToPepperError;

namespace {

size_t g_num_instances = 0;

}  // namespace

namespace content {

PepperTCPServerSocketMessageFilter::PepperTCPServerSocketMessageFilter(
    ContentBrowserPepperHostFactory* factory,
    BrowserPpapiHostImpl* host,
    PP_Instance instance,
    bool private_api)
    : ppapi_host_(host->GetPpapiHost()),
      factory_(factory),
      instance_(instance),
      state_(STATE_BEFORE_LISTENING),
      external_plugin_(host->external_plugin()),
      private_api_(private_api),
      render_process_id_(0),
      render_frame_id_(0) {
  ++g_num_instances;
  DCHECK(factory_);
  DCHECK(ppapi_host_);
  if (!host->GetRenderFrameIDsForInstance(
          instance, &render_process_id_, &render_frame_id_)) {
    NOTREACHED();
  }
}

PepperTCPServerSocketMessageFilter::~PepperTCPServerSocketMessageFilter() {
  --g_num_instances;
}

// static
size_t PepperTCPServerSocketMessageFilter::GetNumInstances() {
  return g_num_instances;
}

scoped_refptr<base::TaskRunner>
PepperTCPServerSocketMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  switch (message.type()) {
    case PpapiHostMsg_TCPServerSocket_Listen::ID:
      return BrowserThread::GetTaskRunnerForThread(BrowserThread::UI);
    case PpapiHostMsg_TCPServerSocket_Accept::ID:
    case PpapiHostMsg_TCPServerSocket_StopListening::ID:
      return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
  }
  return nullptr;
}

int32_t PepperTCPServerSocketMessageFilter::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperTCPServerSocketMessageFilter, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TCPServerSocket_Listen,
                                      OnMsgListen)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_TCPServerSocket_Accept,
                                        OnMsgAccept)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
        PpapiHostMsg_TCPServerSocket_StopListening, OnMsgStopListening)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperTCPServerSocketMessageFilter::OnMsgListen(
    const ppapi::host::HostMessageContext* context,
    const PP_NetAddress_Private& addr,
    int32_t backlog) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(context);

  SocketPermissionRequest request =
      pepper_socket_utils::CreateSocketPermissionRequest(
          content::SocketPermissionRequest::TCP_LISTEN, addr);
  if (!pepper_socket_utils::CanUseSocketAPIs(external_plugin_,
                                             private_api_,
                                             &request,
                                             render_process_id_,
                                             render_frame_id_)) {
    return PP_ERROR_NOACCESS;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PepperTCPServerSocketMessageFilter::DoListen, this,
                     context->MakeReplyMessageContext(), addr, backlog));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPServerSocketMessageFilter::OnMsgAccept(
    const ppapi::host::HostMessageContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(context);

  if (state_ != STATE_LISTENING)
    return PP_ERROR_FAILED;

  state_ = STATE_ACCEPT_IN_PROGRESS;
  ppapi::host::ReplyMessageContext reply_context(
      context->MakeReplyMessageContext());
  int net_result = socket_->Accept(
      &accepted_socket_,
      &accepted_address_,
      base::Bind(&PepperTCPServerSocketMessageFilter::OnAcceptCompleted,
                 base::Unretained(this),
                 reply_context));
  if (net_result != net::ERR_IO_PENDING)
    OnAcceptCompleted(reply_context, net_result);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPServerSocketMessageFilter::OnMsgStopListening(
    const ppapi::host::HostMessageContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(context);

  state_ = STATE_CLOSED;
  socket_.reset();
#if defined(OS_CHROMEOS)
  firewall_hole_.reset();
#endif  // defined(OS_CHROMEOS)
  return PP_OK;
}

void PepperTCPServerSocketMessageFilter::DoListen(
    const ppapi::host::ReplyMessageContext& context,
    const PP_NetAddress_Private& addr,
    int32_t backlog) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  net::IPAddressBytes address;
  uint16_t port;
  if (state_ != STATE_BEFORE_LISTENING ||
      !NetAddressPrivateImpl::NetAddressToIPEndPoint(addr, &address, &port)) {
    SendListenError(context, PP_ERROR_FAILED);
    state_ = STATE_CLOSED;
    return;
  }

  state_ = STATE_LISTEN_IN_PROGRESS;

  socket_.reset(new net::TCPSocket(nullptr, nullptr, net::NetLogSource()));
  int net_result = net::OK;
  do {
    net::IPEndPoint ip_end_point(net::IPAddress(address), port);
    net_result = socket_->Open(ip_end_point.GetFamily());
    if (net_result != net::OK)
      break;
    net_result = socket_->SetDefaultOptionsForServer();
    if (net_result != net::OK)
      break;
    net_result = socket_->Bind(ip_end_point);
    if (net_result != net::OK)
      break;
    net_result = socket_->Listen(backlog);
  } while (false);

  if (net_result != net::ERR_IO_PENDING) {
#if defined(OS_CHROMEOS)
    OpenFirewallHole(context, net_result);
#else
    OnListenCompleted(context, net_result);
#endif
  }
}

void PepperTCPServerSocketMessageFilter::OnListenCompleted(
    const ppapi::host::ReplyMessageContext& context,
    int net_result) {
  if (state_ != STATE_LISTEN_IN_PROGRESS) {
    SendListenError(context, PP_ERROR_FAILED);
    state_ = STATE_CLOSED;
    return;
  }
  if (net_result != net::OK) {
    SendListenError(context, NetErrorToPepperError(net_result));
    state_ = STATE_BEFORE_LISTENING;
    return;
  }

  DCHECK(socket_.get());

  net::IPEndPoint end_point;
  PP_NetAddress_Private addr;

  int32_t pp_result =
      NetErrorToPepperError(socket_->GetLocalAddress(&end_point));
  if (pp_result != PP_OK) {
    SendListenError(context, pp_result);
    state_ = STATE_BEFORE_LISTENING;
    return;
  }
  if (!NetAddressPrivateImpl::IPEndPointToNetAddress(
          end_point.address().bytes(), end_point.port(), &addr)) {
    SendListenError(context, PP_ERROR_FAILED);
    state_ = STATE_BEFORE_LISTENING;
    return;
  }

  SendListenReply(context, PP_OK, addr);
  state_ = STATE_LISTENING;
}

#if defined(OS_CHROMEOS)
void PepperTCPServerSocketMessageFilter::OpenFirewallHole(
    const ppapi::host::ReplyMessageContext& context,
    int net_result) {
  if (net_result != net::OK) {
    return;
  }
  net::IPEndPoint local_addr;
  socket_->GetLocalAddress(&local_addr);
  pepper_socket_utils::FirewallHoleOpenCallback callback =
      base::Bind(&PepperTCPServerSocketMessageFilter::OnFirewallHoleOpened,
                 this, context, net_result);
  pepper_socket_utils::OpenTCPFirewallHole(local_addr, callback);
}

void PepperTCPServerSocketMessageFilter::OnFirewallHoleOpened(
    const ppapi::host::ReplyMessageContext& context,
    int32_t net_result,
    std::unique_ptr<chromeos::FirewallHole> hole) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  LOG_IF(WARNING, !hole.get()) << "Firewall hole could not be opened.";
  firewall_hole_.reset(hole.release());
  OnListenCompleted(context, net_result);
}
#endif  // defined(OS_CHROMEOS)

void PepperTCPServerSocketMessageFilter::OnAcceptCompleted(
    const ppapi::host::ReplyMessageContext& context,
    int net_result) {
  if (state_ != STATE_ACCEPT_IN_PROGRESS) {
    SendAcceptError(context, PP_ERROR_FAILED);
    state_ = STATE_CLOSED;
    return;
  }

  state_ = STATE_LISTENING;

  if (net_result != net::OK) {
    SendAcceptError(context, NetErrorToPepperError(net_result));
    return;
  }

  DCHECK(accepted_socket_.get());

  net::IPEndPoint ip_end_point_local;
  PP_NetAddress_Private local_addr = NetAddressPrivateImpl::kInvalidNetAddress;
  PP_NetAddress_Private remote_addr = NetAddressPrivateImpl::kInvalidNetAddress;

  int32_t pp_result = NetErrorToPepperError(
      accepted_socket_->GetLocalAddress(&ip_end_point_local));
  if (pp_result != PP_OK) {
    SendAcceptError(context, pp_result);
    return;
  }
  if (!NetAddressPrivateImpl::IPEndPointToNetAddress(
          ip_end_point_local.address().bytes(), ip_end_point_local.port(),
          &local_addr) ||
      !NetAddressPrivateImpl::IPEndPointToNetAddress(
          accepted_address_.address().bytes(), accepted_address_.port(),
          &remote_addr)) {
    SendAcceptError(context, PP_ERROR_FAILED);
    return;
  }

  std::unique_ptr<ppapi::host::ResourceHost> host =
      factory_->CreateAcceptedTCPSocket(instance_,
                                        ppapi::TCP_SOCKET_VERSION_PRIVATE,
                                        std::move(accepted_socket_));
  if (!host) {
    SendAcceptError(context, PP_ERROR_NOSPACE);
    return;
  }
  int pending_resource_id =
      ppapi_host_->AddPendingResourceHost(std::move(host));
  if (pending_resource_id) {
    SendAcceptReply(
        context, PP_OK, pending_resource_id, local_addr, remote_addr);
  } else {
    SendAcceptError(context, PP_ERROR_NOSPACE);
  }
}

void PepperTCPServerSocketMessageFilter::SendListenReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result,
    const PP_NetAddress_Private& local_addr) {
  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(pp_result);
  SendReply(reply_context,
            PpapiPluginMsg_TCPServerSocket_ListenReply(local_addr));
}

void PepperTCPServerSocketMessageFilter::SendListenError(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result) {
  SendListenReply(
      context, pp_result, NetAddressPrivateImpl::kInvalidNetAddress);
}

void PepperTCPServerSocketMessageFilter::SendAcceptReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result,
    int pending_resource_id,
    const PP_NetAddress_Private& local_addr,
    const PP_NetAddress_Private& remote_addr) {
  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(pp_result);
  SendReply(reply_context,
            PpapiPluginMsg_TCPServerSocket_AcceptReply(
                pending_resource_id, local_addr, remote_addr));
}

void PepperTCPServerSocketMessageFilter::SendAcceptError(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result) {
  SendAcceptReply(context,
                  pp_result,
                  0,
                  NetAddressPrivateImpl::kInvalidNetAddress,
                  NetAddressPrivateImpl::kInvalidNetAddress);
}

}  // namespace content
