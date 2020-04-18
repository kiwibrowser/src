// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BASE_CHROMECAST_CONFIG_ANDROID_H_
#define CHROMECAST_BASE_CHROMECAST_CONFIG_ANDROID_H_

#include <jni.h>

#include "base/callback.h"
#include "base/macros.h"
#include "base/no_destructor.h"

namespace chromecast {
namespace android {

class ChromecastConfigAndroid {
 public:
  static ChromecastConfigAndroid* GetInstance();

  // Returns whether or not the user has allowed sending usage stats and
  // crash reports.
  bool CanSendUsageStats();

  void SetSendUsageStats(bool enabled);

  // Registers a handler to be notified when SendUsageStats is changed.
  void SetSendUsageStatsChangedCallback(
      base::RepeatingCallback<void(bool)> callback);

  void RunSendUsageStatsChangedCallback(bool enabled);

 private:
  friend class base::NoDestructor<ChromecastConfigAndroid>;

  ChromecastConfigAndroid();
  ~ChromecastConfigAndroid();

  base::RepeatingCallback<void(bool)> send_usage_stats_changed_callback_;

  DISALLOW_COPY_AND_ASSIGN(ChromecastConfigAndroid);
};

}  // namespace android
}  // namespace chromecast

#endif  // CHROMECAST_BASE_CHROMECAST_CONFIG_ANDROID_H_
