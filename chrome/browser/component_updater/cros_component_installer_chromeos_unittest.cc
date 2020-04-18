// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process_platform_part_chromeos.h"
#include "chrome/browser/component_updater/cros_component_installer_chromeos.h"
#include "chrome/browser/component_updater/metadata_table_chromeos.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/component_updater/mock_component_updater_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace component_updater {

class CrOSMockComponentUpdateService
    : public component_updater::MockComponentUpdateService {
 public:
  CrOSMockComponentUpdateService() {}
  ~CrOSMockComponentUpdateService() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CrOSMockComponentUpdateService);
};

class CrOSComponentInstallerTest : public PlatformTest {
 public:
  CrOSComponentInstallerTest() {}
  void SetUp() override { PlatformTest::SetUp(); }

 protected:
  void RunUntilIdle() {
    scoped_task_environment_.RunUntilIdle();
    base::RunLoop().RunUntilIdle();
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  DISALLOW_COPY_AND_ASSIGN(CrOSComponentInstallerTest);
};

class MockCrOSComponentInstallerPolicy : public CrOSComponentInstallerPolicy {
 public:
  explicit MockCrOSComponentInstallerPolicy(const ComponentConfig& config)
      : CrOSComponentInstallerPolicy(config) {}
  MOCK_METHOD2(IsCompatible,
               bool(const std::string& env_version_str,
                    const std::string& min_env_version_str));
};

TEST_F(CrOSComponentInstallerTest, CompatibleCrOSComponent) {
  component_updater::CrOSComponentManager cros_component_manager(nullptr);

  const std::string kComponent = "a";
  EXPECT_FALSE(cros_component_manager.IsCompatible(kComponent));
  EXPECT_EQ(cros_component_manager.GetCompatiblePath(kComponent).value(),
            std::string());

  const base::FilePath kPath("/component/path/v0");
  cros_component_manager.RegisterCompatiblePath(kComponent, kPath);
  EXPECT_TRUE(cros_component_manager.IsCompatible(kComponent));
  EXPECT_EQ(cros_component_manager.GetCompatiblePath(kComponent), kPath);
  cros_component_manager.UnregisterCompatiblePath(kComponent);
  EXPECT_FALSE(cros_component_manager.IsCompatible(kComponent));
}

TEST_F(CrOSComponentInstallerTest, CompatibilityOK) {
  ComponentConfig config{"a", "2.1", ""};
  MockCrOSComponentInstallerPolicy policy(config);
  EXPECT_CALL(policy, IsCompatible(testing::_, testing::_)).Times(1);
  base::Version version;
  base::FilePath path;
  std::unique_ptr<base::DictionaryValue> manifest =
      std::make_unique<base::DictionaryValue>();
  manifest->SetString("min_env_version", "2.1");
  policy.ComponentReady(version, path, std::move(manifest));
}

TEST_F(CrOSComponentInstallerTest, CompatibilityMissingManifest) {
  ComponentConfig config{"a", "2.1", ""};
  MockCrOSComponentInstallerPolicy policy(config);
  EXPECT_CALL(policy, IsCompatible(testing::_, testing::_)).Times(0);
  base::Version version;
  base::FilePath path;
  std::unique_ptr<base::DictionaryValue> manifest =
      std::make_unique<base::DictionaryValue>();
  policy.ComponentReady(version, path, std::move(manifest));
}

TEST_F(CrOSComponentInstallerTest, IsCompatibleOrNot) {
  ComponentConfig config{"", "", ""};
  CrOSComponentInstallerPolicy policy(config);
  EXPECT_TRUE(policy.IsCompatible("1.0", "1.0"));
  EXPECT_TRUE(policy.IsCompatible("1.1", "1.0"));
  EXPECT_FALSE(policy.IsCompatible("1.0", "1.1"));
  EXPECT_FALSE(policy.IsCompatible("1.0", "2.0"));
  EXPECT_FALSE(policy.IsCompatible("1.c", "1.c"));
  EXPECT_FALSE(policy.IsCompatible("1", "1.1"));
  EXPECT_TRUE(policy.IsCompatible("1.1.1", "1.1"));
}

TEST_F(CrOSComponentInstallerTest, RegisterComponent) {
  std::unique_ptr<CrOSMockComponentUpdateService> cus(
      new CrOSMockComponentUpdateService());
  ComponentConfig config{
      "star-cups-driver", "1.1",
      "6d24de30f671da5aee6d463d9e446cafe9ddac672800a9defe86877dcde6c466"};
  EXPECT_CALL(*cus, RegisterComponent(testing::_)).Times(1);
  component_updater::CrOSComponentManager cros_component_manager(nullptr);
  cros_component_manager.Register(cus.get(), config, base::OnceClosure());
  RunUntilIdle();
}

}  // namespace component_updater
