// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/download_protection/check_client_download_request.h"

#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "chrome/common/safe_browsing/file_type_policies_test_util.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

// TODO(nparker): Move some testing from download_protection_service_unittest.cc
// to this file, and add speicific tests here.

class CheckClientDownloadRequestTest : public testing::Test {
 public:
  CheckClientDownloadRequestTest() {}

 protected:
  void SetUp() override {}

  void TearDown() override {}

  void SetMaxArchivedBinariesToReport(uint64_t target_limit) {
    std::unique_ptr<DownloadFileTypeConfig> config =
        policies_.DuplicateConfig();
    config->set_max_archived_binaries_to_report(target_limit);
    policies_.SwapConfig(config);
  }

 protected:
  // This will effectivly mask the global Singleton while this is in scope.
  FileTypePoliciesTestOverlay policies_;
};

TEST_F(CheckClientDownloadRequestTest, CheckLimitArchivedExtensions) {
  CheckClientDownloadRequest::ArchivedBinaries src, dest;

  const int max_to_try = 12;
  for (int i = 0; i < max_to_try; i++) {
    src.Add();
  }

  // First check against the value set in .asciipb, which is currently 10
  // If that is raised above |max_to_try|, raise the latter.
  dest.Clear();
  CheckClientDownloadRequest::CopyArchivedBinaries(src, &dest);
  EXPECT_EQ(10, dest.size());

  SetMaxArchivedBinariesToReport(2);
  dest.Clear();
  CheckClientDownloadRequest::CopyArchivedBinaries(src, &dest);
  EXPECT_EQ(2, dest.size());

  SetMaxArchivedBinariesToReport(100000);
  dest.Clear();
  CheckClientDownloadRequest::CopyArchivedBinaries(src, &dest);
  EXPECT_EQ(max_to_try, dest.size());
}

}  // namespace safe_browsing
