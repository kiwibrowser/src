// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/connection_attempt_base.h"

#include <algorithm>
#include <memory>

#include "base/bind.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "chromeos/services/secure_channel/connection_details.h"
#include "chromeos/services/secure_channel/connection_medium.h"
#include "chromeos/services/secure_channel/fake_authenticated_channel.h"
#include "chromeos/services/secure_channel/fake_connect_to_device_operation.h"
#include "chromeos/services/secure_channel/fake_connect_to_device_operation_factory.h"
#include "chromeos/services/secure_channel/fake_connection_attempt_delegate.h"
#include "chromeos/services/secure_channel/fake_connection_delegate.h"
#include "chromeos/services/secure_channel/fake_pending_connection_request.h"
#include "chromeos/services/secure_channel/pending_connection_request_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace secure_channel {

namespace {

const char kTestDeviceId[] = "testDeviceId";
const char kTestFailureDetail[] = "testFailureDetail";

// Since ConnectionAttemptBase is templatized, a concrete implementation
// is needed for its test.
class TestConnectionAttempt : public ConnectionAttemptBase<std::string> {
 public:
  TestConnectionAttempt(std::unique_ptr<FakeConnectToDeviceOperationFactory>
                            connect_to_device_operation_factory,
                        FakeConnectionAttemptDelegate* delegate,
                        scoped_refptr<base::TestSimpleTaskRunner> task_runner)
      : ConnectionAttemptBase<std::string>(
            std::move(connect_to_device_operation_factory),
            delegate,
            ConnectionDetails(kTestDeviceId,
                              ConnectionMedium::kBluetoothLowEnergy),
            task_runner) {}
  ~TestConnectionAttempt() override = default;
};

}  // namespace

class SecureChannelConnectionAttemptBaseTest : public testing::Test {
 protected:
  SecureChannelConnectionAttemptBaseTest() = default;
  ~SecureChannelConnectionAttemptBaseTest() override = default;

  void SetUp() override {
    fake_authenticated_channel_ = std::make_unique<FakeAuthenticatedChannel>();

    auto factory = std::make_unique<FakeConnectToDeviceOperationFactory>();
    fake_operation_factory_ = factory.get();

    fake_delegate_ = std::make_unique<FakeConnectionAttemptDelegate>();

    test_task_runner_ = base::MakeRefCounted<base::TestSimpleTaskRunner>();

    connection_attempt_ = std::make_unique<TestConnectionAttempt>(
        std::move(factory), fake_delegate_.get(), test_task_runner_);
  }

  void TearDown() override {
    // ExtractClientConnectionParameters() tests destroy |connection_attempt_|,
    // so no additional verifications should be performed.
    if (is_extract_client_data_test_)
      return;

    bool should_test_active_operation_canceled = active_operation_ != nullptr;

    if (should_test_active_operation_canceled) {
      EXPECT_FALSE(active_operation_->canceled());
      EXPECT_FALSE(was_active_operation_canceled_in_teardown_);
      active_operation_->set_destructor_callback(
          base::BindOnce(&SecureChannelConnectionAttemptBaseTest::
                             OnActiveOperationDeletedInTeardown,
                         base::Unretained(this)));
    }

    connection_attempt_.reset();

    if (should_test_active_operation_canceled)
      EXPECT_TRUE(was_active_operation_canceled_in_teardown_);

    // Ensure that running any posted tasks after |connection_attempt_| is
    // deleted do not cause segfaults.
    test_task_runner_->RunUntilIdle();
  }

  FakePendingConnectionRequest* AddNewRequest() {
    auto request = std::make_unique<FakePendingConnectionRequest>(
        connection_attempt_.get());
    FakePendingConnectionRequest* request_raw = request.get();
    active_requests_.insert(request_raw);

    ConnectionAttempt<std::string>* connection_attempt =
        connection_attempt_.get();
    connection_attempt->AddPendingConnectionRequest(std::move(request));

    return request_raw;
  }

  void FinishRequestWithoutConnection(
      FakePendingConnectionRequest* request,
      PendingConnectionRequestDelegate::FailedConnectionReason reason) {
    request->NotifyRequestFinishedWithoutConnection(reason);
    EXPECT_EQ(1u, active_requests_.erase(request));
  }

  FakeConnectToDeviceOperation* GetLastCreatedOperation(
      size_t expected_num_created) {
    VerifyNumOperationsCreated(expected_num_created);
    active_operation_ = fake_operation_factory_->last_created_instance();
    return active_operation_;
  }

  void VerifyNumOperationsCreated(size_t expected_num_created) {
    EXPECT_EQ(expected_num_created,
              fake_operation_factory_->num_instances_created());
  }

  // If |should_run_next_operation| is true, the next operation is started
  // after the current one has failed. |should_run_next_operation| should be
  // false if the test requires that request failures be processed.
  void FailOperation(FakeConnectToDeviceOperation* operation,
                     bool should_run_next_operation = true) {
    // Before failing the operation, check to see how many failure details each
    // request has been passed.
    std::unordered_map<base::UnguessableToken, size_t,
                       base::UnguessableTokenHash>
        id_to_num_details_map;
    for (const auto* request : active_requests_) {
      id_to_num_details_map[request->GetRequestId()] =
          request->handled_failure_details().size();
    }

    EXPECT_EQ(active_operation_, operation);
    operation->OnFailedConnectionAttempt(kTestFailureDetail);
    active_operation_ = nullptr;

    // Now, ensure that each active request had one additional failure detail
    // added, and verify that it was |kTestFailureDetail|.
    for (const auto* request : active_requests_) {
      EXPECT_EQ(id_to_num_details_map[request->GetRequestId()] + 1,
                request->handled_failure_details().size());
      EXPECT_EQ(kTestFailureDetail, request->handled_failure_details().back());
    }

    if (should_run_next_operation)
      RunNextOperationTask();
  }

  void FinishOperationSuccessfully(FakeConnectToDeviceOperation* operation) {
    EXPECT_TRUE(fake_authenticated_channel_);
    auto* fake_authenticated_channel_raw = fake_authenticated_channel_.get();

    EXPECT_EQ(active_operation_, operation);
    operation->OnSuccessfulConnectionAttempt(
        std::move(fake_authenticated_channel_));
    active_operation_ = nullptr;

    // |fake_delegate_|'s delegate should have received the
    // AuthenticatedChannel.
    EXPECT_EQ(connection_attempt_->connection_details(),
              fake_delegate_->connection_details());
    EXPECT_EQ(fake_authenticated_channel_raw,
              fake_delegate_->authenticated_channel());
  }

  void RunNextOperationTask() {
    // This function should only be run if there actually is a pending task.
    EXPECT_EQ(1u, test_task_runner_->NumPendingTasks());
    test_task_runner_->RunPendingTasks();
  }

  void VerifyDelegateNotNotified() {
    EXPECT_FALSE(fake_delegate_->connection_details());
  }

  void VerifyDelegateNotifiedOfFailure() {
    // |fake_delegate_| should have received the failing attempt's ID but no
    // AuthenticatedChannel.
    EXPECT_EQ(connection_attempt_->connection_details(),
              fake_delegate_->connection_details());
    EXPECT_FALSE(fake_delegate_->authenticated_channel());
  }

  std::vector<ClientConnectionParameters> ExtractClientConnectionParameters() {
    is_extract_client_data_test_ = true;
    return ConnectionAttempt<std::string>::ExtractClientConnectionParameters(
        std::move(connection_attempt_));
  }

 private:
  void OnActiveOperationDeletedInTeardown() {
    was_active_operation_canceled_in_teardown_ = true;
  }

  const base::test::ScopedTaskEnvironment scoped_task_environment_;

  FakeConnectToDeviceOperationFactory* fake_operation_factory_;
  std::unique_ptr<FakeConnectionAttemptDelegate> fake_delegate_;
  scoped_refptr<base::TestSimpleTaskRunner> test_task_runner_;

  std::unique_ptr<FakeAuthenticatedChannel> fake_authenticated_channel_;
  std::set<FakePendingConnectionRequest*> active_requests_;
  FakeConnectToDeviceOperation* active_operation_ = nullptr;
  bool was_active_operation_canceled_in_teardown_ = false;

  bool is_extract_client_data_test_ = false;

  std::unique_ptr<TestConnectionAttempt> connection_attempt_;

  DISALLOW_COPY_AND_ASSIGN(SecureChannelConnectionAttemptBaseTest);
};

TEST_F(SecureChannelConnectionAttemptBaseTest, SingleRequest_Success) {
  AddNewRequest();
  FakeConnectToDeviceOperation* operation =
      GetLastCreatedOperation(1u /* expected_num_created */);
  FinishOperationSuccessfully(operation);
}

TEST_F(SecureChannelConnectionAttemptBaseTest, SingleRequest_Fails) {
  FakePendingConnectionRequest* request = AddNewRequest();
  FakeConnectToDeviceOperation* operation =
      GetLastCreatedOperation(1u /* expected_num_created */);

  // Fail the operation; the delegate should not have been notified since no
  // request has yet indicated failure.
  FailOperation(operation, false /* should_run_next_operation */);
  VerifyDelegateNotNotified();

  FinishRequestWithoutConnection(
      request,
      PendingConnectionRequestDelegate::FailedConnectionReason::kRequestFailed);
  VerifyDelegateNotifiedOfFailure();
}

TEST_F(SecureChannelConnectionAttemptBaseTest, SingleRequest_Canceled) {
  // Simulate the request being canceled.
  FakePendingConnectionRequest* request = AddNewRequest();
  FinishRequestWithoutConnection(
      request, PendingConnectionRequestDelegate::FailedConnectionReason::
                   kRequestCanceledByClient);
  VerifyDelegateNotifiedOfFailure();
}

TEST_F(SecureChannelConnectionAttemptBaseTest, SingleRequest_FailThenSuccess) {
  AddNewRequest();
  FakeConnectToDeviceOperation* operation1 =
      GetLastCreatedOperation(1u /* expected_num_created */);

  // Fail the operation; the delegate should not have been notified since no
  // request has yet indicated failure.
  FailOperation(operation1);
  VerifyDelegateNotNotified();

  FakeConnectToDeviceOperation* operation2 =
      GetLastCreatedOperation(2u /* expected_num_created */);
  FinishOperationSuccessfully(operation2);
}

TEST_F(SecureChannelConnectionAttemptBaseTest, TwoRequests_Success) {
  AddNewRequest();
  FakeConnectToDeviceOperation* operation =
      GetLastCreatedOperation(1u /* expected_num_created */);

  // Add a second request; the first operation should still be active.
  AddNewRequest();
  VerifyNumOperationsCreated(1u /* expected_num_created */);

  FinishOperationSuccessfully(operation);
}

TEST_F(SecureChannelConnectionAttemptBaseTest, TwoRequests_Fails) {
  FakePendingConnectionRequest* request1 = AddNewRequest();
  FakeConnectToDeviceOperation* operation =
      GetLastCreatedOperation(1u /* expected_num_created */);

  // Add a second request; the first operation should still be active.
  FakePendingConnectionRequest* request2 = AddNewRequest();
  VerifyNumOperationsCreated(1u /* expected_num_created */);

  // Fail the operation; the delegate should not have been notified since no
  // request has yet indicated failure.
  FailOperation(operation, false /* should_run_next_operation */);
  VerifyDelegateNotNotified();

  // Finish the first request; since a second request remains, the delegate
  // should not have been notified.
  FinishRequestWithoutConnection(
      request1,
      PendingConnectionRequestDelegate::FailedConnectionReason::kRequestFailed);
  VerifyDelegateNotNotified();

  // Finish the second request, which should cause the delegate to be notified.
  FinishRequestWithoutConnection(
      request2,
      PendingConnectionRequestDelegate::FailedConnectionReason::kRequestFailed);
  VerifyDelegateNotifiedOfFailure();
}

TEST_F(SecureChannelConnectionAttemptBaseTest, TwoRequests_Canceled) {
  FakePendingConnectionRequest* request1 = AddNewRequest();
  VerifyNumOperationsCreated(1u /* expected_num_created */);
  FakePendingConnectionRequest* request2 = AddNewRequest();
  VerifyNumOperationsCreated(1u /* expected_num_created */);

  FinishRequestWithoutConnection(
      request1, PendingConnectionRequestDelegate::FailedConnectionReason::
                    kRequestCanceledByClient);
  VerifyDelegateNotNotified();

  FinishRequestWithoutConnection(
      request2, PendingConnectionRequestDelegate::FailedConnectionReason::
                    kRequestCanceledByClient);
  VerifyDelegateNotifiedOfFailure();
}

TEST_F(SecureChannelConnectionAttemptBaseTest, TwoRequests_FailThenSuccess) {
  FakePendingConnectionRequest* request1 = AddNewRequest();
  FakeConnectToDeviceOperation* operation1 =
      GetLastCreatedOperation(1u /* expected_num_created */);

  // Fail the operation; a new operation should have been created.
  FailOperation(operation1);
  VerifyDelegateNotNotified();
  FakeConnectToDeviceOperation* operation2 =
      GetLastCreatedOperation(2u /* expected_num_created */);

  // Add a second request; this should not result in a new operation.
  AddNewRequest();
  VerifyNumOperationsCreated(2u /* expected_num_created */);

  // Fail the second operation.
  FailOperation(operation2, false /* should_run_next_operation */);
  VerifyDelegateNotNotified();

  // Simulate the first request finishing due to failures; since a second
  // request remains, the delegate should not have been notified.
  FinishRequestWithoutConnection(
      request1,
      PendingConnectionRequestDelegate::FailedConnectionReason::kRequestFailed);
  VerifyDelegateNotNotified();

  // Start the next operation running, and simulate it finishing successfully.
  RunNextOperationTask();
  FakeConnectToDeviceOperation* operation3 =
      GetLastCreatedOperation(3u /* expected_num_created */);
  FinishOperationSuccessfully(operation3);
}

TEST_F(SecureChannelConnectionAttemptBaseTest,
       ExtractClientConnectionParameters) {
  FakePendingConnectionRequest* request1 = AddNewRequest();
  auto fake_connection_delegate_1 = std::make_unique<FakeConnectionDelegate>();
  auto fake_connection_delegate_ptr_1 =
      fake_connection_delegate_1->GenerateInterfacePtr();
  auto* fake_connection_delegate_proxy_1 = fake_connection_delegate_ptr_1.get();
  request1->set_client_data_for_extraction(ClientConnectionParameters(
      "request1Feature", std::move(fake_connection_delegate_ptr_1)));

  FakePendingConnectionRequest* request2 = AddNewRequest();
  auto fake_connection_delegate_2 = std::make_unique<FakeConnectionDelegate>();
  auto fake_connection_delegate_ptr_2 =
      fake_connection_delegate_2->GenerateInterfacePtr();
  auto* fake_connection_delegate_proxy_2 = fake_connection_delegate_ptr_2.get();
  request2->set_client_data_for_extraction(ClientConnectionParameters(
      "request2Feature", std::move(fake_connection_delegate_ptr_2)));

  auto extracted_client_data = ExtractClientConnectionParameters();
  EXPECT_EQ(2u, extracted_client_data.size());

  // The extracted client data may not be returned in the same order that as the
  // associated requests were added to |conenction_attempt_|, since
  // ConnectionAttemptBase internally utilizes a std::unordered_map. Sort the
  // data before making verifications to ensure correctness.
  std::sort(extracted_client_data.begin(), extracted_client_data.end());

  EXPECT_EQ("request1Feature", extracted_client_data[0].feature());
  EXPECT_EQ(fake_connection_delegate_proxy_1,
            extracted_client_data[0].connection_delegate_ptr().get());
  EXPECT_EQ("request2Feature", extracted_client_data[1].feature());
  EXPECT_EQ(fake_connection_delegate_proxy_2,
            extracted_client_data[1].connection_delegate_ptr().get());
}

}  // namespace secure_channel

}  // namespace chromeos
