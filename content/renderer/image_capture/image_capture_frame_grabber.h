// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_IMAGE_CAPTURE_IMAGE_CAPTURE_FRAME_GRABBER_H_
#define CONTENT_RENDERER_IMAGE_CAPTURE_IMAGE_CAPTURE_FRAME_GRABBER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/child/scoped_web_callbacks.h"
#include "content/common/content_export.h"
#include "content/public/renderer/media_stream_video_sink.h"
#include "third_party/blink/public/platform/web_image_capture_frame_grabber.h"

namespace blink {
class WebMediaStreamTrack;
}

namespace content {

// This class grabs Video Frames from a given Media Stream Video Track, binding
// a method of an ephemeral SingleShotFrameHandler every time grabFrame() is
// called. This method receives an incoming media::VideoFrame on a background
// thread and converts it into the appropriate SkBitmap which is sent back to
// OnSkBitmap(). This class is single threaded throughout.
class CONTENT_EXPORT ImageCaptureFrameGrabber final
    : public blink::WebImageCaptureFrameGrabber,
      public MediaStreamVideoSink {
 public:
  using SkImageDeliverCB = base::Callback<void(sk_sp<SkImage>)>;

  ImageCaptureFrameGrabber();
  ~ImageCaptureFrameGrabber() override;

  // blink::WebImageCaptureFrameGrabber implementation.
  void GrabFrame(blink::WebMediaStreamTrack* track,
                 blink::WebImageCaptureGrabFrameCallbacks* callbacks) override;

 private:
  // Internal class to receive, convert and forward one frame.
  class SingleShotFrameHandler;

  void OnSkImage(
      ScopedWebCallbacks<blink::WebImageCaptureGrabFrameCallbacks> callbacks,
      sk_sp<SkImage> image);

  // Flag to indicate that there is a frame grabbing in progress.
  bool frame_grab_in_progress_;

  base::ThreadChecker thread_checker_;
  base::WeakPtrFactory<ImageCaptureFrameGrabber> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ImageCaptureFrameGrabber);
};

}  // namespace content

#endif  // CONTENT_RENDERER_IMAGE_CAPTURE_IMAGE_CAPTURE_FRAME_GRABBER_H_
