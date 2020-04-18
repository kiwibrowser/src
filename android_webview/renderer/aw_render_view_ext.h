// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_RENDERER_AW_RENDER_VIEW_EXT_H_
#define ANDROID_WEBVIEW_RENDERER_AW_RENDER_VIEW_EXT_H_

#include "base/timer/timer.h"
#include "content/public/renderer/render_view_observer.h"
#include "ui/gfx/geometry/size.h"

namespace android_webview {

// Render process side of AwRenderViewHostExt, this provides cross-process
// implementation of miscellaneous WebView functions that we need to poke
// WebKit directly to implement (and that aren't needed in the chrome app).
class AwRenderViewExt : public content::RenderViewObserver {
 public:
  static void RenderViewCreated(content::RenderView* render_view);

 private:
  AwRenderViewExt(content::RenderView* render_view);
  ~AwRenderViewExt() override;

  // RenderViewObserver:
  void DidCommitCompositorFrame() override;
  void DidUpdateLayout() override;
  void OnDestruct() override;

  void CheckContentsSize();
  void PostCheckContentsSize();

  gfx::Size last_sent_contents_size_;
  base::OneShotTimer check_contents_size_timer_;

  DISALLOW_COPY_AND_ASSIGN(AwRenderViewExt);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_RENDERER_AW_RENDER_VIEW_EXT_H_
