// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_VR_DEVICE_H
#define DEVICE_VR_VR_DEVICE_H

#include "base/callback.h"
#include "base/macros.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "device/vr/vr_export.h"

namespace device {

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class VrViewerType {
  GVR_UNKNOWN = 0,
  GVR_CARDBOARD = 1,
  GVR_DAYDREAM = 2,
  ORIENTATION_SENSOR_DEVICE = 10,
  FAKE_DEVICE = 11,
  OPENVR_UNKNOWN = 20,
  OPENVR_VIVE = 21,
  OPENVR_RIFT_CV1 = 22,
  VIEWER_TYPE_COUNT,
};

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
enum class XrRuntimeAvailable {
  NONE = 0,
  OPENVR = 1,
  COUNT,
};

const unsigned int VR_DEVICE_LAST_ID = 0xFFFFFFFF;

class VRDeviceEventListener {
 public:
  virtual ~VRDeviceEventListener() {}

  virtual void OnChanged(mojom::VRDisplayInfoPtr vr_device_info) = 0;
  virtual void OnExitPresent() = 0;
  virtual void OnActivate(mojom::VRDisplayEventReason reason,
                          base::OnceCallback<void(bool)> on_handled) = 0;
  virtual void OnDeactivate(mojom::VRDisplayEventReason reason) = 0;
};

// Represents one of the platform's VR devices. Owned by the respective
// VRDeviceProvider.
// TODO(mthiesse, crbug.com/769373): Remove DEVICE_VR_EXPORT.
class DEVICE_VR_EXPORT VRDevice {
 public:
  virtual ~VRDevice() {}

  virtual unsigned int GetId() const = 0;
  virtual void PauseTracking() = 0;
  virtual void ResumeTracking() = 0;
  virtual mojom::VRDisplayInfoPtr GetVRDisplayInfo() = 0;
  virtual void SetMagicWindowEnabled(bool enabled) = 0;
  virtual void ExitPresent() = 0;
  virtual void RequestPresent(
      mojom::VRSubmitFrameClientPtr submit_client,
      mojom::VRPresentationProviderRequest request,
      mojom::VRRequestPresentOptionsPtr present_options,
      mojom::VRDisplayHost::RequestPresentCallback callback) = 0;
  virtual void SetListeningForActivate(bool is_listening) = 0;

  // The fallback device should only be provided in lieu of other devices.
  virtual bool IsFallbackDevice() = 0;

  // TODO(mthiesse): The browser should handle browser-side exiting of
  // presentation before device/ is even aware presentation is being exited.
  // Then the browser should call ExitPresent() on Device, which does device/
  // exiting of presentation before notifying displays. This is currently messy
  // because browser-side notions of presentation are mostly Android-specific.
  virtual void OnExitPresent() = 0;

  virtual void SetVRDeviceEventListener(VRDeviceEventListener* listener) = 0;
};

}  // namespace device

#endif  // DEVICE_VR_VR_DEVICE_H
