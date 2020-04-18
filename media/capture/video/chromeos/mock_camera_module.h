// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CHROMEOS_MOCK_CAMERA_MODULE_H_
#define MEDIA_CAPTURE_VIDEO_CHROMEOS_MOCK_CAMERA_MODULE_H_

#include <stddef.h>
#include <stdint.h>

#include "base/threading/thread.h"
#include "media/capture/video/chromeos/mojo/camera3.mojom.h"
#include "media/capture/video/chromeos/mojo/camera_common.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {
namespace unittest_internal {

class MockCameraModule : public cros::mojom::CameraModule {
 public:
  MockCameraModule();

  ~MockCameraModule();

  void OpenDevice(int32_t camera_id,
                  cros::mojom::Camera3DeviceOpsRequest device_ops_request,
                  OpenDeviceCallback callback) override;
  MOCK_METHOD3(DoOpenDevice,
               void(int32_t camera_id,
                    cros::mojom::Camera3DeviceOpsRequest& device_ops_request,
                    OpenDeviceCallback& callback));

  void GetNumberOfCameras(GetNumberOfCamerasCallback callback) override;
  MOCK_METHOD1(DoGetNumberOfCameras,
               void(GetNumberOfCamerasCallback& callback));

  void GetCameraInfo(int32_t camera_id,
                     GetCameraInfoCallback callback) override;
  MOCK_METHOD2(DoGetCameraInfo,
               void(int32_t camera_id, GetCameraInfoCallback& callback));

  void SetCallbacks(cros::mojom::CameraModuleCallbacksPtr callbacks,
                    SetCallbacksCallback callback) override;
  MOCK_METHOD2(DoSetCallbacks,
               void(cros::mojom::CameraModuleCallbacksPtr& callbacks,
                    SetCallbacksCallback& callback));

  void Init(InitCallback callback) override;
  MOCK_METHOD1(DoInit, void(InitCallback& callback));

  void SetTorchMode(int32_t camera_id,
                    bool enabled,
                    SetTorchModeCallback callback) override;
  MOCK_METHOD3(DoSetTorchMode,
               void(int32_t camera_id,
                    bool enabled,
                    SetTorchModeCallback& callback));

  cros::mojom::CameraModulePtrInfo GetInterfacePtrInfo();

 private:
  void CloseBindingOnThread();

  void BindOnThread(base::WaitableEvent* done,
                    cros::mojom::CameraModulePtrInfo* ptr_info);

  base::Thread mock_module_thread_;
  mojo::Binding<cros::mojom::CameraModule> binding_;
  cros::mojom::CameraModuleCallbacksPtr callbacks_;

  DISALLOW_COPY_AND_ASSIGN(MockCameraModule);
};

}  // namespace unittest_internal
}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CHROMEOS_MOCK_CAMERA_MODULE_H_
