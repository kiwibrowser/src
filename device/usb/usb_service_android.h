// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_USB_SERVICE_ANDROID_H_
#define DEVICE_USB_USB_SERVICE_ANDROID_H_

#include <jni.h>

#include <unordered_map>

#include "base/android/scoped_java_ref.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "device/usb/usb_service.h"

namespace device {

class UsbDeviceAndroid;

// USB service implementation for Android. This is a stub implementation that
// does not return any devices.
class UsbServiceAndroid : public UsbService {
 public:
  UsbServiceAndroid();
  ~UsbServiceAndroid() override;

  // Methods called by Java.
  void DeviceAttached(JNIEnv* env,
                      const base::android::JavaRef<jobject>& caller,
                      const base::android::JavaRef<jobject>& usb_device);
  void DeviceDetached(JNIEnv* env,
                      const base::android::JavaRef<jobject>& caller,
                      jint device_id);
  void DevicePermissionRequestComplete(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& caller,
      jint device_id,
      jboolean granted);

  // Called by UsbDeviceAndroid.
  base::android::ScopedJavaLocalRef<jobject> OpenDevice(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& wrapper);
  void RequestDevicePermission(const base::android::JavaRef<jobject>& wrapper,
                               jint device_id);

 private:
  void AddDevice(scoped_refptr<UsbDeviceAndroid> device);

  std::unordered_map<jint, scoped_refptr<UsbDeviceAndroid>> devices_by_id_;

  // Java object org.chromium.device.usb.ChromeUsbService.
  base::android::ScopedJavaGlobalRef<jobject> j_object_;

  base::WeakPtrFactory<UsbServiceAndroid> weak_factory_;
};

}  // namespace device

#endif  // DEVICE_USB_USB_SERVICE_ANDROID_H_
