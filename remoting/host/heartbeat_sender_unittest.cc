// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/heartbeat_sender.h"

#include <set>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/mock_callback.h"
#include "base/test/test_mock_time_task_runner.h"
#include "remoting/base/constants.h"
#include "remoting/base/rsa_key_pair.h"
#include "remoting/base/test_rsa_key_pair.h"
#include "remoting/signaling/fake_signal_strategy.h"
#include "remoting/signaling/iq_sender.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmpp/constants.h"

using buzz::QName;
using buzz::XmlElement;

using testing::_;
using testing::DeleteArg;
using testing::DoAll;
using testing::Invoke;
using testing::NotNull;
using testing::Return;
using testing::SaveArg;

namespace remoting {

namespace {

const char kTestBotJid[] = "remotingunittest@bot.talk.google.com";
const char kHostId[] = "0";
const char kTestJid[] = "User@gmail.com/chromotingABC123";
const char kTestJidNormalized[] = "user@gmail.com/chromotingABC123";
constexpr base::TimeDelta kTestInterval = base::TimeDelta::FromSeconds(123);
constexpr base::TimeDelta kOfflineReasonTimeout =
    base::TimeDelta::FromSeconds(123);

}  // namespace

class HeartbeatSenderTest
    : public testing::Test {
 protected:
  HeartbeatSenderTest()
      : signal_strategy_(SignalingAddress(kTestJid)),
        bot_signal_strategy_(SignalingAddress(kTestBotJid)) {}

  void SetUp() override {
    FakeSignalStrategy::Connect(&signal_strategy_, &bot_signal_strategy_);

    // Start in disconnected state.
    signal_strategy_.Disconnect();

    key_pair_ = RsaKeyPair::FromString(kTestRsaKeyPair);
    ASSERT_TRUE(key_pair_.get());

    EXPECT_CALL(mock_unknown_host_id_error_callback_, Run()).Times(0);

    heartbeat_sender_.reset(
        new HeartbeatSender(mock_heartbeat_successful_callback_.Get(),
                            mock_unknown_host_id_error_callback_.Get(), kHostId,
                            &signal_strategy_, key_pair_, kTestBotJid));
  }

  void TearDown() override {
    heartbeat_sender_.reset();
    base::RunLoop().RunUntilIdle();
  }

  void ValidateHeartbeatStanza(XmlElement* stanza,
                               const std::string& expected_sequence_id,
                               const std::string& expected_host_offline_reason);

  void SendResponse(int message_index,
                    base::TimeDelta interval = base::TimeDelta(),
                    int expected_sequence_id = 0);

  scoped_refptr<base::TestMockTimeTaskRunner> fake_time_task_runner_ =
      base::MakeRefCounted<base::TestMockTimeTaskRunner>(
          base::TestMockTimeTaskRunner::Type::kBoundToThread);

  FakeSignalStrategy signal_strategy_;
  FakeSignalStrategy bot_signal_strategy_;

  base::MockCallback<base::Closure> mock_heartbeat_successful_callback_;
  base::MockCallback<base::Closure> mock_unknown_host_id_error_callback_;
  scoped_refptr<RsaKeyPair> key_pair_;
  std::unique_ptr<HeartbeatSender> heartbeat_sender_;
};

void HeartbeatSenderTest::ValidateHeartbeatStanza(
    XmlElement* stanza,
    const std::string& expected_sequence_id,
    const std::string& expected_host_offline_reason) {
  EXPECT_EQ(stanza->Attr(buzz::QName(std::string(), "to")),
            std::string(kTestBotJid));
  EXPECT_EQ(stanza->Attr(buzz::QName(std::string(), "type")), "set");
  XmlElement* heartbeat_stanza =
      stanza->FirstNamed(QName(kChromotingXmlNamespace, "heartbeat"));
  ASSERT_TRUE(heartbeat_stanza != nullptr);
  EXPECT_EQ(expected_sequence_id, heartbeat_stanza->Attr(buzz::QName(
                                      kChromotingXmlNamespace, "sequence-id")));
  if (expected_host_offline_reason.empty()) {
    EXPECT_FALSE(heartbeat_stanza->HasAttr(
        buzz::QName(kChromotingXmlNamespace, "host-offline-reason")));
  } else {
    EXPECT_EQ(expected_host_offline_reason,
              heartbeat_stanza->Attr(
                  buzz::QName(kChromotingXmlNamespace, "host-offline-reason")));
  }
  EXPECT_EQ(std::string(kHostId),
            heartbeat_stanza->Attr(QName(kChromotingXmlNamespace, "hostid")));

  QName signature_tag(kChromotingXmlNamespace, "signature");
  XmlElement* signature = heartbeat_stanza->FirstNamed(signature_tag);
  ASSERT_TRUE(signature != nullptr);
  EXPECT_TRUE(heartbeat_stanza->NextNamed(signature_tag) == nullptr);

  scoped_refptr<RsaKeyPair> key_pair = RsaKeyPair::FromString(kTestRsaKeyPair);
  ASSERT_TRUE(key_pair.get());
  std::string expected_signature = key_pair->SignMessage(
      std::string(kTestJidNormalized) + ' ' + expected_sequence_id);
  EXPECT_EQ(expected_signature, signature->BodyText());
}

void HeartbeatSenderTest::SendResponse(int message_index,
                                       base::TimeDelta interval,
                                       int expected_sequence_id) {
  auto response = std::make_unique<XmlElement>(buzz::QN_IQ);
  response->AddAttr(QName(std::string(), "type"), "result");
  response->AddAttr(QName(std::string(), "to"), kTestJid);

  ASSERT_LT(message_index,
            static_cast<int>(bot_signal_strategy_.received_messages().size()));
  std::string iq_id =
      bot_signal_strategy_.received_messages()[message_index]->Attr(
          QName(std::string(), "id"));

  response->SetAttr(QName(std::string(), "id"), iq_id);

  XmlElement* result =
      new XmlElement(QName(kChromotingXmlNamespace, "heartbeat-result"));
  response->AddElement(result);

  if (interval != base::TimeDelta()) {
    XmlElement* set_interval =
        new XmlElement(QName(kChromotingXmlNamespace, "set-interval"));
    result->AddElement(set_interval);
    set_interval->AddText(base::IntToString(interval.InSeconds()));
  }

  if (expected_sequence_id > 0) {
    XmlElement* expected_sequence_id_tag =
        new XmlElement(QName(kChromotingXmlNamespace, "expected-sequence-id"));
    result->AddElement(expected_sequence_id_tag);
    expected_sequence_id_tag->AddText(base::IntToString(expected_sequence_id));
  }

  bot_signal_strategy_.SendStanza(std::move(response));
  base::RunLoop().RunUntilIdle();
}

TEST_F(HeartbeatSenderTest, SendHeartbeat) {
  signal_strategy_.Connect();
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(bot_signal_strategy_.received_messages().size(), 1U);
  ValidateHeartbeatStanza(bot_signal_strategy_.received_messages()[0].get(),
                          "0", std::string());

  EXPECT_CALL(mock_heartbeat_successful_callback_, Run());
  SendResponse(0);

  signal_strategy_.Disconnect();
}

// Ensure a new heartbeat is sent after signaling is connected.
TEST_F(HeartbeatSenderTest, AfterSignalingReconnect) {
  signal_strategy_.Connect();
  base::RunLoop().RunUntilIdle();

  signal_strategy_.Disconnect();

  signal_strategy_.Connect();
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(bot_signal_strategy_.received_messages().size(), 2U);
  ValidateHeartbeatStanza(bot_signal_strategy_.received_messages()[0].get(),
                          "0", std::string());
  ValidateHeartbeatStanza(bot_signal_strategy_.received_messages()[1].get(),
                          "1", std::string());

  signal_strategy_.Disconnect();
}

// Verify that a second heartbeat is sent immediately after a response from
// server with expected-sequence-id field set.
TEST_F(HeartbeatSenderTest, ExpectedSequenceId) {
  signal_strategy_.Connect();
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(bot_signal_strategy_.received_messages().size(), 1U);
  ValidateHeartbeatStanza(bot_signal_strategy_.received_messages()[0].get(),
                          "0", std::string());

  const int kExpectedSequenceId = 456;
  SendResponse(0, base::TimeDelta(), kExpectedSequenceId);

  ASSERT_EQ(bot_signal_strategy_.received_messages().size(), 2U);
  ValidateHeartbeatStanza(bot_signal_strategy_.received_messages()[1].get(),
                          base::IntToString(kExpectedSequenceId).c_str(),
                          std::string());

  signal_strategy_.Disconnect();
}

// Verify that ProcessResponse parses set-interval result.
TEST_F(HeartbeatSenderTest, SetInterval) {
  signal_strategy_.Connect();
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(bot_signal_strategy_.received_messages().size(), 1U);

  EXPECT_CALL(mock_heartbeat_successful_callback_, Run());
  SendResponse(0, kTestInterval);
  EXPECT_EQ(kTestInterval, heartbeat_sender_->interval_);
}

// Make sure SetHostOfflineReason sends a correct stanza and triggers callback
// when the bot responds.
TEST_F(HeartbeatSenderTest, SetHostOfflineReason) {
  base::MockCallback<base::Callback<void(bool success)>> mock_ack_callback;
  EXPECT_CALL(mock_ack_callback, Run(_)).Times(0);

  heartbeat_sender_->SetHostOfflineReason("test_error", kOfflineReasonTimeout,
                                          mock_ack_callback.Get());

  testing::Mock::VerifyAndClearExpectations(&mock_ack_callback);

  signal_strategy_.Connect();
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(bot_signal_strategy_.received_messages().size(), 1U);
  ValidateHeartbeatStanza(bot_signal_strategy_.received_messages()[0].get(),
                          "0", "test_error");

  // Callback should run once, when we get response to offline-reason.
  EXPECT_CALL(mock_ack_callback, Run(_)).Times(1);
  EXPECT_CALL(mock_heartbeat_successful_callback_, Run());
  SendResponse(0);
}

// The first heartbeat should include host OS information.
TEST_F(HeartbeatSenderTest, HostOsInfo) {
  signal_strategy_.Connect();
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(bot_signal_strategy_.received_messages().size(), 1U);
  XmlElement* stanza = bot_signal_strategy_.received_messages()[0].get();

  XmlElement* heartbeat_stanza =
      stanza->FirstNamed(QName(kChromotingXmlNamespace, "heartbeat"));

  std::string host_os_name = heartbeat_stanza->TextNamed(
      QName(kChromotingXmlNamespace, "host-os-name"));
  EXPECT_TRUE(!host_os_name.empty());

  std::string host_os_version = heartbeat_stanza->TextNamed(
      QName(kChromotingXmlNamespace, "host-os-version"));
  EXPECT_TRUE(!host_os_version.empty());

  signal_strategy_.Disconnect();
}

TEST_F(HeartbeatSenderTest, ResponseTimeout) {
  signal_strategy_.Connect();
  base::RunLoop().RunUntilIdle();

  // Simulate heartbeat timeout.
  fake_time_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(1));

  // SignalStrategy should be disconnected in response to the second timeout.
  fake_time_task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(1));

  EXPECT_EQ(SignalStrategy::DISCONNECTED, signal_strategy_.GetState());
}

}  // namespace remoting
