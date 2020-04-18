// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/exported/worker_shadow_page.h"

#include "third_party/blink/public/mojom/page/page_visibility_state.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/loader/frame_load_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/substitute_data.h"

namespace blink {

WorkerShadowPage::WorkerShadowPage(Client* client)
    : client_(client),
      web_view_(WebViewImpl::Create(nullptr,
                                    mojom::PageVisibilityState::kVisible,
                                    nullptr)),
      main_frame_(WebLocalFrameImpl::CreateMainFrame(web_view_,
                                                     this,
                                                     nullptr,
                                                     nullptr,
                                                     g_empty_atom,
                                                     WebSandboxFlags::kNone)) {
  DCHECK(IsMainThread());

  // TODO(http://crbug.com/363843): This needs to find a better way to
  // not create graphics layers.
  web_view_->GetSettings()->SetAcceleratedCompositingEnabled(false);
  // TODO(lunalu): Service worker and shared worker count feature usage on the
  // blink side use counter. Once the blink side use counter is removed
  // (crbug.com/811948), remove this instant from Settings.
  main_frame_->GetFrame()->GetSettings()->SetIsShadowPage(true);

  main_frame_->SetDevToolsAgentImpl(
      WebDevToolsAgentImpl::CreateForWorker(main_frame_, client_));
}

WorkerShadowPage::~WorkerShadowPage() {
  DCHECK(IsMainThread());
  // Detach the client before closing the view to avoid getting called back.
  main_frame_->SetClient(nullptr);
  web_view_->Close();
  main_frame_->Close();
}

void WorkerShadowPage::Initialize(const KURL& script_url) {
  DCHECK(IsMainThread());
  AdvanceState(State::kInitializing);

  // Construct substitute data source. We only need it to have same origin as
  // the worker so the loading checks work correctly.
  CString content("");
  scoped_refptr<SharedBuffer> buffer(
      SharedBuffer::Create(content.data(), content.length()));
  main_frame_->GetFrame()->Loader().CommitNavigation(FrameLoadRequest(
      nullptr, ResourceRequest(script_url), SubstituteData(buffer)));
}

void WorkerShadowPage::SetContentSecurityPolicyAndReferrerPolicy(
    ContentSecurityPolicy* content_security_policy,
    String referrer_policy) {
  DCHECK(IsMainThread());
  content_security_policy->SetOverrideURLForSelf(GetDocument()->Url());
  GetDocument()->InitContentSecurityPolicy(content_security_policy);
  if (!referrer_policy.IsNull())
    GetDocument()->ParseAndSetReferrerPolicy(referrer_policy);
}

void WorkerShadowPage::DidFinishDocumentLoad() {
  DCHECK(IsMainThread());
  AdvanceState(State::kInitialized);
  client_->OnShadowPageInitialized();
}

std::unique_ptr<WebApplicationCacheHost>
WorkerShadowPage::CreateApplicationCacheHost(
    WebApplicationCacheHostClient* appcache_host_client) {
  DCHECK(IsMainThread());
  return client_->CreateApplicationCacheHost(appcache_host_client);
}

std::unique_ptr<blink::WebURLLoaderFactory>
WorkerShadowPage::CreateURLLoaderFactory() {
  DCHECK(IsMainThread());
  return Platform::Current()->CreateDefaultURLLoaderFactory();
}

base::UnguessableToken WorkerShadowPage::GetDevToolsFrameToken() {
  // TODO(dgozman): instrumentation token will have to be passed directly to
  // DevTools once we stop using a frame for workers. Currently, we rely on
  // the frame's instrumentation token to match the worker.
  return client_->GetDevToolsWorkerToken();
}

std::unique_ptr<WebSocketHandshakeThrottle>
WorkerShadowPage::CreateWebSocketHandshakeThrottle() {
  return Platform::Current()->CreateWebSocketHandshakeThrottle();
}

bool WorkerShadowPage::WasInitialized() const {
  return state_ == State::kInitialized;
}

void WorkerShadowPage::AdvanceState(State new_state) {
  switch (new_state) {
    case State::kUninitialized:
      NOTREACHED();
      return;
    case State::kInitializing:
      DCHECK_EQ(State::kUninitialized, state_);
      state_ = new_state;
      return;
    case State::kInitialized:
      DCHECK_EQ(State::kInitializing, state_);
      state_ = new_state;
      return;
  }
}

void WorkerShadowPage::BindDevToolsAgent(
    mojom::blink::DevToolsAgentAssociatedRequest request) {
  main_frame_->DevToolsAgentImpl()->BindRequest(std::move(request));
}

}  // namespace blink
