// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/js/js_event_details.h"

#include "base/json/json_writer.h"

namespace syncer {

JsEventDetails::JsEventDetails() {}

JsEventDetails::JsEventDetails(base::DictionaryValue* details)
    : details_(details) {}

JsEventDetails::JsEventDetails(const JsEventDetails& other) = default;

JsEventDetails::~JsEventDetails() {}

const base::DictionaryValue& JsEventDetails::Get() const {
  return details_.Get();
}

std::string JsEventDetails::ToString() const {
  std::string str;
  base::JSONWriter::Write(Get(), &str);
  return str;
}

}  // namespace syncer
