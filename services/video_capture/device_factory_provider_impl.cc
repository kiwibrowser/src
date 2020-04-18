// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/device_factory_provider_impl.h"

#include "media/capture/video/fake_video_capture_device_factory.h"
#include "media/capture/video/video_capture_buffer_pool.h"
#include "media/capture/video/video_capture_buffer_tracker.h"
#include "media/capture/video/video_capture_jpeg_decoder.h"
#include "media/capture/video/video_capture_system_impl.h"
#include "services/video_capture/device_factory_media_to_mojo_adapter.h"
#include "services/video_capture/virtual_device_enabled_device_factory.h"

namespace {

// TODO(chfremer): Replace with an actual decoder factory.
// https://crbug.com/584797
std::unique_ptr<media::VideoCaptureJpegDecoder> CreateJpegDecoder() {
  return nullptr;
}

}  // anonymous namespace

namespace video_capture {

DeviceFactoryProviderImpl::DeviceFactoryProviderImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    base::Callback<void(float)> set_shutdown_delay_cb)
    : service_ref_(std::move(service_ref)),
      set_shutdown_delay_cb_(std::move(set_shutdown_delay_cb)) {}

DeviceFactoryProviderImpl::~DeviceFactoryProviderImpl() {}

void DeviceFactoryProviderImpl::ConnectToDeviceFactory(
    mojom::DeviceFactoryRequest request) {
  LazyInitializeDeviceFactory();
  factory_bindings_.AddBinding(device_factory_.get(), std::move(request));
}

void DeviceFactoryProviderImpl::SetShutdownDelayInSeconds(float seconds) {
  set_shutdown_delay_cb_.Run(seconds);
}

void DeviceFactoryProviderImpl::LazyInitializeDeviceFactory() {
  if (device_factory_)
    return;

  // Create the platform-specific device factory.
  // The task runner passed to CreateFactory is used for things that need to
  // happen on a "UI thread equivalent", e.g. obtaining screen rotation on
  // Chrome OS.
  std::unique_ptr<media::VideoCaptureDeviceFactory> media_device_factory =
      media::VideoCaptureDeviceFactory::CreateFactory(
          base::ThreadTaskRunnerHandle::Get(),
          // TODO(jcliang): Create a GpuMemoryBufferManager from GpuService
          // here.
          nullptr,
          // TODO(mojahsu): Create a GpuJpegDecoderMojoFactoryCB here.
          base::BindRepeating(
              [](media::mojom::JpegDecodeAcceleratorRequest) {}),
          base::BindRepeating(
              [](media::mojom::JpegEncodeAcceleratorRequest) {}));
  auto video_capture_system = std::make_unique<media::VideoCaptureSystemImpl>(
      std::move(media_device_factory));

  device_factory_ = std::make_unique<VirtualDeviceEnabledDeviceFactory>(
      service_ref_->Clone(),
      std::make_unique<DeviceFactoryMediaToMojoAdapter>(
          service_ref_->Clone(), std::move(video_capture_system),
          base::Bind(CreateJpegDecoder)));
}

}  // namespace video_capture
