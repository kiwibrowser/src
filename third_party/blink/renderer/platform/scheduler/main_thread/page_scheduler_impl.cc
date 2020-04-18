// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/main_thread/page_scheduler_impl.h"

#include "base/logging.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/base/virtual_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/child/default_params.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/budget_pool.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/auto_advancing_virtual_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/frame_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"

namespace blink {
namespace scheduler {

namespace {

constexpr double kDefaultBackgroundBudgetAsCPUFraction = .01;
constexpr double kDefaultMaxBackgroundBudgetLevelInSeconds = 3;
constexpr double kDefaultInitialBackgroundBudgetInSeconds = 1;
constexpr double kDefaultMaxBackgroundThrottlingDelayInSeconds = 0;

// Given that we already align timers to 1Hz, do not report throttling if
// it is under 3s.
constexpr base::TimeDelta kMinimalBackgroundThrottlingDurationToReport =
    base::TimeDelta::FromSeconds(3);

// We do not throttle anything while audio is played and shortly after that.
constexpr base::TimeDelta kRecentAudioDelay = base::TimeDelta::FromSeconds(5);

// Delay for fully throttling the page after backgrounding.
constexpr base::TimeDelta kThrottlingDelayAfterBackgrounding =
    base::TimeDelta::FromSeconds(10);

// Values coming from the field trial config are interpreted as follows:
//   -1 is "not set". Scheduler should use a reasonable default.
//   0 corresponds to base::nullopt.
//   Other values are left without changes.

struct BackgroundThrottlingSettings {
  double budget_recovery_rate;
  base::Optional<base::TimeDelta> max_budget_level;
  base::Optional<base::TimeDelta> max_throttling_delay;
  base::Optional<base::TimeDelta> initial_budget;
};

double GetDoubleParameterFromMap(
    const std::map<std::string, std::string>& settings,
    const std::string& setting_name,
    double default_value) {
  const auto& find_it = settings.find(setting_name);
  if (find_it == settings.end())
    return default_value;
  double parsed_value;
  if (!base::StringToDouble(find_it->second, &parsed_value))
    return default_value;
  if (parsed_value == -1)
    return default_value;
  return parsed_value;
}

base::Optional<base::TimeDelta> DoubleToOptionalTime(double value) {
  if (value == 0)
    return base::nullopt;
  return base::TimeDelta::FromSecondsD(value);
}

BackgroundThrottlingSettings GetBackgroundThrottlingSettings() {
  std::map<std::string, std::string> background_throttling_settings;
  base::GetFieldTrialParams("ExpensiveBackgroundTimerThrottling",
                            &background_throttling_settings);

  BackgroundThrottlingSettings settings;

  settings.budget_recovery_rate =
      GetDoubleParameterFromMap(background_throttling_settings, "cpu_budget",
                                kDefaultBackgroundBudgetAsCPUFraction);

  settings.max_budget_level = DoubleToOptionalTime(
      GetDoubleParameterFromMap(background_throttling_settings, "max_budget",
                                kDefaultMaxBackgroundBudgetLevelInSeconds));

  settings.max_throttling_delay = DoubleToOptionalTime(
      GetDoubleParameterFromMap(background_throttling_settings, "max_delay",
                                kDefaultMaxBackgroundThrottlingDelayInSeconds));

  settings.initial_budget = DoubleToOptionalTime(GetDoubleParameterFromMap(
      background_throttling_settings, "initial_budget",
      kDefaultInitialBackgroundBudgetInSeconds));

  return settings;
}

}  // namespace

PageSchedulerImpl::PageSchedulerImpl(
    PageScheduler::Delegate* delegate,
    MainThreadSchedulerImpl* main_thread_scheduler,
    bool disable_background_timer_throttling)
    : main_thread_scheduler_(main_thread_scheduler),
      page_visibility_(kDefaultPageVisibility),
      disable_background_timer_throttling_(disable_background_timer_throttling),
      audio_state_(AudioState::kSilent),
      is_frozen_(false),
      reported_background_throttling_since_navigation_(false),
      has_active_connection_(false),
      nested_runloop_(false),
      is_main_frame_local_(false),
      is_throttled_(false),
      keep_active_(main_thread_scheduler->SchedulerKeepActive()),
      background_time_budget_pool_(nullptr),
      delegate_(delegate),
      weak_factory_(this) {
  main_thread_scheduler->AddPageScheduler(this);
  do_throttle_page_callback_.Reset(base::BindRepeating(
      &PageSchedulerImpl::DoThrottlePage, base::Unretained(this)));
  on_audio_silent_closure_.Reset(base::BindRepeating(
      &PageSchedulerImpl::OnAudioSilent, base::Unretained(this)));
}

PageSchedulerImpl::~PageSchedulerImpl() {
  // TODO(alexclarke): Find out why we can't rely on the web view outliving the
  // frame.
  for (FrameSchedulerImpl* frame_scheduler : frame_schedulers_) {
    frame_scheduler->DetachFromPageScheduler();
  }
  main_thread_scheduler_->RemovePageScheduler(this);

  if (background_time_budget_pool_)
    background_time_budget_pool_->Close();
}

void PageSchedulerImpl::SetPageVisible(bool page_visible) {
  PageVisibilityState page_visibility = page_visible
                                            ? PageVisibilityState::kVisible
                                            : PageVisibilityState::kHidden;

  if (disable_background_timer_throttling_ ||
      page_visibility_ == page_visibility)
    return;
  page_visibility_ = page_visibility;

  // Visible pages should not be frozen.
  if (page_visibility_ == PageVisibilityState::kVisible)
    SetPageFrozenImpl(false, NotificationPolicy::kDoNotNotifyFrames);

  for (FrameSchedulerImpl* frame_scheduler : frame_schedulers_)
    frame_scheduler->SetPageVisibilityForTracing(page_visibility_);

  UpdateBackgroundThrottlingState(NotificationPolicy::kDoNotNotifyFrames);

  NotifyFrames();
}

void PageSchedulerImpl::SetPageFrozen(bool frozen) {
  SetPageFrozenImpl(frozen, NotificationPolicy::kNotifyFrames);
}

void PageSchedulerImpl::SetPageFrozenImpl(
    bool frozen,
    PageSchedulerImpl::NotificationPolicy notification_policy) {
  if (is_frozen_ == frozen)
    return;
  is_frozen_ = frozen;
  for (FrameSchedulerImpl* frame_scheduler : frame_schedulers_)
    frame_scheduler->SetPageFrozenForTracing(frozen);
  if (delegate_)
    delegate_->SetPageFrozen(frozen);
  if (notification_policy ==
      PageSchedulerImpl::NotificationPolicy::kNotifyFrames)
    NotifyFrames();
}

void PageSchedulerImpl::SetKeepActive(bool keep_active) {
  if (keep_active_ == keep_active)
    return;
  keep_active_ = keep_active;

  for (FrameSchedulerImpl* frame_scheduler : frame_schedulers_)
    frame_scheduler->SetPageKeepActiveForTracing(keep_active);

  NotifyFrames();
}

bool PageSchedulerImpl::KeepActive() const {
  return keep_active_;
}

bool PageSchedulerImpl::IsMainFrameLocal() const {
  return is_main_frame_local_;
}

void PageSchedulerImpl::SetIsMainFrameLocal(bool is_local) {
  is_main_frame_local_ = is_local;
}

std::unique_ptr<FrameSchedulerImpl> PageSchedulerImpl::CreateFrameSchedulerImpl(
    base::trace_event::BlameContext* blame_context,
    FrameScheduler::FrameType frame_type) {
  MaybeInitializeBackgroundCPUTimeBudgetPool();
  std::unique_ptr<FrameSchedulerImpl> frame_scheduler(new FrameSchedulerImpl(
      main_thread_scheduler_, this, blame_context, frame_type));
  frame_scheduler->UpdatePolicy();
  frame_schedulers_.insert(frame_scheduler.get());
  return frame_scheduler;
}

std::unique_ptr<blink::FrameScheduler> PageSchedulerImpl::CreateFrameScheduler(
    blink::BlameContext* blame_context,
    FrameScheduler::FrameType frame_type) {
  return CreateFrameSchedulerImpl(blame_context, frame_type);
}

void PageSchedulerImpl::Unregister(FrameSchedulerImpl* frame_scheduler) {
  DCHECK(frame_schedulers_.find(frame_scheduler) != frame_schedulers_.end());
  frame_schedulers_.erase(frame_scheduler);
}

void PageSchedulerImpl::OnNavigation() {
  reported_background_throttling_since_navigation_ = false;
}

void PageSchedulerImpl::ReportIntervention(const std::string& message) {
  delegate_->ReportIntervention(String::FromUTF8(message.c_str()));
}

base::TimeTicks PageSchedulerImpl::EnableVirtualTime() {
  return main_thread_scheduler_->EnableVirtualTime();
}

void PageSchedulerImpl::DisableVirtualTimeForTesting() {
  main_thread_scheduler_->DisableVirtualTimeForTesting();
}

void PageSchedulerImpl::SetVirtualTimePolicy(VirtualTimePolicy policy) {
  main_thread_scheduler_->SetVirtualTimePolicy(policy);
}

void PageSchedulerImpl::SetInitialVirtualTime(base::Time time) {
  main_thread_scheduler_->SetInitialVirtualTime(time);
}

void PageSchedulerImpl::SetInitialVirtualTimeOffset(base::TimeDelta offset) {
  main_thread_scheduler_->SetInitialVirtualTimeOffset(offset);
}

bool PageSchedulerImpl::VirtualTimeAllowedToAdvance() const {
  return main_thread_scheduler_->VirtualTimeAllowedToAdvance();
}

void PageSchedulerImpl::GrantVirtualTimeBudget(
    base::TimeDelta budget,
    base::OnceClosure budget_exhausted_callback) {
  main_thread_scheduler_->VirtualTimeControlTaskRunner()->PostDelayedTask(
      FROM_HERE, std::move(budget_exhausted_callback), budget);
  // This can shift time forwards if there's a pending MaybeAdvanceVirtualTime,
  // so it's important this is called second.
  main_thread_scheduler_->GetVirtualTimeDomain()->SetVirtualTimeFence(
      main_thread_scheduler_->GetVirtualTimeDomain()->Now() + budget);
}

void PageSchedulerImpl::AddVirtualTimeObserver(VirtualTimeObserver* observer) {
  main_thread_scheduler_->AddVirtualTimeObserver(observer);
}

void PageSchedulerImpl::RemoveVirtualTimeObserver(
    VirtualTimeObserver* observer) {
  main_thread_scheduler_->RemoveVirtualTimeObserver(observer);
}

void PageSchedulerImpl::AudioStateChanged(bool is_audio_playing) {
  if (is_audio_playing) {
    audio_state_ = AudioState::kAudible;
    on_audio_silent_closure_.Cancel();
    NotifyFrames();
    main_thread_scheduler_->OnAudioStateChanged();
  } else {
    if (audio_state_ != AudioState::kAudible)
      return;
    on_audio_silent_closure_.Cancel();

    audio_state_ = AudioState::kRecentlyAudible;
    main_thread_scheduler_->ControlTaskRunner()->PostDelayedTask(
        FROM_HERE, on_audio_silent_closure_.GetCallback(), kRecentAudioDelay);
    // No need to call NotifyFrames or
    // MainThreadScheduler::OnAudioStateChanged here, as for outside world
    // kAudible and kRecentlyAudible are the same thing.
  }
}

void PageSchedulerImpl::OnAudioSilent() {
  DCHECK_EQ(audio_state_, AudioState::kRecentlyAudible);
  audio_state_ = AudioState::kSilent;
  NotifyFrames();
  main_thread_scheduler_->OnAudioStateChanged();
}

bool PageSchedulerImpl::IsExemptFromBudgetBasedThrottling() const {
  return has_active_connection_;
}

bool PageSchedulerImpl::HasActiveConnectionForTest() const {
  return HasActiveConnection();
}

bool PageSchedulerImpl::HasActiveConnection() const {
  return has_active_connection_;
}

void PageSchedulerImpl::RequestBeginMainFrameNotExpected(bool new_state) {
  delegate_->RequestBeginMainFrameNotExpected(new_state);
}

bool PageSchedulerImpl::IsAudioPlaying() const {
  return audio_state_ == AudioState::kAudible ||
         audio_state_ == AudioState::kRecentlyAudible;
}

bool PageSchedulerImpl::IsPageVisible() const {
  return page_visibility_ == PageVisibilityState::kVisible;
}

bool PageSchedulerImpl::IsFrozen() const {
  return is_frozen_;
}

bool PageSchedulerImpl::IsThrottled() const {
  return is_throttled_;
}

void PageSchedulerImpl::OnConnectionUpdated() {
  bool has_active_connection = false;
  for (FrameSchedulerImpl* frame_scheduler : frame_schedulers_) {
    has_active_connection |= frame_scheduler->has_active_connection();
  }

  if (has_active_connection_ != has_active_connection) {
    has_active_connection_ = has_active_connection;
    UpdateBackgroundBudgetPoolThrottlingState();
  }
}

void PageSchedulerImpl::OnTraceLogEnabled() {
  tracing_controller_.OnTraceLogEnabled();
  for (FrameSchedulerImpl* frame_scheduler : frame_schedulers_) {
    frame_scheduler->OnTraceLogEnabled();
  }
}

void PageSchedulerImpl::AsValueInto(
    base::trace_event::TracedValue* state) const {
  state->SetBoolean("page_visible",
                    page_visibility_ == PageVisibilityState::kVisible);
  state->SetBoolean("disable_background_timer_throttling",
                    disable_background_timer_throttling_);
  state->SetBoolean("is_audio_playing", IsAudioPlaying());
  state->SetBoolean("is_frozen", is_frozen_);
  state->SetBoolean("reported_background_throttling_since_navigation",
                    reported_background_throttling_since_navigation_);

  state->BeginDictionary("frame_schedulers");
  for (FrameSchedulerImpl* frame_scheduler : frame_schedulers_) {
    state->BeginDictionaryWithCopiedName(PointerToString(frame_scheduler));
    frame_scheduler->AsValueInto(state);
    state->EndDictionary();
  }
  state->EndDictionary();
}

CPUTimeBudgetPool* PageSchedulerImpl::BackgroundCPUTimeBudgetPool() {
  MaybeInitializeBackgroundCPUTimeBudgetPool();
  return background_time_budget_pool_;
}

void PageSchedulerImpl::MaybeInitializeBackgroundCPUTimeBudgetPool() {
  if (background_time_budget_pool_)
    return;

  if (!RuntimeEnabledFeatures::ExpensiveBackgroundTimerThrottlingEnabled())
    return;

  background_time_budget_pool_ =
      main_thread_scheduler_->task_queue_throttler()->CreateCPUTimeBudgetPool(
          "background");
  base::sequence_manager::LazyNow lazy_now(
      main_thread_scheduler_->tick_clock());

  BackgroundThrottlingSettings settings = GetBackgroundThrottlingSettings();

  background_time_budget_pool_->SetMaxBudgetLevel(lazy_now.Now(),
                                                  settings.max_budget_level);
  background_time_budget_pool_->SetMaxThrottlingDelay(
      lazy_now.Now(), settings.max_throttling_delay);

  background_time_budget_pool_->SetTimeBudgetRecoveryRate(
      lazy_now.Now(), settings.budget_recovery_rate);

  if (settings.initial_budget) {
    background_time_budget_pool_->GrantAdditionalBudget(
        lazy_now.Now(), settings.initial_budget.value());
  }

  UpdateBackgroundBudgetPoolThrottlingState();
}

void PageSchedulerImpl::OnThrottlingReported(
    base::TimeDelta throttling_duration) {
  if (throttling_duration < kMinimalBackgroundThrottlingDurationToReport)
    return;

  if (reported_background_throttling_since_navigation_)
    return;
  reported_background_throttling_since_navigation_ = true;

  std::string message = base::StringPrintf(
      "Timer tasks have taken too much time while the page was in the "
      "background. "
      "As a result, they have been deferred for %.3f seconds. "
      "See https://www.chromestatus.com/feature/6172836527865856 "
      "for more details",
      throttling_duration.InSecondsF());

  delegate_->ReportIntervention(String::FromUTF8(message.c_str()));
}

void PageSchedulerImpl::UpdateBackgroundThrottlingState(
    NotificationPolicy notification_policy) {
  if (page_visibility_ == PageVisibilityState::kVisible) {
    is_throttled_ = false;
    do_throttle_page_callback_.Cancel();
    UpdateBackgroundBudgetPoolThrottlingState();
  } else {
    main_thread_scheduler_->ControlTaskRunner()->PostDelayedTask(
        FROM_HERE, do_throttle_page_callback_.GetCallback(),
        kThrottlingDelayAfterBackgrounding);
  }
  if (notification_policy == NotificationPolicy::kNotifyFrames)
    NotifyFrames();
}

void PageSchedulerImpl::DoThrottlePage() {
  do_throttle_page_callback_.Cancel();
  is_throttled_ = true;

  UpdateBackgroundBudgetPoolThrottlingState();
  NotifyFrames();
}

void PageSchedulerImpl::UpdateBackgroundBudgetPoolThrottlingState() {
  if (!background_time_budget_pool_)
    return;

  base::sequence_manager::LazyNow lazy_now(
      main_thread_scheduler_->tick_clock());
  if (is_throttled_ && !has_active_connection_) {
    background_time_budget_pool_->EnableThrottling(&lazy_now);
  } else {
    background_time_budget_pool_->DisableThrottling(&lazy_now);
  }
}

void PageSchedulerImpl::NotifyFrames() {
  for (FrameSchedulerImpl* frame_scheduler : frame_schedulers_)
    frame_scheduler->UpdatePolicy();
}

size_t PageSchedulerImpl::FrameCount() const {
  return frame_schedulers_.size();
}

void PageSchedulerImpl::SetMaxVirtualTimeTaskStarvationCount(
    int max_task_starvation_count) {
  main_thread_scheduler_->SetMaxVirtualTimeTaskStarvationCount(
      max_task_starvation_count);
}

ukm::UkmRecorder* PageSchedulerImpl::GetUkmRecorder() {
  if (!delegate_)
    return nullptr;
  return delegate_->GetUkmRecorder();
}

int64_t PageSchedulerImpl::GetUkmSourceId() {
  if (!delegate_)
    return 0;
  return delegate_->GetUkmSourceId();
}

}  // namespace scheduler
}  // namespace blink
