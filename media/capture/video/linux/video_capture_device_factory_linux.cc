// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/linux/video_capture_device_factory_linux.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"

#if defined(OS_OPENBSD)
#include <sys/videoio.h>
#else
#include <linux/videodev2.h>
#endif

#if defined(OS_CHROMEOS)
#include "media/capture/video/linux/camera_config_chromeos.h"
#include "media/capture/video/linux/video_capture_device_chromeos.h"
#endif
#include "media/capture/video/linux/video_capture_device_linux.h"

namespace media {

namespace {

// USB VID and PID are both 4 bytes long.
const size_t kVidPidSize = 4;
const size_t kMaxInterfaceNameSize = 256;

// /sys/class/video4linux/video{N}/device is a symlink to the corresponding
// USB device info directory.
const char kVidPathTemplate[] = "/sys/class/video4linux/%s/device/../idVendor";
const char kPidPathTemplate[] = "/sys/class/video4linux/%s/device/../idProduct";
const char kInterfacePathTemplate[] =
    "/sys/class/video4linux/%s/device/interface";

bool ReadIdFile(const std::string& path, std::string* id) {
  char id_buf[kVidPidSize];
  FILE* file = fopen(path.c_str(), "rb");
  if (!file)
    return false;
  const bool success = fread(id_buf, kVidPidSize, 1, file) == 1;
  fclose(file);
  if (!success)
    return false;
  id->append(id_buf, kVidPidSize);
  return true;
}

bool HasUsableFormats(int fd, uint32_t capabilities) {
  if (!(capabilities & V4L2_CAP_VIDEO_CAPTURE))
    return false;

  const std::list<uint32_t>& usable_fourccs =
      VideoCaptureDeviceLinux::GetListOfUsableFourCCs(false);
  v4l2_fmtdesc fmtdesc = {};
  fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  for (; HANDLE_EINTR(ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)) == 0;
       ++fmtdesc.index) {
    if (std::find(usable_fourccs.begin(), usable_fourccs.end(),
                  fmtdesc.pixelformat) != usable_fourccs.end()) {
      return true;
    }
  }

  DLOG(ERROR) << "No usable formats found";
  return false;
}

std::list<float> GetFrameRateList(int fd,
                                  uint32_t fourcc,
                                  uint32_t width,
                                  uint32_t height) {
  std::list<float> frame_rates;

  v4l2_frmivalenum frame_interval = {};
  frame_interval.pixel_format = fourcc;
  frame_interval.width = width;
  frame_interval.height = height;
  for (; HANDLE_EINTR(ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frame_interval)) ==
             0;
       ++frame_interval.index) {
    if (frame_interval.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
      if (frame_interval.discrete.numerator != 0) {
        frame_rates.push_back(
            frame_interval.discrete.denominator /
            static_cast<float>(frame_interval.discrete.numerator));
      }
    } else if (frame_interval.type == V4L2_FRMIVAL_TYPE_CONTINUOUS ||
               frame_interval.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
      // TODO(mcasas): see http://crbug.com/249953, support these devices.
      NOTIMPLEMENTED_LOG_ONCE();
      break;
    }
  }
  // Some devices, e.g. Kinect, do not enumerate any frame rates, see
  // http://crbug.com/412284. Set their frame_rate to zero.
  if (frame_rates.empty())
    frame_rates.push_back(0);
  return frame_rates;
}

void GetSupportedFormatsForV4L2BufferType(
    int fd,
    VideoCaptureFormats* supported_formats) {
  v4l2_fmtdesc v4l2_format = {};
  v4l2_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  for (; HANDLE_EINTR(ioctl(fd, VIDIOC_ENUM_FMT, &v4l2_format)) == 0;
       ++v4l2_format.index) {
    VideoCaptureFormat supported_format;
    supported_format.pixel_format =
        VideoCaptureDeviceLinux::V4l2FourCcToChromiumPixelFormat(
            v4l2_format.pixelformat);

    if (supported_format.pixel_format == PIXEL_FORMAT_UNKNOWN)
      continue;

    v4l2_frmsizeenum frame_size = {};
    frame_size.pixel_format = v4l2_format.pixelformat;
    for (; HANDLE_EINTR(ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frame_size)) == 0;
         ++frame_size.index) {
      if (frame_size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
        supported_format.frame_size.SetSize(frame_size.discrete.width,
                                            frame_size.discrete.height);
      } else if (frame_size.type == V4L2_FRMSIZE_TYPE_STEPWISE ||
                 frame_size.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
        // TODO(mcasas): see http://crbug.com/249953, support these devices.
        NOTIMPLEMENTED_LOG_ONCE();
      }

      const std::list<float> frame_rates = GetFrameRateList(
          fd, v4l2_format.pixelformat, frame_size.discrete.width,
          frame_size.discrete.height);
      for (const auto& frame_rate : frame_rates) {
        supported_format.frame_rate = frame_rate;
        supported_formats->push_back(supported_format);
        DVLOG(1) << VideoCaptureFormat::ToString(supported_format);
      }
    }
  }
}

std::string ExtractFileNameFromDeviceId(const std::string& device_id) {
  // |unique_id| is of the form "/dev/video2".  |file_name| is "video2".
  const char kDevDir[] = "/dev/";
  DCHECK(base::StartsWith(device_id, kDevDir, base::CompareCase::SENSITIVE));
  return device_id.substr(strlen(kDevDir), device_id.length());
}

std::string GetDeviceModelId(const std::string& device_id) {
  const std::string file_name = ExtractFileNameFromDeviceId(device_id);
  std::string usb_id;
  const std::string vid_path =
      base::StringPrintf(kVidPathTemplate, file_name.c_str());
  if (!ReadIdFile(vid_path, &usb_id))
    return usb_id;

  usb_id.append(":");
  const std::string pid_path =
      base::StringPrintf(kPidPathTemplate, file_name.c_str());
  if (!ReadIdFile(pid_path, &usb_id))
    usb_id.clear();

  return usb_id;
}

std::string GetDeviceDisplayName(const std::string& device_id) {
  const std::string file_name = ExtractFileNameFromDeviceId(device_id);
  const std::string interface_path =
      base::StringPrintf(kInterfacePathTemplate, file_name.c_str());
  std::string display_name;
  if (!base::ReadFileToStringWithMaxSize(base::FilePath(interface_path),
                                         &display_name,
                                         kMaxInterfaceNameSize)) {
    return std::string();
  }
  return display_name;
}

}  // namespace

VideoCaptureDeviceFactoryLinux::VideoCaptureDeviceFactoryLinux(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner)
    : ui_task_runner_(ui_task_runner) {
}

VideoCaptureDeviceFactoryLinux::~VideoCaptureDeviceFactoryLinux() = default;

std::unique_ptr<VideoCaptureDevice>
VideoCaptureDeviceFactoryLinux::CreateDevice(
    const VideoCaptureDeviceDescriptor& device_descriptor) {
  DCHECK(thread_checker_.CalledOnValidThread());
#if defined(OS_CHROMEOS)
  VideoCaptureDeviceChromeOS* self =
      new VideoCaptureDeviceChromeOS(ui_task_runner_, device_descriptor);
#else
  VideoCaptureDeviceLinux* self =
      new VideoCaptureDeviceLinux(device_descriptor);
#endif
  if (!self)
    return std::unique_ptr<VideoCaptureDevice>();
  // Test opening the device driver. This is to make sure it is available.
  // We will reopen it again in our worker thread when someone
  // allocates the camera.
  base::ScopedFD fd(
      HANDLE_EINTR(open(device_descriptor.device_id.c_str(), O_RDONLY)));
  if (!fd.is_valid()) {
    DLOG(ERROR) << "Cannot open device";
    delete self;
    return std::unique_ptr<VideoCaptureDevice>();
  }

  return std::unique_ptr<VideoCaptureDevice>(self);
}

void VideoCaptureDeviceFactoryLinux::GetDeviceDescriptors(
    VideoCaptureDeviceDescriptors* device_descriptors) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(device_descriptors->empty());
  const base::FilePath path("/dev/");
  base::FileEnumerator enumerator(path, false, base::FileEnumerator::FILES,
                                  "video*");

  while (!enumerator.Next().empty()) {
    const base::FileEnumerator::FileInfo info = enumerator.GetInfo();
    const std::string unique_id = path.value() + info.GetName().value();
    const base::ScopedFD fd(HANDLE_EINTR(open(unique_id.c_str(), O_RDONLY)));
    if (!fd.is_valid()) {
      DLOG(ERROR) << "Couldn't open " << info.GetName().value();
      continue;
    }
    // Test if this is a V4L2 capture device and if it has at least one
    // supported capture format. Devices that have capture and output
    // capabilities at the same time are memory-to-memory and are skipped, see
    // http://crbug.com/139356.
    v4l2_capability cap;
    if ((HANDLE_EINTR(ioctl(fd.get(), VIDIOC_QUERYCAP, &cap)) == 0) &&
        (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE &&
         !(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) &&
        HasUsableFormats(fd.get(), cap.capabilities)) {
      const std::string model_id = GetDeviceModelId(unique_id);
      std::string display_name = GetDeviceDisplayName(unique_id);
      if (display_name.empty())
        display_name = reinterpret_cast<char*>(cap.card);
#if defined(OS_CHROMEOS)
      static CameraConfigChromeOS* config = new CameraConfigChromeOS();
      device_descriptors->emplace_back(
          display_name, unique_id, model_id,
          VideoCaptureApi::LINUX_V4L2_SINGLE_PLANE,
          VideoCaptureTransportType::OTHER_TRANSPORT,
          config->GetCameraFacing(unique_id, model_id));
#else
      device_descriptors->emplace_back(
          display_name, unique_id, model_id,
          VideoCaptureApi::LINUX_V4L2_SINGLE_PLANE);
#endif
    }
  }
  // Since JS doesn't have API to get camera facing, we sort the list to make
  // sure apps use the front camera by default.
  // TODO(henryhsu): remove this after JS API completed (crbug.com/543997).
  std::sort(device_descriptors->begin(), device_descriptors->end());
}

void VideoCaptureDeviceFactoryLinux::GetSupportedFormats(
    const VideoCaptureDeviceDescriptor& device,
    VideoCaptureFormats* supported_formats) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (device.device_id.empty())
    return;
  base::ScopedFD fd(HANDLE_EINTR(open(device.device_id.c_str(), O_RDONLY)));
  if (!fd.is_valid())  // Failed to open this device.
    return;
  supported_formats->clear();

  DCHECK_NE(device.capture_api, VideoCaptureApi::UNKNOWN);
  GetSupportedFormatsForV4L2BufferType(fd.get(), supported_formats);
}

#if !defined(OS_CHROMEOS)
// static
VideoCaptureDeviceFactory*
VideoCaptureDeviceFactory::CreateVideoCaptureDeviceFactory(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    MojoJpegDecodeAcceleratorFactoryCB jda_factory,
    MojoJpegEncodeAcceleratorFactoryCB jea_factory) {
  return new VideoCaptureDeviceFactoryLinux(ui_task_runner);
}
#endif

}  // namespace media
