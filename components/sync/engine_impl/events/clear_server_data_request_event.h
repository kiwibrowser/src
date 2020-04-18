// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_EVENTS_CLEAR_SERVER_DATA_REQUEST_EVENT_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_EVENTS_CLEAR_SERVER_DATA_REQUEST_EVENT_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/sync/engine/events/protocol_event.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {

// An event representing a ClearServerData request message sent to the server.
class ClearServerDataRequestEvent : public ProtocolEvent {
 public:
  ClearServerDataRequestEvent(base::Time timestamp,
                              const sync_pb::ClientToServerMessage& request);
  ~ClearServerDataRequestEvent() override;

  base::Time GetTimestamp() const override;
  std::string GetType() const override;
  std::string GetDetails() const override;
  std::unique_ptr<base::DictionaryValue> GetProtoMessage(
      bool include_specifics) const override;
  std::unique_ptr<ProtocolEvent> Clone() const override;

  static std::unique_ptr<base::DictionaryValue> ToValue(
      const ProtocolEvent& event);

 private:
  const base::Time timestamp_;
  const sync_pb::ClientToServerMessage request_;

  DISALLOW_COPY_AND_ASSIGN(ClearServerDataRequestEvent);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_EVENTS_CLEAR_SERVER_DATA_REQUEST_EVENT_H_
