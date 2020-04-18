// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_RENDERER_CONTENT_PREVIEWS_RENDER_FRAME_OBSERVER_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_RENDERER_CONTENT_PREVIEWS_RENDER_FRAME_OBSERVER_H_

#include "base/macros.h"
#include "content/public/common/previews_state.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/platform/web_url_response.h"

namespace data_reduction_proxy {

// This class is created by ChromeRenderFrameObserver, and it class manages its
// own lifetime. It deletes itself when the RenderFrame it is observing goes
// away.
class ContentPreviewsRenderFrameObserver : public content::RenderFrameObserver {
 public:
  ContentPreviewsRenderFrameObserver(content::RenderFrame* render_frame);
  ~ContentPreviewsRenderFrameObserver() override;

 private:
  friend class ContentPreviewsRenderFrameObserverTest;

  // content::RenderFrameObserver:
  void OnDestruct() override;
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_document_navigation) override;

  // Returns whether |previews_state| is consistent with data reduction
  // proxy headers found in |web_url_response| with respect to server previews.
  static bool ValidatePreviewsStateWithResponse(
      content::PreviewsState previews_state,
      const blink::WebURLResponse& web_url_response);

  DISALLOW_COPY_AND_ASSIGN(ContentPreviewsRenderFrameObserver);
};

}  // namespace data_reduction_proxy
#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_RENDERER_CONTENT_PREVIEWS_RENDER_FRAME_OBSERVER_H_
