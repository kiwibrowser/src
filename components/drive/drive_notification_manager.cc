// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/drive_notification_manager.h"

#include "base/metrics/histogram_macros.h"
#include "components/drive/drive_notification_observer.h"
#include "components/invalidation/public/invalidation_service.h"
#include "components/invalidation/public/object_id_invalidation_map.h"
#include "google/cacheinvalidation/types.pb.h"

namespace drive {

namespace {

// The polling interval time is used when XMPP is disabled.
const int kFastPollingIntervalInSecs = 60;

// The polling interval time is used when XMPP is enabled.  Theoretically
// polling should be unnecessary if XMPP is enabled, but just in case.
const int kSlowPollingIntervalInSecs = 300;

// The sync invalidation object ID for Google Drive.
const char kDriveInvalidationObjectId[] = "CHANGELOG";

}  // namespace

DriveNotificationManager::DriveNotificationManager(
    invalidation::InvalidationService* invalidation_service)
    : invalidation_service_(invalidation_service),
      push_notification_registered_(false),
      push_notification_enabled_(false),
      observers_notified_(false),
      polling_timer_(true /* retain_user_task */, false /* is_repeating */),
      weak_ptr_factory_(this) {
  DCHECK(invalidation_service_);
  RegisterDriveNotifications();
  RestartPollingTimer();
}

DriveNotificationManager::~DriveNotificationManager() {}

void DriveNotificationManager::Shutdown() {
  // Unregister for Drive notifications.
  if (!invalidation_service_ || !push_notification_registered_)
    return;

  // We unregister the handler without updating unregistering our IDs on
  // purpose.  See the class comment on the InvalidationService interface for
  // more information.
  invalidation_service_->UnregisterInvalidationHandler(this);
  invalidation_service_ = nullptr;
}

void DriveNotificationManager::OnInvalidatorStateChange(
    syncer::InvalidatorState state) {
  push_notification_enabled_ = (state == syncer::INVALIDATIONS_ENABLED);
  if (push_notification_enabled_) {
    DVLOG(1) << "XMPP Notifications enabled";
  } else {
    DVLOG(1) << "XMPP Notifications disabled (state=" << state << ")";
  }
  for (auto& observer : observers_)
    observer.OnPushNotificationEnabled(push_notification_enabled_);
}

void DriveNotificationManager::OnIncomingInvalidation(
    const syncer::ObjectIdInvalidationMap& invalidation_map) {
  DVLOG(2) << "XMPP Drive Notification Received";
  syncer::ObjectIdSet ids = invalidation_map.GetObjectIds();
  DCHECK_EQ(1U, ids.size());
  const invalidation::ObjectId object_id(
      ipc::invalidation::ObjectSource::COSMO_CHANGELOG,
      kDriveInvalidationObjectId);
  DCHECK_EQ(1U, ids.count(object_id));

  // This effectively disables 'local acks'.  It tells the invalidations system
  // to not bother saving invalidations across restarts for us.
  // See crbug.com/320878.
  invalidation_map.AcknowledgeAll();
  NotifyObserversToUpdate(NOTIFICATION_XMPP);
}

std::string DriveNotificationManager::GetOwnerName() const { return "Drive"; }

void DriveNotificationManager::AddObserver(
    DriveNotificationObserver* observer) {
  observers_.AddObserver(observer);
}

void DriveNotificationManager::RemoveObserver(
    DriveNotificationObserver* observer) {
  observers_.RemoveObserver(observer);
}

void DriveNotificationManager::RestartPollingTimer() {
  const int interval_secs = (push_notification_enabled_ ?
                             kSlowPollingIntervalInSecs :
                             kFastPollingIntervalInSecs);
  polling_timer_.Stop();
  polling_timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(interval_secs),
      base::Bind(&DriveNotificationManager::NotifyObserversToUpdate,
                 weak_ptr_factory_.GetWeakPtr(),
                 NOTIFICATION_POLLING));
}

void DriveNotificationManager::NotifyObserversToUpdate(
    NotificationSource source) {
  DVLOG(1) << "Notifying observers: " << NotificationSourceToString(source);
  for (auto& observer : observers_)
    observer.OnNotificationReceived();
  if (!observers_notified_) {
    UMA_HISTOGRAM_BOOLEAN("Drive.PushNotificationInitiallyEnabled",
                          push_notification_enabled_);
  }
  observers_notified_ = true;

  // Note that polling_timer_ is not a repeating timer. Restarting manually
  // here is better as XMPP may be received right before the polling timer is
  // fired (i.e. we don't notify observers twice in a row).
  RestartPollingTimer();
}

void DriveNotificationManager::RegisterDriveNotifications() {
  DCHECK(!push_notification_enabled_);

  if (!invalidation_service_)
    return;

  invalidation_service_->RegisterInvalidationHandler(this);
  syncer::ObjectIdSet ids;
  ids.insert(invalidation::ObjectId(
      ipc::invalidation::ObjectSource::COSMO_CHANGELOG,
      kDriveInvalidationObjectId));
  CHECK(invalidation_service_->UpdateRegisteredInvalidationIds(this, ids));
  push_notification_registered_ = true;
  OnInvalidatorStateChange(invalidation_service_->GetInvalidatorState());

  UMA_HISTOGRAM_BOOLEAN("Drive.PushNotificationRegistered",
                        push_notification_registered_);
}

// static
std::string DriveNotificationManager::NotificationSourceToString(
    NotificationSource source) {
  switch (source) {
    case NOTIFICATION_XMPP:
      return "NOTIFICATION_XMPP";
    case NOTIFICATION_POLLING:
      return "NOTIFICATION_POLLING";
  }

  NOTREACHED();
  return "";
}

}  // namespace drive
