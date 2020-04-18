// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_activity_watcher.h"

#include "base/metrics/histogram_macros.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/resource_coordinator/tab_metrics_logger.h"
#include "chrome/browser/resource_coordinator/tab_ranker/mru_features.h"
#include "chrome/browser/resource_coordinator/tab_ranker/tab_features.h"
#include "chrome/browser/resource_coordinator/tab_ranker/window_features.h"
#include "chrome/browser/resource_coordinator/time.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/window_activity_watcher.h"
#include "components/ukm/content/source_url_recorder.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "third_party/blink/public/platform/web_input_event.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    resource_coordinator::TabActivityWatcher::WebContentsData);

// Use a 1-day max for tab visibility histograms since it's not uncommon to keep
// a tab in the same visibility state for a very long time (see Tab.VisibleTime
// which has 5% of samples in the overflow bucket with a 1-hour max).
#define UMA_TAB_VISIBILITY_HISTOGRAM(visibility, sample)           \
  UMA_HISTOGRAM_CUSTOM_TIMES("Tab.Visibility." visibility, sample, \
                             base::TimeDelta::FromMilliseconds(1), \
                             base::TimeDelta::FromDays(1), 50)

namespace resource_coordinator {

// Per-WebContents helper class that observes its WebContents, notifying
// TabActivityWatcher when interesting events occur. Also provides
// per-WebContents data that TabActivityWatcher uses to log the tab.
class TabActivityWatcher::WebContentsData
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebContentsData>,
      public content::RenderWidgetHost::InputEventObserver {
 public:
  ~WebContentsData() override = default;

  // Calculates the tab reactivation score for a background tab. Returns nullopt
  // if the score could not be calculated, e.g. because the tab is in the
  // foreground.
  base::Optional<float> CalculateReactivationScore() {
    if (web_contents()->IsBeingDestroyed() || backgrounded_time_.is_null())
      return base::nullopt;

    const Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
    if (!browser)
      return base::nullopt;

    tab_ranker::TabFeatures tab = TabMetricsLogger::GetTabFeatures(
        browser, tab_metrics_, NowTicks() - backgrounded_time_);
    tab_ranker::WindowFeatures window =
        WindowActivityWatcher::CreateWindowFeatures(browser);

    float score;
    tab_ranker::TabRankerResult result =
        TabActivityWatcher::GetInstance()->predictor_.ScoreTab(
            tab, window, GetMRUFeatures(), &score);
    if (result == tab_ranker::TabRankerResult::kSuccess)
      return score;
    return base::nullopt;
  }

  // Call when the associated WebContents has been replaced.
  void WasReplaced() { was_replaced_ = true; }

  // Call when the associated WebContents has replaced the WebContents of
  // another tab. Copies info from the other WebContentsData so future events
  // can be logged consistently.
  void DidReplace(const WebContentsData& replaced_tab) {
    // Copy creation and foregrounded times to retain the replaced tab's MRU
    // position.
    creation_time_ = replaced_tab.creation_time_;
    foregrounded_time_ = replaced_tab.foregrounded_time_;

    // Copy background status so ForegroundOrClosed can potentially be logged.
    backgrounded_time_ = replaced_tab.backgrounded_time_;

    // Copy the replaced tab's stats.
    tab_metrics_.page_metrics = replaced_tab.tab_metrics_.page_metrics;
    tab_metrics_.page_transition = replaced_tab.tab_metrics_.page_transition;
  }

  // Call when the WebContents is detached from its tab. If the tab is later
  // re-inserted elsewhere, we use the state it had before being detached.
  void TabDetached() { is_detached_ = true; }

  // Call when the tab is inserted into a tab strip to update state.
  void TabInserted(bool foreground) {
    if (is_detached_) {
      is_detached_ = false;

      // Dragged tabs are normally inserted into their new tab strip in the
      // "background", then "activated", even though the user perceives the tab
      // staying active the whole time. So don't update |background_time_| here.
      //
      // TODO(michaelpg): If a background tab is dragged (as part of a group)
      // and inserted, it may be treated as being foregrounded (depending on tab
      // order). This is a small edge case, but can be fixed by the plan to
      // merge the ForegroundedOrClosed and TabMetrics events.
      return;
    }

    if (foreground) {
      foregrounded_time_ = NowTicks();
    } else {
      // This is a new tab that was opened in the background.
      backgrounded_time_ = NowTicks();
    }
  }

  // Logs TabMetrics for the tab if it is considered to be backgrounded.
  void LogTabIfBackgrounded() {
    if (!backgrounded_time_.is_null()) {
      TabActivityWatcher::GetInstance()->tab_metrics_logger_->LogBackgroundTab(
          ukm_source_id_, tab_metrics_);
    }
  }

  // Sets foregrounded_time_ to NowTicks() so this becomes the
  // most-recently-used tab.
  void TabWindowActivated() { foregrounded_time_ = NowTicks(); }

 private:
  friend class content::WebContentsUserData<WebContentsData>;

  explicit WebContentsData(content::WebContents* web_contents)
      : WebContentsObserver(web_contents) {
    DCHECK(!web_contents->GetBrowserContext()->IsOffTheRecord());
    tab_metrics_.web_contents = web_contents;
    web_contents->GetRenderViewHost()->GetWidget()->AddInputEventObserver(this);

    creation_time_ = NowTicks();

    // A navigation may already have completed if this is a replacement tab.
    ukm_source_id_ = ukm::GetSourceIdForWebContentsDocument(web_contents);
  }

  void WasHidden() {
    // The tab may not be in the tabstrip if it's being moved or replaced.
    Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
    if (!browser)
      return;

    DCHECK(!browser->tab_strip_model()->closing_all());

    if (browser->tab_strip_model()->GetActiveWebContents() == web_contents() &&
        !browser->window()->IsMinimized()) {
      // The active tab is considered to be in the foreground unless its window
      // is minimized. It might still get hidden, e.g. when the browser is about
      // to close, but that shouldn't count as a backgrounded event.
      //
      // TODO(michaelpg): On Mac, hiding the application (e.g. via Cmd+H) should
      // log tabs as backgrounded. Check NSApplication's isHidden property.
      return;
    }

    backgrounded_time_ = NowTicks();
    LogTabIfBackgrounded();
  }

  void WasShown() {
    if (backgrounded_time_.is_null())
      return;

    Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
    if (browser && browser->tab_strip_model()->closing_all())
      return;

    // Log the event before updating times.
    TabActivityWatcher::GetInstance()
        ->tab_metrics_logger_->LogBackgroundTabShown(
            ukm_source_id_, NowTicks() - backgrounded_time_, GetMRUFeatures());

    backgrounded_time_ = base::TimeTicks();
    foregrounded_time_ = NowTicks();
    creation_time_ = NowTicks();

    tab_metrics_.page_metrics.num_reactivations++;
  }

  // content::WebContentsObserver:
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override {
    if (old_host != nullptr)
      old_host->GetWidget()->RemoveInputEventObserver(this);
    new_host->GetWidget()->AddInputEventObserver(this);
  }

  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (!navigation_handle->HasCommitted() ||
        !navigation_handle->IsInMainFrame() ||
        navigation_handle->IsSameDocument()) {
      return;
    }

    // Use the same SourceId that SourceUrlRecorderWebContentsObserver populates
    // and updates.
    ukm::SourceId new_source_id = ukm::ConvertToSourceId(
        navigation_handle->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);
    DCHECK_NE(new_source_id, ukm_source_id_)
        << "Expected a unique Source ID for the navigation";
    ukm_source_id_ = new_source_id;

    // Update navigation time for UKM reporting.
    navigation_time_ = navigation_handle->NavigationStart();

    // Reset the per-page data.
    tab_metrics_.page_metrics = {};

    // Update navigation info.
    tab_metrics_.page_transition = navigation_handle->GetPageTransition();
  }

  // Logs metrics for the tab when it stops loading instead of immediately
  // after a navigation commits, so we can have some idea of its status and
  // contents.
  void DidStopLoading() override {
    // Ignore load events in foreground tabs. The tab state of a foreground tab
    // will be logged if/when it is backgrounded.
    LogTabIfBackgrounded();
  }

  void OnVisibilityChanged(content::Visibility visibility) override {
    // Record Tab.Visibility.* histogram and do associated bookkeeping.
    // Recording is done at every visibility state change rather than just when
    // the WebContents is destroyed to reduce data loss on session end.
    RecordVisibilityHistogram(visibility);

    // Record background tab UKMs and do associated bookkepping.
    if (!web_contents()->IsBeingDestroyed()) {
      // TODO(michaelpg): Consider treating occluded tabs as hidden.
      if (visibility == content::Visibility::HIDDEN) {
        WasHidden();
      } else {
        WasShown();
      }
    }
  }

  void RecordVisibilityHistogram(content::Visibility new_visibility) {
    const base::TimeTicks now = NowTicks();
    const base::TimeDelta duration = now - last_visibility_change_time_;
    switch (visibility_) {
      case content::Visibility::VISIBLE: {
        UMA_TAB_VISIBILITY_HISTOGRAM("Visible", duration);
        break;
      }

      case content::Visibility::OCCLUDED: {
        UMA_TAB_VISIBILITY_HISTOGRAM("Occluded", duration);
        break;
      }

      case content::Visibility::HIDDEN: {
        UMA_TAB_VISIBILITY_HISTOGRAM("Hidden", duration);
        break;
      }
    }

    visibility_ = new_visibility;
    last_visibility_change_time_ = now;
  }

  void WebContentsDestroyed() override {
    RecordVisibilityHistogram(visibility_);

    if (was_replaced_)
      return;

    TabActivityWatcher::GetInstance()->tab_metrics_logger_->LogTabLifetime(
        ukm_source_id_, NowTicks() - navigation_time_);

    if (!backgrounded_time_.is_null()) {
      // TODO(michaelpg): When closing multiple tabs, log the tab metrics as
      // they were before any tabs started to close. Currently, we log each tab
      // one by one as the tabstrip closes, so metrics like MRUIndex are
      // different than what they were when the close event started.
      // See https://crbug.com/817174.
      TabActivityWatcher::GetInstance()
          ->tab_metrics_logger_->LogBackgroundTabClosed(
              ukm_source_id_, NowTicks() - backgrounded_time_,
              GetMRUFeatures());
    }
  }

  // content::RenderWidgetHost::InputEventObserver:
  void OnInputEvent(const blink::WebInputEvent& event) override {
    if (blink::WebInputEvent::IsMouseEventType(event.GetType()))
      tab_metrics_.page_metrics.mouse_event_count++;
    else if (blink::WebInputEvent::IsKeyboardEventType(event.GetType()))
      tab_metrics_.page_metrics.key_event_count++;
    else if (blink::WebInputEvent::IsTouchEventType(event.GetType()))
      tab_metrics_.page_metrics.touch_event_count++;
  }

  // Iterates through tabstrips to determine the index of |contents| in
  // most-recently-used order out of all non-incognito tabs.
  // Linear in the number of tabs (most users have <10 tabs open).
  tab_ranker::MRUFeatures GetMRUFeatures() {
    tab_ranker::MRUFeatures mru_features;
    for (Browser* browser : *BrowserList::GetInstance()) {
      // Ignore incognito browsers.
      if (browser->profile()->IsOffTheRecord())
        continue;

      int count = browser->tab_strip_model()->count();
      mru_features.total += count;

      // Increment the MRU index for each WebContents that was foregrounded more
      // recently than this one.
      for (int i = 0; i < count; i++) {
        auto* other = WebContentsData::FromWebContents(
            browser->tab_strip_model()->GetWebContentsAt(i));
        if (this == other)
          continue;

        // Sort by foregrounded time, then creation time. Both tabs will have a
        // foregrounded time of 0 if they were never foregrounded.
        if (foregrounded_time_ < other->foregrounded_time_ ||
            (foregrounded_time_ == other->foregrounded_time_ &&
             creation_time_ < other->creation_time_)) {
          mru_features.index++;
        }
      }
    }
    return mru_features;
  }

  // Updated when a navigation is finished.
  ukm::SourceId ukm_source_id_ = 0;

  // When the tab was created.
  base::TimeTicks creation_time_;

  // The most recent time the tab became backgrounded. This happens when a
  // different tab in the tabstrip is activated or the tab's window is hidden.
  base::TimeTicks backgrounded_time_;

  // The most recent time the tab became foregrounded. This happens when the
  // tab becomes the active tab in the tabstrip or when the active tab's window
  // is activated.
  base::TimeTicks foregrounded_time_;

  // The last navigation time associated with this tab.
  base::TimeTicks navigation_time_;

  // Stores current stats for the tab.
  TabMetricsLogger::TabMetrics tab_metrics_;

  // Set to true when the WebContents has been detached from its tab.
  bool is_detached_ = false;

  // If true, future events such as the tab being destroyed won't be logged.
  bool was_replaced_ = false;

  // Current tab visibility.
  content::Visibility visibility_ = web_contents()->GetVisibility();

  // The last time at which |visibility_| changed.
  base::TimeTicks last_visibility_change_time_ = NowTicks();

  DISALLOW_COPY_AND_ASSIGN(WebContentsData);
};

TabActivityWatcher::TabActivityWatcher()
    : tab_metrics_logger_(std::make_unique<TabMetricsLogger>()),
      browser_tab_strip_tracker_(this, this, this) {
  browser_tab_strip_tracker_.Init();

  // TabMetrics UKMs reference WindowMetrics UKM entries, so ensure the
  // WindowActivityWatcher is initialized.
  WindowActivityWatcher::GetInstance();
}

TabActivityWatcher::~TabActivityWatcher() = default;

base::Optional<float> TabActivityWatcher::CalculateReactivationScore(
    content::WebContents* web_contents) {
  WebContentsData* web_contents_data =
      WebContentsData::FromWebContents(web_contents);
  if (!web_contents_data)
    return base::nullopt;
  return web_contents_data->CalculateReactivationScore();
}

void TabActivityWatcher::OnBrowserSetLastActive(Browser* browser) {
  if (browser->tab_strip_model()->closing_all())
    return;

  content::WebContents* active_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  if (!active_contents)
    return;

  // Don't assume the WebContentsData already exists in case activation happens
  // before the tabstrip is fully updated.
  WebContentsData* web_contents_data =
      WebContentsData::FromWebContents(active_contents);
  if (web_contents_data)
    web_contents_data->TabWindowActivated();
}

void TabActivityWatcher::TabInsertedAt(TabStripModel* tab_strip_model,
                                       content::WebContents* contents,
                                       int index,
                                       bool foreground) {
  // Ensure the WebContentsData is created to observe this WebContents since it
  // may represent a newly created tab.
  WebContentsData::CreateForWebContents(contents);
  WebContentsData::FromWebContents(contents)->TabInserted(foreground);
}

void TabActivityWatcher::TabDetachedAt(content::WebContents* contents,
                                       int index,
                                       bool was_active) {
  WebContentsData::FromWebContents(contents)->TabDetached();
}

void TabActivityWatcher::TabReplacedAt(TabStripModel* tab_strip_model,
                                       content::WebContents* old_contents,
                                       content::WebContents* new_contents,
                                       int index) {
  WebContentsData* old_web_contents_data =
      WebContentsData::FromWebContents(old_contents);
  old_web_contents_data->WasReplaced();

  // Ensure the WebContentsData is created to observe this WebContents since it
  // likely hasn't been inserted into a tabstrip before.
  WebContentsData::CreateForWebContents(new_contents);

  WebContentsData::FromWebContents(new_contents)
      ->DidReplace(*old_web_contents_data);
}

void TabActivityWatcher::TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                               content::WebContents* contents,
                                               int index) {
  WebContentsData::FromWebContents(contents)->LogTabIfBackgrounded();
}

bool TabActivityWatcher::ShouldTrackBrowser(Browser* browser) {
  // Don't track incognito browsers. This is also enforced by UKM.
  // TODO(michaelpg): Keep counters for incognito browsers so we can score them
  // using the TabScorePredictor. We should be able to do this without logging
  // these values.
  return !browser->profile()->IsOffTheRecord();
}

void TabActivityWatcher::ResetForTesting() {
  tab_metrics_logger_ = std::make_unique<TabMetricsLogger>();
}

// static
TabActivityWatcher* TabActivityWatcher::GetInstance() {
  CR_DEFINE_STATIC_LOCAL(TabActivityWatcher, instance, ());
  return &instance;
}

}  // namespace resource_coordinator
