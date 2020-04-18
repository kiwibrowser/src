// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_REMOTING_REMOTE_TEST_HELPER_H_
#define CHROME_TEST_REMOTING_REMOTE_TEST_HELPER_H_

#include "base/debug/stack_trace.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// ASSERT_TRUE can only be used in void returning functions. This version
// should be used in non-void-returning functions.
inline void _ASSERT_TRUE(bool condition) {
  if (!condition) {
    // ASSERT_TRUE only prints the first call frame in the error message.
    // In our case, this is the _ASSERT_TRUE wrapper function, which is not
    // useful.  To help with debugging, we will dump the full callstack.
    LOG(ERROR) << "Assertion failed.";
    LOG(ERROR) << base::debug::StackTrace().ToString();
  }
  ASSERT_TRUE(condition);
  return;
}

}  // namespace

namespace remoting {

// Mirrored in remoting/tools/remote_test_helper/host.js
enum class Action : int {
  Error = 0,
  None = 1,
  Keydown = 2,
  Buttonpress = 3,
  Mousemove = 4,
  Mousewheel = 5,
  Drag = 6,
};

struct Event {
  Event();

  Action action;
  int value;
  int modifiers;
};


class RemoteTestHelper {
 public:
  explicit RemoteTestHelper(content::WebContents* web_content);

  // Helper to execute a JavaScript code snippet and extract the boolean result.
  static bool ExecuteScriptAndExtractBool(content::WebContents* web_contents,
                                          const std::string& script);

  // Helper to execute a JavaScript code snippet and extract the int result.
  static int ExecuteScriptAndExtractInt(content::WebContents* web_contents,
                                        const std::string& script);

  // Helper to execute a JavaScript code snippet and extract the string result.
  static std::string ExecuteScriptAndExtractString(
      content::WebContents* web_contents, const std::string& script);

  // Helper method to set the clear the last event
  void ClearLastEvent();

  // Helper method to get the last event
  void GetLastEvent(Event* event);

  // Execute an RPC call
  void ExecuteRpc(const std::string& method) {
    ExecuteRpc(method,
               base::TimeDelta::FromSeconds(2),
               base::TimeDelta::FromMilliseconds(500));
  }
  void ExecuteRpc(const std::string& method,
                  base::TimeDelta timeout,
                  base::TimeDelta interval);

 private:
  content::WebContents* web_content_;

  // Check for a valid last event
  bool IsValidEvent();
  DISALLOW_COPY_AND_ASSIGN(RemoteTestHelper);
};

}  // namespace remoting

#endif  // CHROME_TEST_REMOTING_REMOTE_TEST_HELPER_H_
