// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/midi/usb_midi_device_factory_android.h"

#include <stddef.h>
#include <memory>

#include "base/bind.h"
#include "base/containers/hash_tables.h"
#include "base/synchronization/lock.h"
#include "jni/UsbMidiDeviceFactoryAndroid_jni.h"
#include "media/midi/usb_midi_device_android.h"

using base::android::JavaParamRef;

namespace midi {

namespace {

using Callback = UsbMidiDevice::Factory::Callback;

}  // namespace

UsbMidiDeviceFactoryAndroid::UsbMidiDeviceFactoryAndroid() : delegate_(NULL) {}

UsbMidiDeviceFactoryAndroid::~UsbMidiDeviceFactoryAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!raw_factory_.is_null())
    Java_UsbMidiDeviceFactoryAndroid_close(env, raw_factory_);
}

void UsbMidiDeviceFactoryAndroid::EnumerateDevices(
    UsbMidiDeviceDelegate* delegate,
    Callback callback) {
  DCHECK(!delegate_);
  JNIEnv* env = base::android::AttachCurrentThread();
  uintptr_t pointer = reinterpret_cast<uintptr_t>(this);
  raw_factory_.Reset(Java_UsbMidiDeviceFactoryAndroid_create(env, pointer));

  delegate_ = delegate;
  callback_ = std::move(callback);

  if (Java_UsbMidiDeviceFactoryAndroid_enumerateDevices(env, raw_factory_)) {
    // Asynchronous operation.
    return;
  }
  // No devices are found.
  UsbMidiDevice::Devices devices;
  std::move(callback_).Run(true, &devices);
}

// Called from the Java world.
void UsbMidiDeviceFactoryAndroid::OnUsbMidiDeviceRequestDone(
    JNIEnv* env,
    const JavaParamRef<jobject>& caller,
    const JavaParamRef<jobjectArray>& devices) {
  size_t size = env->GetArrayLength(devices);
  UsbMidiDevice::Devices devices_to_pass;
  for (size_t i = 0; i < size; ++i) {
    base::android::ScopedJavaLocalRef<jobject> raw_device(
        env, env->GetObjectArrayElement(devices, i));
    devices_to_pass.push_back(
        std::make_unique<UsbMidiDeviceAndroid>(raw_device, delegate_));
  }

  std::move(callback_).Run(true, &devices_to_pass);
}

// Called from the Java world.
void UsbMidiDeviceFactoryAndroid::OnUsbMidiDeviceAttached(
    JNIEnv* env,
    const JavaParamRef<jobject>& caller,
    const JavaParamRef<jobject>& device) {
  delegate_->OnDeviceAttached(
      std::make_unique<UsbMidiDeviceAndroid>(device, delegate_));
}

// Called from the Java world.
void UsbMidiDeviceFactoryAndroid::OnUsbMidiDeviceDetached(
    JNIEnv* env,
    const JavaParamRef<jobject>& caller,
    jint index) {
  delegate_->OnDeviceDetached(index);
}

}  // namespace midi
