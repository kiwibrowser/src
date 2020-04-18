// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/usb_device_android.h"

#include "base/android/build_info.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "device/usb/usb_configuration_android.h"
#include "device/usb/usb_descriptors.h"
#include "device/usb/usb_device_handle_android.h"
#include "device/usb/usb_interface_android.h"
#include "device/usb/usb_service_android.h"
#include "device/usb/webusb_descriptors.h"
#include "jni/ChromeUsbDevice_jni.h"

using base::android::ConvertJavaStringToUTF16;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace device {

// static
scoped_refptr<UsbDeviceAndroid> UsbDeviceAndroid::Create(
    JNIEnv* env,
    base::WeakPtr<UsbServiceAndroid> service,
    const JavaRef<jobject>& usb_device) {
  ScopedJavaLocalRef<jobject> wrapper =
      Java_ChromeUsbDevice_create(env, usb_device);
  uint16_t device_version = 0;
  if (base::android::BuildInfo::GetInstance()->sdk_int() >=
      base::android::SDK_VERSION_MARSHMALLOW)
    device_version = Java_ChromeUsbDevice_getDeviceVersion(env, wrapper);
  base::string16 manufacturer_string, product_string, serial_number;
  if (base::android::BuildInfo::GetInstance()->sdk_int() >=
      base::android::SDK_VERSION_LOLLIPOP) {
    ScopedJavaLocalRef<jstring> manufacturer_jstring =
        Java_ChromeUsbDevice_getManufacturerName(env, wrapper);
    if (!manufacturer_jstring.is_null())
      manufacturer_string = ConvertJavaStringToUTF16(env, manufacturer_jstring);
    ScopedJavaLocalRef<jstring> product_jstring =
        Java_ChromeUsbDevice_getProductName(env, wrapper);
    if (!product_jstring.is_null())
      product_string = ConvertJavaStringToUTF16(env, product_jstring);
    ScopedJavaLocalRef<jstring> serial_jstring =
        Java_ChromeUsbDevice_getSerialNumber(env, wrapper);
    if (!serial_jstring.is_null())
      serial_number = ConvertJavaStringToUTF16(env, serial_jstring);
  }
  return base::WrapRefCounted(new UsbDeviceAndroid(
      env, service,
      0x0200,  // USB protocol version, not provided by the Android API.
      Java_ChromeUsbDevice_getDeviceClass(env, wrapper),
      Java_ChromeUsbDevice_getDeviceSubclass(env, wrapper),
      Java_ChromeUsbDevice_getDeviceProtocol(env, wrapper),
      Java_ChromeUsbDevice_getVendorId(env, wrapper),
      Java_ChromeUsbDevice_getProductId(env, wrapper), device_version,
      manufacturer_string, product_string, serial_number, wrapper));
}

void UsbDeviceAndroid::RequestPermission(ResultCallback callback) {
  if (!permission_granted_ && service_) {
    request_permission_callbacks_.push_back(std::move(callback));
    service_->RequestDevicePermission(j_object_, device_id_);
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), permission_granted_));
  }
}

void UsbDeviceAndroid::Open(OpenCallback callback) {
  scoped_refptr<UsbDeviceHandle> device_handle;
  if (service_) {
    JNIEnv* env = base::android::AttachCurrentThread();
    ScopedJavaLocalRef<jobject> connection =
        service_->OpenDevice(env, j_object_);
    if (!connection.is_null()) {
      device_handle = UsbDeviceHandleAndroid::Create(env, this, connection);
      handles().push_back(device_handle.get());
    }
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), device_handle));
}

bool UsbDeviceAndroid::permission_granted() const {
  return permission_granted_;
}

UsbDeviceAndroid::UsbDeviceAndroid(
    JNIEnv* env,
    base::WeakPtr<UsbServiceAndroid> service,
    uint16_t usb_version,
    uint8_t device_class,
    uint8_t device_subclass,
    uint8_t device_protocol,
    uint16_t vendor_id,
    uint16_t product_id,
    uint16_t device_version,
    const base::string16& manufacturer_string,
    const base::string16& product_string,
    const base::string16& serial_number,
    const JavaRef<jobject>& wrapper)
    : UsbDevice(usb_version,
                device_class,
                device_subclass,
                device_protocol,
                vendor_id,
                product_id,
                device_version,
                manufacturer_string,
                product_string,
                serial_number),
      device_id_(Java_ChromeUsbDevice_getDeviceId(env, wrapper)),
      service_(service),
      j_object_(wrapper) {
  if (base::android::BuildInfo::GetInstance()->sdk_int() >=
      base::android::SDK_VERSION_LOLLIPOP) {
    ScopedJavaLocalRef<jobjectArray> configurations =
        Java_ChromeUsbDevice_getConfigurations(env, j_object_);
    jsize count = env->GetArrayLength(configurations.obj());
    descriptor_.configurations.reserve(count);
    for (jsize i = 0; i < count; ++i) {
      ScopedJavaLocalRef<jobject> config(
          env, env->GetObjectArrayElement(configurations.obj(), i));
      descriptor_.configurations.push_back(
          UsbConfigurationAndroid::Convert(env, config));
    }
  } else {
    // Pre-lollipop only the first configuration was supported. Build a basic
    // configuration out of the available interfaces.
    UsbConfigDescriptor config(1,      // Configuration value, reasonable guess.
                               false,  // Self powered, arbitrary default.
                               false,  // Remote wakeup, rbitrary default.
                               0);     // Maximum power, aitrary default.

    ScopedJavaLocalRef<jobjectArray> interfaces =
        Java_ChromeUsbDevice_getInterfaces(env, wrapper);
    jsize count = env->GetArrayLength(interfaces.obj());
    config.interfaces.reserve(count);
    for (jsize i = 0; i < count; ++i) {
      ScopedJavaLocalRef<jobject> interface(
          env, env->GetObjectArrayElement(interfaces.obj(), i));
      config.interfaces.push_back(UsbInterfaceAndroid::Convert(env, interface));
    }
    descriptor_.configurations.push_back(config);
  }

  if (configurations().size() > 0)
    ActiveConfigurationChanged(configurations()[0].configuration_value);
}

UsbDeviceAndroid::~UsbDeviceAndroid() {}

void UsbDeviceAndroid::PermissionGranted(bool granted) {
  if (granted)
    Open(base::Bind(&UsbDeviceAndroid::OnDeviceOpenedToReadDescriptors, this));
  else
    CallRequestPermissionCallbacks(granted);
}

void UsbDeviceAndroid::CallRequestPermissionCallbacks(bool granted) {
  permission_granted_ = granted;
  std::list<ResultCallback> callbacks;
  callbacks.swap(request_permission_callbacks_);
  for (auto& callback : callbacks)
    std::move(callback).Run(granted);
}

void UsbDeviceAndroid::OnDeviceOpenedToReadDescriptors(
    scoped_refptr<UsbDeviceHandle> device_handle) {
  if (device_handle) {
    ReadUsbDescriptors(
        device_handle,
        base::Bind(&UsbDeviceAndroid::OnReadDescriptors, this, device_handle));
  } else {
    CallRequestPermissionCallbacks(false);
  }
}

void UsbDeviceAndroid::OnReadDescriptors(
    scoped_refptr<UsbDeviceHandle> device_handle,
    std::unique_ptr<UsbDeviceDescriptor> descriptor) {
  if (!descriptor) {
    device_handle->Close();
    CallRequestPermissionCallbacks(false);
    return;
  }

  descriptor_ = *descriptor;

  if (usb_version() >= 0x0210) {
    ReadWebUsbDescriptors(device_handle,
                          base::Bind(&UsbDeviceAndroid::OnReadWebUsbDescriptors,
                                     this, device_handle));
  } else {
    device_handle->Close();
    CallRequestPermissionCallbacks(true);
  }
}

void UsbDeviceAndroid::OnReadWebUsbDescriptors(
    scoped_refptr<UsbDeviceHandle> device_handle,
    const GURL& landing_page) {
  if (landing_page.is_valid())
    webusb_landing_page_ = landing_page;

  device_handle->Close();
  CallRequestPermissionCallbacks(true);
}

}  // namespace device
