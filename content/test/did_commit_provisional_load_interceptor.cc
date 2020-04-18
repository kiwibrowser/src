// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/did_commit_provisional_load_interceptor.h"

#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/common/frame_messages.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

// Responsible for intercepting DidCommitProvisionalLoad's being disptached to
// a given RenderFrameHostImpl.
class DidCommitProvisionalLoadInterceptor::FrameAgent
    : public mojom::FrameHostInterceptorForTesting {
 public:
  FrameAgent(DidCommitProvisionalLoadInterceptor* interceptor,
             RenderFrameHost* rfh)
      : interceptor_(interceptor),
        rfhi_(static_cast<RenderFrameHostImpl*>(rfh)),
        impl_(binding().SwapImplForTesting(this)) {}

  ~FrameAgent() override {
    auto* old_impl = binding().SwapImplForTesting(impl_);
    // TODO(https://crbug.com/729021): Investigate the scenario where
    // |old_impl| can be nullptr if the renderer process is killed.
    DCHECK_EQ(this, old_impl);
  }

 protected:
  mojo::AssociatedBinding<mojom::FrameHost>& binding() {
    return rfhi_->frame_host_binding_for_testing();
  }

  // mojom::FrameHostInterceptorForTesting:
  FrameHost* GetForwardingInterface() override { return impl_; }
  void DidCommitProvisionalLoad(
      std::unique_ptr<::FrameHostMsg_DidCommitProvisionalLoad_Params> params,
      ::service_manager::mojom::InterfaceProviderRequest
          interface_provider_request) override {
    interceptor_->WillDispatchDidCommitProvisionalLoad(
        rfhi_, params.get(), &interface_provider_request);
    GetForwardingInterface()->DidCommitProvisionalLoad(
        std::move(params), std::move(interface_provider_request));
  }

 private:
  DidCommitProvisionalLoadInterceptor* interceptor_;

  RenderFrameHostImpl* rfhi_;
  mojom::FrameHost* impl_;

  DISALLOW_COPY_AND_ASSIGN(FrameAgent);
};

DidCommitProvisionalLoadInterceptor::DidCommitProvisionalLoadInterceptor(
    WebContents* web_contents)
    : WebContentsObserver(web_contents) {
  for (auto* rfh : web_contents->GetAllFrames()) {
    if (rfh->IsRenderFrameLive())
      RenderFrameCreated(rfh);
  }
}

DidCommitProvisionalLoadInterceptor::~DidCommitProvisionalLoadInterceptor() =
    default;

void DidCommitProvisionalLoadInterceptor::RenderFrameCreated(
    RenderFrameHost* render_frame_host) {
  bool did_insert;
  std::tie(std::ignore, did_insert) = frame_agents_.emplace(
      render_frame_host, std::make_unique<FrameAgent>(this, render_frame_host));
  DCHECK(did_insert);
}

void DidCommitProvisionalLoadInterceptor::RenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  bool did_remove = !!frame_agents_.erase(render_frame_host);
  DCHECK(did_remove);
}

}  // namespace content
