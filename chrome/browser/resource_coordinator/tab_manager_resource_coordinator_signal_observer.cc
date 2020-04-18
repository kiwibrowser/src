// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_resource_coordinator_signal_observer.h"

#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/resource_coordinator/tab_load_tracker.h"
#include "chrome/browser/resource_coordinator/tab_manager_stats_collector.h"
#include "chrome/browser/resource_coordinator/tab_manager_web_contents_data.h"

namespace resource_coordinator {

// A helper class for accessing TabLoadTracker. TabLoadTracker can't directly
// friend TabManager::ResourceCoordinatorSignalObserver as it's a nested class
// and can't be forward declared.
class TabManagerResourceCoordinatorSignalObserverHelper {
 public:
  static void OnPageAlmostIdle(content::WebContents* web_contents) {
    TabLoadTracker::Get()->OnPageAlmostIdle(web_contents);
  }
};

TabManager::ResourceCoordinatorSignalObserver::
    ResourceCoordinatorSignalObserver() {
  if (auto* page_signal_receiver = PageSignalReceiver::GetInstance())
    page_signal_receiver->AddObserver(this);
}

TabManager::ResourceCoordinatorSignalObserver::
    ~ResourceCoordinatorSignalObserver() {
  if (auto* page_signal_receiver = PageSignalReceiver::GetInstance())
    page_signal_receiver->RemoveObserver(this);
}

void TabManager::ResourceCoordinatorSignalObserver::OnPageAlmostIdle(
    content::WebContents* web_contents) {
  TabManagerResourceCoordinatorSignalObserverHelper::OnPageAlmostIdle(
      web_contents);
}

void TabManager::ResourceCoordinatorSignalObserver::
    OnExpectedTaskQueueingDurationSet(content::WebContents* web_contents,
                                      base::TimeDelta duration) {
  g_browser_process->GetTabManager()
      ->stats_collector()
      ->RecordExpectedTaskQueueingDuration(web_contents, duration);
}

}  // namespace resource_coordinator
