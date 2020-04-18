// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"

namespace content {

class MidiBrowserTest : public ContentBrowserTest {
 public:
  MidiBrowserTest() {}
  ~MidiBrowserTest() override {}
};

IN_PROC_BROWSER_TEST_F(MidiBrowserTest, RequestMIDIAccess) {
  bool result;
  NavigateToURL(shell(), GURL("about:blank"));
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell(),
      "navigator.requestMIDIAccess()"
      "  .then("
      "    _ => domAutomationController.send(true),"
      "    _ => domAutomationController.send(false));",
      &result));
  // We cannot check result since it relies on the availabity of system
  // level MIDI on the test runner.
}

IN_PROC_BROWSER_TEST_F(MidiBrowserTest, SubscribeAll) {
  bool result;
  NavigateToURL(shell(), GURL("about:blank"));
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell(),
      "navigator.requestMIDIAccess()"
      "  .then("
      "    e => { e.inputs.forEach(i => i.onmidimessage = console.log);"
      "             domAutomationController.send(true) },"
      "    _ => domAutomationController.send(false));",
      &result));
  // We cannot check result since it relies on the availabity of system
  // level MIDI on the test runner.
}

}  // namespace content
