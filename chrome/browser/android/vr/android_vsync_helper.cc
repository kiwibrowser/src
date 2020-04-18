// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/android_vsync_helper.h"

#include "base/android/jni_android.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "jni/AndroidVSyncHelper_jni.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;

namespace vr {

AndroidVSyncHelper::AndroidVSyncHelper() {
  JNIEnv* env = AttachCurrentThread();
  j_object_.Reset(
      Java_AndroidVSyncHelper_create(env, reinterpret_cast<jlong>(this)));
  float refresh_rate = Java_AndroidVSyncHelper_getRefreshRate(env, j_object_);
  display_vsync_interval_ = base::TimeDelta::FromSecondsD(1.0 / refresh_rate);
  DVLOG(1) << "display_vsync_interval_=" << display_vsync_interval_;
}

AndroidVSyncHelper::~AndroidVSyncHelper() {
  CancelVSyncRequest();
}

void AndroidVSyncHelper::OnVSync(JNIEnv* env,
                                 const JavaParamRef<jobject>& obj,
                                 jlong time_nanos) {
  // See WindowAndroid::OnVSync.
  DCHECK_EQ(base::TimeTicks::GetClock(),
            base::TimeTicks::Clock::LINUX_CLOCK_MONOTONIC);
  DCHECK(!callback_.is_null());
  base::TimeTicks frame_time =
      base::TimeTicks() +
      base::TimeDelta::FromMicroseconds(time_nanos /
                                        base::Time::kNanosecondsPerMicrosecond);
  last_interval_ = frame_time - last_vsync_;
  last_vsync_ = frame_time;
  base::ResetAndReturn(&callback_).Run(frame_time);
}

void AndroidVSyncHelper::RequestVSync(
    const base::RepeatingCallback<void(base::TimeTicks)>& callback) {
  DCHECK(callback_.is_null());
  DCHECK(!callback.is_null());
  callback_ = callback;
  JNIEnv* env = AttachCurrentThread();
  Java_AndroidVSyncHelper_requestVSync(env, j_object_);
}

void AndroidVSyncHelper::CancelVSyncRequest() {
  if (callback_.is_null())
    return;
  JNIEnv* env = AttachCurrentThread();
  Java_AndroidVSyncHelper_cancelVSyncRequest(env, j_object_);
  callback_.Reset();
}

}  // namespace vr
