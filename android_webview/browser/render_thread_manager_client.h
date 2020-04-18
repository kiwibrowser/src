// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_RENDER_THREAD_MANAGER_CLIENT_H_
#define ANDROID_WEBVIEW_BROWSER_RENDER_THREAD_MANAGER_CLIENT_H_

namespace android_webview {

class RenderThreadManagerClient {
 public:
  // Request DrawGL to be in called AwDrawGLInfo::kModeProcess type.
  // |wait_for_completion| will cause the call to block until DrawGL has
  // happened. The callback may never be made, and the mode may be promoted to
  // kModeDraw.
  virtual bool RequestInvokeGL(bool wait_for_completion) = 0;

  // Call postInvalidateOnAnimation for invalidations. This is only used to
  // synchronize draw functor destruction.
  virtual void DetachFunctorFromView() = 0;

 protected:
  virtual ~RenderThreadManagerClient() {}
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_RENDER_THREAD_MANAGER_CLIENT_H_
