// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_condition_observer.h"

#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "content/browser/memory/memory_monitor.h"

namespace content {

namespace {

// An expected renderer size. These values come from the median of appropriate
// UMA stats.
#if defined(OS_ANDROID) || defined(OS_IOS)
const int kExpectedRendererSizeMB = 40;
#elif defined(OS_WIN)
const int kExpectedRendererSizeMB = 70;
#else  // Mac, Linux, and ChromeOS
const int kExpectedRendererSizeMB = 120;
#endif

const int kNewRenderersUntilCritical = 2;
const int kDefaultMonitoringIntervalSeconds = 1;
const int kMonitoringIntervalBackgroundedSeconds = 120;

}  // namespace

MemoryConditionObserver::MemoryConditionObserver(
    MemoryCoordinatorImpl* coordinator,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : coordinator_(coordinator), task_runner_(task_runner) {
  DCHECK(coordinator_);
  monitoring_interval_ =
      base::TimeDelta::FromSeconds(kDefaultMonitoringIntervalSeconds);
  monitoring_interval_foregrounded_ =
      base::TimeDelta::FromSeconds(kDefaultMonitoringIntervalSeconds);
  monitoring_interval_backgrounded_ =
      base::TimeDelta::FromSeconds(kMonitoringIntervalBackgroundedSeconds);
}

MemoryConditionObserver::~MemoryConditionObserver() {}

void MemoryConditionObserver::ScheduleUpdateCondition(base::TimeDelta delay) {
  update_condition_closure_.Reset(base::Bind(
      &MemoryConditionObserver::UpdateCondition, base::Unretained(this)));
  task_runner_->PostDelayedTask(FROM_HERE, update_condition_closure_.callback(),
                                delay);
}

void MemoryConditionObserver::OnForegrounded() {
  SetMonitoringInterval(monitoring_interval_foregrounded_);
}

void MemoryConditionObserver::OnBackgrounded() {
  SetMonitoringInterval(monitoring_interval_backgrounded_);
}

void MemoryConditionObserver::SetMonitoringInterval(base::TimeDelta interval) {
  DCHECK(!interval.is_zero());
  if (interval == monitoring_interval_)
    return;
  monitoring_interval_ = interval;
  ScheduleUpdateCondition(interval);
}

MemoryCondition MemoryConditionObserver::CalculateNextCondition() {
  int available =
      coordinator_->memory_monitor()->GetFreeMemoryUntilCriticalMB();

  // TODO(chrisha): Move this histogram recording to a better place when
  // https://codereview.chromium.org/2479673002/ is landed.
  UMA_HISTOGRAM_MEMORY_LARGE_MB("Memory.Coordinator.FreeMemoryUntilCritical",
                                available);

  int expected_renderer_count = available / kExpectedRendererSizeMB;
  if (available <= 0 || expected_renderer_count < kNewRenderersUntilCritical)
    return MemoryCondition::CRITICAL;
  return MemoryCondition::NORMAL;
}

void MemoryConditionObserver::UpdateCondition() {
  auto next_condition = CalculateNextCondition();
  coordinator_->UpdateConditionIfNeeded(next_condition);
  ScheduleUpdateCondition(monitoring_interval_);
}


}  // namespace content
