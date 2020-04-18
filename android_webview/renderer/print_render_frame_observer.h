// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_RENDERER_PRINT_RENDER_FRAME_OBSERVER_H_
#define ANDROID_WEBVIEW_RENDERER_PRINT_RENDER_FRAME_OBSERVER_H_

#include "base/macros.h"
#include "content/public/renderer/render_frame_observer.h"

namespace android_webview {

class PrintRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit PrintRenderFrameObserver(content::RenderFrame* render_view);

 private:
  ~PrintRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnDestruct() override;

  // IPC handlers
  void OnPrintNodeUnderContextMenu();

  DISALLOW_COPY_AND_ASSIGN(PrintRenderFrameObserver);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_RENDERER_PRINT_RENDER_FRAME_OBSERVER_H_
