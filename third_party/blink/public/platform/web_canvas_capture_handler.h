// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_CANVAS_CAPTURE_HANDLER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_CANVAS_CAPTURE_HANDLER_H_

#include "third_party/blink/public/platform/web_common.h"

#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace blink {

class WebGraphicsContext3DProvider;

// Platform interface of a CanvasCaptureHandler.
class BLINK_PLATFORM_EXPORT WebCanvasCaptureHandler {
 public:
  virtual ~WebCanvasCaptureHandler() = default;
  virtual void SendNewFrame(sk_sp<SkImage>,
                            blink::WebGraphicsContext3DProvider*) {}
  virtual bool NeedsNewFrame() const { return false; }
};

}  // namespace blink

#endif  // WebMediaRecorderHandler_h
