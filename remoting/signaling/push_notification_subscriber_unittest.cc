// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/signaling/push_notification_subscriber.h"

#include "remoting/signaling/mock_signal_strategy.h"
#include "remoting/signaling/signaling_address.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;
using testing::AtLeast;
using testing::DoAll;
using testing::Return;
using testing::SaveArg;

namespace remoting {

TEST(PushNotificationSubscriberTest, Create) {
  MockSignalStrategy signal_strategy(SignalingAddress("user@domain/resource"));
  EXPECT_CALL(signal_strategy, AddListener(_));
  EXPECT_CALL(signal_strategy, RemoveListener(_));

  PushNotificationSubscriber::SubscriptionList subscriptions;
  PushNotificationSubscriber subscriber(&signal_strategy, subscriptions);
}

TEST(PushNotificationSubscriberTest, Subscribe) {
  MockSignalStrategy signal_strategy(SignalingAddress("user@domain/resource"));
  EXPECT_CALL(signal_strategy, GetNextId()).WillOnce(Return("next_id"));
  EXPECT_CALL(signal_strategy, AddListener(_)).Times(AtLeast(1));
  EXPECT_CALL(signal_strategy, RemoveListener(_)).Times(AtLeast(1));
  buzz::XmlElement* sent_stanza;
  EXPECT_CALL(signal_strategy, SendStanzaPtr(_))
      .WillOnce(DoAll(SaveArg<0>(&sent_stanza), Return(true)));

  PushNotificationSubscriber::Subscription subscription;
  subscription.channel = "sub_channel";
  subscription.from = "sub_from";
  PushNotificationSubscriber::SubscriptionList subscriptions;
  subscriptions.push_back(subscription);
  PushNotificationSubscriber subscriber(&signal_strategy, subscriptions);
  SignalStrategy::Listener* listener = &subscriber;
  listener->OnSignalStrategyStateChange(SignalStrategy::CONNECTED);

  EXPECT_EQ(
      "<cli:iq type=\"set\" to=\"user@domain\" id=\"next_id\""
      " xmlns:cli=\"jabber:client\">"
      "<push:subscribe xmlns:push=\"google:push\">"
      "<push:item channel=\"sub_channel\" from=\"sub_from\"/>"
      "</push:subscribe>"
      "</cli:iq>",
      sent_stanza->Str());

  delete sent_stanza;
}

}  // namespace remoting
