// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tracing/background_tracing_field_trial.h"

#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/test/scoped_task_environment.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/tracing/common/trace_startup.h"
#include "components/tracing/common/tracing_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

class BackgroundTracingTest : public testing::Test,
                              public testing::WithParamInterface<bool> {};

namespace {

const char kTestConfig[] = "test";
bool g_test_config_loaded = false;

void CheckConfig(std::string* config) {
  if (*config == kTestConfig)
    g_test_config_loaded = true;
}

}  // namespace

TEST_P(BackgroundTracingTest, SetupBackgroundTracingFieldTrial) {
  const bool enable_trace_startup = GetParam();
  if (enable_trace_startup) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kTraceStartup);
    // Normally is runned from ContentMainRunnerImpl::Initialize().
    tracing::EnableStartupTracingIfNeeded();
  }

  base::FieldTrialList field_trial_list(nullptr);
  const std::string kTrialName = "BackgroundTracing";
  const std::string kExperimentName = "SlowStart";
  base::AssociateFieldTrialParams(kTrialName, kExperimentName,
                                  {{"config", kTestConfig}});
  base::FieldTrialList::CreateFieldTrial(kTrialName, kExperimentName);

  base::test::ScopedTaskEnvironment task_environment;

  TestingProfileManager testing_profile_manager(
      TestingBrowserProcess::GetGlobal());
  ASSERT_TRUE(testing_profile_manager.SetUp());

  // In case it is already set at previous test run.
  g_test_config_loaded = false;

  tracing::SetConfigTextFilterForTesting(&CheckConfig);

  tracing::SetupBackgroundTracingFieldTrial();
  EXPECT_NE(enable_trace_startup, g_test_config_loaded);
}

INSTANTIATE_TEST_CASE_P(
    /* prefix intentionally left blank due to only one parameterization */,
    BackgroundTracingTest,
    testing::Bool());
