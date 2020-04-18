// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_capture_from_element/html_video_element_capturer_source.h"

#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "cc/paint/skia_paint_canvas.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/media/stream/media_stream_video_source.h"
#include "content/renderer/media/webrtc/webrtc_uma_histograms.h"
#include "media/base/limits.h"
#include "media/blink/webmediaplayer_impl.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/blink/public/platform/web_media_player.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/libyuv/include/libyuv.h"

namespace {
const float kMinFramesPerSecond = 1.0;
}  // anonymous namespace

namespace content {

//static
std::unique_ptr<HtmlVideoElementCapturerSource>
HtmlVideoElementCapturerSource::CreateFromWebMediaPlayerImpl(
    blink::WebMediaPlayer* player,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  // Save histogram data so we can see how much HTML Video capture is used.
  // The histogram counts the number of calls to the JS API.
  UpdateWebRTCMethodCount(blink::WebRTCAPIName::kVideoCaptureStream);

  return base::WrapUnique(new HtmlVideoElementCapturerSource(
      static_cast<media::WebMediaPlayerImpl*>(player)->AsWeakPtr(),
      io_task_runner, task_runner));
}

HtmlVideoElementCapturerSource::HtmlVideoElementCapturerSource(
    const base::WeakPtr<blink::WebMediaPlayer>& player,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : web_media_player_(player),
      io_task_runner_(io_task_runner),
      task_runner_(task_runner),
      capture_frame_rate_(0.0),
      weak_factory_(this) {
  DCHECK(web_media_player_);
}

HtmlVideoElementCapturerSource::~HtmlVideoElementCapturerSource() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

media::VideoCaptureFormats
HtmlVideoElementCapturerSource::GetPreferredFormats() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // WebMediaPlayer has a setRate() but can't be read back.
  // TODO(mcasas): Add getRate() to WMPlayer and/or fix the spec to allow users
  // to specify it.
  const media::VideoCaptureFormat format(
      web_media_player_->NaturalSize(),
      MediaStreamVideoSource::kDefaultFrameRate, media::PIXEL_FORMAT_I420);
  media::VideoCaptureFormats formats;
  formats.push_back(format);
  return formats;
}

void HtmlVideoElementCapturerSource::StartCapture(
    const media::VideoCaptureParams& params,
    const VideoCaptureDeliverFrameCB& new_frame_callback,
    const RunningCallback& running_callback) {
  DVLOG(2) << __func__ << " requested "
           << media::VideoCaptureFormat::ToString(params.requested_format);
  DCHECK(params.requested_format.IsValid());
  DCHECK(thread_checker_.CalledOnValidThread());

  running_callback_ = running_callback;
  if (!web_media_player_ || !web_media_player_->HasVideo()) {
    running_callback_.Run(false);
    return;
  }
  const blink::WebSize resolution = web_media_player_->NaturalSize();
  if (!bitmap_.tryAllocPixels(
          SkImageInfo::MakeN32Premul(resolution.width, resolution.height))) {
    running_callback_.Run(false);
    return;
  }
  canvas_ = std::make_unique<cc::SkiaPaintCanvas>(bitmap_);

  new_frame_callback_ = new_frame_callback;
  // Force |capture_frame_rate_| to be in between k{Min,Max}FramesPerSecond.
  capture_frame_rate_ =
      std::max(kMinFramesPerSecond,
               std::min(static_cast<float>(media::limits::kMaxFramesPerSecond),
                        params.requested_format.frame_rate));

  running_callback_.Run(true);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&HtmlVideoElementCapturerSource::sendNewFrame,
                                weak_factory_.GetWeakPtr()));
}

void HtmlVideoElementCapturerSource::StopCapture() {
  DVLOG(2) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
  running_callback_.Reset();
  new_frame_callback_.Reset();
  next_capture_time_ = base::TimeTicks();
}

void HtmlVideoElementCapturerSource::sendNewFrame() {
  DVLOG(3) << __func__;
  TRACE_EVENT0("media", "HtmlVideoElementCapturerSource::sendNewFrame");
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!web_media_player_ || new_frame_callback_.is_null())
    return;

  const base::TimeTicks current_time = base::TimeTicks::Now();
  if (start_capture_time_.is_null())
    start_capture_time_ = current_time;
  const blink::WebSize resolution = web_media_player_->NaturalSize();

  cc::PaintFlags flags;
  flags.setBlendMode(SkBlendMode::kSrc);
  flags.setFilterQuality(kLow_SkFilterQuality);
  web_media_player_->Paint(
      canvas_.get(), blink::WebRect(0, 0, resolution.width, resolution.height),
      flags);
  DCHECK_NE(kUnknown_SkColorType, canvas_->imageInfo().colorType());
  DCHECK_EQ(canvas_->imageInfo().width(), resolution.width);
  DCHECK_EQ(canvas_->imageInfo().height(), resolution.height);

  DCHECK_NE(kUnknown_SkColorType, bitmap_.colorType());
  DCHECK(!bitmap_.drawsNothing());
  DCHECK(bitmap_.getPixels());
  if (bitmap_.colorType() != kN32_SkColorType) {
    DLOG(ERROR) << "Only supported color type is kN32_SkColorType (ARGB/ABGR)";
    return;
  }

  scoped_refptr<media::VideoFrame> frame = frame_pool_.CreateFrame(
      media::PIXEL_FORMAT_I420, resolution, gfx::Rect(resolution), resolution,
      current_time - start_capture_time_);

  const uint32 source_pixel_format =
      (kN32_SkColorType == kRGBA_8888_SkColorType) ? libyuv::FOURCC_ABGR
                                                   : libyuv::FOURCC_ARGB;

  if (frame &&
      libyuv::ConvertToI420(
          static_cast<uint8*>(bitmap_.getPixels()), bitmap_.computeByteSize(),
          frame->visible_data(media::VideoFrame::kYPlane),
          frame->stride(media::VideoFrame::kYPlane),
          frame->visible_data(media::VideoFrame::kUPlane),
          frame->stride(media::VideoFrame::kUPlane),
          frame->visible_data(media::VideoFrame::kVPlane),
          frame->stride(media::VideoFrame::kVPlane), 0 /* crop_x */,
          0 /* crop_y */, frame->visible_rect().size().width(),
          frame->visible_rect().size().height(), bitmap_.info().width(),
          bitmap_.info().height(), libyuv::kRotate0,
          source_pixel_format) == 0) {
    // Success!
    io_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(new_frame_callback_, frame, current_time));
  }

  // Calculate the time in the future where the next frame should be created.
  const base::TimeDelta frame_interval =
      base::TimeDelta::FromMicroseconds(1E6 / capture_frame_rate_);
  if (next_capture_time_.is_null()) {
    next_capture_time_ = current_time + frame_interval;
  } else {
    next_capture_time_ += frame_interval;
    // Don't accumulate any debt if we are lagging behind - just post next frame
    // immediately and continue as normal.
    if (next_capture_time_ < current_time)
      next_capture_time_ = current_time;
  }
  // Schedule next capture.
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&HtmlVideoElementCapturerSource::sendNewFrame,
                     weak_factory_.GetWeakPtr()),
      next_capture_time_ - current_time);
}

}  // namespace content
