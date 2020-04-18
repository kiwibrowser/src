// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/chromeos/video_capture_device_factory_chromeos.h"

#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "media/capture/video/chromeos/camera_hal_dispatcher_impl.h"
#include "media/capture/video/linux/video_capture_device_factory_linux.h"

namespace media {

namespace {

gpu::GpuMemoryBufferManager* g_gpu_buffer_manager = nullptr;

}  // namespace

VideoCaptureDeviceFactoryChromeOS::VideoCaptureDeviceFactoryChromeOS(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner_for_screen_observer,
    gpu::GpuMemoryBufferManager* gpu_buffer_manager,
    MojoJpegDecodeAcceleratorFactoryCB jda_factory,
    MojoJpegEncodeAcceleratorFactoryCB jea_factory)
    : task_runner_for_screen_observer_(task_runner_for_screen_observer),
      camera_hal_ipc_thread_("CameraHalIpcThread"),
      initialized_(Init(std::move(jda_factory), std::move(jea_factory))) {
  g_gpu_buffer_manager = gpu_buffer_manager;
}

VideoCaptureDeviceFactoryChromeOS::~VideoCaptureDeviceFactoryChromeOS() {
  camera_hal_delegate_->Reset();
  camera_hal_ipc_thread_.Stop();
  g_gpu_buffer_manager = nullptr;
}

std::unique_ptr<VideoCaptureDevice>
VideoCaptureDeviceFactoryChromeOS::CreateDevice(
    const VideoCaptureDeviceDescriptor& device_descriptor) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!initialized_) {
    return std::unique_ptr<VideoCaptureDevice>();
  }
  return camera_hal_delegate_->CreateDevice(task_runner_for_screen_observer_,
                                            device_descriptor);
}

void VideoCaptureDeviceFactoryChromeOS::GetSupportedFormats(
    const VideoCaptureDeviceDescriptor& device_descriptor,
    VideoCaptureFormats* supported_formats) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!initialized_) {
    return;
  }
  camera_hal_delegate_->GetSupportedFormats(device_descriptor,
                                            supported_formats);
}

void VideoCaptureDeviceFactoryChromeOS::GetDeviceDescriptors(
    VideoCaptureDeviceDescriptors* device_descriptors) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!initialized_) {
    return;
  }
  camera_hal_delegate_->GetDeviceDescriptors(device_descriptors);
}

// static
bool VideoCaptureDeviceFactoryChromeOS::ShouldEnable() {
  // Checks whether the Chrome OS binary which provides the HAL v3 camera
  // service is installed on the device.  If the binary exists we assume the
  // device is using the new camera HAL v3 stack.
  const base::FilePath kCrosCameraService("/usr/bin/cros_camera_service");
  return base::PathExists(kCrosCameraService);
}

// static
gpu::GpuMemoryBufferManager*
VideoCaptureDeviceFactoryChromeOS::GetBufferManager() {
  return g_gpu_buffer_manager;
}

// static
void VideoCaptureDeviceFactoryChromeOS::SetBufferManagerForTesting(
    gpu::GpuMemoryBufferManager* buffer_manager) {
  g_gpu_buffer_manager = buffer_manager;
}

bool VideoCaptureDeviceFactoryChromeOS::Init(
    MojoJpegDecodeAcceleratorFactoryCB jda_factory,
    MojoJpegEncodeAcceleratorFactoryCB jea_factory) {
  if (!camera_hal_ipc_thread_.Start()) {
    LOG(ERROR) << "Module thread failed to start";
    return false;
  }

  if (!CameraHalDispatcherImpl::GetInstance()->IsStarted() &&
      !CameraHalDispatcherImpl::GetInstance()->Start(std::move(jda_factory),
                                                     std::move(jea_factory))) {
    LOG(ERROR) << "Failed to start CameraHalDispatcherImpl";
    return false;
  }

  camera_hal_delegate_ =
      new CameraHalDelegate(camera_hal_ipc_thread_.task_runner());
  camera_hal_delegate_->RegisterCameraClient();
  return true;
}

#if defined(OS_CHROMEOS)
// static
VideoCaptureDeviceFactory*
VideoCaptureDeviceFactory::CreateVideoCaptureDeviceFactory(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner_for_screen_observer,
    gpu::GpuMemoryBufferManager* gpu_buffer_manager,
    MojoJpegDecodeAcceleratorFactoryCB jda_factory,
    MojoJpegEncodeAcceleratorFactoryCB jea_factory) {
  // On Chrome OS we have to support two use cases:
  //
  // 1. For devices that have the camera HAL v3 service running on Chrome OS,
  //    we use the HAL v3 capture device which VideoCaptureDeviceFactoryChromeOS
  //    provides.
  // 2. Existing devices that use UVC cameras need to use the V4L2 capture
  //    device which VideoCaptureDeviceFacotoryLinux provides; there are also
  //    some special devices that may never be able to implement a camera HAL
  //    v3.
  if (VideoCaptureDeviceFactoryChromeOS::ShouldEnable()) {
    return new VideoCaptureDeviceFactoryChromeOS(
        task_runner_for_screen_observer, gpu_buffer_manager,
        std::move(jda_factory), std::move(jea_factory));
  } else {
    return new VideoCaptureDeviceFactoryLinux(task_runner_for_screen_observer);
  }
}
#endif

}  // namespace media
