// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_RENDERER_FACTORY_H_
#define CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_RENDERER_FACTORY_H_

#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/public/renderer/media_stream_audio_renderer.h"
#include "content/public/renderer/media_stream_video_renderer.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace blink {
class WebMediaStream;
}  // namespace blink

namespace content {

// MediaStreamRendererFactory is used by WebMediaPlayerMS to create audio and
// video feeds from a MediaStream provided an URL.
// The factory methods are virtual in order for blink layouttests to be able to
// override them.
class MediaStreamRendererFactory {
 public:
  virtual ~MediaStreamRendererFactory() {}

  // Returns a MediaStreamVideoRenderer that uses the given task runners.
  // |io_task_runner| is used for passing video frames.
  virtual scoped_refptr<MediaStreamVideoRenderer> GetVideoRenderer(
      const blink::WebMediaStream& web_stream,
      const base::Closure& error_cb,
      const MediaStreamVideoRenderer::RepaintCB& repaint_cb,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner) = 0;

  virtual scoped_refptr<MediaStreamAudioRenderer> GetAudioRenderer(
      const blink::WebMediaStream& web_stream,
      int render_frame_id,
      const std::string& device_id) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_RENDERER_FACTORY_H_
