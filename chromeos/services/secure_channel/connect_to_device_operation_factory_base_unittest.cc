// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/connect_to_device_operation_factory_base.h"

#include <memory>

#include "base/bind_helpers.h"
#include "base/test/gtest_util.h"
#include "chromeos/services/secure_channel/authenticated_channel.h"
#include "chromeos/services/secure_channel/fake_connect_to_device_operation.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "components/cryptauth/secure_channel.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace secure_channel {

namespace {

// Since ConnectToDeviceOperationFactoryBase is templatized, a concrete
// implementation is needed for its test.
class TestConnectToDeviceOperationFactory
    : public ConnectToDeviceOperationFactoryBase<std::string> {
 public:
  TestConnectToDeviceOperationFactory(
      const cryptauth::RemoteDeviceRef& device_to_connect_to)
      : ConnectToDeviceOperationFactoryBase<std::string>(device_to_connect_to),
        remote_device_(device_to_connect_to) {}

  ~TestConnectToDeviceOperationFactory() override = default;

  void InvokePendingDestructorCallback() {
    EXPECT_TRUE(last_destructor_callback_);
    std::move(last_destructor_callback_).Run();
  }

  // ConnectToDeviceOperationFactoryBase<std::string>:
  std::unique_ptr<ConnectToDeviceOperation<std::string>> PerformCreateOperation(
      typename ConnectToDeviceOperation<std::string>::ConnectionSuccessCallback
          success_callback,
      typename ConnectToDeviceOperation<std::string>::ConnectionFailedCallback
          failure_callback,
      const cryptauth::RemoteDeviceRef& device_to_connect_to,
      base::OnceClosure destructor_callback) override {
    // The previous destructor callback should have been invoked by the time
    // this function runs.
    EXPECT_FALSE(last_destructor_callback_);
    last_destructor_callback_ = std::move(destructor_callback);

    EXPECT_EQ(remote_device_, device_to_connect_to);

    return std::make_unique<FakeConnectToDeviceOperation>(
        std::move(success_callback), std::move(failure_callback));
  }

 private:
  const cryptauth::RemoteDeviceRef remote_device_;

  base::OnceClosure last_destructor_callback_;
};

}  // namespace

class SecureChannelConnectToDeviceOperationFactoryBaseTest
    : public testing::Test {
 protected:
  SecureChannelConnectToDeviceOperationFactoryBaseTest()
      : test_device_(cryptauth::CreateRemoteDeviceRefForTest()) {}

  ~SecureChannelConnectToDeviceOperationFactoryBaseTest() override = default;

  void SetUp() override {
    test_factory_ =
        std::make_unique<TestConnectToDeviceOperationFactory>(test_device_);
  }

  std::unique_ptr<ConnectToDeviceOperation<std::string>> CallCreateOperation() {
    // Use a pointer to ConnectToDeviceOperationFactory, since
    // ConnectToDeviceOperationFactoryBase makes CreateOperation private.
    ConnectToDeviceOperationFactory<std::string>* factory = test_factory_.get();
    return factory->CreateOperation(base::DoNothing() /* success_callback */,
                                    base::DoNothing() /* failure_callback */);
  }

  void FinishActiveOperation(
      std::unique_ptr<ConnectToDeviceOperation<std::string>>
          operation_to_finish) {
    EXPECT_TRUE(operation_to_finish);

    // Use a pointer to FakeConnectToDeviceOperation, since
    // ConnectToDeviceOperation makes OnSuccessfulConnectionAttempt() protected.
    FakeConnectToDeviceOperation* operation =
        static_cast<FakeConnectToDeviceOperation*>(operation_to_finish.get());
    operation->OnFailedConnectionAttempt(
        "arbitraryFailureDetail" /* failure_detail */);

    test_factory_->InvokePendingDestructorCallback();
  }

 private:
  const cryptauth::RemoteDeviceRef test_device_;

  std::unique_ptr<TestConnectToDeviceOperationFactory> test_factory_;

  DISALLOW_COPY_AND_ASSIGN(
      SecureChannelConnectToDeviceOperationFactoryBaseTest);
};

TEST_F(SecureChannelConnectToDeviceOperationFactoryBaseTest,
       UseFactoryCorrectly) {
  auto operation1 = CallCreateOperation();
  EXPECT_TRUE(operation1);

  // Invoke the destructor callback and try again.
  FinishActiveOperation(std::move(operation1));
  auto operation2 = CallCreateOperation();
  EXPECT_TRUE(operation2);

  // Invoke the destructor callback and try again.
  FinishActiveOperation(std::move(operation2));
  auto operation3 = CallCreateOperation();
  EXPECT_TRUE(operation3);

  FinishActiveOperation(std::move(operation3));
}

TEST_F(SecureChannelConnectToDeviceOperationFactoryBaseTest,
       UseFactoryIncorrectly) {
  // First operation should always complete successfully.
  auto operation = CallCreateOperation();
  EXPECT_TRUE(operation);

  // The first operation's destructor callback has not yet been invoked, so
  // creating another should fail.
  EXPECT_DCHECK_DEATH(CallCreateOperation());

  FinishActiveOperation(std::move(operation));
}

}  // namespace secure_channel

}  // namespace chromeos
