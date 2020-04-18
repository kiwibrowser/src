// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/resolve_proxy_msg_helper.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "content/common/view_messages.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_with_source.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace content {

ResolveProxyMsgHelper::ResolveProxyMsgHelper(
    net::URLRequestContextGetter* getter)
    : BrowserMessageFilter(ViewMsgStart),
      context_getter_(getter),
      proxy_resolution_service_(nullptr) {}

ResolveProxyMsgHelper::ResolveProxyMsgHelper(
    net::ProxyResolutionService* proxy_resolution_service)
    : BrowserMessageFilter(ViewMsgStart), proxy_resolution_service_(proxy_resolution_service) {}

bool ResolveProxyMsgHelper::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ResolveProxyMsgHelper, message)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ViewHostMsg_ResolveProxy, OnResolveProxy)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ResolveProxyMsgHelper::OnResolveProxy(const GURL& url,
                                           IPC::Message* reply_msg) {
  // Enqueue the pending request.
  pending_requests_.push_back(PendingRequest(url, reply_msg));

  // If nothing is in progress, start.
  if (pending_requests_.size() == 1)
    StartPendingRequest();
}

ResolveProxyMsgHelper::~ResolveProxyMsgHelper() {
  // Clear all pending requests if the ProxyResolutionService is still alive (if
  // we have a default request context or override).
  if (!pending_requests_.empty()) {
    PendingRequest req = pending_requests_.front();
    proxy_resolution_service_->CancelRequest(req.request);
  }

  for (PendingRequestList::iterator it = pending_requests_.begin();
       it != pending_requests_.end();
       ++it) {
    delete it->reply_msg;
  }

  pending_requests_.clear();
}

void ResolveProxyMsgHelper::OnResolveProxyCompleted(int result) {
  CHECK(!pending_requests_.empty());

  const PendingRequest& completed_req = pending_requests_.front();
  ViewHostMsg_ResolveProxy::WriteReplyParams(
      completed_req.reply_msg, result == net::OK, proxy_info_.ToPacString());
  Send(completed_req.reply_msg);

  // Clear the current (completed) request.
  pending_requests_.pop_front();

  // Start the next request.
  if (!pending_requests_.empty())
    StartPendingRequest();
}

void ResolveProxyMsgHelper::StartPendingRequest() {
  PendingRequest& req = pending_requests_.front();

  // Verify the request wasn't started yet.
  DCHECK(nullptr == req.request);

  if (context_getter_.get()) {
    proxy_resolution_service_ = context_getter_->GetURLRequestContext()->proxy_resolution_service();
    context_getter_ = nullptr;
  }

  // Start the request.
  int result = proxy_resolution_service_->ResolveProxy(
      req.url, std::string(), &proxy_info_,
      base::Bind(&ResolveProxyMsgHelper::OnResolveProxyCompleted,
                 base::Unretained(this)),
      &req.request, nullptr, net::NetLogWithSource());

  // Completed synchronously.
  if (result != net::ERR_IO_PENDING)
    OnResolveProxyCompleted(result);
}

}  // namespace content
