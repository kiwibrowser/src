// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/wake_lock/arc_wake_lock_bridge.h"

#include <memory>
#include <utility>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/common/power.mojom.h"
#include "components/arc/test/connection_holder_util.h"
#include "components/arc/test/fake_power_instance.h"
#include "components/arc/test/fake_wake_lock_instance.h"
#include "services/device/public/cpp/test/test_wake_lock_provider.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

using device::mojom::WakeLockType;

class ArcWakeLockBridgeTest : public testing::Test {
 public:
  ArcWakeLockBridgeTest() {
    auto wake_lock_provider_ptr =
        std::make_unique<device::TestWakeLockProvider>();
    wake_lock_provider_ = wake_lock_provider_ptr.get();

    connector_factory_ =
        service_manager::TestConnectorFactory::CreateForUniqueService(
            std::move(wake_lock_provider_ptr));
    connector_ = connector_factory_->CreateConnector();

    bridge_service_ = std::make_unique<ArcBridgeService>();
    wake_lock_bridge_ =
        std::make_unique<ArcWakeLockBridge>(nullptr, bridge_service_.get());
    wake_lock_bridge_->set_connector_for_testing(connector_.get());
    CreateWakeLockInstance();
  }

  ~ArcWakeLockBridgeTest() override { DestroyWakeLockInstance(); }

 protected:
  // Returns true iff there is no failure acquiring a system wake lock.
  bool AcquirePartialWakeLock() {
    base::RunLoop loop;
    bool result = false;
    wake_lock_bridge_->AcquirePartialWakeLock(base::BindOnce(
        [](bool* result_out, bool result) { *result_out = result; }, &result));
    loop.RunUntilIdle();
    wake_lock_bridge_->FlushWakeLocksForTesting();
    return result;
  }

  // Returns true iff there is no failure releasing a system wake lock.
  bool ReleasePartialWakeLock() {
    base::RunLoop loop;
    bool result = false;
    wake_lock_bridge_->ReleasePartialWakeLock(base::BindOnce(
        [](bool* result_out, bool result) { *result_out = result; }, &result));
    loop.RunUntilIdle();
    wake_lock_bridge_->FlushWakeLocksForTesting();
    return result;
  }

  // Creates a FakeWakeLockInstance for |bridge_service_|. This results in
  // ArcWakeLockBridge::OnInstanceReady() being called.
  void CreateWakeLockInstance() {
    instance_ = std::make_unique<FakeWakeLockInstance>();
    bridge_service_->wake_lock()->SetInstance(instance_.get());
    WaitForInstanceReady(bridge_service_->wake_lock());
  }

  // Destroys the FakeWakeLockInstance. This results in
  // ArcWakeLockBridge::OnInstanceClosed() being called.
  void DestroyWakeLockInstance() {
    if (!instance_)
      return;
    bridge_service_->wake_lock()->CloseInstance(instance_.get());
    instance_.reset();
  }

  device::TestWakeLockProvider* GetWakeLockProvider() {
    return wake_lock_provider_;
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
  std::unique_ptr<service_manager::Connector> connector_;
  device::TestWakeLockProvider* wake_lock_provider_;

  std::unique_ptr<ArcBridgeService> bridge_service_;
  std::unique_ptr<FakeWakeLockInstance> instance_;
  std::unique_ptr<ArcWakeLockBridge> wake_lock_bridge_;

  DISALLOW_COPY_AND_ASSIGN(ArcWakeLockBridgeTest);
};

TEST_F(ArcWakeLockBridgeTest, AcquireAndReleaseSinglePartialWakeLock) {
  EXPECT_TRUE(AcquirePartialWakeLock());
  EXPECT_EQ(1, GetWakeLockProvider()->GetActiveWakeLocksOfType(
                   WakeLockType::kPreventAppSuspension));

  EXPECT_TRUE(ReleasePartialWakeLock());
  EXPECT_EQ(0, GetWakeLockProvider()->GetActiveWakeLocksOfType(
                   WakeLockType::kPreventAppSuspension));
}

TEST_F(ArcWakeLockBridgeTest, AcquireAndReleaseMultiplePartialWakeLocks) {
  // Taking multiple wake locks should result in only one active wake lock.
  EXPECT_TRUE(AcquirePartialWakeLock());
  EXPECT_TRUE(AcquirePartialWakeLock());
  EXPECT_TRUE(AcquirePartialWakeLock());
  EXPECT_EQ(1, GetWakeLockProvider()->GetActiveWakeLocksOfType(
                   WakeLockType::kPreventAppSuspension));

  // Releasing two wake locks after acquiring three should not result in
  // releasing a wake lock.
  EXPECT_TRUE(ReleasePartialWakeLock());
  EXPECT_TRUE(ReleasePartialWakeLock());
  EXPECT_EQ(1, GetWakeLockProvider()->GetActiveWakeLocksOfType(
                   WakeLockType::kPreventAppSuspension));

  // Releasing the remaining wake lock should result in the release of the wake
  // lock.
  EXPECT_TRUE(ReleasePartialWakeLock());
  EXPECT_EQ(0, GetWakeLockProvider()->GetActiveWakeLocksOfType(
                   WakeLockType::kPreventAppSuspension));
}

TEST_F(ArcWakeLockBridgeTest, ReleaseWakeLockOnInstanceClosed) {
  EXPECT_TRUE(AcquirePartialWakeLock());
  ASSERT_EQ(1, GetWakeLockProvider()->GetActiveWakeLocksOfType(
                   WakeLockType::kPreventAppSuspension));

  // If the instance is closed, all wake locks should be released.
  base::RunLoop run_loop;
  GetWakeLockProvider()->set_wake_lock_canceled_callback(
      run_loop.QuitClosure());
  DestroyWakeLockInstance();
  run_loop.Run();
  EXPECT_EQ(0, GetWakeLockProvider()->GetActiveWakeLocksOfType(
                   WakeLockType::kPreventDisplaySleep));

  // Check that wake locks can be requested after the instance becomes ready
  // again.
  CreateWakeLockInstance();
  EXPECT_TRUE(AcquirePartialWakeLock());
  EXPECT_EQ(1, GetWakeLockProvider()->GetActiveWakeLocksOfType(
                   WakeLockType::kPreventAppSuspension));
}

}  // namespace arc
