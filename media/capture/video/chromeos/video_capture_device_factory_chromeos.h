// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CHROMEOS_VIDEO_CAPTURE_DEVICE_FACTORY_CHROMEOS_H_
#define MEDIA_CAPTURE_VIDEO_CHROMEOS_VIDEO_CAPTURE_DEVICE_FACTORY_CHROMEOS_H_

#include <memory>

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "media/capture/video/chromeos/camera_hal_delegate.h"
#include "media/capture/video/video_capture_device_factory.h"

namespace media {

class CAPTURE_EXPORT VideoCaptureDeviceFactoryChromeOS final
    : public VideoCaptureDeviceFactory {
 public:
  explicit VideoCaptureDeviceFactoryChromeOS(
      scoped_refptr<base::SingleThreadTaskRunner>
          task_runner_for_screen_observer,
      gpu::GpuMemoryBufferManager* gpu_buffer_manager,
      MojoJpegDecodeAcceleratorFactoryCB jda_factory,
      MojoJpegEncodeAcceleratorFactoryCB jea_factory);

  ~VideoCaptureDeviceFactoryChromeOS() override;

  // VideoCaptureDeviceFactory interface implementations.
  std::unique_ptr<VideoCaptureDevice> CreateDevice(
      const VideoCaptureDeviceDescriptor& device_descriptor) final;
  void GetSupportedFormats(
      const VideoCaptureDeviceDescriptor& device_descriptor,
      VideoCaptureFormats* supported_formats) final;
  void GetDeviceDescriptors(
      VideoCaptureDeviceDescriptors* device_descriptors) final;

  // A run-time check for whether we should enable
  // VideoCaptureDeviceFactoryChromeOS on the device.
  static bool ShouldEnable();

  static gpu::GpuMemoryBufferManager* GetBufferManager();

  // For testing purpose only.
  static void SetBufferManagerForTesting(
      gpu::GpuMemoryBufferManager* buffer_manager);

 private:
  // Initializes the factory. The factory is functional only after this call
  // succeeds.
  bool Init(MojoJpegDecodeAcceleratorFactoryCB jda_factory,
            MojoJpegEncodeAcceleratorFactoryCB jea_factory);

  const scoped_refptr<base::SingleThreadTaskRunner>
      task_runner_for_screen_observer_;

  // The thread that all the Mojo operations of |camera_hal_delegate_| take
  // place.  Started in Init and stopped when the class instance is destroyed.
  base::Thread camera_hal_ipc_thread_;

  // Communication interface to the camera HAL.  |camera_hal_delegate_| is
  // created on the thread on which Init is called.  All the Mojo communication
  // that |camera_hal_delegate_| issues and receives must be sequenced through
  // |camera_hal_ipc_thread_|.
  scoped_refptr<CameraHalDelegate> camera_hal_delegate_;

  bool initialized_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureDeviceFactoryChromeOS);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CHROMEOS_VIDEO_CAPTURE_DEVICE_FACTORY_CHROMEOS_H_
