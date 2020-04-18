// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/metrics/uma_utils.h"

#include <stdint.h>

#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_services_manager_client.h"
#include "components/metrics/metrics_reporting_default_state.h"
#include "jni/UmaUtils_jni.h"

using base::android::JavaParamRef;

class PrefService;

namespace chrome {
namespace android {

base::Time GetMainEntryPointTimeWallClock() {
  JNIEnv* env = base::android::AttachCurrentThread();
  int64_t startTimeUnixMs = Java_UmaUtils_getMainEntryPointWallTime(env);
  return base::Time::UnixEpoch() +
         base::TimeDelta::FromMilliseconds(startTimeUnixMs);
}

base::TimeTicks GetMainEntryPointTimeTicks() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return base::TimeTicks::FromUptimeMillis(
      Java_UmaUtils_getMainEntryPointTicks(env));
}

static jboolean JNI_UmaUtils_IsClientInMetricsReportingSample(
    JNIEnv* env,
    const JavaParamRef<jclass>& obj) {
  return ChromeMetricsServicesManagerClient::IsClientInSample();
}

static void JNI_UmaUtils_RecordMetricsReportingDefaultOptIn(
    JNIEnv* env,
    const JavaParamRef<jclass>& obj,
    jboolean opt_in) {
  DCHECK(g_browser_process);
  PrefService* local_state = g_browser_process->local_state();
  metrics::RecordMetricsReportingDefaultState(
      local_state, opt_in ? metrics::EnableMetricsDefault::OPT_IN
                          : metrics::EnableMetricsDefault::OPT_OUT);
}

}  // namespace android
}  // namespace chrome
