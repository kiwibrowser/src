// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_network_proxy_host.h"

#include "base/bind.h"
#include "content/browser/renderer_host/pepper/browser_ppapi_host_impl.h"
#include "content/browser/renderer_host/pepper/pepper_socket_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/socket_permission_request.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace content {

PepperNetworkProxyHost::PepperNetworkProxyHost(BrowserPpapiHostImpl* host,
                                               PP_Instance instance,
                                               PP_Resource resource)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      proxy_resolution_service_(nullptr),
      is_allowed_(false),
      waiting_for_ui_thread_data_(true),
      weak_factory_(this) {
  int render_process_id(0), render_frame_id(0);
  host->GetRenderFrameIDsForInstance(
      instance, &render_process_id, &render_frame_id);
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&GetUIThreadDataOnUIThread,
                 render_process_id,
                 render_frame_id,
                 host->external_plugin()),
      base::Bind(&PepperNetworkProxyHost::DidGetUIThreadData,
                 weak_factory_.GetWeakPtr()));
}

PepperNetworkProxyHost::~PepperNetworkProxyHost() {
  while (!pending_requests_.empty()) {
    // If the proxy_resolution_service_ is NULL, we shouldn't have any outstanding
    // requests.
    DCHECK(proxy_resolution_service_);
    net::ProxyResolutionService::Request* request = pending_requests_.front();
    proxy_resolution_service_->CancelRequest(request);
    pending_requests_.pop();
  }
}

PepperNetworkProxyHost::UIThreadData::UIThreadData() : is_allowed(false) {}

PepperNetworkProxyHost::UIThreadData::UIThreadData(const UIThreadData& other) =
    default;

PepperNetworkProxyHost::UIThreadData::~UIThreadData() {}

// static
PepperNetworkProxyHost::UIThreadData
PepperNetworkProxyHost::GetUIThreadDataOnUIThread(int render_process_id,
                                                  int render_frame_id,
                                                  bool is_external_plugin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  PepperNetworkProxyHost::UIThreadData result;
  RenderProcessHost* rph = RenderProcessHost::FromID(render_process_id);
  if (rph)
    result.context_getter = rph->GetStoragePartition()->GetURLRequestContext();

  SocketPermissionRequest request(
      content::SocketPermissionRequest::RESOLVE_PROXY, std::string(), 0);
  result.is_allowed =
      pepper_socket_utils::CanUseSocketAPIs(is_external_plugin,
                                            false /* is_private_api */,
                                            &request,
                                            render_process_id,
                                            render_frame_id);
  return result;
}

void PepperNetworkProxyHost::DidGetUIThreadData(
    const UIThreadData& ui_thread_data) {
  is_allowed_ = ui_thread_data.is_allowed;
  if (ui_thread_data.context_getter.get() &&
      ui_thread_data.context_getter->GetURLRequestContext()) {
    proxy_resolution_service_ =
        ui_thread_data.context_getter->GetURLRequestContext()->proxy_resolution_service();
  }
  waiting_for_ui_thread_data_ = false;
  if (!proxy_resolution_service_) {
    DLOG_IF(WARNING, proxy_resolution_service_)
        << "Failed to find a ProxyResolutionService for Pepper plugin.";
  }
  TryToSendUnsentRequests();
}

int32_t PepperNetworkProxyHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperNetworkProxyHost, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_NetworkProxy_GetProxyForURL,
                                      OnMsgGetProxyForURL)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t PepperNetworkProxyHost::OnMsgGetProxyForURL(
    ppapi::host::HostMessageContext* context,
    const std::string& url) {
  GURL gurl(url);
  if (gurl.is_valid()) {
    UnsentRequest request = {gurl, context->MakeReplyMessageContext()};
    unsent_requests_.push(request);
    TryToSendUnsentRequests();
  } else {
    SendFailureReply(PP_ERROR_BADARGUMENT, context->MakeReplyMessageContext());
  }
  return PP_OK_COMPLETIONPENDING;
}

void PepperNetworkProxyHost::TryToSendUnsentRequests() {
  if (waiting_for_ui_thread_data_)
    return;

  while (!unsent_requests_.empty()) {
    const UnsentRequest& request = unsent_requests_.front();
    if (!proxy_resolution_service_) {
      SendFailureReply(PP_ERROR_FAILED, request.reply_context);
    } else if (!is_allowed_) {
      SendFailureReply(PP_ERROR_NOACCESS, request.reply_context);
    } else {
      // Everything looks valid, so try to resolve the proxy.
      net::ProxyInfo* proxy_info = new net::ProxyInfo;
      net::ProxyResolutionService::Request* pending_request = nullptr;
      base::Callback<void(int)> callback =
          base::Bind(&PepperNetworkProxyHost::OnResolveProxyCompleted,
                     weak_factory_.GetWeakPtr(),
                     request.reply_context,
                     base::Owned(proxy_info));
      int result = proxy_resolution_service_->ResolveProxy(
          request.url, std::string(), proxy_info, callback, &pending_request,
          nullptr, net::NetLogWithSource());
      pending_requests_.push(pending_request);
      // If it was handled synchronously, we must run the callback now;
      // proxy_resolution_service_ won't run it for us in this case.
      if (result != net::ERR_IO_PENDING)
        std::move(callback).Run(result);
    }
    unsent_requests_.pop();
  }
}

void PepperNetworkProxyHost::OnResolveProxyCompleted(
    ppapi::host::ReplyMessageContext context,
    net::ProxyInfo* proxy_info,
    int result) {
  pending_requests_.pop();

  if (result != net::OK) {
    // Currently, the only proxy-specific error we could get is
    // MANDATORY_PROXY_CONFIGURATION_FAILED. There's really no action a plugin
    // can take in this case, so there's no need to distinguish it from other
    // failures.
    context.params.set_result(PP_ERROR_FAILED);
  }
  host()->SendReply(context,
                    PpapiPluginMsg_NetworkProxy_GetProxyForURLReply(
                        proxy_info->ToPacString()));
}

void PepperNetworkProxyHost::SendFailureReply(
    int32_t error,
    ppapi::host::ReplyMessageContext context) {
  context.params.set_result(error);
  host()->SendReply(
      context, PpapiPluginMsg_NetworkProxy_GetProxyForURLReply(std::string()));
}

}  // namespace content
