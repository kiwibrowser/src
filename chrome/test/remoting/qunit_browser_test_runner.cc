// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/remoting/qunit_browser_test_runner.h"

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/filename_util.h"

void QUnitBrowserTestRunner::QUnitStart(content::WebContents* web_contents) {
  std::string result;

  // The test suite must include /third_party/qunit/src/browser_test_harness.js
  // which exposes the entry point browserTestHarness.run().
  ASSERT_TRUE(content::ExecuteScriptAndExtractString(
      web_contents, "browserTestHarness.run();", &result));

  // Read in the JSON.
  std::unique_ptr<base::Value> value =
      base::JSONReader::Read(result, base::JSON_ALLOW_TRAILING_COMMAS);

  // Convert to dictionary.
  base::DictionaryValue* dict_value = NULL;
  ASSERT_TRUE(value->GetAsDictionary(&dict_value));

  // Extract the fields.
  bool passed;
  ASSERT_TRUE(dict_value->GetBoolean("passed", &passed));
  std::string error_message;
  ASSERT_TRUE(dict_value->GetString("errorMessage", &error_message));

  EXPECT_TRUE(passed) << error_message;
}

void QUnitBrowserTestRunner::RunTest(const base::FilePath& file) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(PathExists(file)) << "Error: The QUnit test suite <"
                                  << file.value() << "> does not exist.";
  }
  ui_test_utils::NavigateToURL(browser(), net::FilePathToFileURL(file));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  std::string result;
  QUnitStart(web_contents);
}
