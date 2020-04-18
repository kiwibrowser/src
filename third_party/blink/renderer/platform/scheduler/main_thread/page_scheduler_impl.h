// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_PAGE_SCHEDULER_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_PAGE_SCHEDULER_IMPL_H_

#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/child/page_visibility_state.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/task_queue_throttler.h"
#include "third_party/blink/renderer/platform/scheduler/public/page_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/util/tracing_helper.h"

namespace base {
namespace trace_event {
class BlameContext;
class TracedValue;
}  // namespace trace_event
}  // namespace base

namespace blink {
namespace scheduler {

class CPUTimeBudgetPool;
class FrameSchedulerImpl;
class MainThreadSchedulerImpl;

class PLATFORM_EXPORT PageSchedulerImpl : public PageScheduler {
 public:
  PageSchedulerImpl(PageScheduler::Delegate*,
                    MainThreadSchedulerImpl*,
                    bool disable_background_timer_throttling);

  ~PageSchedulerImpl() override;

  // PageScheduler implementation:
  void SetPageVisible(bool page_visible) override;
  void SetPageFrozen(bool) override;
  void SetKeepActive(bool) override;
  bool IsMainFrameLocal() const override;
  void SetIsMainFrameLocal(bool is_local) override;

  std::unique_ptr<FrameScheduler> CreateFrameScheduler(
      BlameContext*,
      FrameScheduler::FrameType) override;
  base::TimeTicks EnableVirtualTime() override;
  void DisableVirtualTimeForTesting() override;
  bool VirtualTimeAllowedToAdvance() const override;
  void SetVirtualTimePolicy(VirtualTimePolicy) override;
  void SetInitialVirtualTime(base::Time time) override;
  void SetInitialVirtualTimeOffset(base::TimeDelta offset) override;
  void GrantVirtualTimeBudget(
      base::TimeDelta budget,
      base::OnceClosure budget_exhausted_callback) override;
  void SetMaxVirtualTimeTaskStarvationCount(
      int max_task_starvation_count) override;
  void AudioStateChanged(bool is_audio_playing) override;
  bool IsAudioPlaying() const override;
  bool IsExemptFromBudgetBasedThrottling() const override;
  bool HasActiveConnectionForTest() const override;
  void RequestBeginMainFrameNotExpected(bool new_state) override;
  void AddVirtualTimeObserver(VirtualTimeObserver*) override;
  void RemoveVirtualTimeObserver(VirtualTimeObserver*) override;

  // Virtual for testing.
  virtual void ReportIntervention(const std::string& message);

  bool IsPageVisible() const;
  bool IsFrozen() const;
  // PageSchedulerImpl::HasActiveConnection can be used in non-test code,
  // while PageScheduler::HasActiveConnectionForTest can't.
  bool HasActiveConnection() const;
  // Note that the frame can throttle queues even when the page is not throttled
  // (e.g. for offscreen frames or recently backgrounded pages).
  bool IsThrottled() const;
  bool KeepActive() const;

  std::unique_ptr<FrameSchedulerImpl> CreateFrameSchedulerImpl(
      base::trace_event::BlameContext*,
      FrameScheduler::FrameType);

  void Unregister(FrameSchedulerImpl*);
  void OnNavigation();

  void OnConnectionUpdated();

  void OnTraceLogEnabled();

  // Return a number of child web frame schedulers for this PageScheduler.
  size_t FrameCount() const;

  void AsValueInto(base::trace_event::TracedValue* state) const;

  ukm::UkmRecorder* GetUkmRecorder();
  int64_t GetUkmSourceId();

  base::WeakPtr<PageSchedulerImpl> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  friend class FrameSchedulerImpl;

  enum class AudioState {
    kSilent,
    kAudible,
    kRecentlyAudible,
  };

  enum class NotificationPolicy { kNotifyFrames, kDoNotNotifyFrames };

  // Support not issuing a notification to frames when we disable freezing as
  // a part of foregrounding the page.
  void SetPageFrozenImpl(bool frozen, NotificationPolicy notification_policy);

  CPUTimeBudgetPool* BackgroundCPUTimeBudgetPool();
  void MaybeInitializeBackgroundCPUTimeBudgetPool();

  void OnThrottlingReported(base::TimeDelta throttling_duration);

  // Depending on page visibility, either turns throttling off, or schedules a
  // call to enable it after a grace period.
  void UpdateBackgroundThrottlingState(NotificationPolicy notification_policy);

  // As a part of UpdateBackgroundThrottlingState set correct
  // background_time_budget_pool_ state depending on page visibility and
  // number of active connections.
  void UpdateBackgroundBudgetPoolThrottlingState();

  // Callback for marking page is silent after a delay since last audible
  // signal.
  void OnAudioSilent();

  // Callback for enabling throttling in background after specified delay.
  // TODO(altimin): Trigger throttling depending on the loading state
  // of the page.
  void DoThrottlePage();

  // Notify frames that the page scheduler state has been updated.
  void NotifyFrames();

  void EnableThrottling();

  TraceableVariableController tracing_controller_;
  std::set<FrameSchedulerImpl*> frame_schedulers_;
  MainThreadSchedulerImpl* main_thread_scheduler_;

  PageVisibilityState page_visibility_;
  bool disable_background_timer_throttling_;
  AudioState audio_state_;
  bool is_frozen_;
  bool reported_background_throttling_since_navigation_;
  bool has_active_connection_;
  bool nested_runloop_;
  bool is_main_frame_local_;
  bool is_throttled_;
  bool keep_active_;
  CPUTimeBudgetPool* background_time_budget_pool_;  // Not owned.
  PageScheduler::Delegate* delegate_;               // Not owned.
  CancelableClosureHolder do_throttle_page_callback_;
  CancelableClosureHolder on_audio_silent_closure_;
  base::WeakPtrFactory<PageSchedulerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PageSchedulerImpl);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_PAGE_SCHEDULER_IMPL_H_
