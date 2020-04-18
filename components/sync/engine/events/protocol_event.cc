// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/events/protocol_event.h"


namespace syncer {

ProtocolEvent::ProtocolEvent() {}

ProtocolEvent::~ProtocolEvent() {}

std::unique_ptr<base::DictionaryValue> ProtocolEvent::ToValue(
    const ProtocolEvent& event,
    bool include_specifics) {
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetDouble("time", event.GetTimestamp().ToJsTime());
  dict->SetString("type", event.GetType());
  dict->SetString("details", event.GetDetails());
  dict->Set("proto", event.GetProtoMessage(include_specifics));
  return dict;
}

}  // namespace syncer
