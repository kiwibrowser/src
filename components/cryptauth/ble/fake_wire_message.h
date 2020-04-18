// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_FAKE_WIRE_MESSAGE_H_
#define COMPONENTS_CRYPTAUTH_FAKE_WIRE_MESSAGE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/cryptauth/wire_message.h"

namespace cryptauth {

class FakeWireMessage : public WireMessage {
 public:
  FakeWireMessage(const std::string& payload, const std::string& feature);

  // WireMessage:
  std::string Serialize() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeWireMessage);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_FAKE_WIRE_MESSAGE_H_
