// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_FRAME_SCHEDULER_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_FRAME_SCHEDULER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/child/page_visibility_state.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_scheduler_proxy.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/frame_origin_type.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/util/tracing_helper.h"

namespace base {
namespace sequence_manager {
class TaskQueue;
}  // namespace sequence_manager
namespace trace_event {
class BlameContext;
class TracedValue;
}  // namespace trace_event
}  // namespace base

namespace blink {
namespace scheduler {

class MainThreadSchedulerImpl;
class MainThreadTaskQueue;
class PageSchedulerImpl;

namespace main_thread_scheduler_impl_unittest {
class MainThreadSchedulerImplTest;
}

namespace frame_scheduler_impl_unittest {
class FrameSchedulerImplTest;
}

namespace page_scheduler_impl_unittest {
class PageSchedulerImplTest;
}

class PLATFORM_EXPORT FrameSchedulerImpl : public FrameScheduler {
 public:
  FrameSchedulerImpl(MainThreadSchedulerImpl* main_thread_scheduler,
                     PageSchedulerImpl* parent_page_scheduler,
                     base::trace_event::BlameContext* blame_context,
                     FrameScheduler::FrameType frame_type);

  ~FrameSchedulerImpl() override;

  // FrameScheduler implementation:
  std::unique_ptr<ThrottlingObserverHandle> AddThrottlingObserver(
      ObserverType,
      Observer*) override;
  void SetFrameVisible(bool frame_visible) override;
  bool IsFrameVisible() const override;
  bool IsPageVisible() const override;
  void SetPaused(bool frame_paused) override;

  void SetCrossOrigin(bool cross_origin) override;
  bool IsCrossOrigin() const override;
  void TraceUrlChange(const String& url) override;
  FrameScheduler::FrameType GetFrameType() const override;
  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner(TaskType) override;
  PageScheduler* GetPageScheduler() const override;
  void DidStartProvisionalLoad(bool is_main_frame) override;
  void DidCommitProvisionalLoad(bool is_web_history_inert_commit,
                                bool is_reload,
                                bool is_main_frame) override;
  WebScopedVirtualTimePauser CreateWebScopedVirtualTimePauser(
      const WTF::String& name,
      WebScopedVirtualTimePauser::VirtualTaskDuration duration) override;
  void OnFirstMeaningfulPaint() override;
  std::unique_ptr<ActiveConnectionHandle> OnActiveConnectionCreated() override;
  void AsValueInto(base::trace_event::TracedValue* state) const;
  bool IsExemptFromBudgetBasedThrottling() const override;

  scoped_refptr<base::SingleThreadTaskRunner> ControlTaskRunner();

  void UpdatePolicy();

  bool has_active_connection() const { return has_active_connection_; }

  void OnTraceLogEnabled() { tracing_controller_.OnTraceLogEnabled(); }

  void SetPageVisibilityForTracing(PageVisibilityState page_visibility);
  void SetPageKeepActiveForTracing(bool keep_active);
  void SetPageFrozenForTracing(bool frozen);

 private:
  friend class PageSchedulerImpl;
  friend class main_thread_scheduler_impl_unittest::MainThreadSchedulerImplTest;
  friend class frame_scheduler_impl_unittest::FrameSchedulerImplTest;
  friend class page_scheduler_impl_unittest::PageSchedulerImplTest;

  class ActiveConnectionHandleImpl : public ActiveConnectionHandle {
   public:
    ActiveConnectionHandleImpl(FrameSchedulerImpl* frame_scheduler);
    ~ActiveConnectionHandleImpl() override;

   private:
    base::WeakPtr<FrameSchedulerImpl> frame_scheduler_;

    DISALLOW_COPY_AND_ASSIGN(ActiveConnectionHandleImpl);
  };

  class ThrottlingObserverHandleImpl : public ThrottlingObserverHandle {
   public:
    ThrottlingObserverHandleImpl(FrameSchedulerImpl* frame_scheduler,
                                 Observer* observer);
    ~ThrottlingObserverHandleImpl() override;

   private:
    base::WeakPtr<FrameSchedulerImpl> frame_scheduler_;
    Observer* observer_;

    DISALLOW_COPY_AND_ASSIGN(ThrottlingObserverHandleImpl);
  };

  void DetachFromPageScheduler();
  void RemoveThrottleableQueueFromBackgroundCPUTimeBudgetPool();
  void ApplyPolicyToThrottleableQueue();
  bool ShouldThrottleTimers() const;
  FrameScheduler::ThrottlingState CalculateThrottlingState(
      ObserverType type) const;
  void RemoveThrottlingObserver(Observer* observer);
  void UpdateQueuePolicy(
      const scoped_refptr<MainThreadTaskQueue>& queue,
      base::sequence_manager::TaskQueue::QueueEnabledVoter* voter);
  void UpdateThrottling();
  void NotifyThrottlingObservers();

  void DidOpenActiveConnection();
  void DidCloseActiveConnection();

  scoped_refptr<base::sequence_manager::TaskQueue> LoadingTaskQueue();
  scoped_refptr<base::sequence_manager::TaskQueue> LoadingControlTaskQueue();
  scoped_refptr<base::sequence_manager::TaskQueue> ThrottleableTaskQueue();
  scoped_refptr<base::sequence_manager::TaskQueue> DeferrableTaskQueue();
  scoped_refptr<base::sequence_manager::TaskQueue> PausableTaskQueue();
  scoped_refptr<base::sequence_manager::TaskQueue> UnpausableTaskQueue();

  base::WeakPtr<FrameSchedulerImpl> GetWeakPtr();

  const FrameScheduler::FrameType frame_type_;

  TraceableVariableController tracing_controller_;
  scoped_refptr<MainThreadTaskQueue> loading_task_queue_;
  scoped_refptr<MainThreadTaskQueue> loading_control_task_queue_;
  scoped_refptr<MainThreadTaskQueue> throttleable_task_queue_;
  scoped_refptr<MainThreadTaskQueue> deferrable_task_queue_;
  scoped_refptr<MainThreadTaskQueue> pausable_task_queue_;
  scoped_refptr<MainThreadTaskQueue> unpausable_task_queue_;
  std::unique_ptr<base::sequence_manager::TaskQueue::QueueEnabledVoter>
      loading_queue_enabled_voter_;
  std::unique_ptr<base::sequence_manager::TaskQueue::QueueEnabledVoter>
      loading_control_queue_enabled_voter_;
  std::unique_ptr<base::sequence_manager::TaskQueue::QueueEnabledVoter>
      throttleable_queue_enabled_voter_;
  std::unique_ptr<base::sequence_manager::TaskQueue::QueueEnabledVoter>
      deferrable_queue_enabled_voter_;
  std::unique_ptr<base::sequence_manager::TaskQueue::QueueEnabledVoter>
      pausable_queue_enabled_voter_;
  MainThreadSchedulerImpl* main_thread_scheduler_;  // NOT OWNED
  PageSchedulerImpl* parent_page_scheduler_;        // NOT OWNED
  base::trace_event::BlameContext* blame_context_;  // NOT OWNED
  // Observers are not owned by the scheduler.
  std::unordered_map<Observer*, ObserverType> throttling_observers_;
  FrameScheduler::ThrottlingState throttling_state_;
  TraceableState<bool, kTracingCategoryNameInfo> frame_visible_;
  TraceableState<bool, kTracingCategoryNameInfo> frame_paused_;
  TraceableState<FrameOriginType, kTracingCategoryNameInfo> frame_origin_type_;
  StateTracer<kTracingCategoryNameInfo> url_tracer_;
  // |task_queue_throttled_| is false if |throttleable_task_queue_| is absent.
  TraceableState<bool, kTracingCategoryNameInfo> task_queue_throttled_;
  // TODO(kraynov): https://crbug.com/827113
  // Trace active connection count.
  int active_connection_count_;
  TraceableState<bool, kTracingCategoryNameInfo> has_active_connection_;

  // These are the states of the Page.
  // They should be accessed via GetPageScheduler()->SetPageState().
  // they are here because we don't support page-level tracing yet.
  TraceableState<bool, kTracingCategoryNameInfo> page_frozen_for_tracing_;
  TraceableState<PageVisibilityState, kTracingCategoryNameInfo>
      page_visibility_for_tracing_;
  TraceableState<bool, kTracingCategoryNameInfo> page_keep_active_for_tracing_;

  base::WeakPtrFactory<FrameSchedulerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FrameSchedulerImpl);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_FRAME_SCHEDULER_IMPL_H_
