// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/events/clear_server_data_response_event.h"

#include "base/strings/stringprintf.h"
#include "components/sync/protocol/proto_value_conversions.h"

namespace syncer {

ClearServerDataResponseEvent::ClearServerDataResponseEvent(
    base::Time timestamp,
    SyncerError result,
    const sync_pb::ClientToServerResponse& response)
    : timestamp_(timestamp), result_(result), response_(response) {}

ClearServerDataResponseEvent::~ClearServerDataResponseEvent() {}

base::Time ClearServerDataResponseEvent::GetTimestamp() const {
  return timestamp_;
}

std::string ClearServerDataResponseEvent::GetType() const {
  return "ClearServerData Response";
}

std::string ClearServerDataResponseEvent::GetDetails() const {
  return base::StringPrintf("Result: %s", GetSyncerErrorString(result_));
}

std::unique_ptr<base::DictionaryValue>
ClearServerDataResponseEvent::GetProtoMessage(bool include_specifics) const {
  return std::unique_ptr<base::DictionaryValue>(
      ClientToServerResponseToValue(response_, include_specifics));
}

std::unique_ptr<ProtocolEvent> ClearServerDataResponseEvent::Clone() const {
  return std::unique_ptr<ProtocolEvent>(
      new ClearServerDataResponseEvent(timestamp_, result_, response_));
}

}  // namespace syncer
