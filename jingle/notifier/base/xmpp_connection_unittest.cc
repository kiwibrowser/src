// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/base/xmpp_connection.h"

#include <string>
#include <utility>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_default.h"
#include "base/run_loop.h"
#include "jingle/glue/mock_task.h"
#include "jingle/glue/task_pump.h"
#include "jingle/notifier/base/weak_xmpp_client.h"
#include "net/cert/cert_verifier.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libjingle_xmpp/xmpp/prexmppauth.h"
#include "third_party/libjingle_xmpp/xmpp/xmppclientsettings.h"

namespace buzz {
class CaptchaChallenge;
class Jid;
}  // namespace buzz

namespace rtc {
class SocketAddress;
class Task;
}  // namespace rtc

namespace notifier {

using ::testing::_;
using ::testing::Return;
using ::testing::SaveArg;

class MockPreXmppAuth : public buzz::PreXmppAuth {
 public:
  ~MockPreXmppAuth() override {}

  MOCK_METHOD2(ChooseBestSaslMechanism,
               std::string(const std::vector<std::string>&, bool));
  MOCK_METHOD1(CreateSaslMechanism,
               buzz::SaslMechanism*(const std::string&));
  MOCK_METHOD5(StartPreXmppAuth,
               void(const buzz::Jid&,
                    const rtc::SocketAddress&,
                    const std::string&,
                    const std::string&,
                    const std::string&));
  MOCK_CONST_METHOD0(IsAuthDone, bool());
  MOCK_CONST_METHOD0(IsAuthorized, bool());
  MOCK_CONST_METHOD0(HadError, bool());
  MOCK_CONST_METHOD0(GetError, int());
  MOCK_CONST_METHOD0(GetCaptchaChallenge, buzz::CaptchaChallenge());
  MOCK_CONST_METHOD0(GetAuthToken, std::string());
  MOCK_CONST_METHOD0(GetAuthMechanism, std::string());
};

class MockXmppConnectionDelegate : public XmppConnection::Delegate {
 public:
  ~MockXmppConnectionDelegate() override {}

  MOCK_METHOD1(OnConnect, void(base::WeakPtr<buzz::XmppTaskParentInterface>));
  MOCK_METHOD3(OnError,
               void(buzz::XmppEngine::Error, int, const buzz::XmlElement*));
};

class XmppConnectionTest : public testing::Test {
 protected:
  XmppConnectionTest()
      : mock_pre_xmpp_auth_(new MockPreXmppAuth()) {
    // GTest death tests by default execute in a fork()ed but not exec()ed
    // process. On macOS, a CoreFoundation-backed MessageLoop will exit with a
    // __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__
    // when called. Use the threadsafe mode to avoid this problem.
    testing::GTEST_FLAG(death_test_style) = "threadsafe";

    url_request_context_getter_ =
        new net::TestURLRequestContextGetter(message_loop_.task_runner());
  }

  ~XmppConnectionTest() override {}

  void TearDown() override {
    // Clear out any messages posted by XmppConnection's destructor.
    base::RunLoop().RunUntilIdle();
  }

  // Needed by XmppConnection.
  base::MessageLoop message_loop_;
  MockXmppConnectionDelegate mock_xmpp_connection_delegate_;
  std::unique_ptr<MockPreXmppAuth> mock_pre_xmpp_auth_;
  scoped_refptr<net::TestURLRequestContextGetter> url_request_context_getter_;
};

TEST_F(XmppConnectionTest, CreateDestroy) {
  XmppConnection xmpp_connection(
      buzz::XmppClientSettings(), url_request_context_getter_,
      &mock_xmpp_connection_delegate_, NULL, TRAFFIC_ANNOTATION_FOR_TESTS);
}

TEST_F(XmppConnectionTest, ImmediateFailure) {
  // ChromeAsyncSocket::Connect() will always return false since we're
  // not setting a valid host, but this gets bubbled up as ERROR_NONE
  // due to XmppClient's inconsistent error-handling.
  EXPECT_CALL(mock_xmpp_connection_delegate_,
              OnError(buzz::XmppEngine::ERROR_NONE, 0, NULL));

  XmppConnection xmpp_connection(
      buzz::XmppClientSettings(), url_request_context_getter_,
      &mock_xmpp_connection_delegate_, NULL, TRAFFIC_ANNOTATION_FOR_TESTS);

  // We need to do this *before* |xmpp_connection| gets destroyed or
  // our delegate won't be called.
  base::RunLoop().RunUntilIdle();
}

TEST_F(XmppConnectionTest, PreAuthFailure) {
  EXPECT_CALL(*mock_pre_xmpp_auth_, StartPreXmppAuth(_, _, _, _,_));
  EXPECT_CALL(*mock_pre_xmpp_auth_, IsAuthDone()).WillOnce(Return(true));
  EXPECT_CALL(*mock_pre_xmpp_auth_, IsAuthorized()).WillOnce(Return(false));
  EXPECT_CALL(*mock_pre_xmpp_auth_, HadError()).WillOnce(Return(true));
  EXPECT_CALL(*mock_pre_xmpp_auth_, GetError()).WillOnce(Return(5));

  EXPECT_CALL(mock_xmpp_connection_delegate_,
              OnError(buzz::XmppEngine::ERROR_AUTH, 5, NULL));

  XmppConnection xmpp_connection(
      buzz::XmppClientSettings(), url_request_context_getter_,
      &mock_xmpp_connection_delegate_, mock_pre_xmpp_auth_.release(),
      TRAFFIC_ANNOTATION_FOR_TESTS);

  // We need to do this *before* |xmpp_connection| gets destroyed or
  // our delegate won't be called.
  base::RunLoop().RunUntilIdle();
}

TEST_F(XmppConnectionTest, FailureAfterPreAuth) {
  EXPECT_CALL(*mock_pre_xmpp_auth_, StartPreXmppAuth(_, _, _, _,_));
  EXPECT_CALL(*mock_pre_xmpp_auth_, IsAuthDone()).WillOnce(Return(true));
  EXPECT_CALL(*mock_pre_xmpp_auth_, IsAuthorized()).WillOnce(Return(true));
  EXPECT_CALL(*mock_pre_xmpp_auth_, GetAuthMechanism()).WillOnce(Return(""));
  EXPECT_CALL(*mock_pre_xmpp_auth_, GetAuthToken()).WillOnce(Return(""));

  EXPECT_CALL(mock_xmpp_connection_delegate_,
              OnError(buzz::XmppEngine::ERROR_NONE, 0, NULL));

  XmppConnection xmpp_connection(
      buzz::XmppClientSettings(), url_request_context_getter_,
      &mock_xmpp_connection_delegate_, mock_pre_xmpp_auth_.release(),
      TRAFFIC_ANNOTATION_FOR_TESTS);

  // We need to do this *before* |xmpp_connection| gets destroyed or
  // our delegate won't be called.
  base::RunLoop().RunUntilIdle();
}

TEST_F(XmppConnectionTest, RaisedError) {
  EXPECT_CALL(mock_xmpp_connection_delegate_,
              OnError(buzz::XmppEngine::ERROR_NONE, 0, NULL));

  XmppConnection xmpp_connection(
      buzz::XmppClientSettings(), url_request_context_getter_,
      &mock_xmpp_connection_delegate_, NULL, TRAFFIC_ANNOTATION_FOR_TESTS);

  xmpp_connection.weak_xmpp_client_->
      SignalStateChange(buzz::XmppEngine::STATE_CLOSED);
}

TEST_F(XmppConnectionTest, Connect) {
  base::WeakPtr<rtc::Task> weak_ptr;
  EXPECT_CALL(mock_xmpp_connection_delegate_, OnConnect(_)).
      WillOnce(SaveArg<0>(&weak_ptr));

  {
    XmppConnection xmpp_connection(
        buzz::XmppClientSettings(), url_request_context_getter_,
        &mock_xmpp_connection_delegate_, NULL, TRAFFIC_ANNOTATION_FOR_TESTS);

    xmpp_connection.weak_xmpp_client_->
        SignalStateChange(buzz::XmppEngine::STATE_OPEN);
    EXPECT_EQ(xmpp_connection.weak_xmpp_client_.get(), weak_ptr.get());
  }

  EXPECT_EQ(NULL, weak_ptr.get());
}

TEST_F(XmppConnectionTest, MultipleConnect) {
  EXPECT_DEBUG_DEATH({
    base::WeakPtr<rtc::Task> weak_ptr;
    EXPECT_CALL(mock_xmpp_connection_delegate_, OnConnect(_)).
        WillOnce(SaveArg<0>(&weak_ptr));

    XmppConnection xmpp_connection(
        buzz::XmppClientSettings(), url_request_context_getter_,
        &mock_xmpp_connection_delegate_, NULL, TRAFFIC_ANNOTATION_FOR_TESTS);

    xmpp_connection.weak_xmpp_client_->
        SignalStateChange(buzz::XmppEngine::STATE_OPEN);
    for (int i = 0; i < 3; ++i) {
      xmpp_connection.weak_xmpp_client_->
          SignalStateChange(buzz::XmppEngine::STATE_OPEN);
    }

    EXPECT_EQ(xmpp_connection.weak_xmpp_client_.get(), weak_ptr.get());
  }, "more than once");
}

TEST_F(XmppConnectionTest, ConnectThenError) {
  base::WeakPtr<rtc::Task> weak_ptr;
  EXPECT_CALL(mock_xmpp_connection_delegate_, OnConnect(_)).
      WillOnce(SaveArg<0>(&weak_ptr));
  EXPECT_CALL(mock_xmpp_connection_delegate_,
              OnError(buzz::XmppEngine::ERROR_NONE, 0, NULL));

  XmppConnection xmpp_connection(
      buzz::XmppClientSettings(), url_request_context_getter_,
      &mock_xmpp_connection_delegate_, NULL, TRAFFIC_ANNOTATION_FOR_TESTS);

  xmpp_connection.weak_xmpp_client_->
      SignalStateChange(buzz::XmppEngine::STATE_OPEN);
  EXPECT_EQ(xmpp_connection.weak_xmpp_client_.get(), weak_ptr.get());

  xmpp_connection.weak_xmpp_client_->
      SignalStateChange(buzz::XmppEngine::STATE_CLOSED);
  EXPECT_EQ(NULL, weak_ptr.get());
}

// We don't destroy XmppConnection's task pump on destruction, but it
// should still not run any more tasks.
TEST_F(XmppConnectionTest, TasksDontRunAfterXmppConnectionDestructor) {
  {
    XmppConnection xmpp_connection(
        buzz::XmppClientSettings(), url_request_context_getter_,
        &mock_xmpp_connection_delegate_, NULL, TRAFFIC_ANNOTATION_FOR_TESTS);

    jingle_glue::MockTask* task =
        new jingle_glue::MockTask(xmpp_connection.task_pump_.get());
    // We have to do this since the state enum is protected in
    // rtc::Task.
    const int TASK_STATE_ERROR = 3;
    ON_CALL(*task, ProcessStart())
        .WillByDefault(Return(TASK_STATE_ERROR));
    EXPECT_CALL(*task, ProcessStart()).Times(0);
    task->Start();
  }

  // This should destroy |task_pump|, but |task| still shouldn't run.
  base::RunLoop().RunUntilIdle();
}

}  // namespace notifier
