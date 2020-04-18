// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/threading/thread_restrictions.h"

namespace {

class DeclarativeNetRequestAPItest : public extensions::ExtensionApiTest {
 public:
  DeclarativeNetRequestAPItest() {}

 protected:
  // ExtensionApiTest override.
  void SetUpOnMainThread() override {
    extensions::ExtensionApiTest::SetUpOnMainThread();

    base::FilePath test_data_dir =
        test_data_dir_.AppendASCII("declarative_net_request");

    // Copy the |test_data_dir| to a temporary location. We do this to ensure
    // that the temporary kMetadata folder created as a result of loading the
    // extension is not written to the src directory and is automatically
    // removed.
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::CopyDirectory(test_data_dir, temp_dir_.GetPath(), true /*recursive*/);

    // Override the path used for loading the extension.
    test_data_dir_ = temp_dir_.GetPath().AppendASCII("declarative_net_request");
  }

 private:
  base::ScopedTempDir temp_dir_;

  DISALLOW_COPY_AND_ASSIGN(DeclarativeNetRequestAPItest);
};

IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestAPItest, PageWhitelistingAPI) {
  ASSERT_TRUE(RunExtensionTest("page_whitelisting_api")) << message_;
}

IN_PROC_BROWSER_TEST_F(DeclarativeNetRequestAPItest, ExtensionWithNoRuleset) {
  ASSERT_TRUE(RunExtensionTest("extension_with_no_ruleset")) << message_;
}

}  // namespace
