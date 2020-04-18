// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/events/commit_response_event.h"

#include "base/strings/stringprintf.h"
#include "components/sync/protocol/proto_value_conversions.h"

namespace syncer {

CommitResponseEvent::CommitResponseEvent(
    base::Time timestamp,
    SyncerError result,
    const sync_pb::ClientToServerResponse& response)
    : timestamp_(timestamp), result_(result), response_(response) {}

CommitResponseEvent::~CommitResponseEvent() {}

base::Time CommitResponseEvent::GetTimestamp() const {
  return timestamp_;
}

std::string CommitResponseEvent::GetType() const {
  return "Commit Response";
}

std::string CommitResponseEvent::GetDetails() const {
  return base::StringPrintf("Result: %s", GetSyncerErrorString(result_));
}

std::unique_ptr<base::DictionaryValue> CommitResponseEvent::GetProtoMessage(
    bool include_specifics) const {
  return std::unique_ptr<base::DictionaryValue>(
      ClientToServerResponseToValue(response_, include_specifics));
}

std::unique_ptr<ProtocolEvent> CommitResponseEvent::Clone() const {
  return std::unique_ptr<ProtocolEvent>(
      new CommitResponseEvent(timestamp_, result_, response_));
}

}  // namespace syncer
