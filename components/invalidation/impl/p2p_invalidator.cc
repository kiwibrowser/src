// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/p2p_invalidator.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "components/invalidation/impl/notifier_reason_util.h"
#include "components/invalidation/public/invalidation_handler.h"
#include "components/invalidation/public/invalidation_util.h"
#include "components/invalidation/public/object_id_invalidation_map.h"
#include "jingle/notifier/listener/push_client.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace syncer {

const char kSyncP2PNotificationChannel[] = "http://www.google.com/chrome/sync";

namespace {

const char kNotifySelf[] = "notifySelf";
const char kNotifyOthers[] = "notifyOthers";
const char kNotifyAll[] = "notifyAll";

const char kSenderIdKey[] = "senderId";
const char kNotificationTypeKey[] = "notificationType";
const char kInvalidationsKey[] = "invalidations";

}  // namespace

std::string P2PNotificationTargetToString(P2PNotificationTarget target) {
  switch (target) {
    case NOTIFY_SELF:
      return kNotifySelf;
    case NOTIFY_OTHERS:
      return kNotifyOthers;
    case NOTIFY_ALL:
      return kNotifyAll;
    default:
      NOTREACHED();
      return std::string();
  }
}

P2PNotificationTarget P2PNotificationTargetFromString(
    const std::string& target_str) {
  if (target_str == kNotifySelf) {
    return NOTIFY_SELF;
  }
  if (target_str == kNotifyOthers) {
    return NOTIFY_OTHERS;
  }
  if (target_str == kNotifyAll) {
    return NOTIFY_ALL;
  }
  LOG(WARNING) << "Could not parse " << target_str;
  return NOTIFY_SELF;
}

P2PNotificationData::P2PNotificationData()
    : target_(NOTIFY_SELF) {}

P2PNotificationData::P2PNotificationData(
    const std::string& sender_id,
    P2PNotificationTarget target,
    const ObjectIdInvalidationMap& invalidation_map)
    : sender_id_(sender_id),
      target_(target),
      invalidation_map_(invalidation_map) {}

P2PNotificationData::~P2PNotificationData() {}

bool P2PNotificationData::IsTargeted(const std::string& id) const {
  switch (target_) {
    case NOTIFY_SELF:
      return sender_id_ == id;
    case NOTIFY_OTHERS:
      return sender_id_ != id;
    case NOTIFY_ALL:
      return true;
    default:
      NOTREACHED();
      return false;
  }
}

const ObjectIdInvalidationMap&
P2PNotificationData::GetIdInvalidationMap() const {
  return invalidation_map_;
}

bool P2PNotificationData::Equals(const P2PNotificationData& other) const {
  return
      (sender_id_ == other.sender_id_) &&
      (target_ == other.target_) &&
      (invalidation_map_ == other.invalidation_map_);
}

std::string P2PNotificationData::ToString() const {
  base::DictionaryValue dict;
  dict.SetString(kSenderIdKey, sender_id_);
  dict.SetString(kNotificationTypeKey, P2PNotificationTargetToString(target_));
  dict.Set(kInvalidationsKey, invalidation_map_.ToValue());
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

bool P2PNotificationData::ResetFromString(const std::string& str) {
  std::unique_ptr<base::Value> data_value = base::JSONReader::Read(str);
  const base::DictionaryValue* data_dict = nullptr;
  if (!data_value || !data_value->GetAsDictionary(&data_dict)) {
    LOG(WARNING) << "Could not parse " << str << " as a dictionary";
    return false;
  }
  if (!data_dict->GetString(kSenderIdKey, &sender_id_)) {
    LOG(WARNING) << "Could not find string value for " << kSenderIdKey;
  }
  std::string target_str;
  if (!data_dict->GetString(kNotificationTypeKey, &target_str)) {
    LOG(WARNING) << "Could not find string value for "
                 << kNotificationTypeKey;
  }
  target_ = P2PNotificationTargetFromString(target_str);
  const base::ListValue* invalidation_map_list = nullptr;
  if (!data_dict->GetList(kInvalidationsKey, &invalidation_map_list) ||
      !invalidation_map_.ResetFromValue(*invalidation_map_list)) {
    LOG(WARNING) << "Could not parse " << kInvalidationsKey;
  }
  return true;
}

P2PInvalidator::P2PInvalidator(
    std::unique_ptr<notifier::PushClient> push_client,
    const std::string& invalidator_client_id,
    P2PNotificationTarget send_notification_target)
    : push_client_(std::move(push_client)),
      invalidator_client_id_(invalidator_client_id),
      logged_in_(false),
      notifications_enabled_(false),
      send_notification_target_(send_notification_target) {
  DCHECK(send_notification_target_ == NOTIFY_OTHERS ||
         send_notification_target_ == NOTIFY_ALL);
  push_client_->AddObserver(this);
}

P2PInvalidator::~P2PInvalidator() {
  DCHECK(thread_checker_.CalledOnValidThread());
  push_client_->RemoveObserver(this);
}

void P2PInvalidator::RegisterHandler(InvalidationHandler* handler) {
  DCHECK(thread_checker_.CalledOnValidThread());
  registrar_.RegisterHandler(handler);
}

bool P2PInvalidator::UpdateRegisteredIds(InvalidationHandler* handler,
                                         const ObjectIdSet& ids) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ObjectIdSet new_ids;
  const ObjectIdSet& old_ids = registrar_.GetRegisteredIds(handler);
  std::set_difference(ids.begin(), ids.end(),
                      old_ids.begin(), old_ids.end(),
                      std::inserter(new_ids, new_ids.end()),
                      ObjectIdLessThan());
  if (!registrar_.UpdateRegisteredIds(handler, ids))
    return false;
  const P2PNotificationData notification_data(
      invalidator_client_id_,
      send_notification_target_,
      ObjectIdInvalidationMap::InvalidateAll(ids));
  SendNotificationData(notification_data);
  return true;
}

void P2PInvalidator::UnregisterHandler(InvalidationHandler* handler) {
  DCHECK(thread_checker_.CalledOnValidThread());
  registrar_.UnregisterHandler(handler);
}

InvalidatorState P2PInvalidator::GetInvalidatorState() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return registrar_.GetInvalidatorState();
}

void P2PInvalidator::UpdateCredentials(
    const std::string& email, const std::string& token) {
  DCHECK(thread_checker_.CalledOnValidThread());
  notifier::Subscription subscription;
  subscription.channel = kSyncP2PNotificationChannel;
  // There may be some subtle issues around case sensitivity of the
  // from field, but it doesn't matter too much since this is only
  // used in p2p mode (which is only used in testing).
  subscription.from = email;
  push_client_->UpdateSubscriptions(
      notifier::SubscriptionList(1, subscription));
  // If already logged in, the new credentials will take effect on the
  // next reconnection.
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("p2p_invalidator", R"(
        semantics {
          sender: "P2P Invalidator"
          description:
            "Chromium uses cacheinvalidation library to receive push "
            "notifications from the server about sync items (bookmarks, "
            "passwords, preferences, etc.) modified on other clients. It uses "
            "XMPP PushClient to communicate with server."
          trigger:
            "Initial communication happens after browser startup to register "
            "client device with server and update online status. Consecutive "
            "communications are triggered by heartbeat timer and push "
            "notifications from server."
          data:
            "Protocol buffers including server generated client_token, "
            "ObjectIds identifying subscriptions, and versions for these "
            "subscriptions. ObjectId is not unique to user. Version is not "
            "related to sync data, it is an internal concept of invalidations "
            "protocol."
          destination: OTHER
          destination_other:
            "The P2P client that user is connected to."
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature is only used for integration testing, never used in "
            "released Chrome."
          policy_exception_justification:
            "This feature is only used for integration testing, never used in "
            "released Chrome."
        }
    )");
  push_client_->UpdateCredentials(email, token, traffic_annotation);
  logged_in_ = true;
}

void P2PInvalidator::RequestDetailedStatus(
    base::Callback<void(const base::DictionaryValue&)> callback) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  // TODO(mferreria): Make the P2P Invalidator work.
  std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
  callback.Run(*value);
}

void P2PInvalidator::SendInvalidation(const ObjectIdSet& ids) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ObjectIdInvalidationMap invalidation_map =
      ObjectIdInvalidationMap::InvalidateAll(ids);
  const P2PNotificationData notification_data(
      invalidator_client_id_, send_notification_target_, invalidation_map);
  SendNotificationData(notification_data);
}

void P2PInvalidator::OnNotificationsEnabled() {
  DCHECK(thread_checker_.CalledOnValidThread());
  bool just_turned_on = (notifications_enabled_ == false);
  notifications_enabled_ = true;
  registrar_.UpdateInvalidatorState(INVALIDATIONS_ENABLED);
  if (just_turned_on) {
    const P2PNotificationData notification_data(
        invalidator_client_id_,
        NOTIFY_SELF,
        ObjectIdInvalidationMap::InvalidateAll(
            registrar_.GetAllRegisteredIds()));
    SendNotificationData(notification_data);
  }
}

void P2PInvalidator::OnNotificationsDisabled(
    notifier::NotificationsDisabledReason reason) {
  DCHECK(thread_checker_.CalledOnValidThread());
  registrar_.UpdateInvalidatorState(FromNotifierReason(reason));
}

void P2PInvalidator::OnIncomingNotification(
    const notifier::Notification& notification) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "Received notification " << notification.ToString();
  if (!logged_in_) {
    DVLOG(1) << "Not logged in yet -- not emitting notification";
    return;
  }
  if (!notifications_enabled_) {
    DVLOG(1) << "Notifications not on -- not emitting notification";
    return;
  }
  if (notification.channel != kSyncP2PNotificationChannel) {
    LOG(WARNING) << "Notification from unexpected source "
                 << notification.channel;
  }
  P2PNotificationData notification_data;
  if (!notification_data.ResetFromString(notification.data)) {
    LOG(WARNING) << "Could not parse notification data from "
                 << notification.data;
    notification_data = P2PNotificationData(
        invalidator_client_id_,
        NOTIFY_ALL,
        ObjectIdInvalidationMap::InvalidateAll(
            registrar_.GetAllRegisteredIds()));
  }
  if (!notification_data.IsTargeted(invalidator_client_id_)) {
    DVLOG(1) << "Not a target of the notification -- "
             << "not emitting notification";
    return;
  }
  registrar_.DispatchInvalidationsToHandlers(
      notification_data.GetIdInvalidationMap());
}

void P2PInvalidator::SendNotificationDataForTest(
    const P2PNotificationData& notification_data) {
  DCHECK(thread_checker_.CalledOnValidThread());
  SendNotificationData(notification_data);
}

void P2PInvalidator::SendNotificationData(
    const P2PNotificationData& notification_data) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (notification_data.GetIdInvalidationMap().Empty()) {
    DVLOG(1) << "Not sending XMPP notification with empty state map: "
             << notification_data.ToString();
    return;
  }
  notifier::Notification notification;
  notification.channel = kSyncP2PNotificationChannel;
  notification.data = notification_data.ToString();
  DVLOG(1) << "Sending XMPP notification: " << notification.ToString();
  push_client_->SendNotification(notification);
}

}  // namespace syncer
