// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_tcp_socket_message_filter.h"

#include <cstring>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "build/build_config.h"
#include "content/browser/renderer_host/pepper/content_browser_pepper_host_factory.h"
#include "content/browser/renderer_host/pepper/pepper_socket_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/socket_permission_request.h"
#include "net/base/address_family.h"
#include "net/base/host_port_pair.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_client_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/error_conversion.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/tcp_socket_resource_constants.h"
#include "ppapi/shared_impl/private/net_address_private_impl.h"

#if defined(OS_CHROMEOS)
#include "chromeos/network/firewall_hole.h"
#endif  // defined(OS_CHROMEOS)

using ppapi::NetAddressPrivateImpl;
using ppapi::host::NetErrorToPepperError;
using ppapi::proxy::TCPSocketResourceConstants;
using ppapi::TCPSocketState;
using ppapi::TCPSocketVersion;

namespace {

size_t g_num_tcp_filter_instances = 0;

}  // namespace

namespace content {

PepperTCPSocketMessageFilter::PepperTCPSocketMessageFilter(
    ContentBrowserPepperHostFactory* factory,
    BrowserPpapiHostImpl* host,
    PP_Instance instance,
    TCPSocketVersion version)
    : version_(version),
      external_plugin_(host->external_plugin()),
      render_process_id_(0),
      render_frame_id_(0),
      host_(host),
      factory_(factory),
      instance_(instance),
      state_(TCPSocketState::INITIAL),
      end_of_file_reached_(false),
      bind_input_addr_(NetAddressPrivateImpl::kInvalidNetAddress),
      socket_options_(SOCKET_OPTION_NODELAY),
      rcvbuf_size_(0),
      sndbuf_size_(0),
      address_index_(0),
      socket_(new net::TCPSocket(nullptr, nullptr, net::NetLogSource())),
      ssl_context_helper_(host->ssl_context_helper()),
      pending_accept_(false),
      pending_read_on_unthrottle_(false),
      pending_read_net_result_(0),
      is_potentially_secure_plugin_context_(
          host->IsPotentiallySecurePluginContext(instance)) {
  DCHECK(host);
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ++g_num_tcp_filter_instances;
  host_->AddInstanceObserver(instance_, this);
  if (!host->GetRenderFrameIDsForInstance(
          instance, &render_process_id_, &render_frame_id_)) {
    NOTREACHED();
  }
}

PepperTCPSocketMessageFilter::PepperTCPSocketMessageFilter(
    BrowserPpapiHostImpl* host,
    PP_Instance instance,
    TCPSocketVersion version,
    std::unique_ptr<net::TCPSocket> socket)
    : version_(version),
      external_plugin_(host->external_plugin()),
      render_process_id_(0),
      render_frame_id_(0),
      host_(host),
      factory_(nullptr),
      instance_(instance),
      state_(TCPSocketState::CONNECTED),
      end_of_file_reached_(false),
      bind_input_addr_(NetAddressPrivateImpl::kInvalidNetAddress),
      socket_options_(SOCKET_OPTION_NODELAY),
      rcvbuf_size_(0),
      sndbuf_size_(0),
      address_index_(0),
      socket_(std::move(socket)),
      ssl_context_helper_(host->ssl_context_helper()),
      pending_accept_(false),
      pending_read_on_unthrottle_(false),
      pending_read_net_result_(0),
      is_potentially_secure_plugin_context_(
          host->IsPotentiallySecurePluginContext(instance)) {
  DCHECK(host);
  DCHECK_NE(version, ppapi::TCP_SOCKET_VERSION_1_0);
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ++g_num_tcp_filter_instances;
  host_->AddInstanceObserver(instance_, this);
  if (!host->GetRenderFrameIDsForInstance(
          instance, &render_process_id_, &render_frame_id_)) {
    NOTREACHED();
  }
}

PepperTCPSocketMessageFilter::~PepperTCPSocketMessageFilter() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (host_)
    host_->RemoveInstanceObserver(instance_, this);
  if (socket_)
    socket_->Close();
  if (ssl_socket_)
    ssl_socket_->Disconnect();
  --g_num_tcp_filter_instances;
}

// static
size_t PepperTCPSocketMessageFilter::GetNumInstances() {
  return g_num_tcp_filter_instances;
}

scoped_refptr<base::TaskRunner>
PepperTCPSocketMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  switch (message.type()) {
    case PpapiHostMsg_TCPSocket_Bind::ID:
    case PpapiHostMsg_TCPSocket_Connect::ID:
    case PpapiHostMsg_TCPSocket_ConnectWithNetAddress::ID:
    case PpapiHostMsg_TCPSocket_Listen::ID:
      return BrowserThread::GetTaskRunnerForThread(BrowserThread::UI);
    case PpapiHostMsg_TCPSocket_SSLHandshake::ID:
    case PpapiHostMsg_TCPSocket_Read::ID:
    case PpapiHostMsg_TCPSocket_Write::ID:
    case PpapiHostMsg_TCPSocket_Accept::ID:
    case PpapiHostMsg_TCPSocket_Close::ID:
    case PpapiHostMsg_TCPSocket_SetOption::ID:
      return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
  }
  return nullptr;
}

int32_t PepperTCPSocketMessageFilter::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperTCPSocketMessageFilter, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TCPSocket_Bind, OnMsgBind)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TCPSocket_Connect,
                                      OnMsgConnect)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
        PpapiHostMsg_TCPSocket_ConnectWithNetAddress,
        OnMsgConnectWithNetAddress)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TCPSocket_SSLHandshake,
                                      OnMsgSSLHandshake)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TCPSocket_Read, OnMsgRead)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TCPSocket_Write, OnMsgWrite)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TCPSocket_Listen,
                                      OnMsgListen)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_TCPSocket_Accept,
                                        OnMsgAccept)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_TCPSocket_Close,
                                        OnMsgClose)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_TCPSocket_SetOption,
                                      OnMsgSetOption)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

void PepperTCPSocketMessageFilter::OnThrottleStateChanged(bool is_throttled) {
  if (pending_read_on_unthrottle_ && !is_throttled) {
    DCHECK(read_buffer_);
    OnReadCompleted(pending_read_reply_message_context_,
                    pending_read_net_result_);
    DCHECK(!read_buffer_);
    pending_read_on_unthrottle_ = false;
  }
}

void PepperTCPSocketMessageFilter::OnHostDestroyed() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  host_->RemoveInstanceObserver(instance_, this);
  host_ = nullptr;
}

int32_t PepperTCPSocketMessageFilter::OnMsgBind(
    const ppapi::host::HostMessageContext* context,
    const PP_NetAddress_Private& net_addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // This is only supported by PPB_TCPSocket v1.1 or above.
  if (version_ != ppapi::TCP_SOCKET_VERSION_1_1_OR_ABOVE) {
    NOTREACHED();
    return PP_ERROR_NOACCESS;
  }

  if (!pepper_socket_utils::CanUseSocketAPIs(
          external_plugin_, false /* private_api */, nullptr,
          render_process_id_, render_frame_id_)) {
    return PP_ERROR_NOACCESS;
  }

  bind_input_addr_ = net_addr;

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PepperTCPSocketMessageFilter::DoBind, this,
                     context->MakeReplyMessageContext(), net_addr));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPSocketMessageFilter::OnMsgConnect(
    const ppapi::host::HostMessageContext* context,
    const std::string& host,
    uint16_t port) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // This is only supported by PPB_TCPSocket_Private.
  if (!IsPrivateAPI()) {
    NOTREACHED();
    return PP_ERROR_NOACCESS;
  }

  SocketPermissionRequest request(
      SocketPermissionRequest::TCP_CONNECT, host, port);
  if (!pepper_socket_utils::CanUseSocketAPIs(external_plugin_,
                                             true /* private_api */,
                                             &request,
                                             render_process_id_,
                                             render_frame_id_)) {
    return PP_ERROR_NOACCESS;
  }

  RenderProcessHost* render_process_host =
      RenderProcessHost::FromID(render_process_id_);
  if (!render_process_host)
    return PP_ERROR_FAILED;
  BrowserContext* browser_context = render_process_host->GetBrowserContext();
  if (!browser_context || !browser_context->GetResourceContext())
    return PP_ERROR_FAILED;

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PepperTCPSocketMessageFilter::DoConnect, this,
                     context->MakeReplyMessageContext(), host, port,
                     browser_context->GetResourceContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPSocketMessageFilter::OnMsgConnectWithNetAddress(
    const ppapi::host::HostMessageContext* context,
    const PP_NetAddress_Private& net_addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  content::SocketPermissionRequest request =
      pepper_socket_utils::CreateSocketPermissionRequest(
          content::SocketPermissionRequest::TCP_CONNECT, net_addr);
  if (!pepper_socket_utils::CanUseSocketAPIs(external_plugin_,
                                             IsPrivateAPI(),
                                             &request,
                                             render_process_id_,
                                             render_frame_id_)) {
    return PP_ERROR_NOACCESS;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PepperTCPSocketMessageFilter::DoConnectWithNetAddress,
                     this, context->MakeReplyMessageContext(), net_addr));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPSocketMessageFilter::OnMsgSSLHandshake(
    const ppapi::host::HostMessageContext* context,
    const std::string& server_name,
    uint16_t server_port,
    const std::vector<std::vector<char> >& trusted_certs,
    const std::vector<std::vector<char> >& untrusted_certs) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Allow to do SSL handshake only if currently the socket has been connected
  // and there isn't pending read or write.
  if (!state_.IsValidTransition(TCPSocketState::SSL_CONNECT) ||
      read_buffer_.get() || write_buffer_base_.get() || write_buffer_.get()) {
    return PP_ERROR_FAILED;
  }

  // TODO(raymes,rsleevi): Use trusted/untrusted certificates when connecting.
  net::IPEndPoint peer_address;
  if (socket_->GetPeerAddress(&peer_address) != net::OK)
    return PP_ERROR_FAILED;

  std::unique_ptr<net::ClientSocketHandle> handle(
      new net::ClientSocketHandle());
  handle->SetSocket(base::WrapUnique<net::StreamSocket>(
      new net::TCPClientSocket(std::move(socket_), peer_address)));
  net::ClientSocketFactory* factory =
      net::ClientSocketFactory::GetDefaultFactory();
  net::HostPortPair host_port_pair(server_name, server_port);
  net::SSLClientSocketContext ssl_context;
  ssl_context.cert_verifier = ssl_context_helper_->GetCertVerifier();
  ssl_context.transport_security_state =
      ssl_context_helper_->GetTransportSecurityState();
  ssl_context.cert_transparency_verifier =
      ssl_context_helper_->GetCertTransparencyVerifier();
  ssl_context.ct_policy_enforcer = ssl_context_helper_->GetCTPolicyEnforcer();
  ssl_socket_ = factory->CreateSSLClientSocket(
      std::move(handle), host_port_pair, ssl_context_helper_->ssl_config(),
      ssl_context);
  if (!ssl_socket_) {
    LOG(WARNING) << "Failed to create an SSL client socket.";
    state_.CompletePendingTransition(false);
    return PP_ERROR_FAILED;
  }

  state_.SetPendingTransition(TCPSocketState::SSL_CONNECT);

  const ppapi::host::ReplyMessageContext reply_context(
      context->MakeReplyMessageContext());
  int net_result = ssl_socket_->Connect(
      base::Bind(&PepperTCPSocketMessageFilter::OnSSLHandshakeCompleted,
                 base::Unretained(this),
                 reply_context));
  if (net_result != net::ERR_IO_PENDING)
    OnSSLHandshakeCompleted(reply_context, net_result);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPSocketMessageFilter::OnMsgRead(
    const ppapi::host::HostMessageContext* context,
    int32_t bytes_to_read) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!state_.IsConnected() || end_of_file_reached_)
    return PP_ERROR_FAILED;
  if (read_buffer_.get())
    return PP_ERROR_INPROGRESS;
  if (bytes_to_read <= 0 ||
      bytes_to_read > TCPSocketResourceConstants::kMaxReadSize) {
    return PP_ERROR_BADARGUMENT;
  }

  ppapi::host::ReplyMessageContext reply_context(
      context->MakeReplyMessageContext());
  read_buffer_ = new net::IOBuffer(bytes_to_read);

  int net_result = net::ERR_FAILED;
  if (socket_) {
    DCHECK_EQ(state_.state(), TCPSocketState::CONNECTED);
    net_result =
        socket_->Read(read_buffer_.get(),
                      bytes_to_read,
                      base::Bind(&PepperTCPSocketMessageFilter::OnReadCompleted,
                                 base::Unretained(this),
                                 reply_context));
  } else if (ssl_socket_) {
    DCHECK_EQ(state_.state(), TCPSocketState::SSL_CONNECTED);
    net_result = ssl_socket_->Read(
        read_buffer_.get(),
        bytes_to_read,
        base::Bind(&PepperTCPSocketMessageFilter::OnReadCompleted,
                   base::Unretained(this),
                   reply_context));
  }
  if (net_result != net::ERR_IO_PENDING)
    OnReadCompleted(reply_context, net_result);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPSocketMessageFilter::OnMsgWrite(
    const ppapi::host::HostMessageContext* context,
    const std::string& data) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!state_.IsConnected())
    return PP_ERROR_FAILED;
  if (write_buffer_base_.get() || write_buffer_.get())
    return PP_ERROR_INPROGRESS;

  size_t data_size = data.size();
  if (data_size == 0 ||
      data_size >
          static_cast<size_t>(TCPSocketResourceConstants::kMaxWriteSize)) {
    return PP_ERROR_BADARGUMENT;
  }

  write_buffer_base_ = new net::IOBuffer(data_size);
  memcpy(write_buffer_base_->data(), data.data(), data_size);
  write_buffer_ =
      new net::DrainableIOBuffer(write_buffer_base_.get(), data_size);
  DoWrite(context->MakeReplyMessageContext());
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPSocketMessageFilter::OnMsgListen(
    const ppapi::host::HostMessageContext* context,
    int32_t backlog) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // This is only supported by PPB_TCPSocket v1.1 or above.
  if (version_ != ppapi::TCP_SOCKET_VERSION_1_1_OR_ABOVE) {
    NOTREACHED();
    return PP_ERROR_NOACCESS;
  }

  content::SocketPermissionRequest request =
      pepper_socket_utils::CreateSocketPermissionRequest(
          content::SocketPermissionRequest::TCP_LISTEN, bind_input_addr_);
  if (!pepper_socket_utils::CanUseSocketAPIs(external_plugin_,
                                             false /* private_api */,
                                             &request,
                                             render_process_id_,
                                             render_frame_id_)) {
    return PP_ERROR_NOACCESS;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PepperTCPSocketMessageFilter::DoListen, this,
                     context->MakeReplyMessageContext(), backlog));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPSocketMessageFilter::OnMsgAccept(
    const ppapi::host::HostMessageContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (pending_accept_)
    return PP_ERROR_INPROGRESS;
  if (state_.state() != TCPSocketState::LISTENING)
    return PP_ERROR_FAILED;

  pending_accept_ = true;
  ppapi::host::ReplyMessageContext reply_context(
      context->MakeReplyMessageContext());
  int net_result = socket_->Accept(
      &accepted_socket_,
      &accepted_address_,
      base::Bind(&PepperTCPSocketMessageFilter::OnAcceptCompleted,
                 base::Unretained(this),
                 reply_context));
  if (net_result != net::ERR_IO_PENDING)
    OnAcceptCompleted(reply_context, net_result);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperTCPSocketMessageFilter::OnMsgClose(
    const ppapi::host::HostMessageContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (state_.state() == TCPSocketState::CLOSED)
    return PP_OK;

  state_.DoTransition(TCPSocketState::CLOSE, true);
#if defined(OS_CHROMEOS)
  // Close the firewall hole, it is no longer needed.
  firewall_hole_.reset();
#endif  // defined(OS_CHROMEOS)
  // Make sure we get no further callbacks from |socket_| or |ssl_socket_|.
  if (socket_) {
    socket_->Close();
  } else if (ssl_socket_) {
    ssl_socket_->Disconnect();
  }
  return PP_OK;
}

int32_t PepperTCPSocketMessageFilter::OnMsgSetOption(
    const ppapi::host::HostMessageContext* context,
    PP_TCPSocket_Option name,
    const ppapi::SocketOptionData& value) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  switch (name) {
    case PP_TCPSOCKET_OPTION_NO_DELAY: {
      bool boolean_value = false;
      if (!value.GetBool(&boolean_value))
        return PP_ERROR_BADARGUMENT;

      // If the socket is already connected, proxy the value to TCPSocket.
      if (state_.state() == TCPSocketState::CONNECTED)
        return socket_->SetNoDelay(boolean_value) ? PP_OK : PP_ERROR_FAILED;

      // TCPSocket instance is not yet created. So remember the value here.
      if (boolean_value) {
        socket_options_ |= SOCKET_OPTION_NODELAY;
      } else {
        socket_options_ &= ~SOCKET_OPTION_NODELAY;
      }
      return PP_OK;
    }
    case PP_TCPSOCKET_OPTION_SEND_BUFFER_SIZE: {
      int32_t integer_value = 0;
      if (!value.GetInt32(&integer_value) || integer_value <= 0 ||
          integer_value > TCPSocketResourceConstants::kMaxSendBufferSize)
        return PP_ERROR_BADARGUMENT;

      // If the socket is already connected, proxy the value to TCPSocket.
      if (state_.state() == TCPSocketState::CONNECTED) {
        return NetErrorToPepperError(
            socket_->SetSendBufferSize(integer_value));
      }

      // TCPSocket instance is not yet created. So remember the value here.
      socket_options_ |= SOCKET_OPTION_SNDBUF_SIZE;
      sndbuf_size_ = integer_value;
      return PP_OK;
    }
    case PP_TCPSOCKET_OPTION_RECV_BUFFER_SIZE: {
      int32_t integer_value = 0;
      if (!value.GetInt32(&integer_value) || integer_value <= 0 ||
          integer_value > TCPSocketResourceConstants::kMaxReceiveBufferSize)
        return PP_ERROR_BADARGUMENT;

      // If the socket is already connected, proxy the value to TCPSocket.
      if (state_.state() == TCPSocketState::CONNECTED) {
        return NetErrorToPepperError(
            socket_->SetReceiveBufferSize(integer_value));
      }

      // TCPSocket instance is not yet created. So remember the value here.
      socket_options_ |= SOCKET_OPTION_RCVBUF_SIZE;
      rcvbuf_size_ = integer_value;
      return PP_OK;
    }
    default: {
      NOTREACHED();
      return PP_ERROR_BADARGUMENT;
    }
  }
}

void PepperTCPSocketMessageFilter::DoBind(
    const ppapi::host::ReplyMessageContext& context,
    const PP_NetAddress_Private& net_addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (state_.IsPending(TCPSocketState::BIND)) {
    SendBindError(context, PP_ERROR_INPROGRESS);
    return;
  }
  if (!state_.IsValidTransition(TCPSocketState::BIND)) {
    SendBindError(context, PP_ERROR_FAILED);
    return;
  }

  int pp_result = PP_OK;
  do {
    net::IPAddressBytes address;
    uint16_t port;
    if (!NetAddressPrivateImpl::NetAddressToIPEndPoint(
            net_addr, &address, &port)) {
      pp_result = PP_ERROR_ADDRESS_INVALID;
      break;
    }
    net::IPEndPoint bind_addr(net::IPAddress(address), port);

    DCHECK(!socket_->IsValid());
    pp_result = NetErrorToPepperError(socket_->Open(bind_addr.GetFamily()));
    if (pp_result != PP_OK)
      break;

    pp_result = NetErrorToPepperError(socket_->SetDefaultOptionsForServer());
    if (pp_result != PP_OK)
      break;

    pp_result = NetErrorToPepperError(socket_->Bind(bind_addr));
    if (pp_result != PP_OK)
      break;

    net::IPEndPoint ip_end_point_local;
    pp_result =
        NetErrorToPepperError(socket_->GetLocalAddress(&ip_end_point_local));
    if (pp_result != PP_OK)
      break;

    PP_NetAddress_Private local_addr =
        NetAddressPrivateImpl::kInvalidNetAddress;
    if (!NetAddressPrivateImpl::IPEndPointToNetAddress(
            ip_end_point_local.address().bytes(), ip_end_point_local.port(),
            &local_addr)) {
      pp_result = PP_ERROR_ADDRESS_INVALID;
      break;
    }

    SendBindReply(context, PP_OK, local_addr);
    state_.DoTransition(TCPSocketState::BIND, true);
    return;
  } while (false);
  if (socket_->IsValid())
    socket_->Close();
  SendBindError(context, pp_result);
  state_.DoTransition(TCPSocketState::BIND, false);
}

void PepperTCPSocketMessageFilter::DoConnect(
    const ppapi::host::ReplyMessageContext& context,
    const std::string& host,
    uint16_t port,
    ResourceContext* resource_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!state_.IsValidTransition(TCPSocketState::CONNECT)) {
    SendConnectError(context, PP_ERROR_FAILED);
    return;
  }

  state_.SetPendingTransition(TCPSocketState::CONNECT);
  address_index_ = 0;
  address_list_.clear();
  net::HostResolver::RequestInfo request_info(net::HostPortPair(host, port));
  net::HostResolver* resolver = resource_context->GetHostResolver();
  int net_result = resolver->Resolve(
      request_info, net::DEFAULT_PRIORITY, &address_list_,
      base::Bind(&PepperTCPSocketMessageFilter::OnResolveCompleted,
                 base::Unretained(this), context),
      &request_, net::NetLogWithSource());
  if (net_result != net::ERR_IO_PENDING)
    OnResolveCompleted(context, net_result);
}

void PepperTCPSocketMessageFilter::DoConnectWithNetAddress(
    const ppapi::host::ReplyMessageContext& context,
    const PP_NetAddress_Private& net_addr) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!state_.IsValidTransition(TCPSocketState::CONNECT)) {
    SendConnectError(context, PP_ERROR_FAILED);
    return;
  }

  state_.SetPendingTransition(TCPSocketState::CONNECT);

  net::IPAddressBytes address;
  uint16_t port;
  if (!NetAddressPrivateImpl::NetAddressToIPEndPoint(
          net_addr, &address, &port)) {
    state_.CompletePendingTransition(false);
    SendConnectError(context, PP_ERROR_ADDRESS_INVALID);
    return;
  }

  // Copy the single IPEndPoint to address_list_.
  address_index_ = 0;
  address_list_.clear();
  address_list_.push_back(net::IPEndPoint(net::IPAddress(address), port));
  StartConnect(context);
}

void PepperTCPSocketMessageFilter::DoWrite(
    const ppapi::host::ReplyMessageContext& context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(write_buffer_base_.get());
  DCHECK(write_buffer_.get());
  DCHECK_GT(write_buffer_->BytesRemaining(), 0);
  DCHECK(state_.IsConnected());

  int net_result = net::ERR_FAILED;
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("pepper_tcp_socket", R"(
        semantics {
          sender: "Pepper TCP Socket"
          description:
            "Pepper plugins use this API to send and receive data over the "
            "network using TCP connections. This inteface is used by Flash and "
            "PDF viewer, and Chrome Apps which use plugins to send/receive TCP "
            "traffic (require Chrome Apps TCP socket permission)."
          trigger:
            "A request from a Pepper plugin."
          data: "Any data that the plugin sends."
          destination: OTHER
          destination_other:
            "Data can be sent to any destination."
        }
        policy {
          cookies_allowed: NO
          setting:
            "These requests cannot be disabled, but will not happen if user "
            "does not use Flash, internal PDF Viewer, or Chrome Apps that use "
            "Pepper interface."
          chrome_policy {
            DefaultPluginsSetting {
              DefaultPluginsSetting: 2
            }
          }
          chrome_policy {
            AlwaysOpenPdfExternally {
              AlwaysOpenPdfExternally: true
            }
          }
          chrome_policy {
            ExtensionInstallBlacklist {
              ExtensionInstallBlacklist: {
                entries: '*'
              }
            }
          }
        })");
  if (socket_) {
    DCHECK_EQ(state_.state(), TCPSocketState::CONNECTED);
    net_result = socket_->Write(
        write_buffer_.get(), write_buffer_->BytesRemaining(),
        base::Bind(&PepperTCPSocketMessageFilter::OnWriteCompleted,
                   base::Unretained(this), context),
        traffic_annotation);
  } else if (ssl_socket_) {
    DCHECK_EQ(state_.state(), TCPSocketState::SSL_CONNECTED);
    net_result = ssl_socket_->Write(
        write_buffer_.get(), write_buffer_->BytesRemaining(),
        base::Bind(&PepperTCPSocketMessageFilter::OnWriteCompleted,
                   base::Unretained(this), context),
        traffic_annotation);
  }
  if (net_result != net::ERR_IO_PENDING)
    OnWriteCompleted(context, net_result);
}

void PepperTCPSocketMessageFilter::DoListen(
    const ppapi::host::ReplyMessageContext& context,
    int32_t backlog) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (state_.IsPending(TCPSocketState::LISTEN)) {
    SendListenReply(context, PP_ERROR_INPROGRESS);
    return;
  }
  if (!state_.IsValidTransition(TCPSocketState::LISTEN)) {
    SendListenReply(context, PP_ERROR_FAILED);
    return;
  }

  int32_t pp_result = NetErrorToPepperError(socket_->Listen(backlog));
#if defined(OS_CHROMEOS)
  OpenFirewallHole(context, pp_result);
#else
  OnListenCompleted(context, pp_result);
#endif
}

void PepperTCPSocketMessageFilter::OnResolveCompleted(
    const ppapi::host::ReplyMessageContext& context,
    int net_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!state_.IsPending(TCPSocketState::CONNECT)) {
    DCHECK(state_.state() == TCPSocketState::CLOSED);
    SendConnectError(context, PP_ERROR_FAILED);
    return;
  }

  if (net_result != net::OK) {
    SendConnectError(context, NetErrorToPepperError(net_result));
    state_.CompletePendingTransition(false);
    return;
  }

  StartConnect(context);
}

void PepperTCPSocketMessageFilter::StartConnect(
    const ppapi::host::ReplyMessageContext& context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(state_.IsPending(TCPSocketState::CONNECT));
  DCHECK_LT(address_index_, address_list_.size());

  if (!socket_->IsValid()) {
    int net_result = socket_->Open(address_list_[address_index_].GetFamily());
    if (net_result != net::OK) {
      OnConnectCompleted(context, net_result);
      return;
    }
  }

  socket_->SetDefaultOptionsForClient();

  if (!(socket_options_ & SOCKET_OPTION_NODELAY)) {
    if (!socket_->SetNoDelay(false)) {
      OnConnectCompleted(context, net::ERR_FAILED);
      return;
    }
  }
  if (socket_options_ & SOCKET_OPTION_RCVBUF_SIZE) {
    int net_result = socket_->SetReceiveBufferSize(rcvbuf_size_);
    if (net_result != net::OK) {
      OnConnectCompleted(context, net_result);
      return;
    }
  }
  if (socket_options_ & SOCKET_OPTION_SNDBUF_SIZE) {
    int net_result = socket_->SetSendBufferSize(sndbuf_size_);
    if (net_result != net::OK) {
      OnConnectCompleted(context, net_result);
      return;
    }
  }

  int net_result = socket_->Connect(
      address_list_[address_index_],
      base::Bind(&PepperTCPSocketMessageFilter::OnConnectCompleted,
                 base::Unretained(this),
                 context));
  if (net_result != net::ERR_IO_PENDING)
    OnConnectCompleted(context, net_result);
}

void PepperTCPSocketMessageFilter::OnConnectCompleted(
    const ppapi::host::ReplyMessageContext& context,
    int net_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!state_.IsPending(TCPSocketState::CONNECT)) {
    DCHECK(state_.state() == TCPSocketState::CLOSED);
    SendConnectError(context, PP_ERROR_FAILED);
    return;
  }

  int32_t pp_result = NetErrorToPepperError(net_result);
  do {
    if (pp_result != PP_OK)
      break;

    net::IPEndPoint ip_end_point_local;
    net::IPEndPoint ip_end_point_remote;
    pp_result =
        NetErrorToPepperError(socket_->GetLocalAddress(&ip_end_point_local));
    if (pp_result != PP_OK)
      break;
    pp_result =
        NetErrorToPepperError(socket_->GetPeerAddress(&ip_end_point_remote));
    if (pp_result != PP_OK)
      break;

    PP_NetAddress_Private local_addr =
        NetAddressPrivateImpl::kInvalidNetAddress;
    PP_NetAddress_Private remote_addr =
        NetAddressPrivateImpl::kInvalidNetAddress;
    if (!NetAddressPrivateImpl::IPEndPointToNetAddress(
            ip_end_point_local.address().bytes(), ip_end_point_local.port(),
            &local_addr) ||
        !NetAddressPrivateImpl::IPEndPointToNetAddress(
            ip_end_point_remote.address().bytes(), ip_end_point_remote.port(),
            &remote_addr)) {
      pp_result = PP_ERROR_ADDRESS_INVALID;
      break;
    }

    SendConnectReply(context, PP_OK, local_addr, remote_addr);
    state_.CompletePendingTransition(true);
    return;
  } while (false);

  if (version_ == ppapi::TCP_SOCKET_VERSION_1_1_OR_ABOVE) {
    DCHECK_EQ(1u, address_list_.size());

    SendConnectError(context, pp_result);
    state_.CompletePendingTransition(false);
  } else {
    // We have to recreate |socket_| because it doesn't allow a second connect
    // attempt. We won't lose any state such as bound address or set options,
    // because in the private or v1.0 API, connect must be the first operation.
    socket_.reset(new net::TCPSocket(nullptr, nullptr, net::NetLogSource()));

    if (address_index_ + 1 < address_list_.size()) {
      DCHECK_EQ(version_, ppapi::TCP_SOCKET_VERSION_PRIVATE);
      address_index_++;
      StartConnect(context);
    } else {
      SendConnectError(context, pp_result);
      // In order to maintain backward compatibility, allow further attempts to
      // connect the socket.
      state_ = TCPSocketState(TCPSocketState::INITIAL);
    }
  }
}

void PepperTCPSocketMessageFilter::OnSSLHandshakeCompleted(
    const ppapi::host::ReplyMessageContext& context,
    int net_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!state_.IsPending(TCPSocketState::SSL_CONNECT)) {
    DCHECK(state_.state() == TCPSocketState::CLOSED);
    SendSSLHandshakeReply(context, PP_ERROR_FAILED);
    return;
  }

  SendSSLHandshakeReply(context, NetErrorToPepperError(net_result));
  state_.CompletePendingTransition(net_result == net::OK);
}

void PepperTCPSocketMessageFilter::OnReadCompleted(
    const ppapi::host::ReplyMessageContext& context,
    int net_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(read_buffer_.get());

  if (host_ && host_->IsThrottled(instance_)) {
    pending_read_on_unthrottle_ = true;
    pending_read_reply_message_context_ = context;
    pending_read_net_result_ = net_result;
    return;
  }

  if (net_result > 0) {
    SendReadReply(
        context, PP_OK, std::string(read_buffer_->data(), net_result));
  } else if (net_result == 0) {
    end_of_file_reached_ = true;
    SendReadReply(context, PP_OK, std::string());
  } else {
    SendReadError(context, NetErrorToPepperError(net_result));
  }
  read_buffer_ = nullptr;
}

void PepperTCPSocketMessageFilter::OnWriteCompleted(
    const ppapi::host::ReplyMessageContext& context,
    int net_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(write_buffer_base_.get());
  DCHECK(write_buffer_.get());

  // Note: For partial writes of 0 bytes, don't continue writing to avoid a
  // likely infinite loop.
  if (net_result > 0) {
    write_buffer_->DidConsume(net_result);
    if (write_buffer_->BytesRemaining() > 0 && state_.IsConnected()) {
      DoWrite(context);
      return;
    }
  }

  if (net_result >= 0)
    SendWriteReply(context, write_buffer_->BytesConsumed());
  else
    SendWriteReply(context, NetErrorToPepperError(net_result));

  write_buffer_ = nullptr;
  write_buffer_base_ = nullptr;
}

void PepperTCPSocketMessageFilter::OnListenCompleted(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  SendListenReply(context, pp_result);
  state_.DoTransition(TCPSocketState::LISTEN, pp_result == PP_OK);
}

#if defined(OS_CHROMEOS)
void PepperTCPSocketMessageFilter::OpenFirewallHole(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result) {
  if (pp_result != PP_OK) {
    return;
  }

  net::IPEndPoint local_addr;
  // Has already been called successfully in DoBind().
  socket_->GetLocalAddress(&local_addr);
  pepper_socket_utils::FirewallHoleOpenCallback callback =
      base::Bind(&PepperTCPSocketMessageFilter::OnFirewallHoleOpened, this,
                 context, pp_result);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&pepper_socket_utils::OpenTCPFirewallHole, local_addr,
                     callback));
}

void PepperTCPSocketMessageFilter::OnFirewallHoleOpened(
    const ppapi::host::ReplyMessageContext& context,
    int32_t result,
    std::unique_ptr<chromeos::FirewallHole> hole) {
  LOG_IF(WARNING, !hole.get()) << "Firewall hole could not be opened.";
  firewall_hole_.reset(hole.release());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PepperTCPSocketMessageFilter::OnListenCompleted, this,
                     context, result));
}
#endif  // defined(OS_CHROMEOS)

void PepperTCPSocketMessageFilter::OnAcceptCompleted(
    const ppapi::host::ReplyMessageContext& context,
    int net_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(pending_accept_);

  pending_accept_ = false;

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
    SendAcceptError(context, PP_ERROR_ADDRESS_INVALID);
    return;
  }

  // |factory_| is guaranteed to be non-NULL here. Only those instances created
  // in CONNECTED state have a NULL |factory_|, while getting here requires
  // LISTENING state.
  std::unique_ptr<ppapi::host::ResourceHost> host =
      factory_->CreateAcceptedTCPSocket(instance_, version_,
                                        std::move(accepted_socket_));
  if (!host) {
    SendAcceptError(context, PP_ERROR_NOSPACE);
    return;
  }
  int pending_host_id =
      host_->GetPpapiHost()->AddPendingResourceHost(std::move(host));
  if (pending_host_id)
    SendAcceptReply(context, PP_OK, pending_host_id, local_addr, remote_addr);
  else
    SendAcceptError(context, PP_ERROR_NOSPACE);
}

void PepperTCPSocketMessageFilter::SendBindReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result,
    const PP_NetAddress_Private& local_addr) {
  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(pp_result);
  SendReply(reply_context, PpapiPluginMsg_TCPSocket_BindReply(local_addr));
}

void PepperTCPSocketMessageFilter::SendBindError(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_error) {
  SendBindReply(context, pp_error, NetAddressPrivateImpl::kInvalidNetAddress);
}

void PepperTCPSocketMessageFilter::SendConnectReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result,
    const PP_NetAddress_Private& local_addr,
    const PP_NetAddress_Private& remote_addr) {
  UMA_HISTOGRAM_BOOLEAN("Pepper.PluginContextSecurity.TCPConnect",
                        is_potentially_secure_plugin_context_);

  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(pp_result);
  SendReply(reply_context,
            PpapiPluginMsg_TCPSocket_ConnectReply(local_addr, remote_addr));
}

void PepperTCPSocketMessageFilter::SendConnectError(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_error) {
  SendConnectReply(context,
                   pp_error,
                   NetAddressPrivateImpl::kInvalidNetAddress,
                   NetAddressPrivateImpl::kInvalidNetAddress);
}

void PepperTCPSocketMessageFilter::SendSSLHandshakeReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result) {
  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(pp_result);
  ppapi::PPB_X509Certificate_Fields certificate_fields;
  if (pp_result == PP_OK) {
    // Our socket is guaranteed to be an SSL socket if we get here.
    net::SSLInfo ssl_info;
    ssl_socket_->GetSSLInfo(&ssl_info);
    if (ssl_info.cert.get()) {
      pepper_socket_utils::GetCertificateFields(*ssl_info.cert.get(),
                                                &certificate_fields);
    }
  }
  SendReply(reply_context,
            PpapiPluginMsg_TCPSocket_SSLHandshakeReply(certificate_fields));
}

void PepperTCPSocketMessageFilter::SendReadReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result,
    const std::string& data) {
  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(pp_result);
  SendReply(reply_context, PpapiPluginMsg_TCPSocket_ReadReply(data));
}

void PepperTCPSocketMessageFilter::SendReadError(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_error) {
  SendReadReply(context, pp_error, std::string());
}

void PepperTCPSocketMessageFilter::SendWriteReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result) {
  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(pp_result);
  SendReply(reply_context, PpapiPluginMsg_TCPSocket_WriteReply());
}

void PepperTCPSocketMessageFilter::SendListenReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result) {
  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(pp_result);
  SendReply(reply_context, PpapiPluginMsg_TCPSocket_ListenReply());
}

void PepperTCPSocketMessageFilter::SendAcceptReply(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_result,
    int pending_host_id,
    const PP_NetAddress_Private& local_addr,
    const PP_NetAddress_Private& remote_addr) {
  ppapi::host::ReplyMessageContext reply_context(context);
  reply_context.params.set_result(pp_result);
  SendReply(reply_context,
            PpapiPluginMsg_TCPSocket_AcceptReply(
                pending_host_id, local_addr, remote_addr));
}

void PepperTCPSocketMessageFilter::SendAcceptError(
    const ppapi::host::ReplyMessageContext& context,
    int32_t pp_error) {
  SendAcceptReply(context,
                  pp_error,
                  0,
                  NetAddressPrivateImpl::kInvalidNetAddress,
                  NetAddressPrivateImpl::kInvalidNetAddress);
}

}  // namespace content
