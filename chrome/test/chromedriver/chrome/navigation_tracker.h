// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_NAVIGATION_TRACKER_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_NAVIGATION_TRACKER_H_

#include <memory>
#include <set>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/test/chromedriver/chrome/devtools_event_listener.h"
#include "chrome/test/chromedriver/chrome/page_load_strategy.h"
#include "chrome/test/chromedriver/chrome/status.h"

namespace base {
class DictionaryValue;
}

struct BrowserInfo;
class DevToolsClient;
class JavaScriptDialogManager;
class Status;
class Timeout;

// Tracks the navigation state of the page.
class NavigationTracker : public DevToolsEventListener,
                          public PageLoadStrategy {
 public:
  NavigationTracker(DevToolsClient* client,
                    const BrowserInfo* browser_info,
                    const JavaScriptDialogManager* dialog_manager);

  NavigationTracker(DevToolsClient* client,
                    LoadingState known_state,
                    const BrowserInfo* browser_info,
                    const JavaScriptDialogManager* dialog_manager);

  ~NavigationTracker() override;

  // Overriden from PageLoadStrategy:
  // Gets whether a navigation is pending for the specified frame. |frame_id|
  // may be empty to signify the main frame.
  Status IsPendingNavigation(const std::string& frame_id,
                             const Timeout* timeout,
                             bool* is_pending) override;
  void set_timed_out(bool timed_out) override;
  bool IsNonBlocking() const override;

  Status CheckFunctionExists(const Timeout* timeout, bool* exists);

  // Overridden from DevToolsEventListener:
  Status OnConnected(DevToolsClient* client) override;
  Status OnEvent(DevToolsClient* client,
                 const std::string& method,
                 const base::DictionaryValue& params) override;
  Status OnCommandSuccess(DevToolsClient* client,
                          const std::string& method,
                          const base::DictionaryValue& result,
                          const Timeout& command_timeout) override;

 private:
  DevToolsClient* client_;
  LoadingState loading_state_;
  const BrowserInfo* browser_info_;
  const JavaScriptDialogManager* dialog_manager_;
  std::set<std::string> pending_frame_set_;
  std::set<std::string> scheduled_frame_set_;
  std::set<int> execution_context_set_;
  std::string dummy_frame_id_;
  int dummy_execution_context_id_;
  bool load_event_fired_;
  bool timed_out_;

  void ResetLoadingState(LoadingState loading_state);

  DISALLOW_COPY_AND_ASSIGN(NavigationTracker);
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_NAVIGATION_TRACKER_H_
