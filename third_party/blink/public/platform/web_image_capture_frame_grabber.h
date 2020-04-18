// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_IMAGE_CAPTURE_FRAME_GRABBER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_IMAGE_CAPTURE_FRAME_GRABBER_H_

#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/skia/include/core/SkRefCnt.h"

class SkImage;

namespace blink {

class WebMediaStreamTrack;

using WebImageCaptureGrabFrameCallbacks = WebCallbacks<sk_sp<SkImage>, void>;

// Platform interface of an ImageCapture class for GrabFrame() calls.
class WebImageCaptureFrameGrabber {
 public:
  virtual ~WebImageCaptureFrameGrabber() = default;

  virtual void GrabFrame(WebMediaStreamTrack*,
                         WebImageCaptureGrabFrameCallbacks*) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_IMAGE_CAPTURE_FRAME_GRABBER_H_
