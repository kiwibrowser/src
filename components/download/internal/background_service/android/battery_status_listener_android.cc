// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/background_service/android/battery_status_listener_android.h"

#include "base/android/jni_android.h"
#include "jni/BatteryStatusListenerAndroid_jni.h"

namespace download {

BatteryStatusListenerAndroid::BatteryStatusListenerAndroid(
    const base::TimeDelta& battery_query_interval)
    : BatteryStatusListener(battery_query_interval) {}

BatteryStatusListenerAndroid::~BatteryStatusListenerAndroid() = default;

int BatteryStatusListenerAndroid::GetBatteryPercentageInternal() {
  return Java_BatteryStatusListenerAndroid_getBatteryPercentage(
      base::android::AttachCurrentThread());
}

}  // namespace download
