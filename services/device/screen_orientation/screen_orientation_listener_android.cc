// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/screen_orientation/screen_orientation_listener_android.h"

#include "base/android/jni_android.h"
#include "base/message_loop/message_loop.h"
#include "jni/ScreenOrientationListener_jni.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

// static
void ScreenOrientationListenerAndroid::Create(
    mojom::ScreenOrientationListenerRequest request) {
  mojo::MakeStrongBinding(
      base::WrapUnique(new ScreenOrientationListenerAndroid()),
      std::move(request));
}

ScreenOrientationListenerAndroid::ScreenOrientationListenerAndroid()
    : listeners_count_(0) {}

ScreenOrientationListenerAndroid::~ScreenOrientationListenerAndroid() {
  DCHECK(base::MessageLoopForIO::IsCurrent());
  if (listeners_count_ > 0) {
    Java_ScreenOrientationListener_startAccurateListening(
        base::android::AttachCurrentThread());
  }
}

void ScreenOrientationListenerAndroid::Start() {
  DCHECK(base::MessageLoopForIO::IsCurrent());
  ++listeners_count_;
  if (listeners_count_ == 1) {
    // Ask the ScreenOrientationListener (Java) to start accurately listening to
    // the screen orientation. It keep track of the number of start request if
    // it is already running an accurate listening.
    Java_ScreenOrientationListener_startAccurateListening(
        base::android::AttachCurrentThread());
  }
}

void ScreenOrientationListenerAndroid::Stop() {
  DCHECK(base::MessageLoopForIO::IsCurrent());
  DCHECK(listeners_count_ > 0);
  --listeners_count_;
  if (listeners_count_ == 0) {
    // Ask the ScreenOrientationListener (Java) to stop accurately listening to
    // the screen orientation. It will actually stop only if the number of stop
    // requests matches the number of start requests.
    Java_ScreenOrientationListener_stopAccurateListening(
        base::android::AttachCurrentThread());
  }
}

void ScreenOrientationListenerAndroid::IsAutoRotateEnabledByUser(
    IsAutoRotateEnabledByUserCallback callback) {
  std::move(callback).Run(
      Java_ScreenOrientationListener_isAutoRotateEnabledByUser(
          base::android::AttachCurrentThread()));
}

}  // namespace device
