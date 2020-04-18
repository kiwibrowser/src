// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_BUFFER_TRACKER_H_
#define MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_BUFFER_TRACKER_H_

#include <memory>

#include "base/synchronization/lock.h"
#include "media/capture/video/video_capture_buffer_handle.h"
#include "media/capture/video_capture_types.h"
#include "mojo/public/cpp/system/buffer.h"

namespace media {

// Keeps track of the state of a given mappable resource. This is a base class
// for implementations using different kinds of storage.
class CAPTURE_EXPORT VideoCaptureBufferTracker {
 public:
  VideoCaptureBufferTracker()
      : max_pixel_count_(0),
        held_by_producer_(false),
        consumer_hold_count_(0),
        frame_feedback_id_(0) {}
  virtual bool Init(const gfx::Size& dimensions, VideoPixelFormat format) = 0;
  virtual ~VideoCaptureBufferTracker(){};

  const gfx::Size& dimensions() const { return dimensions_; }
  void set_dimensions(const gfx::Size& dim) { dimensions_ = dim; }
  size_t max_pixel_count() const { return max_pixel_count_; }
  void set_max_pixel_count(size_t count) { max_pixel_count_ = count; }
  VideoPixelFormat pixel_format() const { return pixel_format_; }
  void set_pixel_format(VideoPixelFormat format) { pixel_format_ = format; }
  bool held_by_producer() const { return held_by_producer_; }
  void set_held_by_producer(bool value) { held_by_producer_ = value; }
  int consumer_hold_count() const { return consumer_hold_count_; }
  void set_consumer_hold_count(int value) { consumer_hold_count_ = value; }
  void set_frame_feedback_id(int value) { frame_feedback_id_ = value; }
  int frame_feedback_id() { return frame_feedback_id_; }

  virtual std::unique_ptr<VideoCaptureBufferHandle> GetMemoryMappedAccess() = 0;
  virtual mojo::ScopedSharedBufferHandle GetHandleForTransit(
      bool read_only) = 0;
  virtual base::SharedMemoryHandle
  GetNonOwnedSharedMemoryHandleForLegacyIPC() = 0;

 private:
  // |dimensions_| may change as a VideoCaptureBufferTracker is re-used, but
  // |max_pixel_count_|, |pixel_format_|, and |storage_type_| are set once for
  // the lifetime of a VideoCaptureBufferTracker.
  gfx::Size dimensions_;
  size_t max_pixel_count_;
  VideoPixelFormat pixel_format_;

  // Indicates whether this VideoCaptureBufferTracker is currently referenced by
  // the producer.
  bool held_by_producer_;

  // Number of consumer processes which hold this VideoCaptureBufferTracker.
  int consumer_hold_count_;

  int frame_feedback_id_;
};

}  // namespace content

#endif  // MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_BUFFER_TRACKER_H_
