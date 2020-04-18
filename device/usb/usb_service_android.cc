// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/usb_service_android.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "components/device_event_log/device_event_log.h"
#include "device/usb/usb_device_android.h"
#include "jni/ChromeUsbService_jni.h"

using base::android::AttachCurrentThread;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace device {

UsbServiceAndroid::UsbServiceAndroid()
    : UsbService(nullptr), weak_factory_(this) {
  JNIEnv* env = AttachCurrentThread();
  j_object_.Reset(
      Java_ChromeUsbService_create(env, reinterpret_cast<jlong>(this)));
  ScopedJavaLocalRef<jobjectArray> devices =
      Java_ChromeUsbService_getDevices(env, j_object_);
  jsize length = env->GetArrayLength(devices.obj());
  for (jsize i = 0; i < length; ++i) {
    ScopedJavaLocalRef<jobject> usb_device(
        env, env->GetObjectArrayElement(devices.obj(), i));
    scoped_refptr<UsbDeviceAndroid> device =
        UsbDeviceAndroid::Create(env, weak_factory_.GetWeakPtr(), usb_device);
    AddDevice(device);
  }
}

UsbServiceAndroid::~UsbServiceAndroid() {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeUsbService_close(env, j_object_);
}

void UsbServiceAndroid::DeviceAttached(JNIEnv* env,
                                       const JavaRef<jobject>& caller,
                                       const JavaRef<jobject>& usb_device) {
  scoped_refptr<UsbDeviceAndroid> device =
      UsbDeviceAndroid::Create(env, weak_factory_.GetWeakPtr(), usb_device);
  AddDevice(device);
  NotifyDeviceAdded(device);
}

void UsbServiceAndroid::DeviceDetached(JNIEnv* env,
                                       const JavaRef<jobject>& caller,
                                       jint device_id) {
  auto it = devices_by_id_.find(device_id);
  if (it == devices_by_id_.end())
    return;

  scoped_refptr<UsbDeviceAndroid> device = it->second;
  devices_by_id_.erase(it);
  devices().erase(device->guid());
  device->OnDisconnect();

  USB_LOG(USER) << "USB device removed: id=" << device->device_id()
                << " guid=" << device->guid();

  NotifyDeviceRemoved(device);
}

void UsbServiceAndroid::DevicePermissionRequestComplete(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& caller,
    jint device_id,
    jboolean granted) {
  const auto it = devices_by_id_.find(device_id);
  DCHECK(it != devices_by_id_.end());
  it->second->PermissionGranted(granted);
}

ScopedJavaLocalRef<jobject> UsbServiceAndroid::OpenDevice(
    JNIEnv* env,
    const JavaRef<jobject>& wrapper) {
  return Java_ChromeUsbService_openDevice(env, j_object_, wrapper);
}

void UsbServiceAndroid::RequestDevicePermission(const JavaRef<jobject>& wrapper,
                                                jint device_id) {
  Java_ChromeUsbService_requestDevicePermission(AttachCurrentThread(),
                                                j_object_, wrapper, device_id);
}

void UsbServiceAndroid::AddDevice(scoped_refptr<UsbDeviceAndroid> device) {
  DCHECK(!ContainsKey(devices_by_id_, device->device_id()));
  DCHECK(!ContainsKey(devices(), device->guid()));
  devices_by_id_[device->device_id()] = device;
  devices()[device->guid()] = device;

  USB_LOG(USER) << "USB device added: id=" << device->device_id()
                << " vendor=" << device->vendor_id() << " \""
                << device->manufacturer_string()
                << "\", product=" << device->product_id() << " \""
                << device->product_string() << "\", serial=\""
                << device->serial_number() << "\", guid=" << device->guid();
}

}  // namespace device
