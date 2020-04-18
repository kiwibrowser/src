// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager_impl.h"

#include <queue>
#include <vector>

#include "base/bind.h"
#include "base/bit_cast.h"
#include "base/compiler_specific.h"
#include "base/debug/crash_logging.h"
#include "base/rand_util.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/trace_event/trace_event.h"
#include "third_party/blink/renderer/platform/scheduler/base/real_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_selector.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_time_observer.h"
#include "third_party/blink/renderer/platform/scheduler/base/thread_controller_impl.h"
#include "third_party/blink/renderer/platform/scheduler/base/work_queue.h"
#include "third_party/blink/renderer/platform/scheduler/base/work_queue_sets.h"

namespace base {
namespace sequence_manager {

namespace {

const double kLongTaskTraceEventThreshold = 0.05;
const double kSamplingRateForRecordingCPUTime = 0.01;

double MonotonicTimeInSeconds(TimeTicks time_ticks) {
  return (time_ticks - TimeTicks()).InSecondsF();
}

// Magic value to protect against memory corruption and bail out
// early when detected.
constexpr int kMemoryCorruptionSentinelValue = 0xdeadbeef;

void SweepCanceledDelayedTasksInQueue(
    internal::TaskQueueImpl* queue,
    std::map<TimeDomain*, TimeTicks>* time_domain_now) {
  TimeDomain* time_domain = queue->GetTimeDomain();
  if (time_domain_now->find(time_domain) == time_domain_now->end())
    time_domain_now->insert(std::make_pair(time_domain, time_domain->Now()));
  queue->SweepCanceledDelayedTasks(time_domain_now->at(time_domain));
}

}  // namespace

// static
std::unique_ptr<TaskQueueManager> TaskQueueManager::TakeOverCurrentThread() {
  return TaskQueueManagerImpl::TakeOverCurrentThread();
}

TaskQueueManagerImpl::TaskQueueManagerImpl(
    std::unique_ptr<internal::ThreadController> controller)
    : graceful_shutdown_helper_(new internal::GracefulQueueShutdownHelper()),
      controller_(std::move(controller)),
      memory_corruption_sentinel_(kMemoryCorruptionSentinelValue),
      weak_factory_(this) {
  // TODO(altimin): Create a sequence checker here.
  DCHECK(controller_->RunsTasksInCurrentSequence());

  TRACE_EVENT_WARMUP_CATEGORY("sequence_manager");
  TRACE_EVENT_WARMUP_CATEGORY(TRACE_DISABLED_BY_DEFAULT("sequence_manager"));
  TRACE_EVENT_WARMUP_CATEGORY(
      TRACE_DISABLED_BY_DEFAULT("sequence_manager.debug"));
  TRACE_EVENT_WARMUP_CATEGORY(
      TRACE_DISABLED_BY_DEFAULT("sequence_manager.verbose_snapshots"));

  TRACE_EVENT_OBJECT_CREATED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("sequence_manager"), "TaskQueueManager", this);
  main_thread_only().selector.SetTaskQueueSelectorObserver(this);

  RegisterTimeDomain(main_thread_only().real_time_domain.get());

  controller_->SetSequencedTaskSource(this);
  controller_->AddNestingObserver(this);
}

TaskQueueManagerImpl::~TaskQueueManagerImpl() {
  TRACE_EVENT_OBJECT_DELETED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("sequence_manager"), "TaskQueueManager", this);

  // TODO(altimin): restore default task runner automatically when
  // ThreadController is destroyed.
  controller_->RestoreDefaultTaskRunner();

  for (internal::TaskQueueImpl* queue : main_thread_only().active_queues) {
    main_thread_only().selector.RemoveQueue(queue);
    queue->UnregisterTaskQueue();
  }

  main_thread_only().active_queues.clear();
  main_thread_only().queues_to_gracefully_shutdown.clear();

  graceful_shutdown_helper_->OnTaskQueueManagerDeleted();

  main_thread_only().selector.SetTaskQueueSelectorObserver(nullptr);
  controller_->RemoveNestingObserver(this);
}

TaskQueueManagerImpl::AnyThread::AnyThread() = default;

TaskQueueManagerImpl::AnyThread::~AnyThread() = default;

TaskQueueManagerImpl::MainThreadOnly::MainThreadOnly()
    : random_generator(RandUint64()),
      uniform_distribution(0.0, 1.0),
      real_time_domain(new RealTimeDomain()) {}

TaskQueueManagerImpl::MainThreadOnly::~MainThreadOnly() = default;

std::unique_ptr<TaskQueueManagerImpl>
TaskQueueManagerImpl::TakeOverCurrentThread() {
  return std::unique_ptr<TaskQueueManagerImpl>(
      new TaskQueueManagerImpl(internal::ThreadControllerImpl::Create(
          MessageLoop::current(), DefaultTickClock::GetInstance())));
}

void TaskQueueManagerImpl::RegisterTimeDomain(TimeDomain* time_domain) {
  main_thread_only().time_domains.insert(time_domain);
  time_domain->OnRegisterWithTaskQueueManager(this);
}

void TaskQueueManagerImpl::UnregisterTimeDomain(TimeDomain* time_domain) {
  main_thread_only().time_domains.erase(time_domain);
}

RealTimeDomain* TaskQueueManagerImpl::GetRealTimeDomain() const {
  return main_thread_only().real_time_domain.get();
}

std::unique_ptr<internal::TaskQueueImpl>
TaskQueueManagerImpl::CreateTaskQueueImpl(const TaskQueue::Spec& spec) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  TimeDomain* time_domain = spec.time_domain
                                ? spec.time_domain
                                : main_thread_only().real_time_domain.get();
  DCHECK(main_thread_only().time_domains.find(time_domain) !=
         main_thread_only().time_domains.end());
  std::unique_ptr<internal::TaskQueueImpl> task_queue =
      std::make_unique<internal::TaskQueueImpl>(this, time_domain, spec);
  main_thread_only().active_queues.insert(task_queue.get());
  main_thread_only().selector.AddQueue(task_queue.get());
  return task_queue;
}

void TaskQueueManagerImpl::SetObserver(Observer* observer) {
  main_thread_only().observer = observer;
}

void TaskQueueManagerImpl::UnregisterTaskQueueImpl(
    std::unique_ptr<internal::TaskQueueImpl> task_queue) {
  TRACE_EVENT1("sequence_manager", "TaskQueueManagerImpl::UnregisterTaskQueue",
               "queue_name", task_queue->GetName());
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  main_thread_only().selector.RemoveQueue(task_queue.get());

  {
    AutoLock lock(any_thread_lock_);
    any_thread().has_incoming_immediate_work.erase(task_queue.get());
  }

  task_queue->UnregisterTaskQueue();

  // Add |task_queue| to |main_thread_only().queues_to_delete| so we can prevent
  // it from being freed while any of our structures hold hold a raw pointer to
  // it.
  main_thread_only().active_queues.erase(task_queue.get());
  main_thread_only().queues_to_delete[task_queue.get()] = std::move(task_queue);
}

void TaskQueueManagerImpl::ReloadEmptyWorkQueues(
    const IncomingImmediateWorkMap& queues_to_reload) const {
  // There are two cases where a queue needs reloading.  First, it might be
  // completely empty and we've just posted a task (this method handles that
  // case). Secondly if the work queue becomes empty in when calling
  // WorkQueue::TakeTaskFromWorkQueue (handled there).
  for (const auto& pair : queues_to_reload) {
    pair.first->ReloadImmediateWorkQueueIfEmpty();
  }
}

void TaskQueueManagerImpl::WakeUpReadyDelayedQueues(LazyNow* lazy_now) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
               "TaskQueueManagerImpl::WakeUpReadyDelayedQueues");

  for (TimeDomain* time_domain : main_thread_only().time_domains) {
    if (time_domain == main_thread_only().real_time_domain.get()) {
      time_domain->WakeUpReadyDelayedQueues(lazy_now);
    } else {
      LazyNow time_domain_lazy_now = time_domain->CreateLazyNow();
      time_domain->WakeUpReadyDelayedQueues(&time_domain_lazy_now);
    }
  }
}

void TaskQueueManagerImpl::OnBeginNestedRunLoop() {
  // We just entered a nested run loop, make sure there's a DoWork posted or
  // the system will grind to a halt.
  main_thread_only().nesting_depth++;
  if (main_thread_only().observer && main_thread_only().nesting_depth == 1)
    main_thread_only().observer->OnBeginNestedRunLoop();
}

void TaskQueueManagerImpl::OnExitNestedRunLoop() {
  main_thread_only().nesting_depth--;
  DCHECK_GE(main_thread_only().nesting_depth, 0);
  if (main_thread_only().nesting_depth == 0) {
    // While we were nested some non-nestable tasks may have become eligible to
    // run. We push them back onto the front of their original work queues.
    while (!main_thread_only().non_nestable_task_queue.empty()) {
      internal::TaskQueueImpl::DeferredNonNestableTask& non_nestable_task =
          *main_thread_only().non_nestable_task_queue.begin();
      non_nestable_task.task_queue->RequeueDeferredNonNestableTask(
          std::move(non_nestable_task));
      main_thread_only().non_nestable_task_queue.pop_front();
    }
    if (main_thread_only().observer)
      main_thread_only().observer->OnExitNestedRunLoop();
  }
}

void TaskQueueManagerImpl::OnQueueHasIncomingImmediateWork(
    internal::TaskQueueImpl* queue,
    internal::EnqueueOrder enqueue_order,
    bool queue_is_blocked) {
  {
    AutoLock lock(any_thread_lock_);
    any_thread().has_incoming_immediate_work.insert(
        std::make_pair(queue, enqueue_order));
  }

  if (!queue_is_blocked)
    controller_->ScheduleWork();
}

void TaskQueueManagerImpl::MaybeScheduleImmediateWork(
    const Location& from_here) {
  controller_->ScheduleWork();
}

void TaskQueueManagerImpl::MaybeScheduleDelayedWork(
    const Location& from_here,
    TimeDomain* requesting_time_domain,
    TimeTicks now,
    TimeTicks run_time) {
  // TODO(kraynov): Convert time domains to use LazyNow.
  LazyNow lazy_now(now);
  controller_->SetNextDelayedDoWork(&lazy_now, run_time);
}

// TODO(kraynov): Remove after simplifying TimeDomain.
void TaskQueueManagerImpl::CancelDelayedWork(TimeDomain* requesting_time_domain,
                                             TimeTicks run_time) {
  controller_->SetNextDelayedDoWork(nullptr, TimeTicks::Max());
}

Optional<PendingTask> TaskQueueManagerImpl::TakeTask() {
  CHECK(Validate());

  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  TRACE_EVENT0("sequence_manager", "TaskQueueManagerImpl::TakeTask");

  IncomingImmediateWorkMap queues_to_reload;

  {
    AutoLock lock(any_thread_lock_);
    std::swap(queues_to_reload, any_thread().has_incoming_immediate_work);
  }

  // It's important we call ReloadEmptyWorkQueues out side of the lock to
  // avoid a lock order inversion.
  ReloadEmptyWorkQueues(queues_to_reload);
  LazyNow lazy_now(main_thread_only().real_time_domain->CreateLazyNow());
  WakeUpReadyDelayedQueues(&lazy_now);

  while (true) {
    internal::WorkQueue* work_queue = nullptr;
    bool should_run =
        main_thread_only().selector.SelectWorkQueueToService(&work_queue);
    TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
        TRACE_DISABLED_BY_DEFAULT("sequence_manager.debug"), "TaskQueueManager",
        this, AsValueWithSelectorResult(should_run, work_queue));

    if (!should_run)
      return nullopt;

    // If the head task was canceled, remove it and run the selector again.
    if (work_queue->RemoveAllCanceledTasksFromFront())
      continue;

    if (work_queue->GetFrontTask()->nestable == Nestable::kNonNestable &&
        main_thread_only().nesting_depth > 0) {
      // Defer non-nestable work. NOTE these tasks can be arbitrarily delayed so
      // the additional delay should not be a problem.
      // Note because we don't delete queues while nested, it's perfectly OK to
      // store the raw pointer for |queue| here.
      internal::TaskQueueImpl::DeferredNonNestableTask deferred_task{
          work_queue->TakeTaskFromWorkQueue(), work_queue->task_queue(),
          work_queue->queue_type()};
      // We push these tasks onto the front to make sure that when requeued they
      // are pushed in the right order.
      main_thread_only().non_nestable_task_queue.push_front(
          std::move(deferred_task));
      continue;
    }

    // Due to nested message loops we need to maintain a stack of currently
    // executing tasks so in TaskQueueManagerImpl::DidRunTask we can run the
    // right observers.
    main_thread_only().task_execution_stack.emplace_back(
        work_queue->TakeTaskFromWorkQueue(), work_queue->task_queue());
    ExecutingTask& executing_task =
        *main_thread_only().task_execution_stack.rbegin();
    NotifyWillProcessTask(&executing_task, &lazy_now);
    return std::move(executing_task.pending_task);
  }
}

void TaskQueueManagerImpl::DidRunTask() {
  LazyNow lazy_now(main_thread_only().real_time_domain->CreateLazyNow());
  ExecutingTask& executing_task =
      *main_thread_only().task_execution_stack.rbegin();
  NotifyDidProcessTask(executing_task, &lazy_now);
  main_thread_only().task_execution_stack.pop_back();

  if (main_thread_only().nesting_depth == 0)
    CleanUpQueues();
}

TimeDelta TaskQueueManagerImpl::DelayTillNextTask(LazyNow* lazy_now) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);

  // If the selector has non-empty queues we trivially know there is immediate
  // work to be done.
  if (!main_thread_only().selector.AllEnabledWorkQueuesAreEmpty())
    return TimeDelta();

  // Its possible the selectors state is dirty because ReloadEmptyWorkQueues
  // hasn't been called yet. This check catches the case of fresh incoming work.
  {
    AutoLock lock(any_thread_lock_);
    for (const auto& pair : any_thread().has_incoming_immediate_work) {
      if (pair.first->CouldTaskRun(pair.second))
        return TimeDelta();
    }
  }

  // Otherwise we need to find the shortest delay, if any.  NB we don't need to
  // call WakeUpReadyDelayedQueues because it's assumed DelayTillNextTask will
  // return TimeDelta>() if the delayed task is due to run now.
  TimeDelta delay_till_next_task = TimeDelta::Max();
  for (TimeDomain* time_domain : main_thread_only().time_domains) {
    Optional<TimeDelta> delay = time_domain->DelayTillNextTask(lazy_now);
    if (!delay)
      continue;

    if (*delay < delay_till_next_task)
      delay_till_next_task = *delay;
  }
  return delay_till_next_task;
}

void TaskQueueManagerImpl::DidQueueTask(
    const internal::TaskQueueImpl::Task& pending_task) {
  controller_->DidQueueTask(pending_task);
}

void TaskQueueManagerImpl::NotifyWillProcessTask(ExecutingTask* executing_task,
                                                 LazyNow* time_before_task) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
               "TaskQueueManagerImpl::NotifyWillProcessTaskObservers");
  if (executing_task->task_queue->GetQuiescenceMonitored())
    main_thread_only().task_was_run_on_quiescence_monitored_queue = true;

  debug::SetCrashKeyString(
      main_thread_only().file_name_crash_key,
      executing_task->pending_task.posted_from.file_name());
  debug::SetCrashKeyString(
      main_thread_only().function_name_crash_key,
      executing_task->pending_task.posted_from.function_name());

  if (executing_task->task_queue->GetShouldNotifyObservers()) {
    {
      TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
                   "TaskQueueManager.WillProcessTaskObservers");
      for (auto& observer : main_thread_only().task_observers)
        observer.WillProcessTask(executing_task->pending_task);
    }

    {
      TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
                   "TaskQueueManager.QueueNotifyWillProcessTask");
      executing_task->task_queue->NotifyWillProcessTask(
          executing_task->pending_task);
    }

    bool notify_time_observers =
        main_thread_only().nesting_depth == 0 &&
        (main_thread_only().task_time_observers.might_have_observers() ||
         executing_task->task_queue->RequiresTaskTiming());
    if (notify_time_observers) {
      executing_task->task_start_time = time_before_task->Now();
      double task_start_time_sec =
          MonotonicTimeInSeconds(executing_task->task_start_time);

      {
        TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
                     "TaskQueueManager.WillProcessTaskTimeObservers");
        for (auto& observer : main_thread_only().task_time_observers)
          observer.WillProcessTask(task_start_time_sec);
      }

      {
        TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
                     "TaskQueueManager.QueueOnTaskStarted");
        executing_task->task_queue->OnTaskStarted(
            executing_task->pending_task, executing_task->task_start_time);
      }
    }
  }

  executing_task->should_record_thread_time = ShouldRecordCPUTimeForTask();
  if (executing_task->should_record_thread_time)
    executing_task->task_start_thread_time = ThreadTicks::Now();
}

void TaskQueueManagerImpl::NotifyDidProcessTask(
    const ExecutingTask& executing_task,
    LazyNow* time_after_task) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
               "TaskQueueManagerImpl::NotifyDidProcessTaskObservers");

  Optional<TimeDelta> thread_time;
  if (executing_task.should_record_thread_time) {
    auto task_end_thread_time = ThreadTicks::Now();
    thread_time = task_end_thread_time - executing_task.task_start_thread_time;
  }

  if (!executing_task.task_queue->GetShouldNotifyObservers())
    return;

  double task_start_time_sec =
      MonotonicTimeInSeconds(executing_task.task_start_time);
  double task_end_time_sec = 0;

  if (task_start_time_sec) {
    task_end_time_sec = MonotonicTimeInSeconds(time_after_task->Now());

    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
                 "TaskQueueManager.DidProcessTaskTimeObservers");
    for (auto& observer : main_thread_only().task_time_observers)
      observer.DidProcessTask(task_start_time_sec, task_end_time_sec);
  }

  {
    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
                 "TaskQueueManager.DidProcessTaskObservers");
    for (auto& observer : main_thread_only().task_observers)
      observer.DidProcessTask(executing_task.pending_task);
  }

  {
    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
                 "TaskQueueManager.QueueNotifyDidProcessTask");
    executing_task.task_queue->NotifyDidProcessTask(
        executing_task.pending_task);
  }

  {
    TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
                 "TaskQueueManager.QueueOnTaskCompleted");
    if (task_start_time_sec && task_end_time_sec) {
      executing_task.task_queue->OnTaskCompleted(
          executing_task.pending_task, executing_task.task_start_time,
          time_after_task->Now(), thread_time);
    }
  }

  if (task_start_time_sec && task_end_time_sec &&
      task_end_time_sec - task_start_time_sec > kLongTaskTraceEventThreshold) {
    TRACE_EVENT_INSTANT1("blink", "LongTask", TRACE_EVENT_SCOPE_THREAD,
                         "duration", task_end_time_sec - task_start_time_sec);
  }
}

void TaskQueueManagerImpl::SetWorkBatchSize(int work_batch_size) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  DCHECK_GE(work_batch_size, 1);
  controller_->SetWorkBatchSize(work_batch_size);
}

void TaskQueueManagerImpl::AddTaskObserver(
    MessageLoop::TaskObserver* task_observer) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  main_thread_only().task_observers.AddObserver(task_observer);
}

void TaskQueueManagerImpl::RemoveTaskObserver(
    MessageLoop::TaskObserver* task_observer) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  main_thread_only().task_observers.RemoveObserver(task_observer);
}

void TaskQueueManagerImpl::AddTaskTimeObserver(
    TaskTimeObserver* task_time_observer) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  main_thread_only().task_time_observers.AddObserver(task_time_observer);
}

void TaskQueueManagerImpl::RemoveTaskTimeObserver(
    TaskTimeObserver* task_time_observer) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  main_thread_only().task_time_observers.RemoveObserver(task_time_observer);
}

bool TaskQueueManagerImpl::GetAndClearSystemIsQuiescentBit() {
  bool task_was_run =
      main_thread_only().task_was_run_on_quiescence_monitored_queue;
  main_thread_only().task_was_run_on_quiescence_monitored_queue = false;
  return !task_was_run;
}

internal::EnqueueOrder TaskQueueManagerImpl::GetNextSequenceNumber() {
  return enqueue_order_generator_.GenerateNext();
}

LazyNow TaskQueueManagerImpl::CreateLazyNow() const {
  return LazyNow(controller_->GetClock());
}

std::unique_ptr<trace_event::ConvertableToTraceFormat>
TaskQueueManagerImpl::AsValueWithSelectorResult(
    bool should_run,
    internal::WorkQueue* selected_work_queue) const {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  std::unique_ptr<trace_event::TracedValue> state(
      new trace_event::TracedValue());
  TimeTicks now = main_thread_only().real_time_domain->CreateLazyNow().Now();
  state->BeginArray("active_queues");
  for (auto* const queue : main_thread_only().active_queues)
    queue->AsValueInto(now, state.get());
  state->EndArray();
  state->BeginArray("queues_to_gracefully_shutdown");
  for (const auto& pair : main_thread_only().queues_to_gracefully_shutdown)
    pair.first->AsValueInto(now, state.get());
  state->EndArray();
  state->BeginArray("queues_to_delete");
  for (const auto& pair : main_thread_only().queues_to_delete)
    pair.first->AsValueInto(now, state.get());
  state->EndArray();
  state->BeginDictionary("selector");
  main_thread_only().selector.AsValueInto(state.get());
  state->EndDictionary();
  if (should_run) {
    state->SetString("selected_queue",
                     selected_work_queue->task_queue()->GetName());
    state->SetString("work_queue_name", selected_work_queue->name());
  }

  state->BeginArray("time_domains");
  for (auto* time_domain : main_thread_only().time_domains)
    time_domain->AsValueInto(state.get());
  state->EndArray();
  {
    AutoLock lock(any_thread_lock_);
    state->BeginArray("has_incoming_immediate_work");
    for (const auto& pair : any_thread().has_incoming_immediate_work) {
      state->AppendString(pair.first->GetName());
    }
    state->EndArray();
  }
  return std::move(state);
}

void TaskQueueManagerImpl::OnTaskQueueEnabled(internal::TaskQueueImpl* queue) {
  DCHECK_CALLED_ON_VALID_THREAD(main_thread_checker_);
  DCHECK(queue->IsQueueEnabled());
  // Only schedule DoWork if there's something to do.
  if (queue->HasTaskToRunImmediately() && !queue->BlockedByFence())
    MaybeScheduleImmediateWork(FROM_HERE);
}

void TaskQueueManagerImpl::SweepCanceledDelayedTasks() {
  std::map<TimeDomain*, TimeTicks> time_domain_now;
  for (auto* const queue : main_thread_only().active_queues)
    SweepCanceledDelayedTasksInQueue(queue, &time_domain_now);
  for (const auto& pair : main_thread_only().queues_to_gracefully_shutdown)
    SweepCanceledDelayedTasksInQueue(pair.first, &time_domain_now);
}

void TaskQueueManagerImpl::TakeQueuesToGracefullyShutdownFromHelper() {
  std::vector<std::unique_ptr<internal::TaskQueueImpl>> queues =
      graceful_shutdown_helper_->TakeQueues();
  for (std::unique_ptr<internal::TaskQueueImpl>& queue : queues) {
    main_thread_only().queues_to_gracefully_shutdown[queue.get()] =
        std::move(queue);
  }
}

void TaskQueueManagerImpl::CleanUpQueues() {
  TakeQueuesToGracefullyShutdownFromHelper();

  for (auto it = main_thread_only().queues_to_gracefully_shutdown.begin();
       it != main_thread_only().queues_to_gracefully_shutdown.end();) {
    if (it->first->IsEmpty()) {
      UnregisterTaskQueueImpl(std::move(it->second));
      main_thread_only().active_queues.erase(it->first);
      main_thread_only().queues_to_gracefully_shutdown.erase(it++);
    } else {
      ++it;
    }
  }
  main_thread_only().queues_to_delete.clear();
}

scoped_refptr<internal::GracefulQueueShutdownHelper>
TaskQueueManagerImpl::GetGracefulQueueShutdownHelper() const {
  return graceful_shutdown_helper_;
}

WeakPtr<TaskQueueManagerImpl> TaskQueueManagerImpl::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void TaskQueueManagerImpl::SetDefaultTaskRunner(
    scoped_refptr<SingleThreadTaskRunner> task_runner) {
  controller_->SetDefaultTaskRunner(task_runner);
}

const TickClock* TaskQueueManagerImpl::GetClock() const {
  return controller_->GetClock();
}

TimeTicks TaskQueueManagerImpl::NowTicks() const {
  return controller_->GetClock()->NowTicks();
}

bool TaskQueueManagerImpl::ShouldRecordCPUTimeForTask() {
  return ThreadTicks::IsSupported() &&
         main_thread_only().uniform_distribution(
             main_thread_only().random_generator) <
             kSamplingRateForRecordingCPUTime;
}

double TaskQueueManagerImpl::GetSamplingRateForRecordingCPUTime() const {
  if (!ThreadTicks::IsSupported())
    return 0;
  return kSamplingRateForRecordingCPUTime;
}

MSVC_DISABLE_OPTIMIZE()
bool TaskQueueManagerImpl::Validate() {
  return memory_corruption_sentinel_ == kMemoryCorruptionSentinelValue;
}
MSVC_ENABLE_OPTIMIZE()

void TaskQueueManagerImpl::EnableCrashKeys(
    const char* file_name_crash_key_name,
    const char* function_name_crash_key_name) {
  DCHECK(!main_thread_only().file_name_crash_key);
  DCHECK(!main_thread_only().function_name_crash_key);
  main_thread_only().file_name_crash_key = debug::AllocateCrashKeyString(
      file_name_crash_key_name, debug::CrashKeySize::Size64);
  main_thread_only().function_name_crash_key = debug::AllocateCrashKeyString(
      function_name_crash_key_name, debug::CrashKeySize::Size64);
}

internal::TaskQueueImpl* TaskQueueManagerImpl::currently_executing_task_queue()
    const {
  if (main_thread_only().task_execution_stack.empty())
    return nullptr;
  return main_thread_only().task_execution_stack.rbegin()->task_queue;
}

}  // namespace sequence_manager
}  // namespace base
