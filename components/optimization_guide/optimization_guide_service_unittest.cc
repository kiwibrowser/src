// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/optimization_guide/optimization_guide_service.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_task_environment.h"
#include "base/version.h"
#include "components/optimization_guide/optimization_guide_service_observer.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace optimization_guide {

const base::FilePath::CharType kFileName[] = FILE_PATH_LITERAL("somefile.pb");

class TestObserver : public OptimizationGuideServiceObserver {
 public:
  TestObserver() : received_notification_(false) {}

  ~TestObserver() override {}

  void OnHintsProcessed(
      const proto::Configuration& config,
      const optimization_guide::ComponentInfo& component_info) override {
    received_notification_ = true;
    received_config_ = config;
    received_version_ = component_info.hints_version;
  }

  bool received_notification() const { return received_notification_; }

  proto::Configuration received_config() const { return received_config_; }

  base::Version received_version() const { return received_version_; }

 private:
  bool received_notification_;
  proto::Configuration received_config_;
  base::Version received_version_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

class OptimizationGuideServiceTest : public testing::Test {
 public:
  OptimizationGuideServiceTest() {}

  ~OptimizationGuideServiceTest() override {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    optimization_guide_service_ = std::make_unique<OptimizationGuideService>(
        scoped_task_environment_.GetMainThreadTaskRunner());

    observer_ = std::make_unique<TestObserver>();
  }

  OptimizationGuideService* optimization_guide_service() {
    return optimization_guide_service_.get();
  }

  TestObserver* observer() { return observer_.get(); }

  void AddObserver() { optimization_guide_service_->AddObserver(observer()); }

  void RemoveObserver() {
    optimization_guide_service_->RemoveObserver(observer());
  }

  void UpdateHints(const base::Version& version,
                   const base::FilePath& filePath) {
    ComponentInfo info(version, filePath);
    optimization_guide_service_->ProcessHints(info);
  }

  void WriteConfigToFile(const base::FilePath& filePath,
                         const proto::Configuration& config) {
    std::string serialized_config;
    ASSERT_TRUE(config.SerializeToString(&serialized_config));
    ASSERT_EQ(static_cast<int32_t>(serialized_config.length()),
              base::WriteFile(filePath, serialized_config.data(),
                              serialized_config.length()));
  }

  base::FilePath temp_dir() const { return temp_dir_.GetPath(); }

 protected:
  void RunUntilIdle() {
    scoped_task_environment_.RunUntilIdle();
    base::RunLoop().RunUntilIdle();
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::ScopedTempDir temp_dir_;

  std::unique_ptr<OptimizationGuideService> optimization_guide_service_;
  std::unique_ptr<TestObserver> observer_;

  DISALLOW_COPY_AND_ASSIGN(OptimizationGuideServiceTest);
};

TEST_F(OptimizationGuideServiceTest, ProcessHintsInvalidVersionIgnored) {
  base::HistogramTester histogram_tester;
  AddObserver();
  UpdateHints(base::Version(""), base::FilePath(kFileName));

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.ProcessHintsResult",
      static_cast<int>(OptimizationGuideService::ProcessHintsResult::
                           FAILED_INVALID_PARAMETERS),
      1);
}

TEST_F(OptimizationGuideServiceTest, ProcessHintsPastVersionIgnored) {
  AddObserver();
  optimization_guide_service()->SetLatestProcessedVersionForTesting(
      base::Version("2.0.0"));

  const base::FilePath filePath = temp_dir().Append(kFileName);
  proto::Configuration config;
  proto::Hint* hint = config.add_hints();
  hint->set_key("google.com");
  ASSERT_NO_FATAL_FAILURE(WriteConfigToFile(filePath, config));

  UpdateHints(base::Version("1.0.0"), filePath);

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
}

TEST_F(OptimizationGuideServiceTest, ProcessHintsSameVersionIgnored) {
  AddObserver();
  const base::Version version("1.0.0");
  optimization_guide_service()->SetLatestProcessedVersionForTesting(version);

  const base::FilePath filePath = temp_dir().Append(kFileName);
  proto::Configuration config;
  proto::Hint* hint = config.add_hints();
  hint->set_key("google.com");
  ASSERT_NO_FATAL_FAILURE(WriteConfigToFile(filePath, config));

  UpdateHints(version, filePath);

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
}

TEST_F(OptimizationGuideServiceTest, ProcessHintsEmptyFileNameIgnored) {
  base::HistogramTester histogram_tester;
  AddObserver();
  UpdateHints(base::Version("1.0.0"), base::FilePath(FILE_PATH_LITERAL("")));

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.ProcessHintsResult",
      static_cast<int>(OptimizationGuideService::ProcessHintsResult::
                           FAILED_INVALID_PARAMETERS),
      1);
}

TEST_F(OptimizationGuideServiceTest, ProcessHintsInvalidFileIgnored) {
  base::HistogramTester histogram_tester;
  AddObserver();
  UpdateHints(base::Version("1.0.0"), base::FilePath(kFileName));

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.ProcessHintsResult",
      static_cast<int>(
          OptimizationGuideService::ProcessHintsResult::FAILED_READING_FILE),
      1);
}

TEST_F(OptimizationGuideServiceTest, ProcessHintsNotAConfigInFileIgnored) {
  base::HistogramTester histogram_tester;
  AddObserver();
  const base::FilePath filePath = temp_dir().Append(kFileName);
  ASSERT_EQ(static_cast<int32_t>(3), base::WriteFile(filePath, "boo", 3));

  UpdateHints(base::Version("1.0.0"), filePath);

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.ProcessHintsResult",
      static_cast<int>(OptimizationGuideService::ProcessHintsResult::
                           FAILED_INVALID_CONFIGURATION),
      1);
}

TEST_F(OptimizationGuideServiceTest, ProcessHintsIssuesNotification) {
  base::HistogramTester histogram_tester;
  AddObserver();
  const base::FilePath filePath = temp_dir().Append(kFileName);
  proto::Configuration config;
  proto::Hint* hint = config.add_hints();
  hint->set_key("google.com");
  ASSERT_NO_FATAL_FAILURE(WriteConfigToFile(filePath, config));

  base::Version hints_version("1.0.0");
  UpdateHints(hints_version, filePath);

  RunUntilIdle();

  EXPECT_TRUE(observer()->received_notification());
  proto::Configuration received_config = observer()->received_config();
  ASSERT_EQ(1, received_config.hints_size());
  ASSERT_EQ("google.com", received_config.hints()[0].key());
  EXPECT_EQ(0, observer()->received_version().CompareTo(hints_version));
  histogram_tester.ExpectUniqueSample(
      "OptimizationGuide.ProcessHintsResult",
      static_cast<int>(OptimizationGuideService::ProcessHintsResult::SUCCESS),
      1);
}

TEST_F(OptimizationGuideServiceTest,
       UnregisteredObserverDoesNotReceiveNotification) {
  // Add and remove observer to ensure that observer properly unregistered.
  AddObserver();
  RemoveObserver();

  const base::FilePath filePath = temp_dir().Append(kFileName);
  proto::Configuration config;
  proto::Hint* hint = config.add_hints();
  hint->set_key("google.com");
  ASSERT_NO_FATAL_FAILURE(WriteConfigToFile(filePath, config));

  UpdateHints(base::Version("1.0.0"), filePath);

  RunUntilIdle();

  EXPECT_FALSE(observer()->received_notification());
}

}  // namespace optimization_guide
