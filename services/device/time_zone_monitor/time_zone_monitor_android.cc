// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/time_zone_monitor/time_zone_monitor_android.h"

#include "base/android/jni_android.h"
#include "base/sequenced_task_runner.h"
#include "jni/TimeZoneMonitor_jni.h"

using base::android::JavaParamRef;

namespace device {

TimeZoneMonitorAndroid::TimeZoneMonitorAndroid() : TimeZoneMonitor() {
  impl_.Reset(
      Java_TimeZoneMonitor_getInstance(base::android::AttachCurrentThread(),
                                       reinterpret_cast<intptr_t>(this)));
}

TimeZoneMonitorAndroid::~TimeZoneMonitorAndroid() {
  Java_TimeZoneMonitor_stop(base::android::AttachCurrentThread(), impl_);
}

void TimeZoneMonitorAndroid::TimeZoneChangedFromJava(
    JNIEnv* env,
    const JavaParamRef<jobject>& caller) {
  NotifyClients();
}

// static
std::unique_ptr<TimeZoneMonitor> TimeZoneMonitor::Create(
    scoped_refptr<base::SequencedTaskRunner> file_task_runner) {
  return std::unique_ptr<TimeZoneMonitor>(new TimeZoneMonitorAndroid());
}

}  // namespace device
