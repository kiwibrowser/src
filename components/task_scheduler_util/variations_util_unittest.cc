// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/task_scheduler_util/variations_util.h"

#include <map>
#include <string>

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/task_scheduler/scheduler_worker_params.h"
#include "base/task_scheduler/scheduler_worker_pool_params.h"
#include "components/variations/variations_params_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace task_scheduler_util {

namespace {

class TaskSchedulerUtilVariationsUtilTest : public testing::Test {
 protected:
  TaskSchedulerUtilVariationsUtilTest() = default;

  void SetVariationParams(
      const std::map<std::string, std::string>& variation_params) {
    variation_params_manager_.SetVariationParams("BrowserScheduler",
                                                 variation_params);
  }

 private:
  variations::testing::VariationParamsManager variation_params_manager_;

  DISALLOW_COPY_AND_ASSIGN(TaskSchedulerUtilVariationsUtilTest);
};

}  // namespace

TEST_F(TaskSchedulerUtilVariationsUtilTest, OrderingParams5) {
  std::map<std::string, std::string> variation_params;
  variation_params["RendererBackground"] = "1;1;1;0;42";
  variation_params["RendererBackgroundBlocking"] = "2;2;1;0;52";
  variation_params["RendererForeground"] = "4;4;1;0;62";
  variation_params["RendererForegroundBlocking"] = "8;8;1;0;72";
  SetVariationParams(variation_params);

  auto init_params = GetTaskSchedulerInitParams("Renderer");
  ASSERT_TRUE(init_params);

  EXPECT_EQ(1, init_params->background_worker_pool_params.max_threads());
  EXPECT_EQ(
      base::TimeDelta::FromMilliseconds(42),
      init_params->background_worker_pool_params.suggested_reclaim_time());
  EXPECT_EQ(
      base::SchedulerBackwardCompatibility::DISABLED,
      init_params->background_worker_pool_params.backward_compatibility());

  EXPECT_EQ(2,
            init_params->background_blocking_worker_pool_params.max_threads());
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(52),
            init_params->background_blocking_worker_pool_params
                .suggested_reclaim_time());
  EXPECT_EQ(base::SchedulerBackwardCompatibility::DISABLED,
            init_params->background_blocking_worker_pool_params
                .backward_compatibility());

  EXPECT_EQ(4, init_params->foreground_worker_pool_params.max_threads());
  EXPECT_EQ(
      base::TimeDelta::FromMilliseconds(62),
      init_params->foreground_worker_pool_params.suggested_reclaim_time());
  EXPECT_EQ(
      base::SchedulerBackwardCompatibility::DISABLED,
      init_params->foreground_worker_pool_params.backward_compatibility());

  EXPECT_EQ(8,
            init_params->foreground_blocking_worker_pool_params.max_threads());
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(72),
            init_params->foreground_blocking_worker_pool_params
                .suggested_reclaim_time());
  EXPECT_EQ(base::SchedulerBackwardCompatibility::DISABLED,
            init_params->foreground_blocking_worker_pool_params
                .backward_compatibility());
}

TEST_F(TaskSchedulerUtilVariationsUtilTest, NoData) {
  EXPECT_FALSE(GetTaskSchedulerInitParams("Renderer"));
}

TEST_F(TaskSchedulerUtilVariationsUtilTest, IncompleteParameters) {
  std::map<std::string, std::string> variation_params;
  variation_params["RendererBackground"] = "1;1;1;0";
  variation_params["RendererBackgroundBlocking"] = "2;2;1;0";
  variation_params["RendererForeground"] = "4;4;1;0";
  variation_params["RendererForegroundBlocking"] = "8;8;1;0";
  SetVariationParams(variation_params);
  EXPECT_FALSE(GetTaskSchedulerInitParams("Renderer"));
}

TEST_F(TaskSchedulerUtilVariationsUtilTest, InvalidParametersFormat) {
  std::map<std::string, std::string> variation_params;
  variation_params["RendererBackground"] = "a;b;c;d;e";
  variation_params["RendererBackgroundBlocking"] = "a;b;c;d;e";
  variation_params["RendererForeground"] = "a;b;c;d;e";
  variation_params["RendererForegroundBlocking"] = "a;b;c;d;e";
  SetVariationParams(variation_params);
  EXPECT_FALSE(GetTaskSchedulerInitParams("Renderer"));
}

TEST_F(TaskSchedulerUtilVariationsUtilTest, ZeroMaxThreads) {
  // The Background pool has a maximum number of threads equal to zero, which is
  // invalid.
  std::map<std::string, std::string> variation_params;
  variation_params["RendererBackground"] = "0;0;0;0;0";
  variation_params["RendererBackgroundBlocking"] = "2;2;1;0;52";
  variation_params["RendererForeground"] = "4;4;1;0;62";
  variation_params["RendererForegroundBlocking"] = "8;8;1;0;72";
  SetVariationParams(variation_params);
  EXPECT_FALSE(GetTaskSchedulerInitParams("Renderer"));
}

TEST_F(TaskSchedulerUtilVariationsUtilTest, NegativeMaxThreads) {
  // The Background pool has a negative maximum number of threads, which is
  // invalid.
  std::map<std::string, std::string> variation_params;
  variation_params["RendererBackground"] = "-5;-5;0;0;0";
  variation_params["RendererBackgroundBlocking"] = "2;2;1;0;52";
  variation_params["RendererForeground"] = "4;4;1;0;62";
  variation_params["RendererForegroundBlocking"] = "8;8;1;0;72";
  SetVariationParams(variation_params);
  EXPECT_FALSE(GetTaskSchedulerInitParams("Renderer"));
}

TEST_F(TaskSchedulerUtilVariationsUtilTest, NegativeSuggestedReclaimTime) {
  // The Background pool has a negative suggested reclaim time, which is
  // invalid.
  std::map<std::string, std::string> variation_params;
  variation_params["RendererBackground"] = "1;1;1;0;-5";
  variation_params["RendererBackgroundBlocking"] = "2;2;1;0;52";
  variation_params["RendererForeground"] = "4;4;1;0;62";
  variation_params["RendererForegroundBlocking"] = "8;8;1;0;72";
  SetVariationParams(variation_params);
  EXPECT_FALSE(GetTaskSchedulerInitParams("Renderer"));
}

}  // namespace task_scheduler_util
