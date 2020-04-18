// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/first_web_contents_profiler.h"

#include <string>

#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/startup_metric_utils/browser/startup_metric_utils.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"

namespace {

class FirstWebContentsProfiler : public content::WebContentsObserver {
 public:
  FirstWebContentsProfiler(content::WebContents* web_contents,
                           startup_metric_utils::WebContentsWorkload workload);

 private:
  // Reasons for which profiling is deemed complete. Logged in UMA (do not re-
  // order or re-assign).
  enum FinishReason {
    // All metrics were successfully gathered.
    DONE = 0,
    // Abandon if blocking UI was shown during startup.
    ABANDON_BLOCKING_UI = 1,
    // Abandon if the content is hidden (lowers scheduling priority).
    ABANDON_CONTENT_HIDDEN = 2,
    // Abandon if the content is destroyed.
    ABANDON_CONTENT_DESTROYED = 3,
    // Abandon if the WebContents navigates away from its initial page.
    ABANDON_NEW_NAVIGATION = 4,
    // Abandon if the WebContents fails to load (e.g. network error, etc.).
    ABANDON_NAVIGATION_ERROR = 5,
    ENUM_MAX
  };

  ~FirstWebContentsProfiler() override = default;

  // content::WebContentsObserver:
  void DidFirstVisuallyNonEmptyPaint() override;
  void DocumentOnLoadCompletedInMainFrame() override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;

  // Whether this instance has finished collecting first-paint and main-frame-
  // load metrics (navigation metrics are recorded on a best effort but don't
  // prevent the FirstWebContentsProfiler from calling it).
  bool IsFinishedCollectingMetrics();

  // Logs |finish_reason| to UMA and deletes this FirstWebContentsProfiler.
  void FinishedCollectingMetrics(FinishReason finish_reason);

  // Whether an attempt was made to collect the "NonEmptyPaint" metric.
  bool collected_paint_metric_;

  // Whether an attempt was made to collect the "MainFrameLoad" metric.
  bool collected_load_metric_;

  // Whether an attempt was made to collect the "MainNavigationStart" metric.
  bool collected_main_navigation_start_metric_;

  // Whether an attempt was made to collect the "MainNavigationFinished" metric.
  bool collected_main_navigation_finished_metric_;

  const startup_metric_utils::WebContentsWorkload workload_;

  DISALLOW_COPY_AND_ASSIGN(FirstWebContentsProfiler);
};

FirstWebContentsProfiler::FirstWebContentsProfiler(
    content::WebContents* web_contents,
    startup_metric_utils::WebContentsWorkload workload)
    : content::WebContentsObserver(web_contents),
      collected_paint_metric_(false),
      collected_load_metric_(false),
      collected_main_navigation_start_metric_(false),
      collected_main_navigation_finished_metric_(false),
      workload_(workload) {}

void FirstWebContentsProfiler::DidFirstVisuallyNonEmptyPaint() {
  if (collected_paint_metric_)
    return;
  if (startup_metric_utils::WasMainWindowStartupInterrupted()) {
    FinishedCollectingMetrics(FinishReason::ABANDON_BLOCKING_UI);
    return;
  }

  collected_paint_metric_ = true;
  startup_metric_utils::RecordFirstWebContentsNonEmptyPaint(
      base::TimeTicks::Now(), web_contents()
                                  ->GetMainFrame()
                                  ->GetProcess()
                                  ->GetInitTimeForNavigationMetrics());

  if (IsFinishedCollectingMetrics())
    FinishedCollectingMetrics(FinishReason::DONE);
}

void FirstWebContentsProfiler::DocumentOnLoadCompletedInMainFrame() {
  if (collected_load_metric_)
    return;
  if (startup_metric_utils::WasMainWindowStartupInterrupted()) {
    FinishedCollectingMetrics(FinishReason::ABANDON_BLOCKING_UI);
    return;
  }

  collected_load_metric_ = true;
  startup_metric_utils::RecordFirstWebContentsMainFrameLoad(
      base::TimeTicks::Now());

  if (IsFinishedCollectingMetrics())
    FinishedCollectingMetrics(FinishReason::DONE);
}

void FirstWebContentsProfiler::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (collected_main_navigation_start_metric_)
    return;
  if (startup_metric_utils::WasMainWindowStartupInterrupted()) {
    FinishedCollectingMetrics(FinishReason::ABANDON_BLOCKING_UI);
    return;
  }
}

void FirstWebContentsProfiler::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (collected_main_navigation_finished_metric_) {
    // Abandon profiling on a top-level navigation to a different page as it:
    //   (1) is no longer a fair timing; and
    //   (2) can cause http://crbug.com/525209 where one of the timing
    //       heuristics (e.g. first paint) didn't fire for the initial content
    //       but fires after a lot of idle time when the user finally navigates
    //       to another page that does trigger it.
    if (navigation_handle->IsInMainFrame() &&
        navigation_handle->HasCommitted() &&
        !navigation_handle->IsSameDocument()) {
      FinishedCollectingMetrics(FinishReason::ABANDON_NEW_NAVIGATION);
    }
    return;
  }

  if (startup_metric_utils::WasMainWindowStartupInterrupted()) {
    FinishedCollectingMetrics(FinishReason::ABANDON_BLOCKING_UI);
    return;
  }

  // The first navigation has to be the main frame's.
  DCHECK(navigation_handle->IsInMainFrame());

  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsErrorPage()) {
    FinishedCollectingMetrics(FinishReason::ABANDON_NAVIGATION_ERROR);
    return;
  }

  startup_metric_utils::RecordFirstWebContentsMainNavigationStart(
      navigation_handle->NavigationStart(), workload_);
  collected_main_navigation_start_metric_ = true;

  collected_main_navigation_finished_metric_ = true;
  startup_metric_utils::RecordFirstWebContentsMainNavigationFinished(
      base::TimeTicks::Now());
}

void FirstWebContentsProfiler::OnVisibilityChanged(
    content::Visibility visibility) {
  if (visibility != content::Visibility::VISIBLE) {
    // Stop profiling if the content gets hidden as its load may be
    // deprioritized and timing it becomes meaningless.
    FinishedCollectingMetrics(FinishReason::ABANDON_CONTENT_HIDDEN);
  }
}

void FirstWebContentsProfiler::WebContentsDestroyed() {
  FinishedCollectingMetrics(FinishReason::ABANDON_CONTENT_DESTROYED);
}

bool FirstWebContentsProfiler::IsFinishedCollectingMetrics() {
  return collected_paint_metric_ && collected_load_metric_;
}

void FirstWebContentsProfiler::FinishedCollectingMetrics(
    FinishReason finish_reason) {
  UMA_HISTOGRAM_ENUMERATION("Startup.FirstWebContents.FinishReason",
                            finish_reason, FinishReason::ENUM_MAX);
  if (!collected_paint_metric_) {
    UMA_HISTOGRAM_ENUMERATION("Startup.FirstWebContents.FinishReason_NoPaint",
                              finish_reason, FinishReason::ENUM_MAX);
  }
  if (!collected_load_metric_) {
    UMA_HISTOGRAM_ENUMERATION("Startup.FirstWebContents.FinishReason_NoLoad",
                              finish_reason, FinishReason::ENUM_MAX);
  }

  delete this;
}

}  // namespace

namespace metrics {

void BeginFirstWebContentsProfiling() {
  using startup_metric_utils::WebContentsWorkload;

  const BrowserList* browser_list = BrowserList::GetInstance();

  const auto first_browser = browser_list->begin();
  if (first_browser == browser_list->end())
    return;

  const TabStripModel* tab_strip = (*first_browser)->tab_strip_model();
  DCHECK(!tab_strip->empty());

  content::WebContents* web_contents = tab_strip->GetActiveWebContents();
  DCHECK(web_contents);

  const bool single_tab = browser_list->size() == 1 && tab_strip->count() == 1;

  // FirstWebContentsProfiler owns itself and is also bound to
  // |web_contents|'s lifetime by observing WebContentsDestroyed().
  new FirstWebContentsProfiler(web_contents,
                               single_tab ? WebContentsWorkload::SINGLE_TAB
                                          : WebContentsWorkload::MULTI_TABS);
}

}  // namespace metrics
