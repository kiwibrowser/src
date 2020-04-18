// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_LINUX_V4L2_CAPTURE_DELEGATE_H_
#define MEDIA_CAPTURE_VIDEO_LINUX_V4L2_CAPTURE_DELEGATE_H_

#include <stddef.h>
#include <stdint.h>

#include "base/containers/queue.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "media/capture/video/video_capture_device.h"

#if defined(OS_OPENBSD)
#include <sys/videoio.h>
#else
#include <linux/videodev2.h>
#endif

namespace base {
class Location;
}  // namespace base

namespace media {

// Class doing the actual Linux capture using V4L2 API. V4L2 SPLANE/MPLANE
// capture specifics are implemented in derived classes. Created on the owner's
// thread, otherwise living, operating and destroyed on |v4l2_task_runner_|.
class CAPTURE_EXPORT V4L2CaptureDelegate final {
 public:
  // Retrieves the #planes for a given |fourcc|, or 0 if unknown.
  static size_t GetNumPlanesForFourCc(uint32_t fourcc);
  // Returns the Chrome pixel format for |v4l2_fourcc| or PIXEL_FORMAT_UNKNOWN.
  static VideoPixelFormat V4l2FourCcToChromiumPixelFormat(
      uint32_t v4l2_fourcc);

  // Composes a list of usable and supported pixel formats, in order of
  // preference, with MJPEG prioritised depending on |prefer_mjpeg|.
  static std::list<uint32_t> GetListOfUsableFourCcs(bool prefer_mjpeg);

  V4L2CaptureDelegate(
      const VideoCaptureDeviceDescriptor& device_descriptor,
      const scoped_refptr<base::SingleThreadTaskRunner>& v4l2_task_runner,
      int power_line_frequency);
  ~V4L2CaptureDelegate();

  // Forward-to versions of VideoCaptureDevice virtual methods.
  void AllocateAndStart(int width,
                        int height,
                        float frame_rate,
                        std::unique_ptr<VideoCaptureDevice::Client> client);
  void StopAndDeAllocate();

  void TakePhoto(VideoCaptureDevice::TakePhotoCallback callback);

  void GetPhotoState(VideoCaptureDevice::GetPhotoStateCallback callback);
  void SetPhotoOptions(mojom::PhotoSettingsPtr settings,
                       VideoCaptureDevice::SetPhotoOptionsCallback callback);

  void SetRotation(int rotation);

  base::WeakPtr<V4L2CaptureDelegate> GetWeakPtr();

 private:
  friend class V4L2CaptureDelegateTest;

  class BufferTracker;

  // VIDIOC_QUERYBUFs a buffer from V4L2, creates a BufferTracker for it and
  // enqueues it (VIDIOC_QBUF) back into V4L2.
  bool MapAndQueueBuffer(int index);

  void DoCapture();

  void SetErrorState(const base::Location& from_here,
                     const std::string& reason);

  const scoped_refptr<base::SingleThreadTaskRunner> v4l2_task_runner_;
  const VideoCaptureDeviceDescriptor device_descriptor_;
  const int power_line_frequency_;

  // The following members are only known on AllocateAndStart().
  VideoCaptureFormat capture_format_;
  v4l2_format video_fmt_;
  std::unique_ptr<VideoCaptureDevice::Client> client_;
  base::ScopedFD device_fd_;

  base::queue<VideoCaptureDevice::TakePhotoCallback> take_photo_callbacks_;

  // Vector of BufferTracker to keep track of mmap()ed pointers and their use.
  std::vector<scoped_refptr<BufferTracker>> buffer_tracker_pool_;

  bool is_capturing_;
  int timeout_count_;

  base::TimeTicks first_ref_time_;

  // Clockwise rotation in degrees. This value should be 0, 90, 180, or 270.
  int rotation_;

  base::WeakPtrFactory<V4L2CaptureDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(V4L2CaptureDelegate);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_LINUX_V4L2_CAPTURE_DELEGATE_H_
