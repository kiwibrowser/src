// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/tab_reactivation_tracker.h"

#include "base/stl_util.h"
#include "content/public/browser/web_contents_observer.h"

namespace metrics {

// This class is used to track the activation/deactivation cycle per
// WebContents.
class TabReactivationTracker::WebContentsHelper
    : public content::WebContentsObserver {
 public:
  WebContentsHelper(TabReactivationTracker* tab_reactivation_tracker,
                    content::WebContents* contents);
  ~WebContentsHelper() override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  void OnTabActivating();
  void OnTabDeactivating();
  void OnTabClosing();

 private:
  // The owning TabReactivationTracker.
  TabReactivationTracker* tab_reactivation_tracker_;

  // Indicates if the tab has been deactivated before as to only count
  // reactivations.
  bool was_deactivated_once_;

  // The deactivation metric is not recorded for closing tabs.
  bool is_closing_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsHelper);
};

TabReactivationTracker::WebContentsHelper::WebContentsHelper(
    TabReactivationTracker* tab_reactivation_tracker,
    content::WebContents* contents)
    : content::WebContentsObserver(contents),
      tab_reactivation_tracker_(tab_reactivation_tracker),
      was_deactivated_once_(false),
      is_closing_(false) {}

TabReactivationTracker::WebContentsHelper::~WebContentsHelper() = default;

void TabReactivationTracker::WebContentsHelper::WebContentsDestroyed() {
  tab_reactivation_tracker_->OnWebContentsDestroyed(web_contents());
}

void TabReactivationTracker::WebContentsHelper::OnTabActivating() {
  if (was_deactivated_once_)
    tab_reactivation_tracker_->NotifyTabReactivating(web_contents());
}

void TabReactivationTracker::WebContentsHelper::OnTabDeactivating() {
  was_deactivated_once_ = true;
  if (!is_closing_)
    tab_reactivation_tracker_->NotifyTabDeactivating(web_contents());
}

void TabReactivationTracker::WebContentsHelper::OnTabClosing() {
  is_closing_ = true;
}

TabReactivationTracker::TabReactivationTracker(Delegate* delegate)
    : delegate_(delegate), browser_tab_strip_tracker_(this, nullptr, nullptr) {
  browser_tab_strip_tracker_.Init();
}

TabReactivationTracker::~TabReactivationTracker() = default;

void TabReactivationTracker::TabInsertedAt(TabStripModel* tab_strip_model,
                                           content::WebContents* contents,
                                           int index,
                                           bool foreground) {}

void TabReactivationTracker::TabClosingAt(TabStripModel* tab_strip_model,
                                          content::WebContents* contents,
                                          int index) {
  GetHelper(contents)->OnTabClosing();
}

void TabReactivationTracker::ActiveTabChanged(
    content::WebContents* old_contents,
    content::WebContents* new_contents,
    int index,
    int reason) {
  if (old_contents)
    GetHelper(old_contents)->OnTabDeactivating();
  GetHelper(new_contents)->OnTabActivating();
}

void TabReactivationTracker::NotifyTabDeactivating(
    content::WebContents* contents) {
  delegate_->OnTabDeactivated(contents);
}

void TabReactivationTracker::NotifyTabReactivating(
    content::WebContents* contents) {
  delegate_->OnTabReactivated(contents);
}

TabReactivationTracker::WebContentsHelper* TabReactivationTracker::GetHelper(
    content::WebContents* contents) {
  // Make sure it exists.
  if (!base::ContainsKey(helper_map_, contents)) {
    helper_map_.insert(std::make_pair(
        contents, std::make_unique<WebContentsHelper>(this, contents)));
  }

  return helper_map_[contents].get();
}

void TabReactivationTracker::OnWebContentsDestroyed(
    content::WebContents* contents) {
  helper_map_.erase(contents);
}

}  // namespace metrics
