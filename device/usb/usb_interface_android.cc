// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/usb_interface_android.h"

#include "base/android/build_info.h"
#include "device/usb/usb_endpoint_android.h"
#include "jni/ChromeUsbInterface_jni.h"

using base::android::ScopedJavaLocalRef;

namespace device {

// static
UsbInterfaceDescriptor UsbInterfaceAndroid::Convert(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& usb_interface) {
  ScopedJavaLocalRef<jobject> wrapper =
      Java_ChromeUsbInterface_create(env, usb_interface);

  uint8_t alternate_setting = 0;
  if (base::android::BuildInfo::GetInstance()->sdk_int() >=
      base::android::SDK_VERSION_LOLLIPOP) {
    alternate_setting =
        Java_ChromeUsbInterface_getAlternateSetting(env, wrapper);
  }

  UsbInterfaceDescriptor interface(
      Java_ChromeUsbInterface_getInterfaceNumber(env, wrapper),
      alternate_setting,
      Java_ChromeUsbInterface_getInterfaceClass(env, wrapper),
      Java_ChromeUsbInterface_getInterfaceSubclass(env, wrapper),
      Java_ChromeUsbInterface_getInterfaceProtocol(env, wrapper));

  ScopedJavaLocalRef<jobjectArray> endpoints =
      Java_ChromeUsbInterface_getEndpoints(env, wrapper);
  jsize count = env->GetArrayLength(endpoints.obj());
  interface.endpoints.reserve(count);
  for (jsize i = 0; i < count; ++i) {
    ScopedJavaLocalRef<jobject> endpoint(
        env, env->GetObjectArrayElement(endpoints.obj(), i));
    interface.endpoints.push_back(UsbEndpointAndroid::Convert(env, endpoint));
  }

  return interface;
}

}  // namespace device
