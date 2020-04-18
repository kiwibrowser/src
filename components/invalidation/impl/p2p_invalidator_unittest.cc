// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/p2p_invalidator.h"

#include <stddef.h>

#include <cstddef>
#include <memory>

#include "base/memory/ptr_util.h"
#include "components/invalidation/impl/fake_invalidation_handler.h"
#include "components/invalidation/impl/invalidator_test_template.h"
#include "components/invalidation/impl/notifier_reason_util.h"
#include "components/invalidation/public/invalidator_state.h"
#include "jingle/notifier/listener/fake_push_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

class P2PInvalidatorTestDelegate {
 public:
  P2PInvalidatorTestDelegate() : fake_push_client_(nullptr) {}

  ~P2PInvalidatorTestDelegate() {
    DestroyInvalidator();
  }

  void CreateInvalidator(
      const std::string& invalidator_client_id,
      const std::string& initial_state,
      const base::WeakPtr<InvalidationStateTracker>&
          invalidation_state_tracker) {
    DCHECK(!fake_push_client_);
    DCHECK(!invalidator_);
    fake_push_client_ = new notifier::FakePushClient();
    invalidator_.reset(new P2PInvalidator(base::WrapUnique(fake_push_client_),
                                          invalidator_client_id,
                                          NOTIFY_OTHERS));
  }

  P2PInvalidator* GetInvalidator() {
    return invalidator_.get();
  }

  notifier::FakePushClient* GetPushClient() {
    return fake_push_client_;
  }

  void DestroyInvalidator() {
    invalidator_.reset();
    fake_push_client_ = nullptr;
  }

  void WaitForInvalidator() {
    // Do Nothing.
  }

  void TriggerOnInvalidatorStateChange(InvalidatorState state) {
    if (state == INVALIDATIONS_ENABLED) {
      fake_push_client_->EnableNotifications();
    } else {
      fake_push_client_->DisableNotifications(ToNotifierReasonForTest(state));
    }
  }

  void TriggerOnIncomingInvalidation(
      const ObjectIdInvalidationMap& invalidation_map) {
    const P2PNotificationData notification_data(
        std::string(), NOTIFY_ALL, invalidation_map);
    notifier::Notification notification;
    notification.channel = kSyncP2PNotificationChannel;
    notification.data = notification_data.ToString();
    fake_push_client_->SimulateIncomingNotification(notification);
  }

 private:
  // Owned by |invalidator_|.
  notifier::FakePushClient* fake_push_client_;
  std::unique_ptr<P2PInvalidator> invalidator_;
};

class P2PInvalidatorTest : public testing::Test {
 protected:
  P2PInvalidatorTest()
      : next_sent_notification_to_reflect_(0) {
    default_enabled_ids_.insert(invalidation::ObjectId(10, "A"));
    default_enabled_ids_.insert(invalidation::ObjectId(10, "B"));
    delegate_.CreateInvalidator("sender",
                                "fake_state",
                                base::WeakPtr<InvalidationStateTracker>());
    delegate_.GetInvalidator()->RegisterHandler(&fake_handler_);
  }

  ~P2PInvalidatorTest() override {
    delegate_.GetInvalidator()->UnregisterHandler(&fake_handler_);
  }

  ObjectIdInvalidationMap MakeInvalidationMap(ObjectIdSet ids) {
    return ObjectIdInvalidationMap::InvalidateAll(ids);
  }

  // Simulate receiving all the notifications we sent out since last
  // time this was called.
  void ReflectSentNotifications() {
    const std::vector<notifier::Notification>& sent_notifications =
        delegate_.GetPushClient()->sent_notifications();
    for(size_t i = next_sent_notification_to_reflect_;
        i < sent_notifications.size(); ++i) {
      delegate_.GetInvalidator()->OnIncomingNotification(sent_notifications[i]);
    }
    next_sent_notification_to_reflect_ = sent_notifications.size();
  }

  ObjectIdSet default_enabled_ids_;

  FakeInvalidationHandler fake_handler_;
  P2PInvalidatorTestDelegate delegate_;

 private:
  size_t next_sent_notification_to_reflect_;
};

// Make sure the P2PNotificationTarget <-> string conversions work.
TEST_F(P2PInvalidatorTest, P2PNotificationTarget) {
  for (int i = FIRST_NOTIFICATION_TARGET;
       i <= LAST_NOTIFICATION_TARGET; ++i) {
    P2PNotificationTarget target = static_cast<P2PNotificationTarget>(i);
    const std::string& target_str = P2PNotificationTargetToString(target);
    EXPECT_FALSE(target_str.empty());
    EXPECT_EQ(target, P2PNotificationTargetFromString(target_str));
  }
  EXPECT_EQ(NOTIFY_SELF, P2PNotificationTargetFromString("unknown"));
}

// Make sure notification targeting works correctly.
TEST_F(P2PInvalidatorTest, P2PNotificationDataIsTargeted) {
  {
    const P2PNotificationData notification_data(
        "sender", NOTIFY_SELF, ObjectIdInvalidationMap());
    EXPECT_TRUE(notification_data.IsTargeted("sender"));
    EXPECT_FALSE(notification_data.IsTargeted("other1"));
    EXPECT_FALSE(notification_data.IsTargeted("other2"));
  }
  {
    const P2PNotificationData notification_data(
        "sender", NOTIFY_OTHERS, ObjectIdInvalidationMap());
    EXPECT_FALSE(notification_data.IsTargeted("sender"));
    EXPECT_TRUE(notification_data.IsTargeted("other1"));
    EXPECT_TRUE(notification_data.IsTargeted("other2"));
  }
  {
    const P2PNotificationData notification_data(
        "sender", NOTIFY_ALL, ObjectIdInvalidationMap());
    EXPECT_TRUE(notification_data.IsTargeted("sender"));
    EXPECT_TRUE(notification_data.IsTargeted("other1"));
    EXPECT_TRUE(notification_data.IsTargeted("other2"));
  }
}

// Make sure the P2PNotificationData <-> string conversions work for a
// default-constructed P2PNotificationData.
TEST_F(P2PInvalidatorTest, P2PNotificationDataDefault) {
  const P2PNotificationData notification_data;
  EXPECT_TRUE(notification_data.IsTargeted(std::string()));
  EXPECT_FALSE(notification_data.IsTargeted("other1"));
  EXPECT_FALSE(notification_data.IsTargeted("other2"));
  EXPECT_TRUE(notification_data.GetIdInvalidationMap().Empty());
  const std::string& notification_data_str = notification_data.ToString();
  EXPECT_EQ(
      "{\"invalidations\":[],\"notificationType\":\"notifySelf\","
      "\"senderId\":\"\"}", notification_data_str);

  P2PNotificationData notification_data_parsed;
  EXPECT_TRUE(notification_data_parsed.ResetFromString(notification_data_str));
  EXPECT_TRUE(notification_data.Equals(notification_data_parsed));
}

// Make sure the P2PNotificationData <-> string conversions work for a
// non-default-constructed P2PNotificationData.
TEST_F(P2PInvalidatorTest, P2PNotificationDataNonDefault) {
  ObjectIdInvalidationMap invalidation_map =
      ObjectIdInvalidationMap::InvalidateAll(default_enabled_ids_);
  const P2PNotificationData notification_data("sender",
                                              NOTIFY_ALL,
                                              invalidation_map);
  EXPECT_TRUE(notification_data.IsTargeted("sender"));
  EXPECT_TRUE(notification_data.IsTargeted("other1"));
  EXPECT_TRUE(notification_data.IsTargeted("other2"));
  EXPECT_EQ(invalidation_map, notification_data.GetIdInvalidationMap());
  const std::string& notification_data_str = notification_data.ToString();
  EXPECT_EQ(
      "{\"invalidations\":["
      "{\"isUnknownVersion\":true,"
      "\"objectId\":{\"name\":\"A\",\"source\":10}},"
      "{\"isUnknownVersion\":true,"
      "\"objectId\":{\"name\":\"B\",\"source\":10}}"
      "],\"notificationType\":\"notifyAll\","
      "\"senderId\":\"sender\"}",
      notification_data_str);

  P2PNotificationData notification_data_parsed;
  EXPECT_TRUE(notification_data_parsed.ResetFromString(notification_data_str));
  EXPECT_TRUE(notification_data.Equals(notification_data_parsed));
}

// Set up the P2PInvalidator, simulate a successful connection, and send
// a notification with the default target (NOTIFY_OTHERS).  The
// observer should receive only a notification from the call to
// UpdateEnabledTypes().
TEST_F(P2PInvalidatorTest, NotificationsBasic) {
  P2PInvalidator* const invalidator = delegate_.GetInvalidator();
  notifier::FakePushClient* const push_client = delegate_.GetPushClient();

  invalidator->UpdateRegisteredIds(&fake_handler_, default_enabled_ids_);

  const char kEmail[] = "foo@bar.com";
  const char kToken[] = "token";
  invalidator->UpdateCredentials(kEmail, kToken);
  {
    notifier::Subscription expected_subscription;
    expected_subscription.channel = kSyncP2PNotificationChannel;
    expected_subscription.from = kEmail;
    EXPECT_TRUE(notifier::SubscriptionListsEqual(
        push_client->subscriptions(),
        notifier::SubscriptionList(1, expected_subscription)));
  }
  EXPECT_EQ(kEmail, push_client->email());
  EXPECT_EQ(kToken, push_client->token());

  ReflectSentNotifications();
  push_client->EnableNotifications();
  EXPECT_EQ(INVALIDATIONS_ENABLED, fake_handler_.GetInvalidatorState());

  ReflectSentNotifications();
  EXPECT_EQ(1, fake_handler_.GetInvalidationCount());
  EXPECT_THAT(MakeInvalidationMap(default_enabled_ids_),
              Eq(fake_handler_.GetLastInvalidationMap()));

  // Sent with target NOTIFY_OTHERS so should not be propagated to
  // |fake_handler_|.
  invalidator->SendInvalidation(default_enabled_ids_);

  ReflectSentNotifications();
  EXPECT_EQ(1, fake_handler_.GetInvalidationCount());
}

// Set up the P2PInvalidator and send out notifications with various
// target settings.  The notifications received by the observer should
// be consistent with the target settings.
TEST_F(P2PInvalidatorTest, SendNotificationData) {
  ObjectIdSet enabled_ids;
  ObjectIdSet changed_ids;
  ObjectIdSet expected_ids;

  enabled_ids.insert(invalidation::ObjectId(20, "A"));
  enabled_ids.insert(invalidation::ObjectId(20, "B"));
  enabled_ids.insert(invalidation::ObjectId(20, "C"));

  changed_ids.insert(invalidation::ObjectId(20, "A"));
  changed_ids.insert(invalidation::ObjectId(20, "Z"));

  expected_ids.insert(invalidation::ObjectId(20, "A"));

  const ObjectIdInvalidationMap& invalidation_map =
      MakeInvalidationMap(changed_ids);

  P2PInvalidator* const invalidator = delegate_.GetInvalidator();
  notifier::FakePushClient* const push_client = delegate_.GetPushClient();

  invalidator->UpdateRegisteredIds(&fake_handler_, enabled_ids);

  invalidator->UpdateCredentials("foo@bar.com", "fake_token");

  ReflectSentNotifications();
  push_client->EnableNotifications();
  EXPECT_EQ(INVALIDATIONS_ENABLED, fake_handler_.GetInvalidatorState());

  ReflectSentNotifications();
  EXPECT_EQ(1, fake_handler_.GetInvalidationCount());
  EXPECT_EQ(enabled_ids, fake_handler_.GetLastInvalidationMap().GetObjectIds());

  // Should be dropped.
  invalidator->SendNotificationDataForTest(P2PNotificationData());
  ReflectSentNotifications();
  EXPECT_EQ(1, fake_handler_.GetInvalidationCount());

  // Should be propagated.
  invalidator->SendNotificationDataForTest(
      P2PNotificationData("sender", NOTIFY_SELF, invalidation_map));
  ReflectSentNotifications();
  EXPECT_EQ(2, fake_handler_.GetInvalidationCount());
  EXPECT_EQ(expected_ids,
            fake_handler_.GetLastInvalidationMap().GetObjectIds());

  // Should be dropped.
  invalidator->SendNotificationDataForTest(
      P2PNotificationData("sender2", NOTIFY_SELF, invalidation_map));
  ReflectSentNotifications();
  EXPECT_EQ(2, fake_handler_.GetInvalidationCount());

  // Should be dropped.
  invalidator->SendNotificationDataForTest(
      P2PNotificationData("sender", NOTIFY_SELF, ObjectIdInvalidationMap()));
  ReflectSentNotifications();
  EXPECT_EQ(2, fake_handler_.GetInvalidationCount());

  // Should be dropped.
  invalidator->SendNotificationDataForTest(
      P2PNotificationData("sender", NOTIFY_OTHERS, invalidation_map));
  ReflectSentNotifications();
  EXPECT_EQ(2, fake_handler_.GetInvalidationCount());

  // Should be propagated.
  invalidator->SendNotificationDataForTest(
      P2PNotificationData("sender2", NOTIFY_OTHERS, invalidation_map));
  ReflectSentNotifications();
  EXPECT_EQ(3, fake_handler_.GetInvalidationCount());
  EXPECT_EQ(expected_ids,
            fake_handler_.GetLastInvalidationMap().GetObjectIds());

  // Should be dropped.
  invalidator->SendNotificationDataForTest(
      P2PNotificationData("sender2", NOTIFY_OTHERS, ObjectIdInvalidationMap()));
  ReflectSentNotifications();
  EXPECT_EQ(3, fake_handler_.GetInvalidationCount());

  // Should be propagated.
  invalidator->SendNotificationDataForTest(
      P2PNotificationData("sender", NOTIFY_ALL, invalidation_map));
  ReflectSentNotifications();
  EXPECT_EQ(4, fake_handler_.GetInvalidationCount());
  EXPECT_EQ(expected_ids,
            fake_handler_.GetLastInvalidationMap().GetObjectIds());

  // Should be propagated.
  invalidator->SendNotificationDataForTest(
      P2PNotificationData("sender2", NOTIFY_ALL, invalidation_map));
  ReflectSentNotifications();
  EXPECT_EQ(5, fake_handler_.GetInvalidationCount());
  EXPECT_EQ(expected_ids,
            fake_handler_.GetLastInvalidationMap().GetObjectIds());

  // Should be dropped.
  invalidator->SendNotificationDataForTest(
  P2PNotificationData("sender2", NOTIFY_ALL, ObjectIdInvalidationMap()));
  ReflectSentNotifications();
  EXPECT_EQ(5, fake_handler_.GetInvalidationCount());
}

INSTANTIATE_TYPED_TEST_CASE_P(
    P2PInvalidatorTest, InvalidatorTest,
    P2PInvalidatorTestDelegate);

}  // namespace

}  // namespace syncer
