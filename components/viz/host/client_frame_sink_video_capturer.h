// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_HOST_CLIENT_FRAME_SINK_VIDEO_CAPTURER_H_
#define COMPONENTS_VIZ_HOST_CLIENT_FRAME_SINK_VIDEO_CAPTURER_H_

#include "base/callback.h"
#include "base/time/time.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/host/viz_host_export.h"
#include "media/base/video_types.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_video_capture.mojom.h"
#include "ui/gfx/geometry/size.h"

namespace viz {

// Client library for using FrameSinkVideoCapturer. Clients should use this
// class instead of talking directly to FrameSinkVideoCapturer in order to
// survive Viz crashes.
// TODO(samans): Move this class and all its dependencies to the client
// directory.
class VIZ_HOST_EXPORT ClientFrameSinkVideoCapturer
    : private mojom::FrameSinkVideoConsumer {
 public:
  using EstablishConnectionCallback =
      base::RepeatingCallback<void(mojom::FrameSinkVideoCapturerRequest)>;

  explicit ClientFrameSinkVideoCapturer(EstablishConnectionCallback callback);
  ~ClientFrameSinkVideoCapturer() override;

  // See FrameSinkVideoCapturer for documentation.
  void SetFormat(media::VideoPixelFormat format, media::ColorSpace color_space);
  void SetMinCapturePeriod(base::TimeDelta min_capture_period);
  void SetMinSizeChangePeriod(base::TimeDelta min_period);
  void SetResolutionConstraints(const gfx::Size& min_size,
                                const gfx::Size& max_size,
                                bool use_fixed_aspect_ratio);
  void SetAutoThrottlingEnabled(bool enabled);
  void ChangeTarget(const FrameSinkId& frame_sink_id);
  void Stop();
  void RequestRefreshFrame();

  // Similar to FrameSinkVideoCapturer::Start, but takes in a pointer directly
  // to the FrameSinkVideoConsumer implemenation class (as opposed to a
  // mojo::InterfacePtr or a proxy object).
  void Start(mojom::FrameSinkVideoConsumer* consumer);

  // Similar to Stop() but also resets the consumer immediately so no further
  // messages (even OnStopped()) will be delivered to the consumer.
  void StopAndResetConsumer();

 private:
  struct Format {
    Format(media::VideoPixelFormat pixel_format, media::ColorSpace color_space);

    media::VideoPixelFormat pixel_format;
    media::ColorSpace color_space;
  };

  struct ResolutionConstraints {
    ResolutionConstraints(const gfx::Size& min_size,
                          const gfx::Size& max_size,
                          bool use_fixed_aspect_ratio);

    gfx::Size min_size;
    gfx::Size max_size;
    bool use_fixed_aspect_ratio;
  };

  // mojom::FrameSinkVideoConsumer implementation.
  void OnFrameCaptured(
      mojo::ScopedSharedBufferHandle buffer,
      uint32_t buffer_size,
      media::mojom::VideoFrameInfoPtr info,
      const gfx::Rect& update_rect,
      const gfx::Rect& content_rect,
      mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) final;
  void OnTargetLost(const FrameSinkId& frame_sink_id) final;
  void OnStopped() final;

  // Establishes connection to FrameSinkVideoCapturer and sends the existing
  // configuration.
  void EstablishConnection();

  // Called when the message pipe is gone. Will call EstablishConnection after
  // some delay.
  void OnConnectionError();

  void StartInternal();

  // The following variables keep the latest arguments provided to their
  // corresponding method in mojom::FrameSinkVideoCapturer. The arguments are
  // saved so we can resend them if viz crashes and a new FrameSinkVideoCapturer
  // has to be created.
  base::Optional<Format> format_;
  base::Optional<base::TimeDelta> min_capture_period_;
  base::Optional<base::TimeDelta> min_size_change_period_;
  base::Optional<ResolutionConstraints> resolution_constraints_;
  base::Optional<bool> auto_throttling_enabled_;
  base::Optional<FrameSinkId> target_;
  bool is_started_ = false;

  mojom::FrameSinkVideoConsumer* consumer_ = nullptr;
  EstablishConnectionCallback establish_connection_callback_;
  mojom::FrameSinkVideoCapturerPtr capturer_;
  mojo::Binding<mojom::FrameSinkVideoConsumer> consumer_binding_;

  base::WeakPtrFactory<ClientFrameSinkVideoCapturer> weak_factory_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_HOST_CLIENT_FRAME_SINK_VIDEO_CAPTURER_H_
