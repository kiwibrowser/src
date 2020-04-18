// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_RENDERER_AW_RENDER_THREAD_OBSERVER_H_
#define ANDROID_WEBVIEW_RENDERER_AW_RENDER_THREAD_OBSERVER_H_

#include "content/public/renderer/render_thread_observer.h"

#include "base/compiler_specific.h"

namespace android_webview {

// A RenderThreadObserver implementation used for handling android_webview
// specific render-process wide IPC messages.
class AwRenderThreadObserver : public content::RenderThreadObserver {
 public:
  AwRenderThreadObserver();
  ~AwRenderThreadObserver() override;

  // content::RenderThreadObserver implementation.
  bool OnControlMessageReceived(const IPC::Message& message) override;

 private:
  void OnClearCache();
  void OnKillProcess();
  void OnSetJsOnlineProperty(bool network_up);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_RENDERER_AW_RENDER_THREAD_OBSERVER_H_

