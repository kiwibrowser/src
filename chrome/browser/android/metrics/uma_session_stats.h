// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_METRICS_UMA_SESSION_STATS_H_
#define CHROME_BROWSER_ANDROID_METRICS_UMA_SESSION_STATS_H_

#include <jni.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/time/time.h"

// The native part of java UmaSessionStats class.
class UmaSessionStats {
 public:
  UmaSessionStats();

  void UmaResumeSession(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj);
  void UmaEndSession(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj);

  static void RegisterSyntheticFieldTrial(const std::string& trial_name,
                                          const std::string& group_name);

  static void RegisterSyntheticMultiGroupFieldTrial(
      const std::string& trial_name,
      const std::vector<uint32_t>& group_name_hashes);

 private:
  ~UmaSessionStats();

  // Start of the current session, used for UMA.
  base::TimeTicks session_start_time_;
  int active_session_count_;

  DISALLOW_COPY_AND_ASSIGN(UmaSessionStats);
};

#endif  // CHROME_BROWSER_ANDROID_METRICS_UMA_SESSION_STATS_H_
