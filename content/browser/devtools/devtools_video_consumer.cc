// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_video_consumer.h"

#include "cc/paint/skia_paint_canvas.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/service/frame_sinks/video_capture/frame_sink_video_capturer_impl.h"
#include "content/browser/compositor/surface_utils.h"
#include "media/base/limits.h"
#include "media/renderers/paint_canvas_video_renderer.h"

namespace content {

namespace {

// Frame capture period is 10 frames per second by default.
constexpr base::TimeDelta kDefaultMinCapturePeriod =
    base::TimeDelta::FromMilliseconds(100);

// Frame size can change every frame.
constexpr base::TimeDelta kDefaultMinPeriod = base::TimeDelta();

// Allow variable aspect ratio.
const bool kDefaultUseFixedAspectRatio = false;

}  // namespace

// static
constexpr gfx::Size DevToolsVideoConsumer::kDefaultMinFrameSize;

// static
constexpr gfx::Size DevToolsVideoConsumer::kDefaultMaxFrameSize;

DevToolsVideoConsumer::DevToolsVideoConsumer(OnFrameCapturedCallback callback)
    : callback_(std::move(callback)),
      min_capture_period_(kDefaultMinCapturePeriod),
      min_frame_size_(kDefaultMinFrameSize),
      max_frame_size_(kDefaultMaxFrameSize),
      binding_(this) {}

DevToolsVideoConsumer::~DevToolsVideoConsumer() = default;

// static
SkBitmap DevToolsVideoConsumer::GetSkBitmapFromFrame(
    scoped_refptr<media::VideoFrame> frame) {
  media::PaintCanvasVideoRenderer renderer;
  SkBitmap skbitmap;
  skbitmap.allocN32Pixels(frame->visible_rect().width(),
                          frame->visible_rect().height());
  cc::SkiaPaintCanvas canvas(skbitmap);
  renderer.Copy(frame, &canvas, media::Context3D());
  return skbitmap;
}

void DevToolsVideoConsumer::StartCapture() {
  if (capturer_)
    return;
  InnerStartCapture(CreateCapturer());
}

void DevToolsVideoConsumer::StopCapture() {
  if (!capturer_)
    return;
  binding_.Close();
  capturer_->Stop();
  capturer_.reset();
}

void DevToolsVideoConsumer::SetFrameSinkId(
    const viz::FrameSinkId& frame_sink_id) {
  frame_sink_id_ = frame_sink_id;
  if (capturer_)
    capturer_->ChangeTarget(frame_sink_id_);
}

void DevToolsVideoConsumer::SetMinCapturePeriod(
    base::TimeDelta min_capture_period) {
  min_capture_period_ = min_capture_period;
  if (capturer_)
    capturer_->SetMinCapturePeriod(min_capture_period_);
}

void DevToolsVideoConsumer::SetMinAndMaxFrameSize(gfx::Size min_frame_size,
                                                  gfx::Size max_frame_size) {
  DCHECK(IsValidMinAndMaxFrameSize(min_frame_size, max_frame_size));
  min_frame_size_ = min_frame_size;
  max_frame_size_ = max_frame_size;
  if (capturer_) {
    capturer_->SetResolutionConstraints(min_frame_size_, max_frame_size_,
                                        kDefaultUseFixedAspectRatio);
  }
}

viz::mojom::FrameSinkVideoCapturerPtrInfo
DevToolsVideoConsumer::CreateCapturer() {
  viz::HostFrameSinkManager* const manager = GetHostFrameSinkManager();
  viz::mojom::FrameSinkVideoCapturerPtr capturer;
  manager->CreateVideoCapturer(mojo::MakeRequest(&capturer));
  return capturer.PassInterface();
}

void DevToolsVideoConsumer::InnerStartCapture(
    viz::mojom::FrameSinkVideoCapturerPtrInfo capturer_info) {
  capturer_.Bind(std::move(capturer_info));

  // Give |capturer_| the capture parameters.
  capturer_->SetMinCapturePeriod(min_capture_period_);
  capturer_->SetMinSizeChangePeriod(kDefaultMinPeriod);
  capturer_->SetResolutionConstraints(min_frame_size_, max_frame_size_,
                                      kDefaultUseFixedAspectRatio);
  capturer_->ChangeTarget(frame_sink_id_);

  viz::mojom::FrameSinkVideoConsumerPtr consumer;
  binding_.Bind(mojo::MakeRequest(&consumer));
  capturer_->Start(std::move(consumer));
}

bool DevToolsVideoConsumer::IsValidMinAndMaxFrameSize(
    gfx::Size min_frame_size,
    gfx::Size max_frame_size) {
  // Returns true if
  // 0 < |min_frame_size| <= |max_frame_size| <= media::limits::kMaxDimension.
  return 0 < min_frame_size.width() && 0 < min_frame_size.height() &&
         min_frame_size.width() <= max_frame_size.width() &&
         min_frame_size.height() <= max_frame_size.height() &&
         max_frame_size.width() <= media::limits::kMaxDimension &&
         max_frame_size.height() <= media::limits::kMaxDimension;
}

void DevToolsVideoConsumer::OnFrameCaptured(
    mojo::ScopedSharedBufferHandle buffer,
    uint32_t buffer_size,
    ::media::mojom::VideoFrameInfoPtr info,
    const gfx::Rect& update_rect,
    const gfx::Rect& content_rect,
    viz::mojom::FrameSinkVideoConsumerFrameCallbacksPtr callbacks) {
  if (!buffer.is_valid())
    return;

  mojo::ScopedSharedBufferMapping mapping = buffer->Map(buffer_size);
  if (!mapping) {
    DLOG(ERROR) << "Shared memory mapping failed.";
    return;
  }

  scoped_refptr<media::VideoFrame> frame;
  // Setting |frame|'s visible rect equal to |content_rect| so that only the
  // portion of the frame that contain content are used.
  frame = media::VideoFrame::WrapExternalData(
      info->pixel_format, info->coded_size, content_rect, content_rect.size(),
      static_cast<uint8_t*>(mapping.get()), buffer_size, info->timestamp);
  if (!frame)
    return;
  frame->AddDestructionObserver(base::BindOnce(
      [](mojo::ScopedSharedBufferMapping mapping) {}, std::move(mapping)));
  frame->metadata()->MergeInternalValuesFrom(info->metadata);

  callback_.Run(std::move(frame));
}

void DevToolsVideoConsumer::OnTargetLost(
    const viz::FrameSinkId& frame_sink_id) {}

void DevToolsVideoConsumer::OnStopped() {}

}  // namespace content
