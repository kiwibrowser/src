// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_VIDEO_RENDERER_SINK_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_VIDEO_RENDERER_SINK_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "content/common/media/video_capture.h"
#include "content/public/renderer/media_stream_video_renderer.h"
#include "content/public/renderer/media_stream_video_sink.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "ui/gfx/geometry/size.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace content {

// MediaStreamVideoRendererSink is a MediaStreamVideoRenderer designed for
// rendering Video MediaStreamTracks [1], MediaStreamVideoRendererSink
// implements MediaStreamVideoSink in order to render video frames provided from
// a MediaStreamVideoTrack, to which it connects itself when the
// MediaStreamVideoRenderer is Start()ed, and disconnects itself when the latter
// is Stop()ed.
//
// [1] http://dev.w3.org/2011/webrtc/editor/getusermedia.html#mediastreamtrack
//
// TODO(wuchengli): Add unit test. See the link below for reference.
// http://src.chromium.org/viewvc/chrome/trunk/src/content/renderer/media/rtc_vi
// deo_decoder_unittest.cc?revision=180591&view=markup
class CONTENT_EXPORT MediaStreamVideoRendererSink
    : public MediaStreamVideoRenderer,
      public MediaStreamVideoSink {
 public:
  MediaStreamVideoRendererSink(
      const blink::WebMediaStreamTrack& video_track,
      const base::Closure& error_cb,
      const MediaStreamVideoRenderer::RepaintCB& repaint_cb,
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);

  // MediaStreamVideoRenderer implementation. Called on the main thread.
  void Start() override;
  void Stop() override;
  void Resume() override;
  void Pause() override;

 protected:
  ~MediaStreamVideoRendererSink() override;

 private:
  friend class MediaStreamVideoRendererSinkTest;
  enum State {
    STARTED,
    PAUSED,
    STOPPED,
  };

  // MediaStreamVideoSink implementation. Called on the main thread.
  void OnReadyStateChanged(
      blink::WebMediaStreamSource::ReadyState state) override;

  // Helper method used for testing.
  State GetStateForTesting();

  const base::Closure error_cb_;
  const RepaintCB repaint_cb_;
  const blink::WebMediaStreamTrack video_track_;

  // Inner class used for transfering frames on compositor thread and running
  // |repaint_cb_|.
  class FrameDeliverer;
  std::unique_ptr<FrameDeliverer> frame_deliverer_;

  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  base::ThreadChecker main_thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamVideoRendererSink);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_VIDEO_RENDERER_SINK_H_
