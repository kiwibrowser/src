// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/image_capture/image_capture_frame_grabber.h"

#include "cc/paint/paint_canvas.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/blink/public/platform/web_callbacks.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/libyuv/include/libyuv.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace content {

using blink::WebImageCaptureGrabFrameCallbacks;

namespace {

void OnError(std::unique_ptr<WebImageCaptureGrabFrameCallbacks> callbacks) {
  callbacks->OnError();
}

}  // anonymous namespace

// Ref-counted class to receive a single VideoFrame on IO thread, convert it and
// send it to |main_task_runner_|, where this class is created and destroyed.
class ImageCaptureFrameGrabber::SingleShotFrameHandler
    : public base::RefCountedThreadSafe<SingleShotFrameHandler> {
 public:
  SingleShotFrameHandler() : first_frame_received_(false) {}

  // Receives a |frame| and converts its pixels into a SkImage via an internal
  // PaintSurface and SkPixmap. Alpha channel, if any, is copied.
  void OnVideoFrameOnIOThread(SkImageDeliverCB callback,
                              const scoped_refptr<media::VideoFrame>& frame,
                              base::TimeTicks current_time);

 private:
  friend class base::RefCountedThreadSafe<SingleShotFrameHandler>;
  virtual ~SingleShotFrameHandler() {}

  // Flag to indicate that the first frames has been processed, and subsequent
  // ones can be safely discarded.
  bool first_frame_received_;

  DISALLOW_COPY_AND_ASSIGN(SingleShotFrameHandler);
};

void ImageCaptureFrameGrabber::SingleShotFrameHandler::OnVideoFrameOnIOThread(
    SkImageDeliverCB callback,
    const scoped_refptr<media::VideoFrame>& frame,
    base::TimeTicks /* current_time */) {
  DCHECK(frame->format() == media::PIXEL_FORMAT_I420 ||
         frame->format() == media::PIXEL_FORMAT_I420A);

  if (first_frame_received_)
    return;
  first_frame_received_ = true;

  const SkAlphaType alpha = media::IsOpaque(frame->format())
                                ? kOpaque_SkAlphaType
                                : kPremul_SkAlphaType;
  const SkImageInfo info = SkImageInfo::MakeN32(
      frame->visible_rect().width(), frame->visible_rect().height(), alpha);

  sk_sp<SkSurface> surface = SkSurface::MakeRaster(info);
  DCHECK(surface);

  SkPixmap pixmap;
  if (!skia::GetWritablePixels(surface->getCanvas(), &pixmap)) {
    DLOG(ERROR) << "Error trying to map SkSurface's pixels";
    std::move(callback).Run(sk_sp<SkImage>());
    return;
  }

  const uint32 destination_pixel_format =
      (kN32_SkColorType == kRGBA_8888_SkColorType) ? libyuv::FOURCC_ABGR
                                                   : libyuv::FOURCC_ARGB;

  libyuv::ConvertFromI420(frame->visible_data(media::VideoFrame::kYPlane),
                          frame->stride(media::VideoFrame::kYPlane),
                          frame->visible_data(media::VideoFrame::kUPlane),
                          frame->stride(media::VideoFrame::kUPlane),
                          frame->visible_data(media::VideoFrame::kVPlane),
                          frame->stride(media::VideoFrame::kVPlane),
                          static_cast<uint8*>(pixmap.writable_addr()),
                          pixmap.width() * 4, pixmap.width(), pixmap.height(),
                          destination_pixel_format);

  if (frame->format() == media::PIXEL_FORMAT_I420A) {
    DCHECK(!info.isOpaque());
    // This function copies any plane into the alpha channel of an ARGB image.
    libyuv::ARGBCopyYToAlpha(frame->visible_data(media::VideoFrame::kAPlane),
                             frame->stride(media::VideoFrame::kAPlane),
                             static_cast<uint8*>(pixmap.writable_addr()),
                             pixmap.width() * 4, pixmap.width(),
                             pixmap.height());
  }

  std::move(callback).Run(surface->makeImageSnapshot());
}

ImageCaptureFrameGrabber::ImageCaptureFrameGrabber()
    : frame_grab_in_progress_(false), weak_factory_(this) {}

ImageCaptureFrameGrabber::~ImageCaptureFrameGrabber() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void ImageCaptureFrameGrabber::GrabFrame(
    blink::WebMediaStreamTrack* track,
    WebImageCaptureGrabFrameCallbacks* callbacks) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!!callbacks);

  DCHECK(track && !track->IsNull() && track->GetTrackData());
  DCHECK_EQ(blink::WebMediaStreamSource::kTypeVideo, track->Source().GetType());

  if (frame_grab_in_progress_) {
    // Reject grabFrame()s too close back to back.
    callbacks->OnError();
    return;
  }

  ScopedWebCallbacks<WebImageCaptureGrabFrameCallbacks> scoped_callbacks =
      make_scoped_web_callbacks(callbacks, base::Bind(&OnError));

  // A SingleShotFrameHandler is bound and given to the Track to guarantee that
  // only one VideoFrame is converted and delivered to OnSkImage(), otherwise
  // SKImages might be sent to resolved |callbacks| while DisconnectFromTrack()
  // is being processed, which might be further held up if UI is busy, see
  // https://crbug.com/623042.
  frame_grab_in_progress_ = true;
  MediaStreamVideoSink::ConnectToTrack(
      *track,
      base::Bind(
          &SingleShotFrameHandler::OnVideoFrameOnIOThread,
          base::MakeRefCounted<SingleShotFrameHandler>(),
          media::BindToCurrentLoop(base::Bind(
              &ImageCaptureFrameGrabber::OnSkImage, weak_factory_.GetWeakPtr(),
              base::Passed(&scoped_callbacks)))),
      false);
}

void ImageCaptureFrameGrabber::OnSkImage(
    ScopedWebCallbacks<blink::WebImageCaptureGrabFrameCallbacks> callbacks,
    sk_sp<SkImage> image) {
  DCHECK(thread_checker_.CalledOnValidThread());

  MediaStreamVideoSink::DisconnectFromTrack();
  frame_grab_in_progress_ = false;
  if (image)
    callbacks.PassCallbacks()->OnSuccess(image);
  else
    callbacks.PassCallbacks()->OnError();
}

}  // namespace content
