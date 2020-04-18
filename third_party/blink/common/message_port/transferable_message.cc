// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/message_port/transferable_message.h"

namespace blink {

TransferableMessage::TransferableMessage() = default;
TransferableMessage::TransferableMessage(TransferableMessage&&) = default;
TransferableMessage& TransferableMessage::operator=(TransferableMessage&&) =
    default;
TransferableMessage::~TransferableMessage() = default;

}  // namespace blink
