// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_MONITOR_H_
#define CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_MONITOR_H_

#include "base/android/jni_android.h"
#include "base/callback.h"
#include "base/callback_list.h"
#include "base/process/process_metrics.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"

// NearOomMonitor tracks memory stats to estimate whether we are in "near-OOM"
// situation. Near-OOM is defined as a situation where the foreground apps
// will be killed soon when we keep allocating memory.
// This monitor periodically checks memory stats and invokes registered
// callbacks when the monitor detects near-OOM situation.
class NearOomMonitor {
 public:
  // Returns nullptr when the monitor isn't available.
  static NearOomMonitor* GetInstance();

  virtual ~NearOomMonitor();

  base::TimeDelta GetMonitoringInterval() const { return monitoring_interval_; }
  base::TimeDelta GetCooldownInterval() const { return cooldown_interval_; }

  using CallbackList = base::CallbackList<void()>;
  using Subscription = CallbackList::Subscription;

  // Registers a callback which is invoked when this monitor detects near-OOM
  // situation. The callback will be called on the task runner on which this
  // monitor is running. Destroy the returned Subscription to unregister.
  std::unique_ptr<Subscription> RegisterCallback(base::Closure callback);

  void OnLowMemory(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& jcaller);

 protected:
  static NearOomMonitor* Create();

  NearOomMonitor(scoped_refptr<base::SequencedTaskRunner> task_runner,
                 int64_t swapfree_threshold);

  // Gets system memory info. This is a virtual method so that we can override
  // this for testing.
  virtual bool GetSystemMemoryInfo(base::SystemMemoryInfoKB* memory_info);

  // Returns true when the monitor uses Android's memory pressure signals.
  // This is a virtual method so that we can override this for testing.
  virtual bool ComponentCallbackIsEnabled();

 private:
  // Checks whether we are in near-OOM situation.
  void Check();
  void ScheduleCheck();

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::Callback<void()> check_callback_;

  // The time between Check() calls. When Check() detects a near-OOM
  // situation, |cooldown_interval_| is used instead of this interval to
  // schedule next Check().
  base::TimeDelta monitoring_interval_;
  // The time which should pass between two successive near-OOM detections.
  base::TimeDelta cooldown_interval_;
  // The time when Check() will be called next.
  base::TimeTicks next_check_time_;

  int64_t swapfree_threshold_;

  CallbackList callbacks_;

  bool component_callback_is_enabled_;
  base::android::ScopedJavaGlobalRef<jobject> j_object_;

  DISALLOW_COPY_AND_ASSIGN(NearOomMonitor);
};

#endif  // CHROME_BROWSER_ANDROID_OOM_INTERVENTION_NEAR_OOM_MONITOR_H_
