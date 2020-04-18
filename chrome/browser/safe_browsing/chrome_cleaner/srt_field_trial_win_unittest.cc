// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/srt_field_trial_win.h"

#include <map>
#include <string>

#include "base/win/windows_version.h"
#include "components/variations/variations_params_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

class SRTDownloadURLTest : public ::testing::Test {
 protected:
  void CreatePromptTrial(const std::string& experiment_name) {
    // Assigned trials will go out of scope when variations_ goes out of scope.
    constexpr char kTrialName[] = "SRTPromptFieldTrial";
    base::FieldTrialList::CreateFieldTrial(kTrialName, experiment_name);
  }

  void CreateDownloadFeature(const std::string& download_group_name) {
    constexpr char kFeatureName[] = "ChromeCleanupDistribution";
    std::map<std::string, std::string> params;
    params["cleaner_download_group"] = download_group_name;
    variations_.SetVariationParamsWithFeatureAssociations(
        "A trial name", params, {kFeatureName});
  }

 private:
  variations::testing::VariationParamsManager variations_;
};

TEST_F(SRTDownloadURLTest, Stable) {
  CreatePromptTrial("On");
  EXPECT_EQ("/dl/softwareremovaltool/win/chrome_cleanup_tool.exe",
            GetSRTDownloadURL().path());
}

TEST_F(SRTDownloadURLTest, Experiment) {
  CreateDownloadFeature("experiment");
  std::string expected_path;
  if (base::win::OSInfo::GetInstance()->architecture() ==
      base::win::OSInfo::X86_ARCHITECTURE) {
    expected_path =
        "/dl/softwareremovaltool/win/x86/experiment/chrome_cleanup_tool.exe";
  } else {
    expected_path =
        "/dl/softwareremovaltool/win/x64/experiment/chrome_cleanup_tool.exe";
  }
  EXPECT_EQ(expected_path, GetSRTDownloadURL().path());
}

TEST_F(SRTDownloadURLTest, DefaultsToStable) {
  EXPECT_EQ("/dl/softwareremovaltool/win/chrome_cleanup_tool.exe",
            GetSRTDownloadURL().path());
}

}  // namespace safe_browsing
