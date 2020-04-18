// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/oom_intervention/near_oom_monitor.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/sys_info.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/common/chrome_features.h"
#include "jni/NearOomMonitor_jni.h"

namespace {

const char kSwapFreeThresholdRatioParamName[] = "swap_free_threshold_ratio";
const char kUseComponentCallbacks[] = "use_component_callbacks";

// Default SwapFree/SwapTotal ratio for detecting near-OOM situation.
// TODO(bashi): Confirm that this is appropriate.
const double kDefaultSwapFreeThresholdRatio = 0.45;

// Default interval to check memory stats.
constexpr base::TimeDelta kDefaultMonitoringDelta =
    base::TimeDelta::FromSeconds(1);

// Default cooldown interval to resume monitoring after a detection.
constexpr base::TimeDelta kDefaultCooldownDelta =
    base::TimeDelta::FromSeconds(30);

}  // namespace

// static
NearOomMonitor* NearOomMonitor::Create() {
  if (!base::FeatureList::IsEnabled(features::kOomIntervention))
    return nullptr;
  if (!base::SysInfo::IsLowEndDevice())
    return nullptr;

  base::SystemMemoryInfoKB memory_info;
  if (!base::GetSystemMemoryInfo(&memory_info))
    return nullptr;

  // If there is no swap (zram) the monitor doesn't work because we use
  // SwapFree as the tracking metric.
  if (memory_info.swap_total == 0)
    return nullptr;

  double threshold_ratio = base::GetFieldTrialParamByFeatureAsDouble(
      features::kOomIntervention, kSwapFreeThresholdRatioParamName,
      kDefaultSwapFreeThresholdRatio);
  int64_t swapfree_threshold =
      static_cast<int64_t>(memory_info.swap_total * threshold_ratio);
  return new NearOomMonitor(base::ThreadTaskRunnerHandle::Get(),
                            swapfree_threshold);
}

// static
NearOomMonitor* NearOomMonitor::GetInstance() {
  static NearOomMonitor* instance = NearOomMonitor::Create();
  return instance;
}

NearOomMonitor::NearOomMonitor(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    int64_t swapfree_threshold)
    : task_runner_(task_runner),
      check_callback_(
          base::Bind(&NearOomMonitor::Check, base::Unretained(this))),
      monitoring_interval_(kDefaultMonitoringDelta),
      cooldown_interval_(kDefaultCooldownDelta),
      swapfree_threshold_(swapfree_threshold),
      component_callback_is_enabled_(
          base::GetFieldTrialParamByFeatureAsBool(features::kOomIntervention,
                                                  kUseComponentCallbacks,
                                                  true)) {
  if (ComponentCallbackIsEnabled()) {
    j_object_.Reset(Java_NearOomMonitor_create(
        base::android::AttachCurrentThread(), reinterpret_cast<jlong>(this)));
  }
}

NearOomMonitor::~NearOomMonitor() = default;

std::unique_ptr<NearOomMonitor::Subscription> NearOomMonitor::RegisterCallback(
    base::Closure callback) {
  if (callbacks_.empty() && !ComponentCallbackIsEnabled())
    ScheduleCheck();
  return callbacks_.Add(std::move(callback));
}

void NearOomMonitor::OnLowMemory(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  callbacks_.Notify();
}

bool NearOomMonitor::GetSystemMemoryInfo(
    base::SystemMemoryInfoKB* memory_info) {
  DCHECK(memory_info);
  return base::GetSystemMemoryInfo(memory_info);
}

bool NearOomMonitor::ComponentCallbackIsEnabled() {
  return component_callback_is_enabled_;
}

void NearOomMonitor::Check() {
  base::SystemMemoryInfoKB memory_info;
  if (!GetSystemMemoryInfo(&memory_info)) {
    LOG(WARNING) << "Failed to get system memory info and stop monitoring.";
    return;
  }

  if (memory_info.swap_free <= swapfree_threshold_) {
    callbacks_.Notify();
    next_check_time_ = base::TimeTicks::Now() + cooldown_interval_;
  } else {
    next_check_time_ = base::TimeTicks::Now() + monitoring_interval_;
  }

  if (!callbacks_.empty())
    ScheduleCheck();
}

void NearOomMonitor::ScheduleCheck() {
  DCHECK(!ComponentCallbackIsEnabled());

  if (next_check_time_.is_null()) {
    task_runner_->PostTask(FROM_HERE, check_callback_);
  } else {
    base::TimeDelta delta =
        std::max(next_check_time_ - base::TimeTicks::Now(), base::TimeDelta());
    task_runner_->PostDelayedTask(FROM_HERE, check_callback_, delta);
  }
}
