// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_usage_recorder.h"

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/page_importance_signals.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(metrics::TabUsageRecorder::WebContentsData);

namespace metrics {

namespace {

// The recorder is never stopped, thus it is ok to leak.
TabUsageRecorder* g_tab_usage_recorder = nullptr;

}  // namespace

// This class is responsible for recording the metrics. It also keeps track of
// the pinned state of the tab.
class TabUsageRecorder::WebContentsData
    : public content::WebContentsUserData<WebContentsData> {
 public:
  ~WebContentsData() override;

  void RecordTabDeactivation();
  void RecordTabReactivation();

  void OnTabPinnedStateChanging(bool is_pinned);

 private:
  friend class content::WebContentsUserData<WebContentsData>;

  explicit WebContentsData(content::WebContents* contents);

  // The WebContents associated to this instance.
  content::WebContents* contents_;

  // Indicates if the tab is pinned to the tab strip.
  bool is_pinned_;

  base::TimeTicks last_inactive_time_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsData);
};

TabUsageRecorder::WebContentsData::~WebContentsData() = default;

void TabUsageRecorder::WebContentsData::OnTabPinnedStateChanging(
    bool is_pinned) {
  is_pinned_ = is_pinned;
}

TabUsageRecorder::WebContentsData::WebContentsData(
    content::WebContents* contents)
    : contents_(contents), is_pinned_(false) {}

void TabUsageRecorder::WebContentsData::RecordTabDeactivation() {
  last_inactive_time_ = base::TimeTicks::Now();
  UMA_HISTOGRAM_BOOLEAN("Tab.Deactivation.Pinned", is_pinned_);
  UMA_HISTOGRAM_BOOLEAN(
      "Tab.Deactivation.HadFormInteraction",
      contents_->GetPageImportanceSignals().had_form_interaction);
}

void TabUsageRecorder::WebContentsData::RecordTabReactivation() {
  bool had_form_interaction =
      contents_->GetPageImportanceSignals().had_form_interaction;

  UMA_HISTOGRAM_BOOLEAN("Tab.Reactivation.Pinned", is_pinned_);
  UMA_HISTOGRAM_BOOLEAN("Tab.Reactivation.HadFormInteraction",
                        had_form_interaction);

  base::TimeDelta time_to_reactivation =
      base::TimeTicks::Now() - last_inactive_time_;
  if (is_pinned_ || had_form_interaction) {
    UMA_HISTOGRAM_CUSTOM_TIMES(
        "Tab.TimeToReactivation.Important", time_to_reactivation,
        base::TimeDelta::FromSeconds(1), base::TimeDelta::FromHours(2), 100);
  } else {
    UMA_HISTOGRAM_CUSTOM_TIMES(
        "Tab.TimeToReactivation.Normal", time_to_reactivation,
        base::TimeDelta::FromSeconds(1), base::TimeDelta::FromHours(2), 100);
  }
}

// static
void TabUsageRecorder::InitializeIfNeeded() {
  if (!g_tab_usage_recorder)
    g_tab_usage_recorder = new TabUsageRecorder();
}

void TabUsageRecorder::OnTabDeactivated(content::WebContents* contents) {
  GetWebContentsData(contents)->RecordTabDeactivation();
}

void TabUsageRecorder::OnTabReactivated(content::WebContents* contents) {
  GetWebContentsData(contents)->RecordTabReactivation();
}

void TabUsageRecorder::TabInsertedAt(TabStripModel* tab_strip_model,
                                     content::WebContents* contents,
                                     int index,
                                     bool foreground) {
  // Set the initial pin state.
  TabPinnedStateChanged(tab_strip_model, contents, index);
}

void TabUsageRecorder::TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                             content::WebContents* contents,
                                             int index) {
  GetWebContentsData(contents)->OnTabPinnedStateChanging(
      tab_strip_model->IsTabPinned(index));
}

TabUsageRecorder::TabUsageRecorder()
    : tab_reactivation_tracker_(this),
      browser_tab_strip_tracker_(this, nullptr, nullptr) {
  browser_tab_strip_tracker_.Init();
}

TabUsageRecorder::~TabUsageRecorder() = default;

TabUsageRecorder::WebContentsData* TabUsageRecorder::GetWebContentsData(
    content::WebContents* contents) {
  WebContentsData::CreateForWebContents(contents);
  return WebContentsData::FromWebContents(contents);
}

}  // namespace metrics
