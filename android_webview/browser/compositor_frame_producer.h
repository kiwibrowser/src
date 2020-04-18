// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_COMPOSITOR_FRAME_PRODUCER_H_
#define ANDROID_WEBVIEW_BROWSER_COMPOSITOR_FRAME_PRODUCER_H_

namespace android_webview {

class CompositorFrameConsumer;

class CompositorFrameProducer {
 public:
  virtual void OnParentDrawConstraintsUpdated(
      CompositorFrameConsumer* compositor_frame_consumer) = 0;
  virtual void RemoveCompositorFrameConsumer(
      CompositorFrameConsumer* compositor_frame_consumer) = 0;

 protected:
  virtual ~CompositorFrameProducer() {}
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_COMPOSITOR_FRAME_PRODUCER_H_
