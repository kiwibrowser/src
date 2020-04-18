// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_host_resolver_message_filter.h"

#include <stddef.h>

#include <memory>

#include "base/logging.h"
#include "content/browser/renderer_host/pepper/browser_ppapi_host_impl.h"
#include "content/browser/renderer_host/pepper/pepper_lookup_request.h"
#include "content/browser/renderer_host/pepper/pepper_socket_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/common/socket_permission_request.h"
#include "net/base/address_list.h"
#include "net/dns/host_resolver.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_host_resolver_private.h"
#include "ppapi/c/private/ppb_net_address_private.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/error_conversion.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/private/net_address_private_impl.h"

using ppapi::host::NetErrorToPepperError;
using ppapi::host::ReplyMessageContext;

namespace content {

namespace {

void PrepareRequestInfo(const PP_HostResolver_Private_Hint& hint,
                        net::HostResolver::RequestInfo* request_info) {
  DCHECK(request_info);

  net::AddressFamily address_family;
  switch (hint.family) {
    case PP_NETADDRESSFAMILY_PRIVATE_IPV4:
      address_family = net::ADDRESS_FAMILY_IPV4;
      break;
    case PP_NETADDRESSFAMILY_PRIVATE_IPV6:
      address_family = net::ADDRESS_FAMILY_IPV6;
      break;
    default:
      address_family = net::ADDRESS_FAMILY_UNSPECIFIED;
  }
  request_info->set_address_family(address_family);

  net::HostResolverFlags host_resolver_flags = 0;
  if (hint.flags & PP_HOST_RESOLVER_PRIVATE_FLAGS_CANONNAME)
    host_resolver_flags |= net::HOST_RESOLVER_CANONNAME;
  if (hint.flags & PP_HOST_RESOLVER_PRIVATE_FLAGS_LOOPBACK_ONLY)
    host_resolver_flags |= net::HOST_RESOLVER_LOOPBACK_ONLY;
  request_info->set_host_resolver_flags(host_resolver_flags);
}

void CreateNetAddressListFromAddressList(
    const net::AddressList& list,
    std::vector<PP_NetAddress_Private>* net_address_list) {
  DCHECK(net_address_list);

  net_address_list->clear();
  net_address_list->reserve(list.size());

  PP_NetAddress_Private address;
  for (size_t i = 0; i < list.size(); ++i) {
    if (!ppapi::NetAddressPrivateImpl::IPEndPointToNetAddress(
            list[i].address().bytes(), list[i].port(), &address)) {
      net_address_list->clear();
      return;
    }
    net_address_list->push_back(address);
  }
}

}  // namespace

PepperHostResolverMessageFilter::PepperHostResolverMessageFilter(
    BrowserPpapiHostImpl* host,
    PP_Instance instance,
    bool private_api)
    : external_plugin_(host->external_plugin()),
      private_api_(private_api),
      render_process_id_(0),
      render_frame_id_(0) {
  DCHECK(host);

  if (!host->GetRenderFrameIDsForInstance(
          instance, &render_process_id_, &render_frame_id_)) {
    NOTREACHED();
  }
}

PepperHostResolverMessageFilter::~PepperHostResolverMessageFilter() {}

scoped_refptr<base::TaskRunner>
PepperHostResolverMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  if (message.type() == PpapiHostMsg_HostResolver_Resolve::ID)
    return BrowserThread::GetTaskRunnerForThread(BrowserThread::UI);
  return nullptr;
}

int32_t PepperHostResolverMessageFilter::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperHostResolverMessageFilter, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_HostResolver_Resolve,
                                      OnMsgResolve)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperHostResolverMessageFilter::OnMsgResolve(
    const ppapi::host::HostMessageContext* context,
    const ppapi::HostPortPair& host_port,
    const PP_HostResolver_Private_Hint& hint) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Check plugin permissions.
  SocketPermissionRequest request(
      SocketPermissionRequest::RESOLVE_HOST, host_port.host, host_port.port);
  if (!pepper_socket_utils::CanUseSocketAPIs(external_plugin_,
                                             private_api_,
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
      base::BindOnce(&PepperHostResolverMessageFilter::DoResolve, this,
                     context->MakeReplyMessageContext(), host_port, hint,
                     browser_context->GetResourceContext()));
  return PP_OK_COMPLETIONPENDING;
}

void PepperHostResolverMessageFilter::DoResolve(
    const ReplyMessageContext& context,
    const ppapi::HostPortPair& host_port,
    const PP_HostResolver_Private_Hint& hint,
    ResourceContext* resource_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  net::HostResolver* host_resolver = resource_context->GetHostResolver();
  if (!host_resolver) {
    SendResolveError(PP_ERROR_FAILED, context);
    return;
  }

  net::HostResolver::RequestInfo request_info(
      net::HostPortPair(host_port.host, host_port.port));
  PrepareRequestInfo(hint, &request_info);

  std::unique_ptr<ReplyMessageContext> bound_info(
      new ReplyMessageContext(context));

  // The lookup request will delete itself on completion.
  PepperLookupRequest<ReplyMessageContext>* lookup_request =
      new PepperLookupRequest<ReplyMessageContext>(
          host_resolver,
          request_info,
          net::DEFAULT_PRIORITY,
          bound_info.release(),
          base::Bind(&PepperHostResolverMessageFilter::OnLookupFinished, this));
  lookup_request->Start();
}

void PepperHostResolverMessageFilter::OnLookupFinished(
    int net_result,
    const net::AddressList& addresses,
    const ReplyMessageContext& context) {
  if (net_result != net::OK) {
    SendResolveError(NetErrorToPepperError(net_result), context);
  } else {
    const std::string& canonical_name = addresses.canonical_name();
    NetAddressList net_address_list;
    CreateNetAddressListFromAddressList(addresses, &net_address_list);
    if (net_address_list.empty())
      SendResolveError(PP_ERROR_FAILED, context);
    else
      SendResolveReply(PP_OK, canonical_name, net_address_list, context);
  }
}

void PepperHostResolverMessageFilter::SendResolveReply(
    int32_t result,
    const std::string& canonical_name,
    const NetAddressList& net_address_list,
    const ReplyMessageContext& context) {
  ReplyMessageContext reply_context = context;
  reply_context.params.set_result(result);
  SendReply(reply_context,
            PpapiPluginMsg_HostResolver_ResolveReply(canonical_name,
                                                     net_address_list));
}

void PepperHostResolverMessageFilter::SendResolveError(
    int32_t error,
    const ReplyMessageContext& context) {
  SendResolveReply(error, std::string(), NetAddressList(), context);
}

}  // namespace content
