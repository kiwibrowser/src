// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/authenticated_channel_impl.h"

#include <iterator>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/stl_util.h"
#include "base/test/scoped_task_environment.h"
#include "chromeos/services/secure_channel/fake_authenticated_channel.h"
#include "chromeos/services/secure_channel/public/mojom/secure_channel.mojom.h"
#include "components/cryptauth/fake_connection.h"
#include "components/cryptauth/fake_secure_channel.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace secure_channel {

const mojom::ConnectionCreationDetail kTestConnectionCreationDetails[] = {
    mojom::ConnectionCreationDetail::
        REMOTE_DEVICE_USED_BACKGROUND_BLE_ADVERTISING};

class SecureChannelAuthenticatedChannelImplTest : public testing::Test {
 protected:
  SecureChannelAuthenticatedChannelImplTest()
      : test_device_(cryptauth::CreateRemoteDeviceRefForTest()) {}

  ~SecureChannelAuthenticatedChannelImplTest() override = default;

  void SetUp() override {
    auto fake_secure_channel = std::make_unique<cryptauth::FakeSecureChannel>(
        std::make_unique<cryptauth::FakeConnection>(test_device_),
        nullptr /* cryptauth_service */);
    fake_secure_channel->ChangeStatus(
        cryptauth::SecureChannel::Status::AUTHENTICATED);
    fake_secure_channel_ = fake_secure_channel.get();

    channel_ = AuthenticatedChannelImpl::Factory::Get()->BuildInstance(
        std::vector<mojom::ConnectionCreationDetail>(
            std::begin(kTestConnectionCreationDetails),
            std::end(kTestConnectionCreationDetails)),
        std::move(fake_secure_channel));

    test_observer_ = std::make_unique<FakeAuthenticatedChannelObserver>();
    channel_->AddObserver(test_observer_.get());
  }

  void TearDown() override { channel_->RemoveObserver(test_observer_.get()); }

  // Returns the sequence number for this SendMessageAndVerifyResults() call.
  // To determine if the message has finished being sent, use
  // HasMessageBeenSent(). If |expected_to_succeed| is false, -1 is returned.
  int SendMessageAndVerifyResults(const std::string& feature,
                                  const std::string& payload,
                                  bool expected_to_succeed = true) {
    size_t num_sent_messages_before_call =
        fake_secure_channel_->sent_messages().size();

    // Note: This relies on an implicit assumption that FakeSecureChannel starts
    // its counter at 0. If that ever changes, this test needs to be updated.
    int sequence_number = num_times_send_message_called_++;

    bool success = channel_->SendMessage(
        feature, payload,
        base::BindOnce(
            &SecureChannelAuthenticatedChannelImplTest::OnMessageSent,
            base::Unretained(this), sequence_number));
    EXPECT_EQ(expected_to_succeed, success);

    if (!expected_to_succeed)
      return -1;

    std::vector<cryptauth::FakeSecureChannel::SentMessage> sent_messages =
        fake_secure_channel_->sent_messages();
    EXPECT_EQ(num_sent_messages_before_call + 1u, sent_messages.size());
    EXPECT_EQ(feature, sent_messages.back().feature);
    EXPECT_EQ(payload, sent_messages.back().payload);

    return sequence_number;
  }

  bool HasMessageBeenSent(int sequence_number) {
    // -1 is returned by SendMessageAndVerifyResults() when
    // |expected_to_succeed| is false.
    EXPECT_NE(-1, sequence_number);
    return base::ContainsKey(sent_sequence_numbers_, sequence_number);
  }

  cryptauth::FakeSecureChannel* fake_secure_channel() {
    return fake_secure_channel_;
  }

  FakeAuthenticatedChannelObserver* test_observer() {
    return test_observer_.get();
  }

  AuthenticatedChannel* channel() { return channel_.get(); }

 private:
  void OnMessageSent(int sequence_number) {
    sent_sequence_numbers_.insert(sequence_number);
  }

  const base::test::ScopedTaskEnvironment scoped_task_environment_;
  const cryptauth::RemoteDeviceRef test_device_;

  int num_times_send_message_called_ = 0;

  std::unordered_set<int> sent_sequence_numbers_;

  cryptauth::FakeSecureChannel* fake_secure_channel_;
  std::unique_ptr<FakeAuthenticatedChannelObserver> test_observer_;

  std::unique_ptr<AuthenticatedChannel> channel_;

  DISALLOW_COPY_AND_ASSIGN(SecureChannelAuthenticatedChannelImplTest);
};

TEST_F(SecureChannelAuthenticatedChannelImplTest, ConnectionMetadata) {
  const auto& connection_metadata = channel()->GetConnectionMetadata();
  EXPECT_EQ(std::vector<mojom::ConnectionCreationDetail>(
                std::begin(kTestConnectionCreationDetails),
                std::end(kTestConnectionCreationDetails)),
            connection_metadata.creation_details);
  // TODO(khorimoto): Update test to test RSSI rolling average when implemented.
  // https://crbug.com/844759.
  EXPECT_EQ(mojom::ConnectionMetadata::kNoRssiAvailable,
            connection_metadata.rssi_rolling_average);
}

TEST_F(SecureChannelAuthenticatedChannelImplTest, DisconnectRequestFromClient) {
  // Call Disconnect(). The underlying SecureChannel should have started
  // the disconnection process but not yet finished it.
  channel()->Disconnect();
  EXPECT_FALSE(test_observer()->has_been_notified_of_disconnection());

  // Complete the disconnection process.
  fake_secure_channel()->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  EXPECT_TRUE(test_observer()->has_been_notified_of_disconnection());
}

TEST_F(SecureChannelAuthenticatedChannelImplTest,
       SendReceiveAndDisconnect_RemoteDeviceDisconnects) {
  const auto& received_messages = test_observer()->received_messages();

  int sequence_number_1 = SendMessageAndVerifyResults("feature1", "payload1");
  EXPECT_FALSE(HasMessageBeenSent(sequence_number_1));
  fake_secure_channel()->CompleteSendingMessage(sequence_number_1);
  EXPECT_TRUE(HasMessageBeenSent(sequence_number_1));

  fake_secure_channel()->ReceiveMessage("feature1", "payload2");
  EXPECT_EQ(1u, received_messages.size());
  EXPECT_EQ("feature1", received_messages[0].first);
  EXPECT_EQ("payload2", received_messages[0].second);

  int sequence_number_2 = SendMessageAndVerifyResults("feature1", "payload3");
  EXPECT_FALSE(HasMessageBeenSent(sequence_number_2));
  fake_secure_channel()->CompleteSendingMessage(sequence_number_2);
  EXPECT_TRUE(HasMessageBeenSent(sequence_number_2));

  fake_secure_channel()->ReceiveMessage("feature1", "payload4");
  EXPECT_EQ(2u, received_messages.size());
  EXPECT_EQ("feature1", received_messages[1].first);
  EXPECT_EQ("payload4", received_messages[1].second);

  EXPECT_FALSE(test_observer()->has_been_notified_of_disconnection());
  fake_secure_channel()->ChangeStatus(
      cryptauth::SecureChannel::Status::DISCONNECTED);
  EXPECT_TRUE(test_observer()->has_been_notified_of_disconnection());

  SendMessageAndVerifyResults("feature1", "payload5",
                              false /* expected_to_succeed */);
}

TEST_F(SecureChannelAuthenticatedChannelImplTest, SendReceive_Async) {
  const auto& received_messages = test_observer()->received_messages();

  // Start sending a message, but do not complete it.
  int sequence_number_1 = SendMessageAndVerifyResults("feature1", "payload1");
  EXPECT_FALSE(HasMessageBeenSent(sequence_number_1));

  // Receive a message for a different feature.
  fake_secure_channel()->ReceiveMessage("feature2", "payload2");
  EXPECT_EQ(1u, received_messages.size());
  EXPECT_EQ("feature2", received_messages[0].first);
  EXPECT_EQ("payload2", received_messages[0].second);

  // Finish sending the first message.
  EXPECT_FALSE(HasMessageBeenSent(sequence_number_1));
  fake_secure_channel()->CompleteSendingMessage(sequence_number_1);
  EXPECT_TRUE(HasMessageBeenSent(sequence_number_1));

  // Start sending a second message for a different feature, but do not complete
  // it.
  int sequence_number_2 = SendMessageAndVerifyResults("feature3", "payload3");
  EXPECT_FALSE(HasMessageBeenSent(sequence_number_2));

  // Receive a message for yet another feature.
  fake_secure_channel()->ReceiveMessage("feature4", "payload4");
  EXPECT_EQ(2u, received_messages.size());
  EXPECT_EQ("feature4", received_messages[1].first);
  EXPECT_EQ("payload4", received_messages[1].second);

  // Finish sending the second message.
  EXPECT_FALSE(HasMessageBeenSent(sequence_number_2));
  fake_secure_channel()->CompleteSendingMessage(sequence_number_2);
  EXPECT_TRUE(HasMessageBeenSent(sequence_number_2));
}

}  // namespace secure_channel

}  // namespace chromeos
