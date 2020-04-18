// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_CAST_CHANNEL_CAST_MESSAGE_UTIL_H_
#define EXTENSIONS_BROWSER_API_CAST_CHANNEL_CAST_MESSAGE_UTIL_H_

#include <string>

namespace cast_channel {
class CastMessage;
}  // namespace cast_channel

namespace extensions {

namespace api {
namespace cast_channel {
struct MessageInfo;
}  // namespace cast_channel
}  // namespace api

// Fills |message_proto| from |message| and returns true on success.
bool MessageInfoToCastMessage(
    const extensions::api::cast_channel::MessageInfo& message,
    ::cast_channel::CastMessage* message_proto);

// Fills |message| from |message_proto| and returns true on success.
bool CastMessageToMessageInfo(
    const ::cast_channel::CastMessage& message_proto,
    extensions::api::cast_channel::MessageInfo* message);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_CAST_CHANNEL_CAST_MESSAGE_UTIL_H_
