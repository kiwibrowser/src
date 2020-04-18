/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "sdk/android/native_api/base/networkmonitor.h"

#include <memory>

#include "rtc_base/ptr_util.h"
#include "sdk/android/src/jni/androidnetworkmonitor.h"

namespace webrtc {

std::unique_ptr<rtc::NetworkMonitorFactory> CreateAndroidNetworkMonitorFactory(
    JNIEnv* env,
    jobject application_context) {
  return rtc::MakeUnique<jni::AndroidNetworkMonitorFactory>(
      env, JavaParamRef<jobject>(application_context));
}

std::unique_ptr<rtc::NetworkMonitorFactory>
CreateAndroidNetworkMonitorFactory() {
  return rtc::MakeUnique<jni::AndroidNetworkMonitorFactory>();
}

}  // namespace webrtc
