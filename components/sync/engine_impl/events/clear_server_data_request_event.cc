// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/events/clear_server_data_request_event.h"

#include "base/strings/stringprintf.h"
#include "components/sync/protocol/proto_value_conversions.h"

namespace syncer {

ClearServerDataRequestEvent::ClearServerDataRequestEvent(
    base::Time timestamp,
    const sync_pb::ClientToServerMessage& request)
    : timestamp_(timestamp), request_(request) {}

ClearServerDataRequestEvent::~ClearServerDataRequestEvent() {}

base::Time ClearServerDataRequestEvent::GetTimestamp() const {
  return timestamp_;
}

std::string ClearServerDataRequestEvent::GetType() const {
  return "ClearServerData Request";
}

std::string ClearServerDataRequestEvent::GetDetails() const {
  return std::string();
}

std::unique_ptr<base::DictionaryValue>
ClearServerDataRequestEvent::GetProtoMessage(bool include_specifics) const {
  return std::unique_ptr<base::DictionaryValue>(
      ClientToServerMessageToValue(request_, include_specifics));
}

std::unique_ptr<ProtocolEvent> ClearServerDataRequestEvent::Clone() const {
  return std::unique_ptr<ProtocolEvent>(
      new ClearServerDataRequestEvent(timestamp_, request_));
}

}  // namespace syncer
