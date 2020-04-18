// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_SERVICE_VIDEO_CAPTURE_PROVIDER_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_SERVICE_VIDEO_CAPTURE_PROVIDER_H_

#include "base/threading/thread_checker.h"
#include "content/browser/renderer_host/media/video_capture_provider.h"
#include "services/video_capture/public/mojom/device_factory.mojom.h"
#include "services/video_capture/public/mojom/device_factory_provider.mojom.h"

namespace content {

class VideoCaptureFactoryDelegate;

// Implementation of VideoCaptureProvider that uses the "video_capture" service.
// Connects to the service lazily on demand and disconnects from the service as
// soon as all previously handed out VideoCaptureDeviceLauncher instances have
// been released and no more answers to GetDeviceInfosAsync() calls are pending.
class CONTENT_EXPORT ServiceVideoCaptureProvider : public VideoCaptureProvider {
 public:
  class ServiceConnector {
   public:
    virtual ~ServiceConnector() {}
    virtual void BindFactoryProvider(
        video_capture::mojom::DeviceFactoryProviderPtr* provider) = 0;
  };

  // The parameterless constructor creates a default ServiceConnector which
  // uses the ServiceManager associated with the current process to connect
  // to the video capture service.
  explicit ServiceVideoCaptureProvider(
      base::RepeatingCallback<void(const std::string&)> emit_log_message_cb);
  // Lets clients provide a custom ServiceConnector.
  ServiceVideoCaptureProvider(
      std::unique_ptr<ServiceConnector> service_connector,
      base::RepeatingCallback<void(const std::string&)> emit_log_message_cb);
  ~ServiceVideoCaptureProvider() override;

  // VideoCaptureProvider implementation.
  void GetDeviceInfosAsync(GetDeviceInfosCallback result_callback) override;
  std::unique_ptr<VideoCaptureDeviceLauncher> CreateDeviceLauncher() override;

 private:
  enum class ReasonForUninitialize { kShutdown, kUnused, kConnectionLost };

  void ConnectToDeviceFactory(
      std::unique_ptr<VideoCaptureFactoryDelegate>* out_factory);
  void LazyConnectToService();
  void OnDeviceInfosReceived(GetDeviceInfosCallback result_callback,
                             const std::vector<media::VideoCaptureDeviceInfo>&);
  void OnLostConnectionToDeviceFactory();
  void IncreaseUsageCount();
  void DecreaseUsageCount();
  void UninitializeInternal(ReasonForUninitialize reason);

  std::unique_ptr<ServiceConnector> service_connector_;
  base::RepeatingCallback<void(const std::string&)> emit_log_message_cb_;
  // We must hold on to |device_factory_provider_| because it holds the
  // service-side binding for |device_factory_|.
  video_capture::mojom::DeviceFactoryProviderPtr device_factory_provider_;
  video_capture::mojom::DeviceFactoryPtr device_factory_;
  // Used for automatically uninitializing when no longer in use.
  int usage_count_;
  SEQUENCE_CHECKER(sequence_checker_);

  bool launcher_has_connected_to_device_factory_;
  base::TimeTicks time_of_last_connect_;
  base::TimeTicks time_of_last_uninitialize_;

  base::WeakPtrFactory<ServiceVideoCaptureProvider> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_SERVICE_VIDEO_CAPTURE_PROVIDER_H_
