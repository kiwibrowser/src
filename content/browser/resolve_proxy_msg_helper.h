// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RESOLVE_PROXY_MSG_HELPER_H_
#define CONTENT_BROWSER_RESOLVE_PROXY_MSG_HELPER_H_

#include <string>

#include "base/containers/circular_deque.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_message_filter.h"
#include "net/base/completion_callback.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace content {

// Responds to ChildProcessHostMsg_ResolveProxy, kicking off a ProxyResolve
// request on the IO thread using the specified proxy service.  Completion is
// notified through the delegate.  If multiple requests are started at the same
// time, they will run in FIFO order, with only 1 being outstanding at a time.
//
// When an instance of ResolveProxyMsgHelper is destroyed, it cancels any
// outstanding proxy resolve requests with the proxy service. It also deletes
// the stored IPC::Message pointers for pending requests.
//
// This object is expected to live on the IO thread.
class CONTENT_EXPORT ResolveProxyMsgHelper : public BrowserMessageFilter {
 public:
  explicit ResolveProxyMsgHelper(net::URLRequestContextGetter* getter);
  // Constructor used by unittests.
  explicit ResolveProxyMsgHelper(
      net::ProxyResolutionService* proxy_resolution_service);

  // BrowserMessageFilter implementation
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnResolveProxy(const GURL& url, IPC::Message* reply_msg);

 protected:
  // Destruction cancels the current outstanding request, and clears the
  // pending queue.
  ~ResolveProxyMsgHelper() override;

 private:
  // Callback for the ProxyResolutionService (bound to |callback_|).
  void OnResolveProxyCompleted(int result);

  // Starts the first pending request.
  void StartPendingRequest();

  // A PendingRequest is a resolve request that is in progress, or queued.
  struct PendingRequest {
   public:
    PendingRequest(const GURL& url, IPC::Message* reply_msg)
        : url(url), reply_msg(reply_msg), request(NULL) {}

    // The URL of the request.
    GURL url;

    // Data to pass back to the delegate on completion (we own it until then).
    IPC::Message* reply_msg;

    // Handle for cancelling the current request if it has started (else NULL).
    net::ProxyResolutionService::Request* request;
  };

  // Info about the current outstanding proxy request.
  net::ProxyInfo proxy_info_;

  // FIFO queue of pending requests. The first entry is always the current one.
  using PendingRequestList = base::circular_deque<PendingRequest>;
  PendingRequestList pending_requests_;

  scoped_refptr<net::URLRequestContextGetter> context_getter_;
  net::ProxyResolutionService* proxy_resolution_service_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RESOLVE_PROXY_MSG_HELPER_H_
