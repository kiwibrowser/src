// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILE_MANAGER_FILE_MANAGER_JSTEST_BASE_H_
#define CHROME_BROWSER_CHROMEOS_FILE_MANAGER_FILE_MANAGER_JSTEST_BASE_H_

#include "base/files/file_path.h"
#include "chrome/test/base/in_process_browser_test.h"

class FileManagerJsTestBase : public InProcessBrowserTest {
 protected:
  explicit FileManagerJsTestBase(const base::FilePath& base_path);

  // Runs all test functions in |file|, waiting for them to complete.
  void RunTest(const base::FilePath& file);

 private:
  base::FilePath base_path_;
};

#endif  // CHROME_BROWSER_CHROMEOS_FILE_MANAGER_FILE_MANAGER_JSTEST_BASE_H_
