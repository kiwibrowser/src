// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_manager/file_manager_jstest_base.h"

#include <vector>
#include "base/path_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "net/base/filename_util.h"

FileManagerJsTestBase::FileManagerJsTestBase(const base::FilePath& base_path)
    : base_path_(base_path) {}

void FileManagerJsTestBase::RunTest(const base::FilePath& file) {
  base::FilePath root_path;
  ASSERT_TRUE(base::PathService::Get(base::DIR_SOURCE_ROOT, &root_path));

  const GURL url = net::FilePathToFileURL(
      root_path.Append(base_path_)
      .Append(file));
  ui_test_utils::NavigateToURL(browser(), url);

  content::WebContents* const web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  const std::vector<int> empty_libraries;
  EXPECT_TRUE(ExecuteWebUIResourceTest(web_contents, empty_libraries));
}
