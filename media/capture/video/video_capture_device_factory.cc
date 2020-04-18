// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/video_capture_device_factory.h"

#include <utility>

#include "base/command_line.h"
#include "build/build_config.h"
#include "media/base/media_switches.h"
#include "media/capture/video/fake_video_capture_device_factory.h"
#include "media/capture/video/file_video_capture_device_factory.h"

namespace media {

// static
std::unique_ptr<VideoCaptureDeviceFactory>
VideoCaptureDeviceFactory::CreateFactory(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    gpu::GpuMemoryBufferManager* gpu_buffer_manager,
    MojoJpegDecodeAcceleratorFactoryCB jda_factory,
    MojoJpegEncodeAcceleratorFactoryCB jea_factory) {
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  // Use a Fake or File Video Device Factory if the command line flags are
  // present, otherwise use the normal, platform-dependent, device factory.
  if (command_line->HasSwitch(switches::kUseFakeDeviceForMediaStream)) {
    if (command_line->HasSwitch(switches::kUseFileForFakeVideoCapture)) {
      return std::unique_ptr<VideoCaptureDeviceFactory>(
          new FileVideoCaptureDeviceFactory());
    } else {
      std::vector<FakeVideoCaptureDeviceSettings> config;
      FakeVideoCaptureDeviceFactory::ParseFakeDevicesConfigFromOptionsString(
          command_line->GetSwitchValueASCII(
              switches::kUseFakeDeviceForMediaStream),
          &config);
      auto result = std::make_unique<FakeVideoCaptureDeviceFactory>();
      result->SetToCustomDevicesConfig(config);
      return std::move(result);
    }
  } else {
    // |ui_task_runner| is needed for the Linux ChromeOS factory to retrieve
    // screen rotations.
    return std::unique_ptr<VideoCaptureDeviceFactory>(
        CreateVideoCaptureDeviceFactory(ui_task_runner, gpu_buffer_manager,
                                        std::move(jda_factory),
                                        std::move(jea_factory)));
  }
}

VideoCaptureDeviceFactory::VideoCaptureDeviceFactory() {
  thread_checker_.DetachFromThread();
}

VideoCaptureDeviceFactory::~VideoCaptureDeviceFactory() = default;

#if !defined(OS_MACOSX) && !defined(OS_LINUX) && !defined(OS_ANDROID) && \
    !defined(OS_WIN)
// static
VideoCaptureDeviceFactory*
VideoCaptureDeviceFactory::CreateVideoCaptureDeviceFactory(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    gpu::GpuMemoryBufferManager* gpu_buffer_manager,
    MojoJpegDecodeAcceleratorFactoryCB jda_factory,
    MojoJpegEncodeAcceleratorFactoryCB jea_factory) {
  NOTIMPLEMENTED();
  return NULL;
}
#endif

void VideoCaptureDeviceFactory::GetCameraLocationsAsync(
    std::unique_ptr<VideoCaptureDeviceDescriptors> device_descriptors,
    DeviceDescriptorsCallback result_callback) {
  NOTIMPLEMENTED();
}

}  // namespace media
