// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/no_op_user_event_service.h"

#include "base/memory/weak_ptr.h"

using sync_pb::UserEventSpecifics;

namespace syncer {

NoOpUserEventService::NoOpUserEventService() {}

NoOpUserEventService::~NoOpUserEventService() {}

void NoOpUserEventService::RecordUserEvent(
    std::unique_ptr<UserEventSpecifics> specifics) {}

void NoOpUserEventService::RecordUserEvent(
    const UserEventSpecifics& specifics) {}

base::WeakPtr<ModelTypeSyncBridge> NoOpUserEventService::GetSyncBridge() {
  return base::WeakPtr<ModelTypeSyncBridge>();
}

}  // namespace syncer
