// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/pending_connection_request_base.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "chromeos/services/secure_channel/client_connection_parameters.h"
#include "chromeos/services/secure_channel/fake_connection_delegate.h"
#include "chromeos/services/secure_channel/fake_pending_connection_request_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace secure_channel {

namespace {

const char kTestReadableRequestTypeForLogging[] = "Test Request Type";
const char kTestFeature[] = "testFeature";
const mojom::ConnectionAttemptFailureReason kTestFailureReason =
    mojom::ConnectionAttemptFailureReason::TIMEOUT_FINDING_DEVICE;

enum class TestFailureDetail {
  kReasonWhichCausesRequestToBecomeInactive,
  kReasonWhichDoesNotCauseRequestToBecomeInactive
};

// Since PendingConnectionRequestBase is templatized, a concrete implementation
// is needed for its test.
class TestPendingConnectionRequest
    : public PendingConnectionRequestBase<TestFailureDetail> {
 public:
  TestPendingConnectionRequest(
      ClientConnectionParameters client_connection_parameters,
      PendingConnectionRequestDelegate* delegate)
      : PendingConnectionRequestBase<TestFailureDetail>(
            std::move(client_connection_parameters),
            kTestReadableRequestTypeForLogging,
            delegate) {}
  ~TestPendingConnectionRequest() override = default;

  // PendingConnectionRequestBase<TestFailureDetail>:
  void HandleConnectionFailure(TestFailureDetail failure_detail) override {
    switch (failure_detail) {
      case TestFailureDetail::kReasonWhichCausesRequestToBecomeInactive:
        StopRequestDueToConnectionFailures(kTestFailureReason);
        break;
      case TestFailureDetail::kReasonWhichDoesNotCauseRequestToBecomeInactive:
        // Do nothing.
        break;
    }
  }
};

}  // namespace

class SecureChannelPendingConnectionRequestBaseTest : public testing::Test {
 protected:
  SecureChannelPendingConnectionRequestBaseTest() = default;
  ~SecureChannelPendingConnectionRequestBaseTest() override = default;

  void SetUp() override {
    fake_connection_delegate_ = std::make_unique<FakeConnectionDelegate>();
    fake_pending_connection_request_delegate_ =
        std::make_unique<FakePendingConnectionRequestDelegate>();
    auto fake_connection_delegate_interface_ptr =
        fake_connection_delegate_->GenerateInterfacePtr();
    fake_connection_delegate_proxy_ =
        fake_connection_delegate_interface_ptr.get();

    test_pending_connection_request_ =
        std::make_unique<TestPendingConnectionRequest>(
            ClientConnectionParameters(
                kTestFeature,
                std::move(fake_connection_delegate_interface_ptr)),
            fake_pending_connection_request_delegate_.get());
  }

  void HandleConnectionFailure(TestFailureDetail test_failure_detail,
                               bool wait_for_mojo_call) {
    base::RunLoop run_loop;
    if (wait_for_mojo_call) {
      fake_connection_delegate_->set_closure_for_next_delegate_callback(
          run_loop.QuitClosure());
    }

    test_pending_connection_request_->HandleConnectionFailure(
        test_failure_detail);

    if (wait_for_mojo_call)
      run_loop.Run();
  }

  const base::Optional<
      PendingConnectionRequestDelegate::FailedConnectionReason>&
  GetFailedConnectionReason() {
    return fake_pending_connection_request_delegate_
        ->GetFailedConnectionReasonForId(
            test_pending_connection_request_->GetRequestId());
  }

  void DisconnectConnectionDelegatePtr() {
    base::RunLoop run_loop;
    fake_pending_connection_request_delegate_
        ->set_closure_for_next_delegate_callback(run_loop.QuitClosure());
    fake_connection_delegate_->DisconnectGeneratedPtrs();
    run_loop.Run();
  }

  const base::Optional<mojom::ConnectionAttemptFailureReason>&
  GetConnectionAttemptFailureReason() const {
    return fake_connection_delegate_->connection_attempt_failure_reason();
  }

  ClientConnectionParameters ExtractClientConnectionParameters() {
    return PendingConnectionRequest<TestFailureDetail>::
        ExtractClientConnectionParameters(
            std::move(test_pending_connection_request_));
  }

  mojom::ConnectionDelegate::Proxy_* fake_connection_delegate_proxy() {
    return fake_connection_delegate_proxy_;
  }

 private:
  const base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<FakeConnectionDelegate> fake_connection_delegate_;
  mojom::ConnectionDelegate::Proxy_* fake_connection_delegate_proxy_ = nullptr;
  std::unique_ptr<FakePendingConnectionRequestDelegate>
      fake_pending_connection_request_delegate_;

  std::unique_ptr<TestPendingConnectionRequest>
      test_pending_connection_request_;

  DISALLOW_COPY_AND_ASSIGN(SecureChannelPendingConnectionRequestBaseTest);
};

TEST_F(SecureChannelPendingConnectionRequestBaseTest,
       HandleConnectionFailureWhichCausesRequestToBecomeInactive) {
  HandleConnectionFailure(
      TestFailureDetail::kReasonWhichCausesRequestToBecomeInactive,
      true /* wait_for_mojo_call */);
  EXPECT_EQ(
      PendingConnectionRequestDelegate::FailedConnectionReason::kRequestFailed,
      *GetFailedConnectionReason());
  EXPECT_EQ(kTestFailureReason, *GetConnectionAttemptFailureReason());
}

TEST_F(SecureChannelPendingConnectionRequestBaseTest,
       HandleConnectionFailureWhichDoesNotCauseRequestToBecomeInactive) {
  // Repeat 5 connection failures, none of which should cause the request to
  // become inactive.
  for (int i = 0; i < 5; ++i) {
    HandleConnectionFailure(
        TestFailureDetail::kReasonWhichDoesNotCauseRequestToBecomeInactive,
        false /* wait_for_mojo_call */);
    EXPECT_FALSE(GetFailedConnectionReason());
    EXPECT_FALSE(GetConnectionAttemptFailureReason());
  }
}

TEST_F(SecureChannelPendingConnectionRequestBaseTest,
       ConnectionDelegateInvalidated) {
  DisconnectConnectionDelegatePtr();
  EXPECT_EQ(PendingConnectionRequestDelegate::FailedConnectionReason::
                kRequestCanceledByClient,
            *GetFailedConnectionReason());
}

TEST_F(SecureChannelPendingConnectionRequestBaseTest,
       ExtractClientConnectionParameters) {
  auto extracted_client_data = ExtractClientConnectionParameters();
  EXPECT_EQ(kTestFeature, extracted_client_data.feature());
  EXPECT_EQ(fake_connection_delegate_proxy(),
            extracted_client_data.connection_delegate_ptr().get());
}

}  // namespace secure_channel

}  // namespace chromeos
