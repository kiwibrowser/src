// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_INSTRUMENTATION_PLATFORM_INSTRUMENTATION_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_INSTRUMENTATION_PLATFORM_INSTRUMENTATION_H_

#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class PLATFORM_EXPORT PlatformInstrumentation {
 public:
  class LazyPixelRefTracker : TraceEvent::TraceScopedTrackableObject<void*> {
   public:
    LazyPixelRefTracker(void* instance)
        : TraceEvent::TraceScopedTrackableObject<void*>(kCategoryName,
                                                        kLazyPixelRef,
                                                        instance) {}
  };

  static const char kImageDecodeEvent[];
  static const char kImageResizeEvent[];
  static const char kDrawLazyPixelRefEvent[];
  static const char kDecodeLazyPixelRefEvent[];

  static const char kImageTypeArgument[];
  static const char kCachedArgument[];

  static const char kLazyPixelRef[];

  static void WillDecodeImage(const String& image_type);
  static void DidDecodeImage();
  static void DidDrawLazyPixelRef(unsigned long long lazy_pixel_ref_id);
  static void WillDecodeLazyPixelRef(unsigned long long lazy_pixel_ref_id);
  static void DidDecodeLazyPixelRef();

 private:
  static const char kCategoryName[];
};

inline void PlatformInstrumentation::WillDecodeImage(const String& image_type) {
  TRACE_EVENT_BEGIN1(kCategoryName, kImageDecodeEvent, kImageTypeArgument,
                     image_type.Ascii());
}

inline void PlatformInstrumentation::DidDecodeImage() {
  TRACE_EVENT_END0(kCategoryName, kImageDecodeEvent);
}

inline void PlatformInstrumentation::DidDrawLazyPixelRef(
    unsigned long long lazy_pixel_ref_id) {
  TRACE_EVENT_INSTANT1(kCategoryName, kDrawLazyPixelRefEvent,
                       TRACE_EVENT_SCOPE_THREAD, kLazyPixelRef,
                       lazy_pixel_ref_id);
}

inline void PlatformInstrumentation::WillDecodeLazyPixelRef(
    unsigned long long lazy_pixel_ref_id) {
  TRACE_EVENT_BEGIN1(kCategoryName, kDecodeLazyPixelRefEvent, kLazyPixelRef,
                     lazy_pixel_ref_id);
}

inline void PlatformInstrumentation::DidDecodeLazyPixelRef() {
  TRACE_EVENT_END0(kCategoryName, kDecodeLazyPixelRefEvent);
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_INSTRUMENTATION_PLATFORM_INSTRUMENTATION_H_
