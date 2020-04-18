// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_ANDROID_BATTERY_STATUS_LISTENER_ANDROID_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_ANDROID_BATTERY_STATUS_LISTENER_ANDROID_H_

#include "components/download/internal/background_service/scheduler/device_status_listener.h"

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"

namespace download {

// Backed by a Java class that holds helper functions to query battery status.
class BatteryStatusListenerAndroid : public BatteryStatusListener {
 public:
  BatteryStatusListenerAndroid(const base::TimeDelta& battery_query_interval);
  ~BatteryStatusListenerAndroid() override;

  // BatteryStatusListener implementation.
  int GetBatteryPercentageInternal() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BatteryStatusListenerAndroid);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_BACKGROUND_SERVICE_ANDROID_BATTERY_STATUS_LISTENER_ANDROID_H_
