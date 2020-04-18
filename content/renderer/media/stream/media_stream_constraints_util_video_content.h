// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_CONTENT_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_CONTENT_H_

#include <string>

#include "content/common/content_export.h"
#include "content/renderer/media/stream/media_stream_constraints_util.h"
#include "third_party/webrtc/api/optional.h"

namespace blink {
class WebMediaConstraints;
}

namespace content {

CONTENT_EXPORT extern const int kMinScreenCastDimension;
CONTENT_EXPORT extern const int kMaxScreenCastDimension;
CONTENT_EXPORT extern const int kDefaultScreenCastWidth;
CONTENT_EXPORT extern const int kDefaultScreenCastHeight;

CONTENT_EXPORT extern const double kMaxScreenCastFrameRate;
CONTENT_EXPORT extern const double kDefaultScreenCastFrameRate;

// This function performs source, source-settings and track-settings selection
// for content video capture based on the given |constraints|.
VideoCaptureSettings CONTENT_EXPORT
SelectSettingsVideoContentCapture(const blink::WebMediaConstraints& constraints,
                                  const std::string& stream_source,
                                  int screen_width,
                                  int screen_height);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_CONTENT_H_
