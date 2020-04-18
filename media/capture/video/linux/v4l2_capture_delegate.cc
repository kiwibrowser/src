// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/linux/v4l2_capture_delegate.h"

#include <linux/version.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <utility>

#include "base/bind.h"
#include "base/files/file_enumerator.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "media/base/bind_to_current_loop.h"
#include "media/capture/video/blob_utils.h"
#include "media/capture/video/linux/video_capture_device_linux.h"

using media::mojom::MeteringMode;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
// 16 bit depth, Realsense F200.
#define V4L2_PIX_FMT_Z16 v4l2_fourcc('Z', '1', '6', ' ')
#endif

// TODO(aleksandar.stojiljkovic): Wrap this with kernel version check once the
// format is introduced to kernel.
// See https://crbug.com/661877
#ifndef V4L2_PIX_FMT_INVZ
// 16 bit depth, Realsense SR300.
#define V4L2_PIX_FMT_INVZ v4l2_fourcc('I', 'N', 'V', 'Z')
#endif

namespace media {

// Desired number of video buffers to allocate. The actual number of allocated
// buffers by v4l2 driver can be higher or lower than this number.
// kNumVideoBuffers should not be too small, or Chrome may not return enough
// buffers back to driver in time.
const uint32_t kNumVideoBuffers = 4;
// Timeout in milliseconds v4l2_thread_ blocks waiting for a frame from the hw.
// This value has been fine tuned. Before changing or modifying it see
// https://crbug.com/470717
const int kCaptureTimeoutMs = 1000;
// The number of continuous timeouts tolerated before treated as error.
const int kContinuousTimeoutLimit = 10;
// MJPEG is preferred if the requested width or height is larger than this.
const int kMjpegWidth = 640;
const int kMjpegHeight = 480;
// Typical framerate, in fps
const int kTypicalFramerate = 30;

// V4L2 color formats supported by V4L2CaptureDelegate derived classes.
// This list is ordered by precedence of use -- but see caveats for MJPEG.
static struct {
  uint32_t fourcc;
  VideoPixelFormat pixel_format;
  size_t num_planes;
} const kSupportedFormatsAndPlanarity[] = {
    {V4L2_PIX_FMT_YUV420, PIXEL_FORMAT_I420, 1},
    {V4L2_PIX_FMT_Y16, PIXEL_FORMAT_Y16, 1},
    {V4L2_PIX_FMT_Z16, PIXEL_FORMAT_Y16, 1},
    {V4L2_PIX_FMT_INVZ, PIXEL_FORMAT_Y16, 1},
    {V4L2_PIX_FMT_YUYV, PIXEL_FORMAT_YUY2, 1},
    {V4L2_PIX_FMT_UYVY, PIXEL_FORMAT_UYVY, 1},
    {V4L2_PIX_FMT_RGB24, PIXEL_FORMAT_RGB24, 1},
    // MJPEG is usually sitting fairly low since we don't want to have to
    // decode. However, it is needed for large resolutions due to USB bandwidth
    // limitations, so GetListOfUsableFourCcs() can duplicate it on top, see
    // that method.
    {V4L2_PIX_FMT_MJPEG, PIXEL_FORMAT_MJPEG, 1},
    // JPEG works as MJPEG on some gspca webcams from field reports, see
    // https://code.google.com/p/webrtc/issues/detail?id=529, put it as the
    // least preferred format.
    {V4L2_PIX_FMT_JPEG, PIXEL_FORMAT_MJPEG, 1},
};

// Maximum number of ioctl retries before giving up trying to reset controls.
const int kMaxIOCtrlRetries = 5;

// Base id and class identifier for Controls to be reset.
static struct {
  uint32_t control_base;
  uint32_t class_id;
} const kControls[] = {{V4L2_CID_USER_BASE, V4L2_CID_USER_CLASS},
                       {V4L2_CID_CAMERA_CLASS_BASE, V4L2_CID_CAMERA_CLASS}};

// Fill in |format| with the given parameters.
static void FillV4L2Format(v4l2_format* format,
                           uint32_t width,
                           uint32_t height,
                           uint32_t pixelformat_fourcc) {
  memset(format, 0, sizeof(*format));
  format->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format->fmt.pix.width = width;
  format->fmt.pix.height = height;
  format->fmt.pix.pixelformat = pixelformat_fourcc;
}

// Fills all parts of |buffer|.
static void FillV4L2Buffer(v4l2_buffer* buffer, int index) {
  memset(buffer, 0, sizeof(*buffer));
  buffer->memory = V4L2_MEMORY_MMAP;
  buffer->index = index;
  buffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

static void FillV4L2RequestBuffer(v4l2_requestbuffers* request_buffer,
                                  int count) {
  memset(request_buffer, 0, sizeof(*request_buffer));
  request_buffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  request_buffer->memory = V4L2_MEMORY_MMAP;
  request_buffer->count = count;
}

// Returns the input |fourcc| as a std::string four char representation.
static std::string FourccToString(uint32_t fourcc) {
  return base::StringPrintf("%c%c%c%c", fourcc & 0xFF, (fourcc >> 8) & 0xFF,
                            (fourcc >> 16) & 0xFF, (fourcc >> 24) & 0xFF);
}

// Running ioctl() on some devices, especially shortly after (re)opening the
// device file descriptor or (re)starting streaming, can fail but works after
// retrying (https://crbug.com/670262).
// Returns false if the |request| ioctl fails too many times.
static bool RunIoctl(int fd, int request, void* argp) {
  int num_retries = 0;
  for (; HANDLE_EINTR(ioctl(fd, request, argp)) < 0 &&
         num_retries < kMaxIOCtrlRetries;
       ++num_retries) {
    DPLOG(WARNING) << "ioctl";
  }
  DPLOG_IF(ERROR, num_retries != kMaxIOCtrlRetries);
  return num_retries != kMaxIOCtrlRetries;
}

// Creates a mojom::RangePtr with the (min, max, current, step) values of the
// control associated with |control_id|. Returns an empty Range otherwise.
static mojom::RangePtr RetrieveUserControlRange(int device_fd, int control_id) {
  mojom::RangePtr capability = mojom::Range::New();

  v4l2_queryctrl range = {};
  range.id = control_id;
  range.type = V4L2_CTRL_TYPE_INTEGER;
  if (!RunIoctl(device_fd, VIDIOC_QUERYCTRL, &range))
    return mojom::Range::New();
  capability->max = range.maximum;
  capability->min = range.minimum;
  capability->step = range.step;

  v4l2_control current = {};
  current.id = control_id;
  if (!RunIoctl(device_fd, VIDIOC_G_CTRL, &current))
    return mojom::Range::New();
  capability->current = current.value;

  return capability;
}

// Determines if |control_id| is special, i.e. controls another one's state.
static bool IsSpecialControl(int control_id) {
  switch (control_id) {
    case V4L2_CID_AUTO_WHITE_BALANCE:
    case V4L2_CID_EXPOSURE_AUTO:
    case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
    case V4L2_CID_FOCUS_AUTO:
      return true;
  }
  return false;
}

// Determines if |control_id| should be skipped, https://crbug.com/697885.
#if !defined(V4L2_CID_PAN_SPEED)
#define V4L2_CID_PAN_SPEED (V4L2_CID_CAMERA_CLASS_BASE + 32)
#endif
#if !defined(V4L2_CID_TILT_SPEED)
#define V4L2_CID_TILT_SPEED (V4L2_CID_CAMERA_CLASS_BASE + 33)
#endif
#if !defined(V4L2_CID_PANTILT_CMD)
#define V4L2_CID_PANTILT_CMD (V4L2_CID_CAMERA_CLASS_BASE + 34)
#endif
static bool IsBlacklistedControl(int control_id) {
  switch (control_id) {
    case V4L2_CID_PAN_RELATIVE:
    case V4L2_CID_TILT_RELATIVE:
    case V4L2_CID_PAN_RESET:
    case V4L2_CID_TILT_RESET:
    case V4L2_CID_PAN_ABSOLUTE:
    case V4L2_CID_TILT_ABSOLUTE:
    case V4L2_CID_ZOOM_ABSOLUTE:
    case V4L2_CID_ZOOM_RELATIVE:
    case V4L2_CID_ZOOM_CONTINUOUS:
    case V4L2_CID_PAN_SPEED:
    case V4L2_CID_TILT_SPEED:
    case V4L2_CID_PANTILT_CMD:
      return true;
  }
  return false;
}

// Sets all user control to their default. Some controls are enabled by another
// flag, usually having the word "auto" in the name, see IsSpecialControl().
// These flags are preset beforehand, then set to their defaults individually
// afterwards.
static void ResetUserAndCameraControlsToDefault(int device_fd) {
  // Set V4L2_CID_AUTO_WHITE_BALANCE to false first.
  v4l2_control auto_white_balance = {};
  auto_white_balance.id = V4L2_CID_AUTO_WHITE_BALANCE;
  auto_white_balance.value = false;
  if (!RunIoctl(device_fd, VIDIOC_S_CTRL, &auto_white_balance))
    return;

  std::vector<struct v4l2_ext_control> special_camera_controls;
  // Set V4L2_CID_EXPOSURE_AUTO to V4L2_EXPOSURE_MANUAL.
  v4l2_ext_control auto_exposure = {};
  auto_exposure.id = V4L2_CID_EXPOSURE_AUTO;
  auto_exposure.value = V4L2_EXPOSURE_MANUAL;
  special_camera_controls.push_back(auto_exposure);
  // Set V4L2_CID_EXPOSURE_AUTO_PRIORITY to false.
  v4l2_ext_control priority_auto_exposure = {};
  priority_auto_exposure.id = V4L2_CID_EXPOSURE_AUTO_PRIORITY;
  priority_auto_exposure.value = false;
  special_camera_controls.push_back(priority_auto_exposure);
  // Set V4L2_CID_FOCUS_AUTO to false.
  v4l2_ext_control auto_focus = {};
  auto_focus.id = V4L2_CID_FOCUS_AUTO;
  auto_focus.value = false;
  special_camera_controls.push_back(auto_focus);

  struct v4l2_ext_controls ext_controls = {};
  ext_controls.ctrl_class = V4L2_CID_CAMERA_CLASS;
  ext_controls.count = special_camera_controls.size();
  ext_controls.controls = special_camera_controls.data();
  if (HANDLE_EINTR(ioctl(device_fd, VIDIOC_S_EXT_CTRLS, &ext_controls)) < 0)
    DPLOG(ERROR) << "VIDIOC_S_EXT_CTRLS";

  std::vector<struct v4l2_ext_control> camera_controls;
  for (const auto& control : kControls) {
    std::vector<struct v4l2_ext_control> camera_controls;

    v4l2_queryctrl range = {};
    range.id = control.control_base | V4L2_CTRL_FLAG_NEXT_CTRL;
    while (0 == HANDLE_EINTR(ioctl(device_fd, VIDIOC_QUERYCTRL, &range))) {
      if (V4L2_CTRL_ID2CLASS(range.id) != V4L2_CTRL_ID2CLASS(control.class_id))
        break;
      range.id |= V4L2_CTRL_FLAG_NEXT_CTRL;

      if (IsSpecialControl(range.id & ~V4L2_CTRL_FLAG_NEXT_CTRL))
        continue;
      if (IsBlacklistedControl(range.id & ~V4L2_CTRL_FLAG_NEXT_CTRL))
        continue;

      struct v4l2_ext_control ext_control = {};
      ext_control.id = range.id & ~V4L2_CTRL_FLAG_NEXT_CTRL;
      ext_control.value = range.default_value;
      camera_controls.push_back(ext_control);
    }

    if (!camera_controls.empty()) {
      struct v4l2_ext_controls ext_controls = {};
      ext_controls.ctrl_class = control.class_id;
      ext_controls.count = camera_controls.size();
      ext_controls.controls = camera_controls.data();
      if (HANDLE_EINTR(ioctl(device_fd, VIDIOC_S_EXT_CTRLS, &ext_controls)) < 0)
        DPLOG(ERROR) << "VIDIOC_S_EXT_CTRLS";
    }
  }

  // Now set the special flags to the default values
  v4l2_queryctrl range = {};
  range.id = V4L2_CID_AUTO_WHITE_BALANCE;
  HANDLE_EINTR(ioctl(device_fd, VIDIOC_QUERYCTRL, &range));
  auto_white_balance.value = range.default_value;
  HANDLE_EINTR(ioctl(device_fd, VIDIOC_S_CTRL, &auto_white_balance));

  special_camera_controls.clear();
  memset(&range, 0, sizeof(struct v4l2_queryctrl));
  range.id = V4L2_CID_EXPOSURE_AUTO;
  HANDLE_EINTR(ioctl(device_fd, VIDIOC_QUERYCTRL, &range));
  auto_exposure.value = range.default_value;
  special_camera_controls.push_back(auto_exposure);

  memset(&range, 0, sizeof(struct v4l2_queryctrl));
  range.id = V4L2_CID_EXPOSURE_AUTO_PRIORITY;
  HANDLE_EINTR(ioctl(device_fd, VIDIOC_QUERYCTRL, &range));
  priority_auto_exposure.value = range.default_value;
  special_camera_controls.push_back(priority_auto_exposure);

  memset(&range, 0, sizeof(struct v4l2_queryctrl));
  range.id = V4L2_CID_FOCUS_AUTO;
  HANDLE_EINTR(ioctl(device_fd, VIDIOC_QUERYCTRL, &range));
  auto_focus.value = range.default_value;
  special_camera_controls.push_back(auto_focus);

  memset(&ext_controls, 0, sizeof(struct v4l2_ext_controls));
  ext_controls.ctrl_class = V4L2_CID_CAMERA_CLASS;
  ext_controls.count = special_camera_controls.size();
  ext_controls.controls = special_camera_controls.data();
  if (HANDLE_EINTR(ioctl(device_fd, VIDIOC_S_EXT_CTRLS, &ext_controls)) < 0)
    DPLOG(ERROR) << "VIDIOC_S_EXT_CTRLS";
}

// Class keeping track of a SPLANE V4L2 buffer, mmap()ed on construction and
// munmap()ed on destruction.
class V4L2CaptureDelegate::BufferTracker
    : public base::RefCounted<BufferTracker> {
 public:
  BufferTracker();
  // Abstract method to mmap() given |fd| according to |buffer|.
  bool Init(int fd, const v4l2_buffer& buffer);

  const uint8_t* start() const { return start_; }
  size_t payload_size() const { return payload_size_; }
  void set_payload_size(size_t payload_size) {
    DCHECK_LE(payload_size, length_);
    payload_size_ = payload_size;
  }

 private:
  friend class base::RefCounted<BufferTracker>;
  virtual ~BufferTracker();

  uint8_t* start_;
  size_t length_;
  size_t payload_size_;
};

// static
size_t V4L2CaptureDelegate::GetNumPlanesForFourCc(uint32_t fourcc) {
  for (const auto& fourcc_and_pixel_format : kSupportedFormatsAndPlanarity) {
    if (fourcc_and_pixel_format.fourcc == fourcc)
      return fourcc_and_pixel_format.num_planes;
  }
  DVLOG(1) << "Unknown fourcc " << FourccToString(fourcc);
  return 0;
}

// static
VideoPixelFormat V4L2CaptureDelegate::V4l2FourCcToChromiumPixelFormat(
    uint32_t v4l2_fourcc) {
  for (const auto& fourcc_and_pixel_format : kSupportedFormatsAndPlanarity) {
    if (fourcc_and_pixel_format.fourcc == v4l2_fourcc)
      return fourcc_and_pixel_format.pixel_format;
  }
  // Not finding a pixel format is OK during device capabilities enumeration.
  // Let the caller decide if PIXEL_FORMAT_UNKNOWN is an error or
  // not.
  DVLOG(1) << "Unsupported pixel format: " << FourccToString(v4l2_fourcc);
  return PIXEL_FORMAT_UNKNOWN;
}

// static
std::list<uint32_t> V4L2CaptureDelegate::GetListOfUsableFourCcs(
    bool prefer_mjpeg) {
  std::list<uint32_t> supported_formats;
  for (const auto& format : kSupportedFormatsAndPlanarity)
    supported_formats.push_back(format.fourcc);

  // Duplicate MJPEG on top of the list depending on |prefer_mjpeg|.
  if (prefer_mjpeg)
    supported_formats.push_front(V4L2_PIX_FMT_MJPEG);

  return supported_formats;
}

V4L2CaptureDelegate::V4L2CaptureDelegate(
    const VideoCaptureDeviceDescriptor& device_descriptor,
    const scoped_refptr<base::SingleThreadTaskRunner>& v4l2_task_runner,
    int power_line_frequency)
    : v4l2_task_runner_(v4l2_task_runner),
      device_descriptor_(device_descriptor),
      power_line_frequency_(power_line_frequency),
      is_capturing_(false),
      timeout_count_(0),
      rotation_(0),
      weak_factory_(this) {}

void V4L2CaptureDelegate::AllocateAndStart(
    int width,
    int height,
    float frame_rate,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
  DCHECK(v4l2_task_runner_->BelongsToCurrentThread());
  DCHECK(client);
  client_ = std::move(client);

  // Need to open camera with O_RDWR after Linux kernel 3.3.
  device_fd_.reset(
      HANDLE_EINTR(open(device_descriptor_.device_id.c_str(), O_RDWR)));
  if (!device_fd_.is_valid()) {
    SetErrorState(FROM_HERE, "Failed to open V4L2 device driver file.");
    return;
  }

  ResetUserAndCameraControlsToDefault(device_fd_.get());

  v4l2_capability cap = {};
  if (!((HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_QUERYCAP, &cap)) == 0) &&
        ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) &&
         !(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)))) {
    device_fd_.reset();
    SetErrorState(FROM_HERE, "This is not a V4L2 video capture device");
    return;
  }

  // Get supported video formats in preferred order. For large resolutions,
  // favour mjpeg over raw formats.
  const std::list<uint32_t>& desired_v4l2_formats =
      GetListOfUsableFourCcs(width > kMjpegWidth || height > kMjpegHeight);
  std::list<uint32_t>::const_iterator best = desired_v4l2_formats.end();

  v4l2_fmtdesc fmtdesc = {};
  fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  for (; HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_ENUM_FMT, &fmtdesc)) == 0;
       ++fmtdesc.index) {
    best = std::find(desired_v4l2_formats.begin(), best, fmtdesc.pixelformat);
  }
  if (best == desired_v4l2_formats.end()) {
    SetErrorState(FROM_HERE, "Failed to find a supported camera format.");
    return;
  }

  DVLOG(1) << "Chosen pixel format is " << FourccToString(*best);
  FillV4L2Format(&video_fmt_, width, height, *best);

  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_FMT, &video_fmt_)) < 0) {
    SetErrorState(FROM_HERE, "Failed to set video capture format");
    return;
  }
  const VideoPixelFormat pixel_format =
      V4l2FourCcToChromiumPixelFormat(video_fmt_.fmt.pix.pixelformat);
  if (pixel_format == PIXEL_FORMAT_UNKNOWN) {
    SetErrorState(FROM_HERE, "Unsupported pixel format");
    return;
  }

  // Set capture framerate in the form of capture interval.
  v4l2_streamparm streamparm = {};
  streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  // The following line checks that the driver knows about framerate get/set.
  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_G_PARM, &streamparm)) >= 0) {
    // Now check if the device is able to accept a capture framerate set.
    if (streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
      // |frame_rate| is float, approximate by a fraction.
      streamparm.parm.capture.timeperframe.numerator = kFrameRatePrecision;
      streamparm.parm.capture.timeperframe.denominator =
          (frame_rate) ? (frame_rate * kFrameRatePrecision)
                       : (kTypicalFramerate * kFrameRatePrecision);

      if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_PARM, &streamparm)) <
          0) {
        SetErrorState(FROM_HERE, "Failed to set camera framerate");
        return;
      }
      DVLOG(2) << "Actual camera driverframerate: "
               << streamparm.parm.capture.timeperframe.denominator << "/"
               << streamparm.parm.capture.timeperframe.numerator;
    }
  }
  // TODO(mcasas): what should be done if the camera driver does not allow
  // framerate configuration, or the actual one is different from the desired?

  // Set anti-banding/anti-flicker to 50/60Hz. May fail due to not supported
  // operation (|errno| == EINVAL in this case) or plain failure.
  if ((power_line_frequency_ == V4L2_CID_POWER_LINE_FREQUENCY_50HZ) ||
      (power_line_frequency_ == V4L2_CID_POWER_LINE_FREQUENCY_60HZ) ||
      (power_line_frequency_ == V4L2_CID_POWER_LINE_FREQUENCY_AUTO)) {
    struct v4l2_control control = {};
    control.id = V4L2_CID_POWER_LINE_FREQUENCY;
    control.value = power_line_frequency_;
    const int retval =
        HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &control));
    if (retval != 0)
      DVLOG(1) << "Error setting power line frequency removal";
  }

  capture_format_.frame_size.SetSize(video_fmt_.fmt.pix.width,
                                     video_fmt_.fmt.pix.height);
  capture_format_.frame_rate = frame_rate;
  capture_format_.pixel_format = pixel_format;

  v4l2_requestbuffers r_buffer;
  FillV4L2RequestBuffer(&r_buffer, kNumVideoBuffers);
  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_REQBUFS, &r_buffer)) < 0) {
    SetErrorState(FROM_HERE, "Error requesting MMAP buffers from V4L2");
    return;
  }
  for (unsigned int i = 0; i < r_buffer.count; ++i) {
    if (!MapAndQueueBuffer(i)) {
      SetErrorState(FROM_HERE, "Allocate buffer failed");
      return;
    }
  }

  v4l2_buf_type capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_STREAMON, &capture_type)) <
      0) {
    SetErrorState(FROM_HERE, "VIDIOC_STREAMON failed");
    return;
  }

  client_->OnStarted();
  is_capturing_ = true;

  // Post task to start fetching frames from v4l2.
  v4l2_task_runner_->PostTask(
      FROM_HERE, base::Bind(&V4L2CaptureDelegate::DoCapture, GetWeakPtr()));
}

void V4L2CaptureDelegate::StopAndDeAllocate() {
  DCHECK(v4l2_task_runner_->BelongsToCurrentThread());
  // The order is important: stop streaming, clear |buffer_pool_|,
  // thus munmap()ing the v4l2_buffers, and then return them to the OS.
  v4l2_buf_type capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_STREAMOFF, &capture_type)) <
      0) {
    SetErrorState(FROM_HERE, "VIDIOC_STREAMOFF failed");
    return;
  }

  buffer_tracker_pool_.clear();

  v4l2_requestbuffers r_buffer;
  FillV4L2RequestBuffer(&r_buffer, 0);
  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_REQBUFS, &r_buffer)) < 0)
    SetErrorState(FROM_HERE, "Failed to VIDIOC_REQBUFS with count = 0");

  // At this point we can close the device.
  // This is also needed for correctly changing settings later via VIDIOC_S_FMT.
  device_fd_.reset();
  is_capturing_ = false;
  client_.reset();
}

void V4L2CaptureDelegate::TakePhoto(
    VideoCaptureDevice::TakePhotoCallback callback) {
  DCHECK(v4l2_task_runner_->BelongsToCurrentThread());
  take_photo_callbacks_.push(std::move(callback));
}

void V4L2CaptureDelegate::GetPhotoState(
    VideoCaptureDevice::GetPhotoStateCallback callback) {
  DCHECK(v4l2_task_runner_->BelongsToCurrentThread());
  if (!device_fd_.is_valid() || !is_capturing_)
    return;

  mojom::PhotoStatePtr photo_capabilities = mojom::PhotoState::New();

  photo_capabilities->zoom =
      RetrieveUserControlRange(device_fd_.get(), V4L2_CID_ZOOM_ABSOLUTE);

  v4l2_queryctrl manual_focus_ctrl = {};
  manual_focus_ctrl.id = V4L2_CID_FOCUS_ABSOLUTE;
  if (RunIoctl(device_fd_.get(), VIDIOC_QUERYCTRL, &manual_focus_ctrl))
    photo_capabilities->supported_focus_modes.push_back(MeteringMode::MANUAL);

  v4l2_queryctrl auto_focus_ctrl = {};
  auto_focus_ctrl.id = V4L2_CID_FOCUS_AUTO;
  if (RunIoctl(device_fd_.get(), VIDIOC_QUERYCTRL, &auto_focus_ctrl)) {
    photo_capabilities->supported_focus_modes.push_back(
        MeteringMode::CONTINUOUS);
  }

  photo_capabilities->current_focus_mode = MeteringMode::NONE;
  v4l2_control auto_focus_current = {};
  auto_focus_current.id = V4L2_CID_FOCUS_AUTO;
  if (HANDLE_EINTR(
          ioctl(device_fd_.get(), VIDIOC_G_CTRL, &auto_focus_current)) >= 0) {
    photo_capabilities->current_focus_mode = auto_focus_current.value
                                                 ? MeteringMode::CONTINUOUS
                                                 : MeteringMode::MANUAL;
  }

  v4l2_queryctrl auto_exposure_ctrl = {};
  auto_exposure_ctrl.id = V4L2_CID_EXPOSURE_AUTO;
  if (RunIoctl(device_fd_.get(), VIDIOC_QUERYCTRL, &auto_exposure_ctrl)) {
    photo_capabilities->supported_exposure_modes.push_back(
        MeteringMode::MANUAL);
    photo_capabilities->supported_exposure_modes.push_back(
        MeteringMode::CONTINUOUS);
  }

  photo_capabilities->current_exposure_mode = MeteringMode::NONE;
  v4l2_control exposure_current = {};
  exposure_current.id = V4L2_CID_EXPOSURE_AUTO;
  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_G_CTRL, &exposure_current)) >=
      0) {
    photo_capabilities->current_exposure_mode =
        exposure_current.value == V4L2_EXPOSURE_MANUAL
            ? MeteringMode::MANUAL
            : MeteringMode::CONTINUOUS;
  }

  photo_capabilities->exposure_compensation =
      RetrieveUserControlRange(device_fd_.get(), V4L2_CID_EXPOSURE_ABSOLUTE);

  photo_capabilities->color_temperature = RetrieveUserControlRange(
      device_fd_.get(), V4L2_CID_WHITE_BALANCE_TEMPERATURE);
  if (photo_capabilities->color_temperature) {
    photo_capabilities->supported_white_balance_modes.push_back(
        MeteringMode::MANUAL);
  }

  v4l2_queryctrl white_balance_ctrl = {};
  white_balance_ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
  if (RunIoctl(device_fd_.get(), VIDIOC_QUERYCTRL, &white_balance_ctrl)) {
    photo_capabilities->supported_white_balance_modes.push_back(
        MeteringMode::CONTINUOUS);
  }

  photo_capabilities->current_white_balance_mode = MeteringMode::NONE;
  v4l2_control white_balance_current = {};
  white_balance_current.id = V4L2_CID_AUTO_WHITE_BALANCE;
  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_G_CTRL,
                         &white_balance_current)) >= 0) {
    photo_capabilities->current_white_balance_mode =
        white_balance_current.value ? MeteringMode::CONTINUOUS
                                    : MeteringMode::MANUAL;
  }

  photo_capabilities->iso = mojom::Range::New();
  photo_capabilities->height = mojom::Range::New(
      capture_format_.frame_size.height(), capture_format_.frame_size.height(),
      capture_format_.frame_size.height(), 0 /* step */);
  photo_capabilities->width = mojom::Range::New(
      capture_format_.frame_size.width(), capture_format_.frame_size.width(),
      capture_format_.frame_size.width(), 0 /* step */);
  photo_capabilities->red_eye_reduction = mojom::RedEyeReduction::NEVER;
  photo_capabilities->torch = false;

  photo_capabilities->brightness =
      RetrieveUserControlRange(device_fd_.get(), V4L2_CID_BRIGHTNESS);
  photo_capabilities->contrast =
      RetrieveUserControlRange(device_fd_.get(), V4L2_CID_CONTRAST);
  photo_capabilities->saturation =
      RetrieveUserControlRange(device_fd_.get(), V4L2_CID_SATURATION);
  photo_capabilities->sharpness =
      RetrieveUserControlRange(device_fd_.get(), V4L2_CID_SHARPNESS);

  std::move(callback).Run(std::move(photo_capabilities));
}

void V4L2CaptureDelegate::SetPhotoOptions(
    mojom::PhotoSettingsPtr settings,
    VideoCaptureDevice::SetPhotoOptionsCallback callback) {
  DCHECK(v4l2_task_runner_->BelongsToCurrentThread());
  if (!device_fd_.is_valid() || !is_capturing_)
    return;

  if (settings->has_zoom) {
    v4l2_control zoom_current = {};
    zoom_current.id = V4L2_CID_ZOOM_ABSOLUTE;
    zoom_current.value = settings->zoom;
    if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &zoom_current)) < 0)
      DPLOG(ERROR) << "setting zoom to " << settings->zoom;
  }

  if (settings->has_white_balance_mode &&
      (settings->white_balance_mode == mojom::MeteringMode::CONTINUOUS ||
       settings->white_balance_mode == mojom::MeteringMode::MANUAL)) {
    v4l2_control white_balance_set = {};
    white_balance_set.id = V4L2_CID_AUTO_WHITE_BALANCE;
    white_balance_set.value =
        settings->white_balance_mode == mojom::MeteringMode::CONTINUOUS;
    HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &white_balance_set));
  }

  if (settings->has_color_temperature) {
    v4l2_control auto_white_balance_current = {};
    auto_white_balance_current.id = V4L2_CID_AUTO_WHITE_BALANCE;
    const int result = HANDLE_EINTR(
        ioctl(device_fd_.get(), VIDIOC_G_CTRL, &auto_white_balance_current));
    // Color temperature can only be applied if Auto White Balance is off.
    if (result >= 0 && !auto_white_balance_current.value) {
      v4l2_control set_temperature = {};
      set_temperature.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
      set_temperature.value = settings->color_temperature;
      HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &set_temperature));
    }
  }

  if (settings->has_exposure_mode &&
      (settings->exposure_mode == mojom::MeteringMode::CONTINUOUS ||
       settings->exposure_mode == mojom::MeteringMode::MANUAL)) {
    v4l2_control exposure_mode_set = {};
    exposure_mode_set.id = V4L2_CID_EXPOSURE_AUTO;
    exposure_mode_set.value =
        settings->exposure_mode == mojom::MeteringMode::CONTINUOUS
            ? V4L2_EXPOSURE_APERTURE_PRIORITY
            : V4L2_EXPOSURE_MANUAL;
    HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &exposure_mode_set));
  }

  if (settings->has_exposure_compensation) {
    v4l2_control auto_exposure_current = {};
    auto_exposure_current.id = V4L2_CID_EXPOSURE_AUTO;
    const int result = HANDLE_EINTR(
        ioctl(device_fd_.get(), VIDIOC_G_CTRL, &auto_exposure_current));
    // Exposure Compensation can only be applied if Auto Exposure is off.
    if (result >= 0 && auto_exposure_current.value == V4L2_EXPOSURE_MANUAL) {
      v4l2_control set_exposure = {};
      set_exposure.id = V4L2_CID_EXPOSURE_ABSOLUTE;
      set_exposure.value = settings->exposure_compensation;
      HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &set_exposure));
    }
  }

  if (settings->has_brightness) {
    v4l2_control current = {};
    current.id = V4L2_CID_BRIGHTNESS;
    current.value = settings->brightness;
    if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &current)) < 0)
      DPLOG(ERROR) << "setting brightness to " << settings->brightness;
  }
  if (settings->has_contrast) {
    v4l2_control current = {};
    current.id = V4L2_CID_CONTRAST;
    current.value = settings->contrast;
    if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &current)) < 0)
      DPLOG(ERROR) << "setting contrast to " << settings->contrast;
  }
  if (settings->has_saturation) {
    v4l2_control current = {};
    current.id = V4L2_CID_SATURATION;
    current.value = settings->saturation;
    if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &current)) < 0)
      DPLOG(ERROR) << "setting saturation to " << settings->saturation;
  }
  if (settings->has_sharpness) {
    v4l2_control current = {};
    current.id = V4L2_CID_SHARPNESS;
    current.value = settings->sharpness;
    if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_S_CTRL, &current)) < 0)
      DPLOG(ERROR) << "setting sharpness to " << settings->sharpness;
  }

  std::move(callback).Run(true);
}

void V4L2CaptureDelegate::SetRotation(int rotation) {
  DCHECK(v4l2_task_runner_->BelongsToCurrentThread());
  DCHECK(rotation >= 0 && rotation < 360 && rotation % 90 == 0);
  rotation_ = rotation;
}

base::WeakPtr<V4L2CaptureDelegate> V4L2CaptureDelegate::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

V4L2CaptureDelegate::~V4L2CaptureDelegate() = default;

bool V4L2CaptureDelegate::MapAndQueueBuffer(int index) {
  v4l2_buffer buffer;
  FillV4L2Buffer(&buffer, index);

  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_QUERYBUF, &buffer)) < 0) {
    DLOG(ERROR) << "Error querying status of a MMAP V4L2 buffer";
    return false;
  }

  const scoped_refptr<BufferTracker> buffer_tracker(new BufferTracker());
  if (!buffer_tracker->Init(device_fd_.get(), buffer)) {
    DLOG(ERROR) << "Error creating BufferTracker";
    return false;
  }
  buffer_tracker_pool_.push_back(buffer_tracker);

  // Enqueue the buffer in the drivers incoming queue.
  if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_QBUF, &buffer)) < 0) {
    DLOG(ERROR) << "Error enqueuing a V4L2 buffer back into the driver";
    return false;
  }
  return true;
}

void V4L2CaptureDelegate::DoCapture() {
  DCHECK(v4l2_task_runner_->BelongsToCurrentThread());
  if (!is_capturing_)
    return;

  pollfd device_pfd = {};
  device_pfd.fd = device_fd_.get();
  device_pfd.events = POLLIN;
  const int result = HANDLE_EINTR(poll(&device_pfd, 1, kCaptureTimeoutMs));
  if (result < 0) {
    SetErrorState(FROM_HERE, "Poll failed");
    return;
  }
  // Check if poll() timed out; track the amount of times it did in a row and
  // throw an error if it times out too many times.
  if (result == 0) {
    timeout_count_++;
    if (timeout_count_ >= kContinuousTimeoutLimit) {
      SetErrorState(FROM_HERE,
                    "Multiple continuous timeouts while read-polling.");
      timeout_count_ = 0;
      return;
    }
  } else {
    timeout_count_ = 0;
  }

  // Deenqueue, send and reenqueue a buffer if the driver has filled one in.
  if (device_pfd.revents & POLLIN) {
    v4l2_buffer buffer;
    FillV4L2Buffer(&buffer, 0);

    if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_DQBUF, &buffer)) < 0) {
      SetErrorState(FROM_HERE, "Failed to dequeue capture buffer");
      return;
    }

    buffer_tracker_pool_[buffer.index]->set_payload_size(buffer.bytesused);
    const scoped_refptr<BufferTracker>& buffer_tracker =
        buffer_tracker_pool_[buffer.index];

    // There's a wide-spread issue where the kernel does not report accurate,
    // monotonically-increasing timestamps in the v4l2_buffer::timestamp
    // field (goo.gl/Nlfamz).
    // Until this issue is fixed, just use the reference clock as a source of
    // media timestamps.
    const base::TimeTicks now = base::TimeTicks::Now();
    if (first_ref_time_.is_null())
      first_ref_time_ = now;
    const base::TimeDelta timestamp = now - first_ref_time_;

#ifdef V4L2_BUF_FLAG_ERROR
    if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
      LOG(ERROR) << "Dequeued v4l2 buffer contains corrupted data ("
                 << buffer.bytesused << " bytes).";
      buffer.bytesused = 0;
    } else
#endif
        if (buffer.bytesused < capture_format_.ImageAllocationSize()) {
      LOG(ERROR) << "Dequeued v4l2 buffer contains invalid length ("
                 << buffer.bytesused << " bytes).";
      buffer.bytesused = 0;
    } else
      client_->OnIncomingCapturedData(
          buffer_tracker->start(), buffer_tracker->payload_size(),
          capture_format_, rotation_, now, timestamp);

    while (!take_photo_callbacks_.empty()) {
      VideoCaptureDevice::TakePhotoCallback cb =
          std::move(take_photo_callbacks_.front());
      take_photo_callbacks_.pop();

      mojom::BlobPtr blob =
          Blobify(buffer_tracker->start(), buffer.bytesused, capture_format_);
      if (blob)
        std::move(cb).Run(std::move(blob));
    }

    if (HANDLE_EINTR(ioctl(device_fd_.get(), VIDIOC_QBUF, &buffer)) < 0) {
      SetErrorState(FROM_HERE, "Failed to enqueue capture buffer");
      return;
    }
  }

  v4l2_task_runner_->PostTask(
      FROM_HERE, base::Bind(&V4L2CaptureDelegate::DoCapture, GetWeakPtr()));
}

void V4L2CaptureDelegate::SetErrorState(const base::Location& from_here,
                                        const std::string& reason) {
  DCHECK(v4l2_task_runner_->BelongsToCurrentThread());
  is_capturing_ = false;
  client_->OnError(from_here, reason);
}

V4L2CaptureDelegate::BufferTracker::BufferTracker() = default;

V4L2CaptureDelegate::BufferTracker::~BufferTracker() {
  if (start_ == nullptr)
    return;
  const int result = munmap(start_, length_);
  PLOG_IF(ERROR, result < 0) << "Error munmap()ing V4L2 buffer";
}

bool V4L2CaptureDelegate::BufferTracker::Init(int fd,
                                              const v4l2_buffer& buffer) {
  // Some devices require mmap() to be called with both READ and WRITE.
  // See http://crbug.com/178582.
  void* const start = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, buffer.m.offset);
  if (start == MAP_FAILED) {
    DLOG(ERROR) << "Error mmap()ing a V4L2 buffer into userspace";
    return false;
  }
  start_ = static_cast<uint8_t*>(start);
  length_ = buffer.length;
  payload_size_ = 0;
  return true;
}

}  // namespace media
