// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_NTP_NTP_SNIPPETS_LAUNCHER_H_
#define CHROME_BROWSER_ANDROID_NTP_NTP_SNIPPETS_LAUNCHER_H_

#include "base/android/jni_android.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "components/ntp_snippets/remote/persistent_scheduler.h"

// Android implementation of ntp_snippets::NTPSnippetsScheduler.
// The NTPSnippetsLauncher singleton owns the Java SnippetsLauncher object, and
// is used to schedule the fetching of snippets. Runs on the UI thread.
class NTPSnippetsLauncher : public ntp_snippets::PersistentScheduler {
 public:
  static NTPSnippetsLauncher* Get();

  // ntp_snippets::NTPSnippetsScheduler implementation.
  bool Schedule(base::TimeDelta period_wifi,
                base::TimeDelta period_fallback) override;
  bool Unschedule() override;
  bool IsOnUnmeteredConnection() override;

 private:
  friend struct base::LazyInstanceTraitsBase<NTPSnippetsLauncher>;

  // Constructor and destructor marked private to enforce singleton.
  NTPSnippetsLauncher();
  virtual ~NTPSnippetsLauncher();

  base::android::ScopedJavaGlobalRef<jobject> java_launcher_;

  DISALLOW_COPY_AND_ASSIGN(NTPSnippetsLauncher);
};

#endif  // CHROME_BROWSER_ANDROID_NTP_NTP_SNIPPETS_LAUNCHER_H_
