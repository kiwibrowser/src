// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gpu/context_cache_controller.h"

#include "base/test/test_mock_time_task_runner.h"
#include "components/viz/test/test_context_support.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Mock;
using ::testing::StrictMock;

namespace viz {
namespace {

class MockContextSupport : public TestContextSupport {
 public:
  MockContextSupport() {}
  MOCK_METHOD1(SetAggressivelyFreeResources,
               void(bool aggressively_free_resources));
};

TEST(ContextCacheControllerTest, ScopedVisibilityBasic) {
  StrictMock<MockContextSupport> context_support;
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  ContextCacheController cache_controller(&context_support, task_runner);

  EXPECT_CALL(context_support, SetAggressivelyFreeResources(false));
  std::unique_ptr<ContextCacheController::ScopedVisibility> visibility =
      cache_controller.ClientBecameVisible();
  Mock::VerifyAndClearExpectations(&context_support);

  EXPECT_CALL(context_support, SetAggressivelyFreeResources(true));
  cache_controller.ClientBecameNotVisible(std::move(visibility));
}

TEST(ContextCacheControllerTest, ScopedVisibilityMulti) {
  StrictMock<MockContextSupport> context_support;
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  ContextCacheController cache_controller(&context_support, task_runner);

  EXPECT_CALL(context_support, SetAggressivelyFreeResources(false));
  auto visibility_1 = cache_controller.ClientBecameVisible();
  Mock::VerifyAndClearExpectations(&context_support);
  auto visibility_2 = cache_controller.ClientBecameVisible();

  cache_controller.ClientBecameNotVisible(std::move(visibility_1));
  EXPECT_CALL(context_support, SetAggressivelyFreeResources(true));
  cache_controller.ClientBecameNotVisible(std::move(visibility_2));
}

TEST(ContextCacheControllerTest, ScopedBusyWhileVisible) {
  StrictMock<MockContextSupport> context_support;
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  ContextCacheController cache_controller(&context_support, task_runner);

  EXPECT_CALL(context_support, SetAggressivelyFreeResources(false));
  auto visibility = cache_controller.ClientBecameVisible();
  Mock::VerifyAndClearExpectations(&context_support);

  // Now that we're visible, ensure that going idle triggers a delayed cleanup.
  auto busy = cache_controller.ClientBecameBusy();
  cache_controller.ClientBecameNotBusy(std::move(busy));

  EXPECT_CALL(context_support, SetAggressivelyFreeResources(true));
  EXPECT_CALL(context_support, SetAggressivelyFreeResources(false));
  task_runner->FastForwardBy(base::TimeDelta::FromSeconds(5));
  Mock::VerifyAndClearExpectations(&context_support);

  EXPECT_CALL(context_support, SetAggressivelyFreeResources(true));
  cache_controller.ClientBecameNotVisible(std::move(visibility));
}

TEST(ContextCacheControllerTest, ScopedBusyWhileNotVisible) {
  StrictMock<MockContextSupport> context_support;
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  ContextCacheController cache_controller(&context_support, task_runner);

  auto busy = cache_controller.ClientBecameBusy();

  // We are not visible, so becoming busy should not trigger an idle callback.
  cache_controller.ClientBecameNotBusy(std::move(busy));
  task_runner->FastForwardBy(base::TimeDelta::FromSeconds(5));
}

TEST(ContextCacheControllerTest, ScopedBusyMulitpleWhileVisible) {
  StrictMock<MockContextSupport> context_support;
  auto task_runner = base::MakeRefCounted<base::TestMockTimeTaskRunner>();
  ContextCacheController cache_controller(&context_support, task_runner);

  EXPECT_CALL(context_support, SetAggressivelyFreeResources(false));
  auto visible = cache_controller.ClientBecameVisible();
  Mock::VerifyAndClearExpectations(&context_support);

  auto busy_1 = cache_controller.ClientBecameBusy();
  cache_controller.ClientBecameNotBusy(std::move(busy_1));
  auto busy_2 = cache_controller.ClientBecameBusy();
  cache_controller.ClientBecameNotBusy(std::move(busy_2));

  // When we fast forward, only one cleanup should happen.
  EXPECT_CALL(context_support, SetAggressivelyFreeResources(true));
  EXPECT_CALL(context_support, SetAggressivelyFreeResources(false));
  task_runner->FastForwardBy(base::TimeDelta::FromSeconds(5));
  Mock::VerifyAndClearExpectations(&context_support);

  EXPECT_CALL(context_support, SetAggressivelyFreeResources(true));
  cache_controller.ClientBecameNotVisible(std::move(visible));
}

}  // namespace
}  // namespace viz
