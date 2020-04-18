// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "chrome/test/remoting/remote_desktop_browsertest.h"
#include "chrome/test/remoting/waiter.h"

namespace remoting {

class ScrollbarBrowserTest : public RemoteDesktopBrowserTest {
};

IN_PROC_BROWSER_TEST_F(ScrollbarBrowserTest, MANUAL_Scrollbar_Visibility) {
  content::WebContents* content = SetUpTest();
  LoadScript(content, FILE_PATH_LITERAL("scrollbar_browser_test.js"));

  RunJavaScriptTest(content, "Scrollbars", "{}");

  Cleanup();
}

}  // namespace remoting
