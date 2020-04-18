// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WORKER_SHADOW_PAGE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WORKER_SHADOW_PAGE_H_

#include "third_party/blink/public/web/web_document_loader.h"
#include "third_party/blink/public/web/web_frame_client.h"
#include "third_party/blink/public/web/web_view.h"
#include "third_party/blink/renderer/core/exported/web_dev_tools_agent_impl.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"

namespace blink {

class ContentSecurityPolicy;
class WebApplicationCacheHost;
class WebApplicationCacheHostClient;
class WebSettings;

// WorkerShadowPage implements the 'shadow page' concept.
//
// Loading components are strongly associated with frames, but out-of-process
// workers (i.e., SharedWorker and ServiceWorker) don't have frames. To enable
// loading on such workers, this class provides a virtual frame (a.k.a, shadow
// page) to them.
//
// WorkerShadowPage lives on the main thread.
//
// TODO(nhiroki): Move this into core/workers once all dependencies on
// core/exported are gone (now depending on core/exported/WebViewImpl.h in
// *.cpp).
// TODO(kinuko): Make this go away (https://crbug.com/538751).
class CORE_EXPORT WorkerShadowPage : public WebFrameClient {
 public:
  class CORE_EXPORT Client : public WebDevToolsAgentImpl::WorkerClient {
   public:
    ~Client() override = default;

    // Called when the shadow page is requested to create an application cache
    // host.
    virtual std::unique_ptr<WebApplicationCacheHost> CreateApplicationCacheHost(
        WebApplicationCacheHostClient*) = 0;

    // Called when Initialize() is completed.
    virtual void OnShadowPageInitialized() = 0;

    virtual const base::UnguessableToken& GetDevToolsWorkerToken() = 0;
  };

  explicit WorkerShadowPage(Client*);
  ~WorkerShadowPage() override;

  // Calls Client::OnShadowPageInitialized() when complete.
  void Initialize(const KURL& script_url);

  void SetContentSecurityPolicyAndReferrerPolicy(ContentSecurityPolicy*,
                                                 String referrer_policy);

  // WebFrameClient overrides.
  std::unique_ptr<WebApplicationCacheHost> CreateApplicationCacheHost(
      WebApplicationCacheHostClient*) override;
  // Note: usually WebFrameClient implementations override WebFrameClient to
  // call Close() on the corresponding WebLocalFrame. Shadow pages are set up a
  // bit differently and clear the WebFrameClient pointer before shutting down,
  // so the shadow page must also manually call Close() on the corresponding
  // frame and its widget.
  void DidFinishDocumentLoad() override;
  std::unique_ptr<blink::WebURLLoaderFactory> CreateURLLoaderFactory() override;
  base::UnguessableToken GetDevToolsFrameToken() override;

  // TODO(nhiroki): Remove this once the off-main-thread WebSocket is enabled by
  // default (https://crbug.com/825740).
  std::unique_ptr<WebSocketHandshakeThrottle> CreateWebSocketHandshakeThrottle()
      override;

  Document* GetDocument() { return main_frame_->GetFrame()->GetDocument(); }
  WebSettings* GetSettings() { return web_view_->GetSettings(); }
  WebDocumentLoader* DocumentLoader() {
    return main_frame_->GetDocumentLoader();
  }
  void BindDevToolsAgent(mojom::blink::DevToolsAgentAssociatedRequest);

  bool WasInitialized() const;

 private:
  enum class State { kUninitialized, kInitializing, kInitialized };
  void AdvanceState(State);

  Client* client_;
  WebView* web_view_;
  Persistent<WebLocalFrameImpl> main_frame_;

  State state_ = State::kUninitialized;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WORKER_SHADOW_PAGE_H_
