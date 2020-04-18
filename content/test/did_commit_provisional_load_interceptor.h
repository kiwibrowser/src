// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_
#define CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "content/common/frame.mojom.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class RenderFrameHost;

// Allows intercepting calls to RFHI::DidCommitProvisionalLoad just before they
// are dispatched to the implementation. This enables unit/browser tests to
// scrutinize/alter the parameters, or simulate race conditions by triggering
// other calls just before dispatching DidCommitProvisionalLoad.
class DidCommitProvisionalLoadInterceptor : public WebContentsObserver {
 public:
  // Constructs an instance that will intercept DidCommitProvisionalLoad calls
  // in any frame of the |web_contents| while the instance is in scope.
  explicit DidCommitProvisionalLoadInterceptor(WebContents* web_contents);
  ~DidCommitProvisionalLoadInterceptor() override;

  // Called just before DidCommitProvisionalLoad with |params| and
  // |interface_provider_request| would be dispatched to |render_frame_host|.
  virtual void WillDispatchDidCommitProvisionalLoad(
      RenderFrameHost* render_frame_host,
      ::FrameHostMsg_DidCommitProvisionalLoad_Params* params,
      service_manager::mojom::InterfaceProviderRequest*
          interface_provider_request) = 0;

 private:
  class FrameAgent;

  // WebContentsObserver:
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;

  std::map<RenderFrameHost*, std::unique_ptr<FrameAgent>> frame_agents_;

  DISALLOW_COPY_AND_ASSIGN(DidCommitProvisionalLoadInterceptor);
};

}  // namespace content

#endif  // CONTENT_TEST_DID_COMMIT_PROVISIONAL_LOAD_INTERCEPTOR_H_
