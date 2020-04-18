// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/strings/string_number_conversions.h"
#include "chrome/test/remoting/remote_desktop_browsertest.h"

namespace remoting {

class It2MeBrowserTest : public RemoteDesktopBrowserTest {
 protected:
  std::string GetAccessCode(content::WebContents* contents);

  // Launches a Chromoting app instance for the helper.
  content::WebContents* SetUpHelperInstance();
};

std::string It2MeBrowserTest::GetAccessCode(content::WebContents* contents) {
  RunJavaScriptTest(contents, "GetAccessCode", "{}");
  std::string access_code = RemoteTestHelper::ExecuteScriptAndExtractString(
      contents, "document.getElementById('access-code-display').innerText");
  return access_code;
}

content::WebContents* It2MeBrowserTest::SetUpHelperInstance() {
  content::WebContents* helper_content =
      LaunchChromotingApp(false, WindowOpenDisposition::NEW_FOREGROUND_TAB);
  LoadBrowserTestJavaScript(helper_content);
  LoadScript(helper_content, FILE_PATH_LITERAL("it2me_browser_test.js"));
  return helper_content;
}

IN_PROC_BROWSER_TEST_F(It2MeBrowserTest, MANUAL_Connect) {
  content::WebContents* helpee_content = SetUpTest();
  LoadScript(helpee_content, FILE_PATH_LITERAL("it2me_browser_test.js"));

  content::WebContents* helper_content = SetUpHelperInstance();
  RunJavaScriptTest(helper_content, "ConnectIt2Me", "{"
    "accessCode: '" + GetAccessCode(helpee_content) + "'"
  "}");

  Cleanup();
}

IN_PROC_BROWSER_TEST_F(It2MeBrowserTest, MANUAL_CancelShare) {
  content::WebContents* helpee_content = SetUpTest();
  LoadScript(helpee_content, FILE_PATH_LITERAL("it2me_browser_test.js"));
  std::string access_code = GetAccessCode(helpee_content);
  RunJavaScriptTest(helpee_content, "CancelShare", "{}");

  content::WebContents* helper_content = SetUpHelperInstance();
  RunJavaScriptTest(helper_content, "InvalidAccessCode", "{"
    "accessCode: '" + access_code + "'"
  "}");
  Cleanup();
}

IN_PROC_BROWSER_TEST_F(It2MeBrowserTest, MANUAL_VerifyAccessCodeNonReusable) {
  content::WebContents* helpee_content = SetUpTest();
  LoadScript(helpee_content, FILE_PATH_LITERAL("it2me_browser_test.js"));
  std::string access_code = GetAccessCode(helpee_content);

  content::WebContents* helper_content = SetUpHelperInstance();
  RunJavaScriptTest(helper_content, "ConnectIt2Me", "{"
    "accessCode: '" + access_code + "'"
  "}");

  RunJavaScriptTest(helper_content, "InvalidAccessCode", "{"
    "accessCode: '" + access_code + "'"
  "}");
  Cleanup();
}

IN_PROC_BROWSER_TEST_F(It2MeBrowserTest, MANUAL_InvalidAccessCode) {
  content::WebContents* helpee_content = SetUpTest();
  LoadScript(helpee_content, FILE_PATH_LITERAL("it2me_browser_test.js"));

  // Generate an invalid access code by generating a valid access code and
  // changing its PIN portion.
  std::string access_code = GetAccessCode(helpee_content);

  uint64_t invalid_access_code = 0;
  ASSERT_TRUE(base::StringToUint64(access_code, &invalid_access_code));
  std::ostringstream invalid_access_code_string;

  invalid_access_code_string << ++invalid_access_code;

  content::WebContents* helper_content = SetUpHelperInstance();
  RunJavaScriptTest(helper_content, "InvalidAccessCode", "{"
    "accessCode: '" + invalid_access_code_string.str() + "'"
  "}");

  Cleanup();
}

}  // namespace remoting
