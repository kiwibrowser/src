// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/metrics/metrics_pref_names.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

class ChromeMetricsServiceAccessorTest : public testing::Test {
 public:
  ChromeMetricsServiceAccessorTest()
      : testing_local_state_(TestingBrowserProcess::GetGlobal()) {
  }

  PrefService* GetLocalState() {
    return testing_local_state_.Get();
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  ScopedTestingLocalState testing_local_state_;

  DISALLOW_COPY_AND_ASSIGN(ChromeMetricsServiceAccessorTest);
};

TEST_F(ChromeMetricsServiceAccessorTest, MetricsReportingEnabled) {
#if defined(GOOGLE_CHROME_BUILD)
  const char* pref = metrics::prefs::kMetricsReportingEnabled;
  GetLocalState()->SetDefaultPrefValue(pref, base::Value(false));

  GetLocalState()->SetBoolean(pref, false);
  EXPECT_FALSE(
      ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled());
  GetLocalState()->SetBoolean(pref, true);
  EXPECT_TRUE(
      ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled());
  GetLocalState()->ClearPref(pref);
  EXPECT_FALSE(
      ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled());

  // If field trials are forced, metrics should always be disabled, regardless
  // of the value of the pref.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kForceFieldTrials);
  EXPECT_FALSE(
      ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled());
  GetLocalState()->SetBoolean(pref, true);
  EXPECT_FALSE(
      ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled());
#else
  // Metrics Reporting is never enabled when GOOGLE_CHROME_BUILD is undefined.
  EXPECT_FALSE(
      ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled());
#endif
}
