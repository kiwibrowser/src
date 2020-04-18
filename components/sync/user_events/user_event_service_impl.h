// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_USER_EVENTS_USER_EVENT_SERVICE_IMPL_H_
#define COMPONENTS_SYNC_USER_EVENTS_USER_EVENT_SERVICE_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/protocol/user_event_specifics.pb.h"
#include "components/sync/user_events/trial_recorder.h"
#include "components/sync/user_events/user_event_service.h"

namespace syncer {

class ModelTypeSyncBridge;
class SyncService;
class UserEventSyncBridge;

class UserEventServiceImpl : public UserEventService {
 public:
  UserEventServiceImpl(SyncService* sync_service,
                       std::unique_ptr<UserEventSyncBridge> bridge);
  ~UserEventServiceImpl() override;

  // KeyedService implementation.
  void Shutdown() override;

  // UserEventService implementation.
  void RecordUserEvent(
      std::unique_ptr<sync_pb::UserEventSpecifics> specifics) override;
  void RecordUserEvent(const sync_pb::UserEventSpecifics& specifics) override;
  base::WeakPtr<ModelTypeSyncBridge> GetSyncBridge() override;

  // Checks known (and immutable) conditions that should not change at runtime.
  static bool MightRecordEvents(bool off_the_record, SyncService* sync_service);

 private:
  // Whether allowed to record events that link to navigation data.
  bool CanRecordHistory();

  // Checks dynamic or event specific conditions.
  bool ShouldRecordEvent(const sync_pb::UserEventSpecifics& specifics);

  SyncService* sync_service_;

  std::unique_ptr<UserEventSyncBridge> bridge_;

  // Holds onto a random number for the duration of this execution of chrome. On
  // restart it will be regenerated. This can be attached to events to know
  // which events came from the same session.
  uint64_t session_id_;

  // Tracks and records field trails when appropriate.
  TrialRecorder trial_recorder_;

  DISALLOW_COPY_AND_ASSIGN(UserEventServiceImpl);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_USER_EVENTS_USER_EVENT_SERVICE_IMPL_H_
