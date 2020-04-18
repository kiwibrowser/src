// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_coordinator_impl.h"

#include "base/memory/memory_coordinator_client_registry.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/memory/memory_condition_observer.h"
#include "content/browser/memory/memory_coordinator_default_policy.h"
#include "content/browser/memory/memory_monitor.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

namespace {

const int kDefaultMinimumTransitionPeriodSeconds = 30;

mojom::MemoryState ToMojomMemoryState(MemoryState state) {
  switch (state) {
    case MemoryState::UNKNOWN:
      return mojom::MemoryState::UNKNOWN;
    case MemoryState::NORMAL:
      return mojom::MemoryState::NORMAL;
    case MemoryState::THROTTLED:
      return mojom::MemoryState::THROTTLED;
    case MemoryState::SUSPENDED:
      return mojom::MemoryState::SUSPENDED;
    default:
      NOTREACHED();
      return mojom::MemoryState::UNKNOWN;
  }
}

const char* MemoryConditionToString(MemoryCondition condition) {
  switch (condition) {
    case MemoryCondition::NORMAL:
      return "normal";
    case MemoryCondition::CRITICAL:
      return "critical";
  }
  NOTREACHED();
  return "N/A";
}

}  // namespace

// The implementation of MemoryCoordinatorHandle. See memory_coordinator.mojom
// for the role of this class.
class MemoryCoordinatorHandleImpl : public mojom::MemoryCoordinatorHandle {
 public:
  MemoryCoordinatorHandleImpl(mojom::MemoryCoordinatorHandleRequest request,
                              MemoryCoordinatorImpl* coordinator,
                              int render_process_id);
  ~MemoryCoordinatorHandleImpl() override;

  // mojom::MemoryCoordinatorHandle:
  void AddChild(mojom::ChildMemoryCoordinatorPtr child) override;

  mojom::ChildMemoryCoordinatorPtr& child() { return child_; }
  mojo::Binding<mojom::MemoryCoordinatorHandle>& binding() { return binding_; }

 private:
  MemoryCoordinatorImpl* coordinator_;
  int render_process_id_;
  mojom::ChildMemoryCoordinatorPtr child_;
  mojo::Binding<mojom::MemoryCoordinatorHandle> binding_;

  DISALLOW_COPY_AND_ASSIGN(MemoryCoordinatorHandleImpl);
};

MemoryCoordinatorHandleImpl::MemoryCoordinatorHandleImpl(
    mojom::MemoryCoordinatorHandleRequest request,
    MemoryCoordinatorImpl* coordinator,
    int render_process_id)
    : coordinator_(coordinator),
      render_process_id_(render_process_id),
      binding_(this, std::move(request)) {
  DCHECK(coordinator_);
}

MemoryCoordinatorHandleImpl::~MemoryCoordinatorHandleImpl() {}

void MemoryCoordinatorHandleImpl::AddChild(
    mojom::ChildMemoryCoordinatorPtr child) {
  DCHECK(!child_.is_bound());
  child_ = std::move(child);
  coordinator_->OnChildAdded(render_process_id_);
}

// static
MemoryCoordinator* MemoryCoordinator::GetInstance() {
  return MemoryCoordinatorImpl::GetInstance();
}

// static
MemoryCoordinatorImpl* MemoryCoordinatorImpl::GetInstance() {
  if (!base::FeatureList::IsEnabled(features::kMemoryCoordinator))
    return nullptr;
  static MemoryCoordinatorImpl* instance = new MemoryCoordinatorImpl(
      base::ThreadTaskRunnerHandle::Get(), CreateMemoryMonitor());
  return instance;
}

MemoryCoordinatorImpl::MemoryCoordinatorImpl(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    std::unique_ptr<MemoryMonitor> memory_monitor)
    : task_runner_(task_runner),
      policy_(std::make_unique<MemoryCoordinatorDefaultPolicy>(this)),
      delegate_(GetContentClient()->browser()->GetMemoryCoordinatorDelegate()),
      memory_monitor_(std::move(memory_monitor)),
      condition_observer_(
          std::make_unique<MemoryConditionObserver>(this, task_runner)),
      tick_clock_(base::DefaultTickClock::GetInstance()),
      minimum_state_transition_period_(base::TimeDelta::FromSeconds(
          kDefaultMinimumTransitionPeriodSeconds)) {
  DCHECK(memory_monitor_.get());
  base::MemoryCoordinatorProxy::SetMemoryCoordinator(this);

  // Force the "memory_coordinator" category to show up in the trace viewer.
  base::trace_event::TraceLog::GetCategoryGroupEnabled(
      TRACE_DISABLED_BY_DEFAULT("memory_coordinator"));
}

MemoryCoordinatorImpl::~MemoryCoordinatorImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::MemoryCoordinatorProxy::SetMemoryCoordinator(nullptr);
}

void MemoryCoordinatorImpl::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(last_state_change_.is_null());

  notification_registrar_.Add(
      this, NOTIFICATION_RENDER_WIDGET_VISIBILITY_CHANGED,
      NotificationService::AllBrowserContextsAndSources());
  condition_observer_->ScheduleUpdateCondition(base::TimeDelta());
}

void MemoryCoordinatorImpl::OnForegrounded() {
  condition_observer_->OnForegrounded();
}

void MemoryCoordinatorImpl::OnBackgrounded() {
  condition_observer_->OnBackgrounded();
}

void MemoryCoordinatorImpl::CreateHandle(
    int render_process_id,
    mojom::MemoryCoordinatorHandleRequest request) {
  std::unique_ptr<MemoryCoordinatorHandleImpl> handle(
      new MemoryCoordinatorHandleImpl(std::move(request), this,
                                      render_process_id));
  handle->binding().set_connection_error_handler(
      base::BindOnce(&MemoryCoordinatorImpl::OnConnectionError,
                     base::Unretained(this), render_process_id));
  CreateChildInfoMapEntry(render_process_id, std::move(handle));
}

void MemoryCoordinatorImpl::SetBrowserMemoryState(MemoryState memory_state) {
  if (memory_state == browser_memory_state_)
    return;

  base::TimeTicks now = tick_clock_->NowTicks();
  base::TimeDelta elapsed = now - last_state_change_;
  if (!last_state_change_.is_null() &&
      (elapsed < minimum_state_transition_period_)) {
    base::TimeDelta delay = minimum_state_transition_period_ - elapsed +
                            base::TimeDelta::FromSeconds(1);
    delayed_browser_memory_state_setter_.Reset(
        base::Bind(&MemoryCoordinatorImpl::SetBrowserMemoryState,
                   base::Unretained(this), memory_state));
    task_runner_->PostDelayedTask(
        FROM_HERE, delayed_browser_memory_state_setter_.callback(), delay);
    return;
  }

  if (!delayed_browser_memory_state_setter_.IsCancelled())
    delayed_browser_memory_state_setter_.Cancel();

  last_state_change_ = now;
  browser_memory_state_ = memory_state;
  NotifyStateToClients(memory_state);
}

bool MemoryCoordinatorImpl::SetChildMemoryState(int render_process_id,
                                                MemoryState memory_state) {
  // Can't set an invalid memory state.
  if (memory_state == MemoryState::UNKNOWN)
    return false;

  // SUSPENDED state isn't supported yet.
  if (memory_state == MemoryState::SUSPENDED)
    return false;

  // Can't send a message to a child that doesn't exist.
  auto iter = children_.find(render_process_id);
  if (iter == children_.end())
    return false;

  // Can't send a message to a child that isn't bound.
  if (!iter->second.handle->child().is_bound())
    return false;

  memory_state = OverrideState(memory_state, iter->second);

  // A nop doesn't need to be sent, but is considered successful.
  if (iter->second.memory_state == memory_state)
    return true;

  // Update the internal state and send the message.
  iter->second.memory_state = memory_state;
  iter->second.handle->child()->OnStateChange(ToMojomMemoryState(memory_state));
  return true;
}

bool MemoryCoordinatorImpl::TryToPurgeMemoryFromChild(int render_process_id) {
  auto iter = children().find(render_process_id);
  if (iter == children().end())
    return false;
  MemoryCoordinatorHandleImpl* handle = iter->second.handle.get();
  if (!handle || !handle->child() || !handle->child().is_bound())
    return false;
  if (!iter->second.can_purge_after.is_null() &&
      iter->second.can_purge_after > tick_clock_->NowTicks())
    return false;

  // Set |can_purge_after| to the maximum value to suppress another purge
  // request until the child process goes foreground and then goes background
  // again.
  iter->second.can_purge_after = base::TimeTicks::Max();
  handle->child()->PurgeMemory();
  return true;
}

MemoryState MemoryCoordinatorImpl::GetCurrentMemoryState() const {
  return browser_memory_state_;
}

void MemoryCoordinatorImpl::ForceSetMemoryCondition(MemoryCondition condition,
                                                    base::TimeDelta duration) {
  UpdateConditionIfNeeded(condition);
  suppress_condition_change_until_ = tick_clock_->NowTicks() + duration;
}

void MemoryCoordinatorImpl::Observe(int type,
                                    const NotificationSource& source,
                                    const NotificationDetails& details) {
  DCHECK(type == NOTIFICATION_RENDER_WIDGET_VISIBILITY_CHANGED);
  RenderWidgetHost* render_widget_host = Source<RenderWidgetHost>(source).ptr();
  RenderProcessHost* process = render_widget_host->GetProcess();
  if (!process)
    return;
  bool is_visible = *Details<bool>(details).ptr();
  policy_->OnChildVisibilityChanged(process->GetID(), is_visible);
}

MemoryState MemoryCoordinatorImpl::GetStateForProcess(
    base::ProcessHandle handle) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (handle == base::kNullProcessHandle)
    return MemoryState::UNKNOWN;
  if (handle == base::GetCurrentProcessHandle())
    return browser_memory_state_;

  for (auto& iter : children()) {
    auto* render_process_host = GetRenderProcessHost(iter.first);
    if (render_process_host &&
        render_process_host->GetProcess().Handle() == handle)
      return iter.second.memory_state;
  }
  return MemoryState::UNKNOWN;
}

void MemoryCoordinatorImpl::UpdateConditionIfNeeded(
    MemoryCondition next_condition) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (next_condition == MemoryCondition::CRITICAL)
    policy_->OnCriticalCondition();

  if (suppress_condition_change_until_ > tick_clock_->NowTicks() ||
      memory_condition_ == next_condition)
    return;

  TRACE_EVENT2(TRACE_DISABLED_BY_DEFAULT("memory_coordinator"),
               "MemoryCoordinatorImpl::UpdateConditionIfNeeded", "prev",
               MemoryConditionToString(memory_condition_), "next",
               MemoryConditionToString(next_condition));
  policy_->OnConditionChanged(memory_condition_, next_condition);
  memory_condition_ = next_condition;
}

void MemoryCoordinatorImpl::DiscardTab(bool skip_unload_handlers) {
  if (delegate_)
    delegate_->DiscardTab(skip_unload_handlers);
}

RenderProcessHost* MemoryCoordinatorImpl::GetRenderProcessHost(
    int render_process_id) {
  return RenderProcessHost::FromID(render_process_id);
}

void MemoryCoordinatorImpl::SetDelegateForTesting(
    std::unique_ptr<MemoryCoordinatorDelegate> delegate) {
  CHECK(!delegate_);
  delegate_ = std::move(delegate);
}

void MemoryCoordinatorImpl::SetPolicyForTesting(
    std::unique_ptr<Policy> policy) {
  policy_ = std::move(policy);
}

void MemoryCoordinatorImpl::AddChildForTesting(
    int dummy_render_process_id, mojom::ChildMemoryCoordinatorPtr child) {
  mojom::MemoryCoordinatorHandlePtr mch;
  auto request = mojo::MakeRequest(&mch);
  std::unique_ptr<MemoryCoordinatorHandleImpl> handle(
      new MemoryCoordinatorHandleImpl(std::move(request), this,
                                      dummy_render_process_id));
  handle->AddChild(std::move(child));
  CreateChildInfoMapEntry(dummy_render_process_id, std::move(handle));
}

void MemoryCoordinatorImpl::SetTickClockForTesting(
    const base::TickClock* tick_clock) {
  tick_clock_ = tick_clock;
}

void MemoryCoordinatorImpl::OnConnectionError(int render_process_id) {
  children_.erase(render_process_id);
}

void MemoryCoordinatorImpl::OnChildAdded(int render_process_id) {
  RenderProcessHost* render_process_host =
      GetRenderProcessHost(render_process_id);
  if (!render_process_host)
    return;

  // Populate an initial state of a newly created process.
  // TODO(bashi): IsProcessBackgrounded() may return true even when tabs in the
  // renderer process are invisible (e.g. restoring tabs all at once).
  // Figure out a better way to set visibility.
  policy_->OnChildVisibilityChanged(
      render_process_id, !render_process_host->IsProcessBackgrounded());
}

MemoryState MemoryCoordinatorImpl::OverrideState(MemoryState memory_state,
                                                 const ChildInfo& child) {
  // We don't suspend foreground renderers. Throttle them instead.
  if (child.is_visible && memory_state == MemoryState::SUSPENDED)
    return MemoryState::THROTTLED;
#if defined(OS_ANDROID)
  // On Android, we throttle background renderers immediately.
  // TODO(bashi): Create a specialized class of MemoryCoordinator for Android
  // and move this ifdef to the class.
  if (!child.is_visible && memory_state == MemoryState::NORMAL)
    return MemoryState::THROTTLED;
  // TODO(bashi): Suspend background renderers after a certain period of time.
#endif  // defined(OS_ANDROID)
  return memory_state;
}

void MemoryCoordinatorImpl::CreateChildInfoMapEntry(
    int render_process_id,
    std::unique_ptr<MemoryCoordinatorHandleImpl> handle) {
  auto& child_info = children_[render_process_id];
  // Process always start with normal memory state.
  // We'll set renderer's memory state to the current global state when the
  // corresponding renderer process is ready to communicate. Renderer processes
  // call AddChild() when they are ready.
  child_info.memory_state = MemoryState::NORMAL;
  child_info.is_visible = true;
  child_info.handle = std::move(handle);
}

void MemoryCoordinatorImpl::NotifyStateToClients(MemoryState state) {
  base::MemoryCoordinatorClientRegistry::GetInstance()->Notify(state);
}

MemoryCoordinatorImpl::ChildInfo::ChildInfo() {}

MemoryCoordinatorImpl::ChildInfo::ChildInfo(const ChildInfo& rhs) {
  // This is a nop, but exists for compatibility with STL containers.
}

MemoryCoordinatorImpl::ChildInfo::~ChildInfo() {}

}  // namespace content
