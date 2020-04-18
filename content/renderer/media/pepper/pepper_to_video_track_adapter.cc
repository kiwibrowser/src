// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/pepper/pepper_to_video_track_adapter.h"

#include <string>

#include "base/base64.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/rand_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "content/renderer/media/stream/media_stream_registry_interface.h"
#include "content/renderer/media/stream/media_stream_video_source.h"
#include "content/renderer/media/stream/media_stream_video_track.h"
#include "content/renderer/pepper/ppb_image_data_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "media/base/video_frame_pool.h"
#include "media/capture/video_capture_types.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_media_stream_registry.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "url/gurl.h"

namespace content {

// PpFrameWriter implements MediaStreamVideoSource and can therefore provide
// video frames to MediaStreamVideoTracks. It also implements
// FrameWriterInterface, which will be used by Pepper plugins (notably the
// Effects plugin) to inject the processed frame.
class PpFrameWriter : public MediaStreamVideoSource,
                      public FrameWriterInterface,
                      public base::SupportsWeakPtr<PpFrameWriter> {
 public:
  PpFrameWriter();
  ~PpFrameWriter() override;

  // FrameWriterInterface implementation.
  // This method will be called by the Pepper host from render thread.
  void PutFrame(PPB_ImageData_Impl* image_data, int64_t time_stamp_ns) override;

 protected:
  // MediaStreamVideoSource implementation.
  void StartSourceImpl(
      const VideoCaptureDeliverFrameCB& frame_callback) override;
  void StopSourceImpl() override;

 private:
  media::VideoFramePool frame_pool_;

  class FrameWriterDelegate;
  scoped_refptr<FrameWriterDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(PpFrameWriter);
};

class PpFrameWriter::FrameWriterDelegate
    : public base::RefCountedThreadSafe<FrameWriterDelegate> {
 public:
  FrameWriterDelegate(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
      const VideoCaptureDeliverFrameCB& new_frame_callback);

  void DeliverFrame(const scoped_refptr<media::VideoFrame>& frame);
 private:
  friend class base::RefCountedThreadSafe<FrameWriterDelegate>;
  virtual ~FrameWriterDelegate();

  void DeliverFrameOnIO(const scoped_refptr<media::VideoFrame>& frame);

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  VideoCaptureDeliverFrameCB new_frame_callback_;
};

PpFrameWriter::FrameWriterDelegate::FrameWriterDelegate(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    const VideoCaptureDeliverFrameCB& new_frame_callback)
    : io_task_runner_(io_task_runner), new_frame_callback_(new_frame_callback) {
}

PpFrameWriter::FrameWriterDelegate::~FrameWriterDelegate() {
}

void PpFrameWriter::FrameWriterDelegate::DeliverFrame(
    const scoped_refptr<media::VideoFrame>& frame) {
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&FrameWriterDelegate::DeliverFrameOnIO, this, frame));
}

void PpFrameWriter::FrameWriterDelegate::DeliverFrameOnIO(
     const scoped_refptr<media::VideoFrame>& frame) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  // The local time when this frame is generated is unknown so give a null
  // value to |estimated_capture_time|.
  new_frame_callback_.Run(frame, base::TimeTicks());
}

PpFrameWriter::PpFrameWriter() {
  DVLOG(3) << "PpFrameWriter ctor";
}

PpFrameWriter::~PpFrameWriter() {
  DVLOG(3) << "PpFrameWriter dtor";
}

void PpFrameWriter::StartSourceImpl(
    const VideoCaptureDeliverFrameCB& frame_callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!delegate_.get());
  DVLOG(3) << "PpFrameWriter::StartSourceImpl()";
  delegate_ = new FrameWriterDelegate(io_task_runner(), frame_callback);
  OnStartDone(MEDIA_DEVICE_OK);
}

void PpFrameWriter::StopSourceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// Note: PutFrame must copy or process image_data directly in this function,
// because it may be overwritten as soon as we return from this function.
void PpFrameWriter::PutFrame(PPB_ImageData_Impl* image_data,
                             int64_t time_stamp_ns) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT0("media", "PpFrameWriter::PutFrame");
  DVLOG(3) << "PpFrameWriter::PutFrame()";

  if (!image_data) {
    LOG(ERROR) << "PpFrameWriter::PutFrame - Called with NULL image_data.";
    return;
  }
  ImageDataAutoMapper mapper(image_data);
  if (!mapper.is_valid()) {
    LOG(ERROR) << "PpFrameWriter::PutFrame - "
               << "The image could not be mapped and is unusable.";
    return;
  }
  SkBitmap bitmap(image_data->GetMappedBitmap());
  if (bitmap.empty()) {
    LOG(ERROR) << "PpFrameWriter::PutFrame - "
               << "The image_data's mapped bitmap failed.";
    return;
  }

  const uint8_t* src_data = static_cast<uint8_t*>(bitmap.getPixels());
  const int src_stride = static_cast<int>(bitmap.rowBytes());
  const int width = bitmap.width();
  const int height = bitmap.height();

  // We only support PP_IMAGEDATAFORMAT_BGRA_PREMUL at the moment.
  DCHECK(image_data->format() == PP_IMAGEDATAFORMAT_BGRA_PREMUL);

  const gfx::Size frame_size(width, height);

  if (state() != MediaStreamVideoSource::STARTED)
    return;

  const base::TimeDelta timestamp = base::TimeDelta::FromMicroseconds(
      time_stamp_ns / base::Time::kNanosecondsPerMicrosecond);

  scoped_refptr<media::VideoFrame> new_frame =
      frame_pool_.CreateFrame(media::PIXEL_FORMAT_I420, frame_size,
                              gfx::Rect(frame_size), frame_size, timestamp);

  libyuv::ARGBToI420(src_data,
                     src_stride,
                     new_frame->data(media::VideoFrame::kYPlane),
                     new_frame->stride(media::VideoFrame::kYPlane),
                     new_frame->data(media::VideoFrame::kUPlane),
                     new_frame->stride(media::VideoFrame::kUPlane),
                     new_frame->data(media::VideoFrame::kVPlane),
                     new_frame->stride(media::VideoFrame::kVPlane),
                     width,
                     height);

  delegate_->DeliverFrame(new_frame);
}

// PpFrameWriterProxy is a helper class to make sure the user won't use
// PpFrameWriter after it is released (IOW its owner - WebMediaStreamSource -
// is released).
class PpFrameWriterProxy : public FrameWriterInterface {
 public:
  explicit PpFrameWriterProxy(const base::WeakPtr<PpFrameWriter>& writer)
      : writer_(writer) {
    DCHECK(writer_);
  }

  ~PpFrameWriterProxy() override {}

  void PutFrame(PPB_ImageData_Impl* image_data,
                int64_t time_stamp_ns) override {
    writer_->PutFrame(image_data, time_stamp_ns);
  }

 private:
  base::WeakPtr<PpFrameWriter> writer_;

  DISALLOW_COPY_AND_ASSIGN(PpFrameWriterProxy);
};

bool PepperToVideoTrackAdapter::Open(MediaStreamRegistryInterface* registry,
                                     const std::string& url,
                                     FrameWriterInterface** frame_writer) {
  DVLOG(3) << "PepperToVideoTrackAdapter::Open";
  blink::WebMediaStream stream;
  if (registry) {
    stream = registry->GetMediaStream(url);
  } else {
    stream =
        blink::WebMediaStreamRegistry::LookupMediaStreamDescriptor(GURL(url));
  }
  if (stream.IsNull()) {
    LOG(ERROR) << "PepperToVideoTrackAdapter::Open - invalid url: " << url;
    return false;
  }

  // Create a new native video track and add it to |stream|.
  std::string track_id;
  // According to spec, a media stream source's id should be unique per
  // application. There's no easy way to strictly achieve that. The id
  // generated with this method should be unique for most of the cases but
  // theoretically it's possible we can get an id that's duplicated with the
  // existing sources.
  base::Base64Encode(base::RandBytesAsString(64), &track_id);

  PpFrameWriter* writer = new PpFrameWriter();

  // Create a new webkit video track.
  blink::WebMediaStreamSource webkit_source;
  blink::WebMediaStreamSource::Type type =
      blink::WebMediaStreamSource::kTypeVideo;
  blink::WebString webkit_track_id = blink::WebString::FromUTF8(track_id);
  webkit_source.Initialize(webkit_track_id, type, webkit_track_id,
                           false /* remote */);
  webkit_source.SetExtraData(writer);

  bool track_enabled = true;
  stream.AddTrack(MediaStreamVideoTrack::CreateVideoTrack(
      writer, MediaStreamVideoSource::ConstraintsCallback(), track_enabled));

  *frame_writer = new PpFrameWriterProxy(writer->AsWeakPtr());
  return true;
}

}  // namespace content
