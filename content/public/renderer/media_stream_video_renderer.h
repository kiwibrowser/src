// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_VIDEO_RENDERER_H_
#define CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_VIDEO_RENDERER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"

namespace media {
class VideoFrame;
}

namespace content {

// Interface returned by MediaStreamRendererFactory that provides controls for
// the flow of video frame callbacks being made.
// TODO(wjia): remove ref count.
class MediaStreamVideoRenderer
    : public base::RefCountedThreadSafe<MediaStreamVideoRenderer> {
 public:
  typedef base::Callback<void(scoped_refptr<media::VideoFrame>)> RepaintCB;

  // Start to provide video frames to the caller.
  virtual void Start() = 0;

  // Stop to provide video frames to the caller.
  virtual void Stop() = 0;

  // Resume to provide video frames to the caller after being paused.
  virtual void Resume() = 0;

  // Put the provider in pause state and the caller will not receive video
  // frames, but the provider might still generate video frames which are
  // thrown away.
  virtual void Pause() = 0;

 protected:
  friend class base::RefCountedThreadSafe<MediaStreamVideoRenderer>;

  virtual ~MediaStreamVideoRenderer() {}

};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_MEDIA_STREAM_VIDEO_RENDERER_H_
