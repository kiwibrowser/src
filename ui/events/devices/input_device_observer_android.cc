// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/devices/input_device_observer_android.h"

#include "base/memory/singleton.h"
#include "jni/InputDeviceObserver_jni.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;

// This macro provides the implementation for the observer notification methods.
#define NOTIFY_ANDROID_OBSERVERS(method_decl, observer_call)   \
  void InputDeviceObserverAndroid::method_decl {               \
    for (ui::InputDeviceEventObserver & observer : observers_) \
      observer.observer_call;                                  \
  }

namespace ui {

InputDeviceObserverAndroid::InputDeviceObserverAndroid() {}

InputDeviceObserverAndroid::~InputDeviceObserverAndroid() {}

InputDeviceObserverAndroid* InputDeviceObserverAndroid::GetInstance() {
  return base::Singleton<
      InputDeviceObserverAndroid,
      base::LeakySingletonTraits<InputDeviceObserverAndroid>>::get();
}

void InputDeviceObserverAndroid::AddObserver(
    ui::InputDeviceEventObserver* observer) {
  observers_.AddObserver(observer);
  JNIEnv* env = AttachCurrentThread();
  Java_InputDeviceObserver_addObserver(env);
}

void InputDeviceObserverAndroid::RemoveObserver(
    ui::InputDeviceEventObserver* observer) {
  observers_.RemoveObserver(observer);
  JNIEnv* env = AttachCurrentThread();
  Java_InputDeviceObserver_removeObserver(env);
}

static void JNI_InputDeviceObserver_InputConfigurationChanged(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  InputDeviceObserverAndroid::GetInstance()
      ->NotifyObserversTouchpadDeviceConfigurationChanged();
  InputDeviceObserverAndroid::GetInstance()
      ->NotifyObserversKeyboardDeviceConfigurationChanged();
  InputDeviceObserverAndroid::GetInstance()
      ->NotifyObserversMouseDeviceConfigurationChanged();
}

NOTIFY_ANDROID_OBSERVERS(NotifyObserversMouseDeviceConfigurationChanged(),
                         OnMouseDeviceConfigurationChanged());
NOTIFY_ANDROID_OBSERVERS(NotifyObserversTouchpadDeviceConfigurationChanged(),
                         OnTouchpadDeviceConfigurationChanged());
NOTIFY_ANDROID_OBSERVERS(NotifyObserversKeyboardDeviceConfigurationChanged(),
                         OnKeyboardDeviceConfigurationChanged());

}  // namespace ui
