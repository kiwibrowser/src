// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_RENDER_PROCESS_GONE_DELEGATE_H_
#define ANDROID_WEBVIEW_BROWSER_AW_RENDER_PROCESS_GONE_DELEGATE_H_

namespace content {
class WebContents;
}

namespace android_webview {

// Delegate interface to handle the events that render process was gone.
class AwRenderProcessGoneDelegate {
 public:
  // Returns the AwRenderProcessGoneDelegate instance associated with
  // the given |web_contents|.
  static AwRenderProcessGoneDelegate* FromWebContents(
      content::WebContents* web_contents);

  // Notify render process's termination is detected.
  virtual void OnRenderProcessGone(int child_process_id) = 0;

  // Notify if render process crashed or was killed, this callback is
  // invoked after OnRenderProcessGone() because the detail information
  // can't be obtained immediately when render process's termination is
  // detected.
  virtual bool OnRenderProcessGoneDetail(int child_process_id,
                                         bool crashed) = 0;

 protected:
  AwRenderProcessGoneDelegate() {}
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_RENDER_PROCESS_GONE_DELEGATE_H_
