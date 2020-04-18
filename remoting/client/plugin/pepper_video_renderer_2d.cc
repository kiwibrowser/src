// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_video_renderer_2d.h"

#include <stdint.h>

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/task_runner_util.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/point.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/cpp/size.h"
#include "remoting/base/util.h"
#include "remoting/client/client_context.h"
#include "remoting/proto/video.pb.h"
#include "remoting/protocol/performance_tracker.h"
#include "third_party/libyuv/include/libyuv/scale_argb.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_frame.h"

namespace remoting {

namespace {

// DesktopFrame that wraps a supplied pp::ImageData
class PepperDesktopFrame : public webrtc::DesktopFrame {
 public:
  // Wraps the supplied ImageData.
  explicit PepperDesktopFrame(const pp::ImageData& buffer)
      : DesktopFrame(
            webrtc::DesktopSize(buffer.size().width(), buffer.size().height()),
            buffer.stride(),
            reinterpret_cast<uint8_t*>(buffer.data()),
            nullptr),
        buffer_(buffer) {}

  // Access to underlying pepper representation.
  const pp::ImageData& buffer() const {
    return buffer_;
  }

 private:
  pp::ImageData buffer_;
};

}  // namespace

PepperVideoRenderer2D::PepperVideoRenderer2D()
    : software_video_renderer_(this),
      callback_factory_(this),
      weak_factory_(this) {}

PepperVideoRenderer2D::~PepperVideoRenderer2D() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void PepperVideoRenderer2D::SetPepperContext(
    pp::Instance* instance,
    EventHandler* event_handler) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!instance_);
  DCHECK(!event_handler_);
  DCHECK(instance);
  DCHECK(event_handler);

  instance_ = instance;
  event_handler_ = event_handler;
}

void PepperVideoRenderer2D::OnViewChanged(const pp::View& view) {
  DCHECK(thread_checker_.CalledOnValidThread());

  pp::Rect pp_size = view.GetRect();
  view_size_ = webrtc::DesktopSize(pp_size.width(), pp_size.height());

  // Update scale if graphics2d has been initialized.
  if (!graphics2d_.is_null() && source_size_.width() > 0) {
    graphics2d_.SetScale(static_cast<float>(view_size_.width()) /
                         source_size_.width());

    // Bind graphics2d_ again after changing the scale to work around
    // crbug.com/521745 .
    instance_->BindGraphics(graphics2d_);
    bool result = instance_->BindGraphics(graphics2d_);
    DCHECK(result) << "Couldn't bind the device context.";
  }
}

void PepperVideoRenderer2D::EnableDebugDirtyRegion(bool enable) {
  debug_dirty_region_ = enable;
}

bool PepperVideoRenderer2D::Initialize(
    const ClientContext& client_context,
    protocol::FrameStatsConsumer* stats_consumer) {
  DCHECK(thread_checker_.CalledOnValidThread());

  return software_video_renderer_.Initialize(client_context, stats_consumer);
}

void PepperVideoRenderer2D::OnSessionConfig(
    const protocol::SessionConfig& config) {
  DCHECK(thread_checker_.CalledOnValidThread());

  software_video_renderer_.OnSessionConfig(config);
}

protocol::VideoStub* PepperVideoRenderer2D::GetVideoStub() {
  DCHECK(thread_checker_.CalledOnValidThread());

  return software_video_renderer_.GetVideoStub();
}

protocol::FrameConsumer* PepperVideoRenderer2D::GetFrameConsumer() {
  DCHECK(thread_checker_.CalledOnValidThread());

  return software_video_renderer_.GetFrameConsumer();
}

protocol::FrameStatsConsumer* PepperVideoRenderer2D::GetFrameStatsConsumer() {
  DCHECK(thread_checker_.CalledOnValidThread());

  return software_video_renderer_.GetFrameStatsConsumer();
}

std::unique_ptr<webrtc::DesktopFrame> PepperVideoRenderer2D::AllocateFrame(
    const webrtc::DesktopSize& size) {
  DCHECK(thread_checker_.CalledOnValidThread());

  pp::ImageData buffer_data(instance_, PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                            pp::Size(size.width(), size.height()), false);
  return base::WrapUnique(new PepperDesktopFrame(buffer_data));
}

void PepperVideoRenderer2D::DrawFrame(
    std::unique_ptr<webrtc::DesktopFrame> frame,
    const base::Closure& done) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!frame_received_) {
    event_handler_->OnVideoFirstFrameReceived();
    frame_received_ = true;
  }

  bool size_changed = !source_size_.equals(frame->size());
  if (size_changed) {
    source_size_ = frame->size();

    // Create a 2D rendering context with the new dimensions.
    graphics2d_ = pp::Graphics2D(
        instance_, pp::Size(source_size_.width(), source_size_.height()), true);
    graphics2d_.SetScale(static_cast<float>(view_size_.width()) /
                         source_size_.width());
    bool result = instance_->BindGraphics(graphics2d_);
    DCHECK(result) << "Couldn't bind the device context.";
  }

  // If Debug dirty region is enabled then emit it.
  if (debug_dirty_region_)
    event_handler_->OnVideoFrameDirtyRegion(frame->updated_region());

  const pp::ImageData& image_data =
      static_cast<PepperDesktopFrame*>(frame.get())->buffer();
  for (webrtc::DesktopRegion::Iterator i(frame->updated_region()); !i.IsAtEnd();
       i.Advance()) {
    graphics2d_.PaintImageData(image_data, pp::Point(0, 0),
                               pp::Rect(i.rect().left(), i.rect().top(),
                                        i.rect().width(), i.rect().height()));
  }

  if (!done.is_null()) {
    pending_frames_done_callbacks_.push_back(
        std::make_unique<base::ScopedClosureRunner>(done));
  }

  need_flush_ = true;

  Flush();
}

protocol::FrameConsumer::PixelFormat PepperVideoRenderer2D::GetPixelFormat() {
  return FORMAT_BGRA;
}

void PepperVideoRenderer2D::Flush() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (flush_pending_ || !need_flush_)
    return;

  need_flush_ = false;

  // Move callbacks from |pending_frames_done_callbacks_| to
  // |flushing_frames_done_callbacks_| so the callbacks are called when flush is
  // finished.
  DCHECK(flushing_frames_done_callbacks_.empty());
  flushing_frames_done_callbacks_ = std::move(pending_frames_done_callbacks_);

  // Flush the updated areas to the screen.
  int error = graphics2d_.Flush(
      callback_factory_.NewCallback(&PepperVideoRenderer2D::OnFlushDone));
  CHECK(error == PP_OK_COMPLETIONPENDING);
  flush_pending_ = true;
}

void PepperVideoRenderer2D::OnFlushDone(int result) {
  DCHECK(thread_checker_.CalledOnValidThread());

  DCHECK(flush_pending_);
  flush_pending_ = false;

  // Call all callbacks for the frames we've just flushed.
  flushing_frames_done_callbacks_.clear();

  // Flush again if necessary.
  Flush();
}

}  // namespace remoting
