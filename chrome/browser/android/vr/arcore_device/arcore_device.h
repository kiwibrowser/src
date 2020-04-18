// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_DEVICE_H_
#define CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_DEVICE_H_

#include <jni.h>
#include "base/android/jni_android.h"
#include "base/macros.h"
#include "base/optional.h"
#include "device/vr/vr_device.h"
#include "device/vr/vr_device_base.h"

namespace vr {
class MailboxToSurfaceBridge;
}  // namespace vr

namespace device {

class ARCoreGlThread;

class ARCoreDevice : public VRDeviceBase {
 public:
  ARCoreDevice();
  ~ARCoreDevice() override;

  base::WeakPtr<ARCoreDevice> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  void OnMagicWindowFrameDataRequest(
      const gfx::Size& frame_size,
      display::Display::Rotation frame_rotation,
      mojom::VRMagicWindowProvider::GetFrameDataCallback callback) override;
  void OnMagicWindowFrameDataRequestComplete(
      mojom::VRMagicWindowProvider::GetFrameDataCallback callback,
      mojom::VRMagicWindowFrameDataPtr frame_data);
  void OnMailboxBridgeReady();
  void OnARCoreGlThreadInitialized(bool success);

  template <typename DataType>
  static void RunCallbackOnTaskRunner(
      scoped_refptr<base::TaskRunner> task_runner,
      base::OnceCallback<void(DataType)> callback,
      DataType data) {
    task_runner->PostTask(FROM_HERE,
                          base::BindOnce(std::move(callback), std::move(data)));
  }
  template <typename DataType>
  base::OnceCallback<void(DataType)> CreateMainThreadCallback(
      base::OnceCallback<void(DataType)> callback) {
    return base::BindOnce(&ARCoreDevice::RunCallbackOnTaskRunner<DataType>,
                          main_thread_task_runner_, std::move(callback));
  }

  void PostTaskToGlThread(base::OnceClosure task);

  bool IsOnMainThread();
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  std::unique_ptr<vr::MailboxToSurfaceBridge> mailbox_bridge_;
  std::unique_ptr<ARCoreGlThread> arcore_gl_thread_;

  bool is_arcore_gl_thread_initialized_ = false;

  // Must be last.
  base::WeakPtrFactory<ARCoreDevice> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(ARCoreDevice);
};

}  // namespace device

#endif  // CHROME_BROWSER_ANDROID_VR_ARCORE_DEVICE_ARCORE_DEVICE_H_
