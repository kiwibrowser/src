// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/secure_channel/single_client_message_proxy_impl.h"

#include <memory>
#include <string>
#include <unordered_set>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/test/scoped_task_environment.h"
#include "chromeos/services/secure_channel/client_connection_parameters.h"
#include "chromeos/services/secure_channel/fake_connection_delegate.h"
#include "chromeos/services/secure_channel/fake_message_receiver.h"
#include "chromeos/services/secure_channel/fake_single_client_message_proxy.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace secure_channel {

namespace {
const char kTestFeature[] = "testFeature";
}  // namespace

class SecureChannelSingleClientMessageProxyImplTest : public testing::Test {
 protected:
  SecureChannelSingleClientMessageProxyImplTest() = default;
  ~SecureChannelSingleClientMessageProxyImplTest() override = default;

  void SetUp() override {
    fake_proxy_delegate_ =
        std::make_unique<FakeSingleClientMessageProxyDelegate>();

    auto fake_message_receiver = std::make_unique<FakeMessageReceiver>();
    fake_message_receiver_ = fake_message_receiver.get();

    fake_connection_delegate_ = std::make_unique<FakeConnectionDelegate>();
    fake_connection_delegate_->set_message_receiver(
        std::move(fake_message_receiver));

    proxy_ = SingleClientMessageProxyImpl::Factory::Get()->BuildInstance(
        fake_proxy_delegate_.get(),
        ClientConnectionParameters(
            kTestFeature, fake_connection_delegate_->GenerateInterfacePtr()));

    CompletePendingMojoCalls();
    EXPECT_TRUE(fake_connection_delegate_->channel());
  }

  void CompletePendingMojoCalls() {
    // FlushForTesting is a function on SingleClientMessageProxyImpl, so a cast
    // is necessary.
    auto* proxy = static_cast<SingleClientMessageProxyImpl*>(proxy_.get());
    proxy->FlushForTesting();
  }

  void TearDown() override {}

  // If |complete_sending| is true, the "on sent" callback is invoked.
  int SendMessageAndVerifyState(const std::string& message,
                                bool complete_sending = true) {
    auto& send_message_requests =
        fake_proxy_delegate()->send_message_requests();
    size_t num_send_message_requests_before_call = send_message_requests.size();

    int message_counter = next_message_counter_++;

    mojom::ChannelPtr& channel = fake_connection_delegate_->channel();
    channel->SendMessage(
        message,
        base::BindOnce(
            &SecureChannelSingleClientMessageProxyImplTest::OnMessageSent,
            base::Unretained(this), message_counter));
    channel.FlushForTesting();

    EXPECT_EQ(num_send_message_requests_before_call + 1u,
              send_message_requests.size());
    EXPECT_EQ(kTestFeature, std::get<0>(send_message_requests.back()));
    EXPECT_EQ(message, std::get<1>(send_message_requests.back()));
    EXPECT_FALSE(WasMessageSent(message_counter));

    if (complete_sending) {
      std::move(std::get<2>(send_message_requests.back())).Run();
      CompletePendingMojoCalls();
      EXPECT_TRUE(WasMessageSent(message_counter));
    }

    return message_counter;
  }

  void HandleReceivedMessageAndVerifyState(const std::string& feature,
                                           const std::string& payload) {
    const std::vector<std::string>& received_messages =
        fake_message_receiver_->received_messages();
    size_t num_received_messages_before_call = received_messages.size();

    proxy_->HandleReceivedMessage(feature, payload);
    CompletePendingMojoCalls();

    // If message's feature was not the type specified by the client, no
    // additional message should have been passed to |fake_message_receiver_|.
    if (feature != kTestFeature) {
      EXPECT_EQ(num_received_messages_before_call, received_messages.size());
      return;
    }

    // Otherwise, a message should have been passed.
    EXPECT_EQ(num_received_messages_before_call + 1u, received_messages.size());
    EXPECT_EQ(payload, received_messages.back());
  }

  FakeSingleClientMessageProxyDelegate* fake_proxy_delegate() {
    return fake_proxy_delegate_.get();
  }

  FakeMessageReceiver* fake_message_receiver() {
    return fake_message_receiver_;
  }

  FakeConnectionDelegate* fake_connection_delegate() {
    return fake_connection_delegate_.get();
  }

  bool WasMessageSent(int message_counter) {
    return base::ContainsKey(sent_message_counters_, message_counter);
  }

  void DisconnectFromClientSide() {
    EXPECT_FALSE(WasDelegateNotifiedOfDisconnection());

    base::RunLoop run_loop;
    fake_proxy_delegate_->set_on_client_disconnected_closure(
        run_loop.QuitClosure());
    fake_connection_delegate_->channel().reset();
    run_loop.Run();

    EXPECT_TRUE(WasDelegateNotifiedOfDisconnection());
  }

  void DisconnectFromRemoteDeviceSide() {
    EXPECT_TRUE(fake_connection_delegate()->channel());

    proxy_->HandleRemoteDeviceDisconnection();
    CompletePendingMojoCalls();

    EXPECT_FALSE(fake_connection_delegate()->channel());
    EXPECT_EQ(static_cast<uint32_t>(mojom::Channel::kConnectionDroppedReason),
              fake_connection_delegate()->disconnection_reason());
  }

  bool WasDelegateNotifiedOfDisconnection() {
    return proxy_->GetProxyId() ==
           fake_proxy_delegate_->disconnected_proxy_id();
  }

  const mojom::ConnectionMetadata& GetConnectionMetadataFromChannel() {
    mojom::ChannelPtr& channel = fake_connection_delegate_->channel();
    channel->GetConnectionMetadata(base::BindOnce(
        &SecureChannelSingleClientMessageProxyImplTest::OnConnectionMetadata,
        base::Unretained(this)));
    channel.FlushForTesting();
    return last_metadata_from_channel_;
  }

 private:
  void OnMessageSent(int message_counter) {
    sent_message_counters_.insert(message_counter);
  }

  void OnConnectionMetadata(
      mojom::ConnectionMetadataPtr connection_metadata_ptr) {
    last_metadata_from_channel_ = *connection_metadata_ptr;
  }

  const base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<FakeSingleClientMessageProxyDelegate> fake_proxy_delegate_;
  std::unique_ptr<FakeConnectionDelegate> fake_connection_delegate_;
  FakeMessageReceiver* fake_message_receiver_;

  int next_message_counter_ = 0;
  std::unordered_set<int> sent_message_counters_;

  mojom::ConnectionMetadata last_metadata_from_channel_;

  std::unique_ptr<SingleClientMessageProxy> proxy_;

  DISALLOW_COPY_AND_ASSIGN(SecureChannelSingleClientMessageProxyImplTest);
};

TEST_F(SecureChannelSingleClientMessageProxyImplTest,
       SendReceiveAndDisconnect_ClientDisconnection) {
  SendMessageAndVerifyState("message1");
  HandleReceivedMessageAndVerifyState(kTestFeature, "message2");
  DisconnectFromClientSide();
}

TEST_F(SecureChannelSingleClientMessageProxyImplTest,
       SendReceiveAndDisconnect_RemoteDeviceDisconnection) {
  SendMessageAndVerifyState("message1");
  HandleReceivedMessageAndVerifyState(kTestFeature, "message2");

  DisconnectFromRemoteDeviceSide();
}

TEST_F(SecureChannelSingleClientMessageProxyImplTest,
       SendWithDeferredCompletion) {
  auto& send_message_requests = fake_proxy_delegate()->send_message_requests();

  // Send two messages, but do not wait for the first to send successfully
  // before sending the second one.
  int counter1 =
      SendMessageAndVerifyState("message1", false /* complete_sending */);
  int counter2 =
      SendMessageAndVerifyState("message2", false /* complete_sending */);
  EXPECT_EQ(2u, send_message_requests.size());
  EXPECT_FALSE(WasMessageSent(counter1));
  EXPECT_FALSE(WasMessageSent(counter2));

  // Complete sending the first message.
  std::move(std::get<2>(send_message_requests[0])).Run();
  CompletePendingMojoCalls();
  EXPECT_TRUE(WasMessageSent(counter1));

  // Before the second one completes, disconnect from the remote side.
  DisconnectFromRemoteDeviceSide();
}

TEST_F(SecureChannelSingleClientMessageProxyImplTest,
       ReceiveMessagesFromMultipleFeatures) {
  HandleReceivedMessageAndVerifyState(kTestFeature, "message1");
  HandleReceivedMessageAndVerifyState("otherFeature", "message2");
  DisconnectFromRemoteDeviceSide();
}

TEST_F(SecureChannelSingleClientMessageProxyImplTest, ConnectionMetadata) {
  mojom::ConnectionMetadata metadata;
  fake_proxy_delegate()->set_connection_metadata(metadata);
  EXPECT_TRUE(metadata.Equals(GetConnectionMetadataFromChannel()));

  metadata.rssi_rolling_average = -5.5f;
  metadata.creation_details.push_back(
      mojom::ConnectionCreationDetail::
          REMOTE_DEVICE_USED_BACKGROUND_BLE_ADVERTISING);
  fake_proxy_delegate()->set_connection_metadata(metadata);
  EXPECT_TRUE(metadata.Equals(GetConnectionMetadataFromChannel()));
}

}  // namespace secure_channel

}  // namespace chromeos
