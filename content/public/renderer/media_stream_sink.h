// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_SINK_H_
#define CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_SINK_H_

#include "base/compiler_specific.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"

namespace content {

// MediaStreamSink is the base interface for MediaStreamAudioSink and
// MediaStreamVideoSink. It allows an implementation to receive notifications
// about state changes on a blink::WebMediaStreamSource object or such an
// object underlying a blink::WebMediaStreamTrack.
class CONTENT_EXPORT MediaStreamSink {
 public:
  virtual void OnReadyStateChanged(
      blink::WebMediaStreamSource::ReadyState state) {}
  virtual void OnEnabledChanged(bool enabled) {}
  virtual void OnContentHintChanged(
      blink::WebMediaStreamTrack::ContentHintType content_hint) {}

 protected:
  virtual ~MediaStreamSink() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_SINK_H_
