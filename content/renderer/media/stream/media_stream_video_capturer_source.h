// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_VIDEO_CAPTURER_SOURCE_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_VIDEO_CAPTURER_SOURCE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/common/media/media_stream.mojom.h"
#include "content/common/media/video_capture.h"
#include "content/renderer/media/stream/media_stream_video_source.h"

namespace media {
class VideoCapturerSource;
}  // namespace media

namespace content {

class RenderFrame;

// Representation of a video stream coming from a camera, owned by Blink as
// WebMediaStreamSource. Objects of this class are created and live on main
// Render thread. Objects can be constructed either by indicating a |device| to
// look for, or by plugging in a |source| constructed elsewhere.
class CONTENT_EXPORT MediaStreamVideoCapturerSource
    : public MediaStreamVideoSource {
 public:
  MediaStreamVideoCapturerSource(
      const SourceStoppedCallback& stop_callback,
      std::unique_ptr<media::VideoCapturerSource> source);
  MediaStreamVideoCapturerSource(
      int render_frame_id,
      const SourceStoppedCallback& stop_callback,
      const MediaStreamDevice& device,
      const media::VideoCaptureParams& capture_params);
  ~MediaStreamVideoCapturerSource() override;

 private:
  friend class CanvasCaptureHandlerTest;
  friend class MediaStreamVideoCapturerSourceTest;
  friend class MediaStreamVideoCapturerSourceOldConstraintsTest;
  FRIEND_TEST_ALL_PREFIXES(MediaStreamVideoCapturerSourceTest, StartAndStop);
  FRIEND_TEST_ALL_PREFIXES(MediaStreamVideoCapturerSourceTest,
                           CaptureTimeAndMetadataPlumbing);

  // MediaStreamVideoSource overrides.
  void RequestRefreshFrame() override;
  void OnHasConsumers(bool has_consumers) override;
  void OnCapturingLinkSecured(bool is_secure) override;
  void StartSourceImpl(
      const VideoCaptureDeliverFrameCB& frame_callback) override;
  void StopSourceImpl() override;
  void StopSourceForRestartImpl() override;
  void RestartSourceImpl(const media::VideoCaptureFormat& new_format) override;
  base::Optional<media::VideoCaptureFormat> GetCurrentFormat() const override;
  base::Optional<media::VideoCaptureParams> GetCurrentCaptureParams()
      const override;

  // Method to bind as RunningCallback in VideoCapturerSource::StartCapture().
  void OnRunStateChanged(const media::VideoCaptureParams& new_capture_params,
                         bool is_running);

  const mojom::MediaStreamDispatcherHostPtr& GetMediaStreamDispatcherHost(
      RenderFrame* render_frame);

  mojom::MediaStreamDispatcherHostPtr dispatcher_host_;

  int render_frame_id_;

  // The source that provides video frames.
  const std::unique_ptr<media::VideoCapturerSource> source_;

  enum State { STARTING, STARTED, STOPPING_FOR_RESTART, RESTARTING, STOPPED };
  State state_ = STOPPED;

  media::VideoCaptureParams capture_params_;
  VideoCaptureDeliverFrameCB frame_callback_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamVideoCapturerSource);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_VIDEO_CAPTURER_SOURCE_H_
