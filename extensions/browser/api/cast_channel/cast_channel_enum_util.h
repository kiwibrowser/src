// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_CAST_CHANNEL_CAST_CHANNEL_TYPE_UTIL_H_
#define EXTENSIONS_BROWSER_API_CAST_CHANNEL_CAST_CHANNEL_TYPE_UTIL_H_

#include "components/cast_channel/cast_channel_enum.h"
#include "extensions/common/api/cast_channel.h"

namespace extensions {

api::cast_channel::ReadyState ToReadyState(
    ::cast_channel::ReadyState ready_state);
api::cast_channel::ChannelError ToChannelError(
    ::cast_channel::ChannelError channel_error);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_CAST_CHANNEL_CAST_CHANNEL_TYPE_UTIL_H_
