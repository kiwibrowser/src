// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_NETWORK_PROXY_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_NETWORK_PROXY_HOST_H_

#include <stdint.h>

#include <queue>
#include <string>

#include "base/compiler_specific.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/resource_host.h"

namespace net {
class ProxyInfo;
class URLRequestContextGetter;
}

namespace ppapi {
namespace host {
struct ReplyMessageContext;
}
}

namespace content {

class BrowserPpapiHostImpl;

// The host for PPB_NetworkProxy. This class lives on the IO thread.
class CONTENT_EXPORT PepperNetworkProxyHost : public ppapi::host::ResourceHost {
 public:
  PepperNetworkProxyHost(BrowserPpapiHostImpl* host,
                         PP_Instance instance,
                         PP_Resource resource);

  ~PepperNetworkProxyHost() override;

 private:
  // We retrieve the appropriate URLRequestContextGetter and whether this API
  // is allowed for the instance on the UI thread and pass those to
  // DidGetUIThreadData, which sets allowed_ and proxy_resolution_service_.
  struct UIThreadData {
    UIThreadData();
    UIThreadData(const UIThreadData& other);
    ~UIThreadData();
    bool is_allowed;
    scoped_refptr<net::URLRequestContextGetter> context_getter;
  };
  static UIThreadData GetUIThreadDataOnUIThread(int render_process_id,
                                                int render_frame_id,
                                                bool is_external_plugin);
  void DidGetUIThreadData(const UIThreadData&);

  // ResourceHost implementation.
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

  int32_t OnMsgGetProxyForURL(ppapi::host::HostMessageContext* context,
                              const std::string& url);

  // If we have a valid proxy_resolution_service_, send all messages in
  // unsent_requests_.
  void TryToSendUnsentRequests();

  void OnResolveProxyCompleted(ppapi::host::ReplyMessageContext context,
                               net::ProxyInfo* proxy_info,
                               int result);
  void SendFailureReply(int32_t error,
                        ppapi::host::ReplyMessageContext context);

  // The following two members are invalid until we get some information from
  // the UI thread. However, these are only ever set or accessed on the IO
  // thread.
  net::ProxyResolutionService* proxy_resolution_service_;
  bool is_allowed_;

  // True initially, but set to false once the values for
  // proxy_resolution_service_ and is_allowed_ have been set.
  bool waiting_for_ui_thread_data_;

  // We have to get the URLRequestContextGetter from the UI thread before we
  // can retrieve proxy_resolution_service_. If we receive any calls for
  // GetProxyForURL before proxy_resolution_service_ is available, we save them
  // in unsent_requests_.
  struct UnsentRequest {
    GURL url;
    ppapi::host::ReplyMessageContext reply_context;
  };
  base::queue<UnsentRequest> unsent_requests_;

  // Requests awaiting a response from ProxyResolutionService. We need to store
  // these so that we can cancel them if we get destroyed.
  base::queue<net::ProxyResolutionService::Request*> pending_requests_;

  base::WeakPtrFactory<PepperNetworkProxyHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PepperNetworkProxyHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_PEPPER_NETWORK_PROXY_HOST_H_
