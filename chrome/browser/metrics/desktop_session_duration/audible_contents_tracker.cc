// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/desktop_session_duration/audible_contents_tracker.h"

#include "chrome/browser/metrics/desktop_session_duration/desktop_session_duration_tracker.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace metrics {

AudibleContentsTracker::AudibleContentsTracker(Observer* observer)
    : observer_(observer) {
  BrowserList* browser_list = BrowserList::GetInstance();
  for (Browser* browser : *browser_list)
    browser->tab_strip_model()->AddObserver(this);
  browser_list->AddObserver(this);
}

AudibleContentsTracker::~AudibleContentsTracker() {
  BrowserList::GetInstance()->RemoveObserver(this);
}

void AudibleContentsTracker::OnBrowserAdded(Browser* browser) {
  browser->tab_strip_model()->AddObserver(this);
}

void AudibleContentsTracker::OnBrowserRemoved(Browser* browser) {
  browser->tab_strip_model()->RemoveObserver(this);
}

void AudibleContentsTracker::TabClosingAt(TabStripModel* model,
                                          content::WebContents* web_contents,
                                          int index) {
  RemoveAudibleWebContents(web_contents);
}

void AudibleContentsTracker::TabChangedAt(content::WebContents* web_contents,
                                          int index,
                                          TabChangeType change_type) {
  // Ignore 'loading' and 'title' changes.
  if (change_type != TabChangeType::kAll)
    return;

  if (web_contents->WasRecentlyAudible())
    AddAudibleWebContents(web_contents);
  else
    RemoveAudibleWebContents(web_contents);
}

void AudibleContentsTracker::TabReplacedAt(
    TabStripModel* model,
    content::WebContents* old_web_contents,
    content::WebContents* new_web_contents,
    int index) {
  RemoveAudibleWebContents(old_web_contents);
  if (new_web_contents->WasRecentlyAudible())
    AddAudibleWebContents(new_web_contents);
}

void AudibleContentsTracker::AddAudibleWebContents(
    content::WebContents* web_contents) {
  // The first web contents to become audible indicates that audio has started.
  bool added = audible_contents_.insert(web_contents).second;
  if (added && audible_contents_.size() == 1)
    observer_->OnAudioStart();
}

void AudibleContentsTracker::RemoveAudibleWebContents(
    content::WebContents* web_contents) {
  // If the web content was previously audible and there are no other audible
  // web contents then notify that audio ended.
  bool removed = (audible_contents_.erase(web_contents) == 1);
  if (removed && audible_contents_.empty())
    observer_->OnAudioEnd();
}

}  // namespace metrics
