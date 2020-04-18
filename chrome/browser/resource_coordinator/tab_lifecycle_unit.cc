// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_lifecycle_unit.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/optional.h"
#include "base/process/process_metrics.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/resource_coordinator/lifecycle_state.h"
#include "chrome/browser/resource_coordinator/tab_activity_watcher.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_observer.h"
#include "chrome/browser/resource_coordinator/tab_load_tracker.h"
#include "chrome/browser/resource_coordinator/tab_manager_features.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_importance_signals.h"
#include "url/gurl.h"

namespace resource_coordinator {

TabLifecycleUnitSource::TabLifecycleUnit::TabLifecycleUnit(
    base::ObserverList<TabLifecycleObserver>* observers,
    content::WebContents* web_contents,
    TabStripModel* tab_strip_model)
    : LifecycleUnitBase(web_contents->GetVisibility()),
      content::WebContentsObserver(web_contents),
      observers_(observers),
      tab_strip_model_(tab_strip_model) {
  DCHECK(observers_);
  DCHECK(GetWebContents());
  DCHECK(tab_strip_model_);
}

TabLifecycleUnitSource::TabLifecycleUnit::~TabLifecycleUnit() {
  OnLifecycleUnitDestroyed();
}

void TabLifecycleUnitSource::TabLifecycleUnit::SetTabStripModel(
    TabStripModel* tab_strip_model) {
  tab_strip_model_ = tab_strip_model;
}

void TabLifecycleUnitSource::TabLifecycleUnit::SetWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  Observe(web_contents);
}

void TabLifecycleUnitSource::TabLifecycleUnit::SetFocused(bool focused) {
  const bool was_focused = last_focused_time_ == base::TimeTicks::Max();
  if (focused == was_focused)
    return;
  last_focused_time_ = focused ? base::TimeTicks::Max() : NowTicks();

  if (focused && (GetState() == LifecycleState::DISCARDED ||
                  GetState() == LifecycleState::PENDING_DISCARD)) {
    bool was_discarded = GetState() == LifecycleState::DISCARDED;
    SetState(LifecycleState::ACTIVE);

    // If the tab was fully discarded, the tab needs to be reloaded.
    if (was_discarded) {
      // See comment in Discard() for an explanation of why "needs reload" is
      // false when a tab is discarded.
      // TODO(fdoray): Remove NavigationControllerImpl::needs_reload_ once
      // session restore is handled by LifecycleManager.
      GetWebContents()->GetController().SetNeedsReload();
      GetWebContents()->GetController().LoadIfNecessary();
    }

    // Stop |freeze_timeout_timer_| to avoid completing the discard, as the tab
    // was focused.
    if (freeze_timeout_timer_)
      freeze_timeout_timer_->Stop();

    OnDiscardedStateChange();
  }
}

void TabLifecycleUnitSource::TabLifecycleUnit::SetRecentlyAudible(
    bool recently_audible) {
  if (recently_audible)
    recently_audible_time_ = base::TimeTicks::Max();
  else if (recently_audible_time_ == base::TimeTicks::Max())
    recently_audible_time_ = NowTicks();
}

void TabLifecycleUnitSource::TabLifecycleUnit::UpdateLifecycleState(
    mojom::LifecycleState state) {
  DCHECK_NE(mojom::LifecycleState::kDiscarded, state);

  switch (state) {
    case mojom::LifecycleState::kFrozen: {
      if (GetState() == LifecycleState::PENDING_DISCARD) {
        freeze_timeout_timer_->Stop();
        FinishDiscard(discard_reason_);
      } else {
        SetState(LifecycleState::FROZEN);
      }
      break;
    }

    case mojom::LifecycleState::kRunning: {
      SetState(LifecycleState::ACTIVE);
      break;
    }

    default: {
      NOTREACHED();
      break;
    }
  }
}

void TabLifecycleUnitSource::TabLifecycleUnit::RequestFreezeForDiscard(
    DiscardReason reason) {
  // Ensure this isn't happening on an urgent discard.
  DCHECK_EQ(reason, DiscardReason::kProactive);

  // Ensure the tab is not already pending a discard.
  DCHECK_NE(GetState(), LifecycleState::PENDING_DISCARD);

  SetState(LifecycleState::PENDING_DISCARD);

  // External observers should now view this tab as discarded, hiding the
  // pending discard implementation detail.
  // TODO(fdoray) This should be done as part of SetState() instead of manually
  // after each SetState() that might change IsDiscarded(). We would have less
  // chances of breaking things in a future change.
  OnDiscardedStateChange();

  if (!freeze_timeout_timer_) {
    freeze_timeout_timer_ =
        std::make_unique<base::OneShotTimer>(GetTickClock());
  }

  freeze_timeout_timer_->Start(
      FROM_HERE, kProactiveDiscardFreezeTimeout,
      base::BindRepeating(&TabLifecycleUnit::FinishDiscard,
                          base::Unretained(this), reason));

  Freeze();
}

TabLifecycleUnitExternal*
TabLifecycleUnitSource::TabLifecycleUnit::AsTabLifecycleUnitExternal() {
  return this;
}

base::string16 TabLifecycleUnitSource::TabLifecycleUnit::GetTitle() const {
  return GetWebContents()->GetTitle();
}

base::TimeTicks TabLifecycleUnitSource::TabLifecycleUnit::GetLastFocusedTime()
    const {
  return last_focused_time_;
}

base::ProcessHandle TabLifecycleUnitSource::TabLifecycleUnit::GetProcessHandle()
    const {
  content::RenderFrameHost* main_frame = GetWebContents()->GetMainFrame();
  if (!main_frame)
    return base::ProcessHandle();
  content::RenderProcessHost* process = main_frame->GetProcess();
  if (!process)
    return base::ProcessHandle();
  return process->GetProcess().Handle();
}

LifecycleUnit::SortKey TabLifecycleUnitSource::TabLifecycleUnit::GetSortKey()
    const {
  if (base::FeatureList::IsEnabled(features::kTabRanker)) {
    base::Optional<float> reactivation_score =
        resource_coordinator::TabActivityWatcher::GetInstance()
            ->CalculateReactivationScore(GetWebContents());
    if (reactivation_score.has_value())
      return SortKey(reactivation_score.value(), last_focused_time_);
    return SortKey(SortKey::kMaxScore, last_focused_time_);
  }

  return SortKey(last_focused_time_);
}

content::Visibility TabLifecycleUnitSource::TabLifecycleUnit::GetVisibility()
    const {
  return GetWebContents()->GetVisibility();
}

bool TabLifecycleUnitSource::TabLifecycleUnit::Freeze() {
  // Can't request to freeze a discarded tab.
  if (GetState() == LifecycleState::DISCARDED)
    return false;

  // If currently in PENDING_DISCARD, the tab IsDiscarded() to external
  // observers. To maintain this, don't set it to PENDING_FREEZE, but keep the
  // tab in the PENDING_DISCARD state.
  if (GetState() != LifecycleState::PENDING_DISCARD)
    SetState(LifecycleState::PENDING_FREEZE);

  GetWebContents()->FreezePage();
  return true;
}

int TabLifecycleUnitSource::TabLifecycleUnit::
    GetEstimatedMemoryFreedOnDiscardKB() const {
#if defined(OS_CHROMEOS)
  std::unique_ptr<base::ProcessMetrics> process_metrics(
      base::ProcessMetrics::CreateProcessMetrics(GetProcessHandle()));
  base::ProcessMetrics::TotalsSummary summary =
      process_metrics->GetTotalsSummary();
  return summary.private_clean_kb + summary.private_dirty_kb + summary.swap_kb;
#else
  // TODO(fdoray): Implement this. https://crbug.com/775644
  return 0;
#endif
}

bool TabLifecycleUnitSource::TabLifecycleUnit::CanPurge() const {
  // A renderer can be purged if it's not playing media.
  return !IsMediaTab();
}

bool TabLifecycleUnitSource::TabLifecycleUnit::CanFreeze() const {
  if (IsFrozen())
    return false;

  if (GetWebContents()->GetVisibility() == content::Visibility::VISIBLE)
    return false;

  if (IsMediaTab())
    return false;

  // Allow a page to load fully before freezing it.
  if (TabLoadTracker::Get()->GetLoadingState(GetWebContents()) !=
      TabLoadTracker::LoadingState::LOADED) {
    return false;
  }

  return true;
}

bool TabLifecycleUnitSource::TabLifecycleUnit::CanDiscard(
    DiscardReason reason) const {
  // Can't discard a tab that isn't in a TabStripModel.
  if (!tab_strip_model_)
    return false;

  // Can't discard a tab that is already discarded.
  if (IsDiscarded())
    return false;

  if (GetWebContents()->IsCrashed())
    return false;

#if defined(OS_CHROMEOS)
  if (GetWebContents()->GetVisibility() == content::Visibility::VISIBLE)
    return false;
#else
  // Do not discard the tab if it is currently active in its window.
  if (tab_strip_model_->GetActiveWebContents() == GetWebContents())
    return false;
#endif  // defined(OS_CHROMEOS)

  // Do not discard tabs that don't have a valid URL (most probably they have
  // just been opened and discarding them would lose the URL).
  // TODO(fdoray): Look into a workaround to be able to kill the tab without
  // losing the pending navigation.
  if (!GetWebContents()->GetLastCommittedURL().is_valid() ||
      GetWebContents()->GetLastCommittedURL().is_empty()) {
    return false;
  }

  // Do not discard tabs in which the user has entered text in a form.
  if (GetWebContents()->GetPageImportanceSignals().had_form_interaction)
    return false;

  // Do not discard media tabs as it's too distruptive to the user experience.
  if (IsMediaTab())
    return false;

  // Do not discard PDFs as they might contain entry that is not saved and they
  // don't remember their scrolling positions. See crbug.com/547286 and
  // crbug.com/65244.
  // TODO(fdoray): Remove this workaround when the bugs are fixed.
  if (GetWebContents()->GetContentsMimeType() == "application/pdf")
    return false;

  // Do not discard a tab that was explicitly disallowed to.
  if (!IsAutoDiscardable())
    return false;

#if defined(OS_CHROMEOS)
  // The following protections are ignored on ChromeOS during urgent discard,
  // because running out of memory would lead to a kernel panic.
  if (reason == DiscardReason::kUrgent)
    return true;
#endif  // defined(OS_CHROMEOS)

  // Do not discard a tab that has already been discarded.
  // TODO(fdoray): Allow tabs to be discarded more than once.
  // https://crbug.com/794622
  if (discard_count_ > 0)
    return false;

  return true;
}

bool TabLifecycleUnitSource::TabLifecycleUnit::Discard(
    DiscardReason discard_reason) {
  // Can't discard a tab when it isn't in a tabstrip.
  if (!tab_strip_model_)
    return false;

  // Can't discard a tab if it is already discarded.
  if (GetState() == LifecycleState::DISCARDED)
    return false;

  // If a non-urgent discard is requested when the state is PENDING_DISCARD,
  // returns false to indicate that it is incorrect to request a non-urgent
  // discard again.
  if (GetState() == LifecycleState::PENDING_DISCARD &&
      discard_reason == DiscardReason::kProactive) {
    return false;
  }

  discard_reason_ = discard_reason;

  // If the tab is not going through an urgent discard, it should be frozen
  // first. Freeze the tab and set a timer to callback to FinishDiscard() incase
  // the freeze callback takes too long.
  //
  // TODO(fdoray): Request a freeze for kExternal discards too once that doesn't
  // cause asynchronous change of tab id. https://crbug.com/632839
  if (discard_reason == DiscardReason::kProactive &&
      GetState() != LifecycleState::FROZEN) {
    RequestFreezeForDiscard(discard_reason);

    // Returning true because even though the discard did not happen yet, the
    // tab is in PENDING_DISCARD state and will be discarded either when the
    // freeze callback occurs, or the kProactiveDiscardFreezeTimeout timeout is
    // reached.
    return true;
  }

  FinishDiscard(discard_reason);
  return true;
}

void TabLifecycleUnitSource::TabLifecycleUnit::FinishDiscard(
    DiscardReason discard_reason) {
  UMA_HISTOGRAM_BOOLEAN(
      "TabManager.Discarding.DiscardedTabHasBeforeUnloadHandler",
      GetWebContents()->NeedToFireBeforeUnload());

  content::WebContents* const old_contents = GetWebContents();
  content::WebContents::CreateParams create_params(tab_strip_model_->profile());
  // TODO(fdoray): Consider setting |initially_hidden| to true when the tab is
  // OCCLUDED. Will require checking that the tab reload correctly when it
  // becomes VISIBLE.
  create_params.initially_hidden =
      old_contents->GetVisibility() == content::Visibility::HIDDEN;
  create_params.desired_renderer_state =
      content::WebContents::CreateParams::kNoRendererProcess;
  std::unique_ptr<content::WebContents> null_contents =
      content::WebContents::Create(create_params);
  content::WebContents* raw_null_contents = null_contents.get();
  // Copy over the state from the navigation controller to preserve the
  // back/forward history and to continue to display the correct title/favicon.
  //
  // Set |needs_reload| to false so that the tab is not automatically reloaded
  // when activated. If it was true, there would be an immediate reload when the
  // active tab of a non-visible window is discarded. SetFocused() will take
  // care of reloading the tab when it becomes active in a focused window.
  null_contents->GetController().CopyStateFrom(old_contents->GetController(),
                                               /* needs_reload */ false);

  // Persist the last active time property.
  null_contents->SetLastActiveTime(old_contents->GetLastActiveTime());

  // First try to fast-kill the process, if it's just running a single tab.
  bool fast_shutdown_success =
      GetRenderProcessHost()->FastShutdownIfPossible(1u, false);

#if defined(OS_CHROMEOS)
  if (!fast_shutdown_success && discard_reason == DiscardReason::kUrgent) {
    content::RenderFrameHost* main_frame = old_contents->GetMainFrame();
    // We avoid fast shutdown on tabs with beforeunload handlers on the main
    // frame, as that is often an indication of unsaved user state.
    DCHECK(main_frame);
    if (!main_frame->GetSuddenTerminationDisablerState(
            blink::kBeforeUnloadHandler)) {
      fast_shutdown_success = GetRenderProcessHost()->FastShutdownIfPossible(
          1u, /* skip_unload_handlers */ true);
    }
    UMA_HISTOGRAM_BOOLEAN(
        "TabManager.Discarding.DiscardedTabCouldUnsafeFastShutdown",
        fast_shutdown_success);
  }
#endif
  UMA_HISTOGRAM_BOOLEAN("TabManager.Discarding.DiscardedTabCouldFastShutdown",
                        fast_shutdown_success);

  // Replace the discarded tab with the null version.
  const int index = tab_strip_model_->GetIndexOfWebContents(old_contents);
  DCHECK_NE(index, TabStripModel::kNoTab);
  std::unique_ptr<content::WebContents> old_contents_deleter =
      tab_strip_model_->ReplaceWebContentsAt(index, std::move(null_contents));
  DCHECK_EQ(GetWebContents(), raw_null_contents);

  // This ensures that on reload after discard, the document has
  // "wasDiscarded" set to true.
  raw_null_contents->SetWasDiscarded(true);

  // Discard the old tab's renderer.
  // TODO(jamescook): This breaks script connections with other tabs. Find a
  // different approach that doesn't do that, perhaps based on
  // RenderFrameProxyHosts.
  old_contents_deleter.reset();

  LifecycleState previous_state = GetState();
  SetState(LifecycleState::DISCARDED);
  if (previous_state != LifecycleState::PENDING_DISCARD)
    OnDiscardedStateChange();
  ++discard_count_;
}

content::WebContents* TabLifecycleUnitSource::TabLifecycleUnit::GetWebContents()
    const {
  return web_contents();
}

bool TabLifecycleUnitSource::TabLifecycleUnit::IsMediaTab() const {
  // TODO(fdoray): Consider being notified of audible, capturing and mirrored
  // state changes via WebContentsDelegate::NavigationStateChanged().
  // https://crbug.com/775644

  if (recently_audible_time_ == base::TimeTicks::Max() ||
      (!recently_audible_time_.is_null() &&
       NowTicks() - recently_audible_time_ < kTabAudioProtectionTime)) {
    return true;
  }

  scoped_refptr<MediaStreamCaptureIndicator> media_indicator =
      MediaCaptureDevicesDispatcher::GetInstance()
          ->GetMediaStreamCaptureIndicator();
  if (media_indicator->IsCapturingUserMedia(GetWebContents()) ||
      media_indicator->IsBeingMirrored(GetWebContents())) {
    return true;
  }

  return false;
}

bool TabLifecycleUnitSource::TabLifecycleUnit::IsAutoDiscardable() const {
  return auto_discardable_;
}

void TabLifecycleUnitSource::TabLifecycleUnit::SetAutoDiscardable(
    bool auto_discardable) {
  if (auto_discardable_ == auto_discardable)
    return;
  auto_discardable_ = auto_discardable;
  for (auto& observer : *observers_)
    observer.OnAutoDiscardableStateChange(GetWebContents(), auto_discardable_);
}

bool TabLifecycleUnitSource::TabLifecycleUnit::DiscardTab() {
  return Discard(DiscardReason::kExternal);
}

bool TabLifecycleUnitSource::TabLifecycleUnit::FreezeTab() {
  return Freeze();
}

bool TabLifecycleUnitSource::TabLifecycleUnit::IsDiscarded() const {
  // External code does not need to know about the intermediary PENDING_DISCARD
  // state. To external callers, the tab is discarded while in the
  // PENDING_DISCARD state.
  LifecycleState current_state = GetState();
  return current_state == LifecycleState::PENDING_DISCARD ||
         current_state == LifecycleState::DISCARDED;
}

bool TabLifecycleUnitSource::TabLifecycleUnit::IsFrozen() const {
  // External code does not need to know about the intermediary PENDING_FREEZE
  // state.  To external callers, the tab is frozen while in the PENDING_FREEZE
  // state.
  LifecycleState current_state = GetState();
  return current_state == LifecycleState::PENDING_FREEZE ||
         current_state == LifecycleState::FROZEN;
}

int TabLifecycleUnitSource::TabLifecycleUnit::GetDiscardCount() const {
  return discard_count_;
}

void TabLifecycleUnitSource::TabLifecycleUnit::OnDiscardedStateChange() {
  for (auto& observer : *observers_)
    observer.OnDiscardedStateChange(GetWebContents(), IsDiscarded());
}

content::RenderProcessHost*
TabLifecycleUnitSource::TabLifecycleUnit::GetRenderProcessHost() const {
  return GetWebContents()->GetMainFrame()->GetProcess();
}

void TabLifecycleUnitSource::TabLifecycleUnit::DidStartLoading() {
  if (IsDiscarded()) {
    SetState(LifecycleState::ACTIVE);
    OnDiscardedStateChange();
  }
}

void TabLifecycleUnitSource::TabLifecycleUnit::OnVisibilityChanged(
    content::Visibility visibility) {
  OnLifecycleUnitVisibilityChanged(visibility);
}

}  // namespace resource_coordinator
