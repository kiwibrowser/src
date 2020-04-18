// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/antivirus_metrics_provider_win.h"

#include <vector>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_checker.h"
#include "base/threading/thread_restrictions.h"
#include "base/version.h"
#include "base/win/windows_version.h"
#include "components/variations/hashing.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

struct Testcase {
  const char* input;
  const char* output;
};

void VerifySystemProfileData(const metrics::SystemProfileProto& system_profile,
                             bool expect_unhashed_value) {
  const char kWindowsDefender[] = "Windows Defender";

  if (base::win::GetVersion() >= base::win::VERSION_WIN8) {
    bool defender_found = false;
    for (const auto& av : system_profile.antivirus_product()) {
      if (av.product_name_hash() == variations::HashName(kWindowsDefender)) {
        defender_found = true;
        if (expect_unhashed_value) {
          EXPECT_TRUE(av.has_product_name());
          EXPECT_EQ(kWindowsDefender, av.product_name());
        } else {
          EXPECT_FALSE(av.has_product_name());
        }
        break;
      }
    }
    EXPECT_TRUE(defender_found);
  }
}

}  // namespace

class AntiVirusMetricsProviderSimpleTest : public ::testing::Test {};

class AntiVirusMetricsProviderTest : public ::testing::TestWithParam<bool> {
 public:
  AntiVirusMetricsProviderTest()
      : got_results_(false),
        expect_unhashed_value_(GetParam()),
        provider_(std::make_unique<AntiVirusMetricsProvider>()),
        weak_ptr_factory_(this) {}

  void GetMetricsCallback() {
    // Check that the callback runs on the main loop.
    ASSERT_TRUE(thread_checker_.CalledOnValidThread());

    got_results_ = true;

    metrics::SystemProfileProto system_profile;
    provider_->ProvideSystemProfileMetrics(&system_profile);

    VerifySystemProfileData(system_profile, expect_unhashed_value_);
    // This looks weird, but it's to make sure that reading the data out of the
    // AntiVirusMetricsProvider does not invalidate it, as the class should be
    // resilient to this.
    system_profile.Clear();
    provider_->ProvideSystemProfileMetrics(&system_profile);
    VerifySystemProfileData(system_profile, expect_unhashed_value_);
  }

  // Helper function to toggle whether the ReportFullAVProductDetails feature is
  // enabled or not.
  void SetFullNamesFeatureEnabled(bool enabled) {
    if (enabled) {
      scoped_feature_list_.InitAndEnableFeature(
          AntiVirusMetricsProvider::kReportNamesFeature);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          AntiVirusMetricsProvider::kReportNamesFeature);
    }
  }

  bool got_results_;
  bool expect_unhashed_value_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<AntiVirusMetricsProvider> provider_;
  base::test::ScopedFeatureList scoped_feature_list_;
  base::ThreadChecker thread_checker_;
  base::WeakPtrFactory<AntiVirusMetricsProviderTest> weak_ptr_factory_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AntiVirusMetricsProviderTest);
};

// TODO(crbug.com/682286): Flaky on windows 10.
TEST_P(AntiVirusMetricsProviderTest, DISABLED_GetMetricsFullName) {
  ASSERT_TRUE(thread_checker_.CalledOnValidThread());
  base::HistogramTester histograms;
  SetFullNamesFeatureEnabled(expect_unhashed_value_);
  // Make sure the I/O is happening on a valid thread by disallowing it on the
  // main thread.
  bool previous_value = base::ThreadRestrictions::SetIOAllowed(false);
  provider_->AsyncInit(
      base::Bind(&AntiVirusMetricsProviderTest::GetMetricsCallback,
                 weak_ptr_factory_.GetWeakPtr()));
  scoped_task_environment_.RunUntilIdle();
  EXPECT_TRUE(got_results_);
  base::ThreadRestrictions::SetIOAllowed(previous_value);

  AntiVirusMetricsProvider::ResultCode expected_result =
      AntiVirusMetricsProvider::RESULT_SUCCESS;
  if (base::win::OSInfo::GetInstance()->version_type() ==
      base::win::SUITE_SERVER)
    expected_result = AntiVirusMetricsProvider::RESULT_WSC_NOT_AVAILABLE;
  histograms.ExpectUniqueSample("UMA.AntiVirusMetricsProvider.Result",
                                expected_result, 1);
}

INSTANTIATE_TEST_CASE_P(, AntiVirusMetricsProviderTest, ::testing::Bool());

TEST_F(AntiVirusMetricsProviderSimpleTest, StripProductVersion) {
  Testcase testcases[] = {
      {"", ""},
      {" ", ""},
      {"1.0 AV 2.0", "1.0 AV"},
      {"Anti  Virus", "Anti Virus"},
      {"Windows Defender", "Windows Defender"},
      {"McAfee AntiVirus has a space at the end ",
       "McAfee AntiVirus has a space at the end"},
      {"ESET NOD32 Antivirus 8.0", "ESET NOD32 Antivirus"},
      {"Norton 360", "Norton 360"},
      {"ESET Smart Security 9.0.381.1", "ESET Smart Security"},
      {"Trustwave AV 3_0_2547", "Trustwave AV"},
      {"nProtect Anti-Virus/Spyware V4.0", "nProtect Anti-Virus/Spyware"},
      {"ESET NOD32 Antivirus 9.0.349.15P", "ESET NOD32 Antivirus"}};

  for (const auto testcase : testcases) {
    auto output =
        AntiVirusMetricsProvider::TrimVersionOfAvProductName(testcase.input);
    EXPECT_STREQ(testcase.output, output.c_str());
  }
}
