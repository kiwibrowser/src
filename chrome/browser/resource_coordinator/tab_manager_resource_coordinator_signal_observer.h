// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_RESOURCE_COORDINATOR_SIGNAL_OBSERVER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_RESOURCE_COORDINATOR_SIGNAL_OBSERVER_H_

#include "base/macros.h"
#include "chrome/browser/resource_coordinator/page_signal_receiver.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"

namespace resource_coordinator {

// TabManager::ResourceCoordinatorSignalObserver implements some observer
// interfaces, for example it currently implements PageSignalObserver but can
// implement more; and receives signals from resource coordinator through those
// interfaces.
class TabManager::ResourceCoordinatorSignalObserver
    : public PageSignalObserver {
 public:
  ResourceCoordinatorSignalObserver();
  ~ResourceCoordinatorSignalObserver() override;

  // PageSignalObserver implementation.
  void OnPageAlmostIdle(content::WebContents* web_contents) override;
  void OnExpectedTaskQueueingDurationSet(content::WebContents* web_contents,
                                         base::TimeDelta duration) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResourceCoordinatorSignalObserver);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_MANAGER_RESOURCE_COORDINATOR_SIGNAL_OBSERVER_H_
