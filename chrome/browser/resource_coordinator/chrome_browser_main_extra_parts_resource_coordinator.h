// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_CHROME_BROWSER_MAIN_EXTRA_PARTS_RESOURCE_COORDINATOR_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_CHROME_BROWSER_MAIN_EXTRA_PARTS_RESOURCE_COORDINATOR_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"
#include "chrome/browser/resource_coordinator/browser_child_process_watcher.h"

namespace resource_coordinator {

class ProcessResourceCoordinator;

}  // namespace resource_coordinator

class ChromeBrowserMainExtraPartsResourceCoordinator
    : public ChromeBrowserMainExtraParts {
 public:
  ChromeBrowserMainExtraPartsResourceCoordinator();
  ~ChromeBrowserMainExtraPartsResourceCoordinator() override;

 private:
  // ChromeBrowserMainExtraParts overrides.
  void ServiceManagerConnectionStarted(
      content::ServiceManagerConnection* connection) override;

  std::unique_ptr<resource_coordinator::ProcessResourceCoordinator>
      process_resource_coordinator_;

  std::unique_ptr<resource_coordinator::BrowserChildProcessWatcher>
      browser_child_process_watcher_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainExtraPartsResourceCoordinator);
};

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_CHROME_BROWSER_MAIN_EXTRA_PARTS_RESOURCE_COORDINATOR_H_
