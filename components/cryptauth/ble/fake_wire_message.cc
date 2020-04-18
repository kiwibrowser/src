// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/ble/fake_wire_message.h"

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/cryptauth/wire_message.h"

namespace cryptauth {

FakeWireMessage::FakeWireMessage(
    const std::string& payload, const std::string& feature)
    : WireMessage(payload, feature) {}

std::string FakeWireMessage::Serialize() const {
  return feature() + "," + payload();
}

}
