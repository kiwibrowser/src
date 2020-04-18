// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_video_capturer_adapter.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "cc/paint/skia_paint_canvas.h"
#include "content/renderer/media/webrtc/webrtc_video_frame_adapter.h"
#include "media/base/timestamp_constants.h"
#include "media/base/video_util.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"
#include "third_party/libyuv/include/libyuv/scale.h"
#include "third_party/webrtc/api/video/video_rotation.h"
#include "third_party/webrtc/rtc_base/refcountedobject.h"

namespace content {

namespace {

// Empty method used for keeping a reference to the original media::VideoFrame.
// The reference to |frame| is kept in the closure that calls this method.
void CapturerReleaseOriginalFrame(
    const scoped_refptr<media::VideoFrame>& frame) {}

}  // anonymous namespace

WebRtcVideoCapturerAdapter::WebRtcVideoCapturerAdapter(
    bool is_screencast,
    blink::WebMediaStreamTrack::ContentHintType content_hint)
    : is_screencast_(is_screencast),
      content_hint_(content_hint),
      running_(false) {
  thread_checker_.DetachFromThread();
}

WebRtcVideoCapturerAdapter::~WebRtcVideoCapturerAdapter() {
  DVLOG(3) << __func__;
}

void WebRtcVideoCapturerAdapter::OnFrameCaptured(
    const scoped_refptr<media::VideoFrame>& input_frame) {
  DCHECK(thread_checker_.CalledOnValidThread());
  TRACE_EVENT0("media", "WebRtcVideoCapturerAdapter::OnFrameCaptured");
  if (!(input_frame->IsMappable() &&
        (input_frame->format() == media::PIXEL_FORMAT_I420 ||
         input_frame->format() == media::PIXEL_FORMAT_I420A)) &&
      !input_frame->HasTextures()) {
    // Since connecting sources and sinks do not check the format, we need to
    // just ignore formats that we can not handle.
    LOG(ERROR) << "We cannot send frame with storage type: "
               << input_frame->AsHumanReadableString();
    NOTREACHED();
    return;
  }
  scoped_refptr<media::VideoFrame> frame = input_frame;
  const int orig_width = frame->natural_size().width();
  const int orig_height = frame->natural_size().height();
  int adapted_width;
  int adapted_height;
  // The VideoAdapter is only used for cpu-adaptation downscaling, no
  // aspect changes. So we ignore these crop-related outputs.
  int crop_width;
  int crop_height;
  int crop_x;
  int crop_y;
  int64_t translated_camera_time_us;

  if (!AdaptFrame(orig_width, orig_height,
                  frame->timestamp().InMicroseconds(),
                  rtc::TimeMicros(),
                  &adapted_width, &adapted_height,
                  &crop_width, &crop_height, &crop_x, &crop_y,
                  &translated_camera_time_us)) {
    return;
  }

  // Return |frame| directly if it is texture backed, because there is no
  // cropping support for texture yet. See http://crbug/503653.
  if (frame->HasTextures()) {
    OnFrame(webrtc::VideoFrame(
                new rtc::RefCountedObject<WebRtcVideoFrameAdapter>(frame),
                webrtc::kVideoRotation_0, translated_camera_time_us),
            orig_width, orig_height);
    return;
  }

  // Translate crop rectangle from natural size to visible size.
  gfx::Rect cropped_visible_rect(
      frame->visible_rect().x() +
          crop_x * frame->visible_rect().width() / orig_width,
      frame->visible_rect().y() +
          crop_y * frame->visible_rect().height() / orig_height,
      crop_width * frame->visible_rect().width() / orig_width,
      crop_height * frame->visible_rect().height() / orig_height);

  const gfx::Size adapted_size(adapted_width, adapted_height);
  scoped_refptr<media::VideoFrame> video_frame =
      media::VideoFrame::WrapVideoFrame(frame, frame->format(),
                                        cropped_visible_rect, adapted_size);
  if (!video_frame)
    return;

  video_frame->AddDestructionObserver(
      base::BindOnce(&CapturerReleaseOriginalFrame, frame));

  // If no scaling is needed, return a wrapped version of |frame| directly.
  if (video_frame->natural_size() == video_frame->visible_rect().size()) {
    OnFrame(webrtc::VideoFrame(
                new rtc::RefCountedObject<WebRtcVideoFrameAdapter>(video_frame),
                webrtc::kVideoRotation_0, translated_camera_time_us),
            orig_width, orig_height);
    return;
  }

  // We need to scale the frame before we hand it over to webrtc.
  const bool has_alpha = video_frame->format() == media::PIXEL_FORMAT_I420A;
  scoped_refptr<media::VideoFrame> scaled_frame =
      scaled_frame_pool_.CreateFrame(
          has_alpha ? media::PIXEL_FORMAT_I420A : media::PIXEL_FORMAT_I420,
          adapted_size, gfx::Rect(adapted_size), adapted_size,
          frame->timestamp());
  libyuv::I420Scale(video_frame->visible_data(media::VideoFrame::kYPlane),
                    video_frame->stride(media::VideoFrame::kYPlane),
                    video_frame->visible_data(media::VideoFrame::kUPlane),
                    video_frame->stride(media::VideoFrame::kUPlane),
                    video_frame->visible_data(media::VideoFrame::kVPlane),
                    video_frame->stride(media::VideoFrame::kVPlane),
                    video_frame->visible_rect().width(),
                    video_frame->visible_rect().height(),
                    scaled_frame->data(media::VideoFrame::kYPlane),
                    scaled_frame->stride(media::VideoFrame::kYPlane),
                    scaled_frame->data(media::VideoFrame::kUPlane),
                    scaled_frame->stride(media::VideoFrame::kUPlane),
                    scaled_frame->data(media::VideoFrame::kVPlane),
                    scaled_frame->stride(media::VideoFrame::kVPlane),
                    adapted_width, adapted_height, libyuv::kFilterBilinear);
  if (has_alpha) {
    libyuv::ScalePlane(video_frame->visible_data(media::VideoFrame::kAPlane),
                       video_frame->stride(media::VideoFrame::kAPlane),
                       video_frame->visible_rect().width(),
                       video_frame->visible_rect().height(),
                       scaled_frame->data(media::VideoFrame::kAPlane),
                       scaled_frame->stride(media::VideoFrame::kAPlane),
                       adapted_width, adapted_height, libyuv::kFilterBilinear);
  }

  OnFrame(webrtc::VideoFrame(
              new rtc::RefCountedObject<WebRtcVideoFrameAdapter>(scaled_frame),
              webrtc::kVideoRotation_0, translated_camera_time_us),
          orig_width, orig_height);
}

cricket::CaptureState WebRtcVideoCapturerAdapter::Start(
    const cricket::VideoFormat& capture_format) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!running_);
  DVLOG(3) << __func__ << " capture format: " << capture_format.ToString();

  running_ = true;
  return cricket::CS_RUNNING;
}

void WebRtcVideoCapturerAdapter::Stop() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(3) << __func__;
  DCHECK(running_);
  running_ = false;
  SetCaptureFormat(nullptr);
  SignalStateChange(this, cricket::CS_STOPPED);
}

bool WebRtcVideoCapturerAdapter::IsRunning() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return running_;
}

bool WebRtcVideoCapturerAdapter::GetPreferredFourccs(
    std::vector<uint32_t>* fourccs) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!fourccs)
    return false;
  DCHECK(fourccs->empty());
  fourccs->push_back(cricket::FOURCC_I420);
  return true;
}

void WebRtcVideoCapturerAdapter::SetContentHint(
    blink::WebMediaStreamTrack::ContentHintType content_hint) {
  DCHECK(thread_checker_.CalledOnValidThread());
  content_hint_ = content_hint;
}

bool WebRtcVideoCapturerAdapter::IsScreencast() const {
  // IsScreencast() is misleading since content hints were added to
  // MediaStreamTracks. What IsScreencast() really signals is whether or not
  // video frames should ever be scaled before being handed over to WebRTC.
  // TODO(pbos): Remove the need for IsScreencast() -> ShouldAdaptResolution()
  // by inlining VideoCapturer::AdaptFrame() and removing it from VideoCapturer.
  return !ShouldAdaptResolution();
}

bool WebRtcVideoCapturerAdapter::ShouldAdaptResolution() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (content_hint_ ==
      blink::WebMediaStreamTrack::ContentHintType::kVideoMotion) {
    return true;
  }
  if (content_hint_ ==
      blink::WebMediaStreamTrack::ContentHintType::kVideoDetail) {
    return false;
  }
  // Screencast does not adapt by default.
  return !is_screencast_;
}

bool WebRtcVideoCapturerAdapter::GetBestCaptureFormat(
    const cricket::VideoFormat& desired,
    cricket::VideoFormat* best_format) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(3) << __func__ << " desired: " << desired.ToString();

  // Capability enumeration is done in MediaStreamVideoSource. The adapter can
  // just use what is provided.
  // Use the desired format as the best format.
  best_format->width = desired.width;
  best_format->height = desired.height;
  best_format->fourcc = cricket::FOURCC_I420;
  best_format->interval = desired.interval;
  return true;
}

}  // namespace content
