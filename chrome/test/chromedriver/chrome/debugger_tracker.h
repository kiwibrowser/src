// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_DEBUGGER_TRACKER_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_DEBUGGER_TRACKER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/test/chromedriver/chrome/devtools_event_listener.h"

namespace base {
class DictionaryValue;
}

class DevToolsClient;
class Status;

// Tracks the debugger state of the page.
class DebuggerTracker : public DevToolsEventListener {
 public:
  explicit DebuggerTracker(DevToolsClient* client);
  ~DebuggerTracker() override;

  // Overridden from DevToolsEventListener:
  Status OnEvent(DevToolsClient* client,
                 const std::string& method,
                 const base::DictionaryValue& params) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DebuggerTracker);
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_DEBUGGER_TRACKER_H_
