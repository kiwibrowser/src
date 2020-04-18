// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_VIDEO_CONSUMER_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_VIDEO_CONSUMER_H_

#include "base/time/time.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_video_capture.mojom.h"
#include "ui/gfx/geometry/size.h"

class SkBitmap;

namespace content {

// This class is the video consumer to FrameSinkVideoCapturerImpl. This class,
// in turn sends video frames to its host via the OnFrameCapturedCallback. Used
// when the VizDisplayCompositor feature is enabled.
// TODO(https://crbug.com/813929): Use this class everywhere even if viz is not
// enabled.
class CONTENT_EXPORT DevToolsVideoConsumer
    : public viz::mojom::FrameSinkVideoConsumer {
 public:
  using OnFrameCapturedCallback =
      base::RepeatingCallback<void(scoped_refptr<media::VideoFrame> frame)>;

  explicit DevToolsVideoConsumer(OnFrameCapturedCallback callback);
  ~DevToolsVideoConsumer() override;

  // Copies |frame| onto a SkBitmap and returns it.
  static SkBitmap GetSkBitmapFromFrame(scoped_refptr<media::VideoFrame> frame);

  // If not currently capturing, this creates the capturer and starts capturing.
  void StartCapture();

  // Closes |binding_|. Stops capturing and resets |capturer_|.
  void StopCapture();

  // These functions cache the values passed to them and if we're currently
  // capturing, they call the corresponding |capturer_| functions.
  // TODO(samans): Add a SetFormat function here so that ARGB pixel format can
  // be used.
  void SetFrameSinkId(const viz::FrameSinkId& frame_sink_id);
  void SetMinCapturePeriod(base::TimeDelta min_capture_period);
  void SetMinAndMaxFrameSize(gfx::Size min_frame_size,
                             gfx::Size max_frame_size);

 private:
  friend class DevToolsVideoConsumerTest;
  // Sets up a mojo message pipe and requests the HostFrameSinkManager create a
  // new capturer instance bound to it. Returns the client-side interface.
  viz::mojom::FrameSinkVideoCapturerPtrInfo CreateCapturer();

  // Binds |capturer_info| to the |capturer_|, sets capture parameters, and
  // starts capture. Normally, CreateCapturer produces the |capturer_info|, but
  // unittests can provide a mock.
  void InnerStartCapture(
      viz::mojom::FrameSinkVideoCapturerPtrInfo capturer_info);

  // Checks that |min_frame_size| and |max_frame_size| are in the expected
  // range. Limits are specified in media::limits.
  bool IsValidMinAndMaxFrameSize(gfx::Size min_frame_size,
                                 gfx::Size max_frame_size);

  // viz::mojom::FrameSinkVideoConsumer:
  void OnFrameCaptured(
      mojo::ScopedSharedBufferHandle buffer,
      uint32_t buffer_size,
      ::media::mojom::VideoFrameInfoPtr info,
      const gfx::Rect& update_rect,
      const gfx::Rect& content_rect,
      viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) override;
  void OnTargetLost(const viz::FrameSinkId& frame_sink_id) override;
  void OnStopped() override;

  // Default min frame size is 1x1, as otherwise, nothing would be captured.
  static constexpr gfx::Size kDefaultMinFrameSize = gfx::Size(1, 1);

  // Using an arbitrary default max frame size of 500x500.
  static constexpr gfx::Size kDefaultMaxFrameSize = gfx::Size(500, 500);

  // Callback that is run when a frame is received.
  const OnFrameCapturedCallback callback_;

  // Capture parameters.
  base::TimeDelta min_capture_period_;
  gfx::Size min_frame_size_;
  gfx::Size max_frame_size_;
  viz::FrameSinkId frame_sink_id_;

  // Mojo pointer to the viz::FrameSinkVideoCapturer instance. If |capturer_|
  // is alive, then we are currently capturing.
  viz::mojom::FrameSinkVideoCapturerPtr capturer_;

  mojo::Binding<viz::mojom::FrameSinkVideoConsumer> binding_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsVideoConsumer);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_VIDEO_CONSUMER_H_
