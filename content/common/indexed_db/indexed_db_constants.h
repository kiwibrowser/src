// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INDEXED_DB_INDEXED_DB_CONSTANTS_H_
#define CONTENT_COMMON_INDEXED_DB_INDEXED_DB_CONSTANTS_H_

#include <stddef.h>
#include <stdint.h>

#include "ipc/ipc_channel.h"

namespace content {

const int32_t kNoDatabase = -1;

const size_t kMaxIDBMessageOverhead = 1024 * 1024;  // 1MB; arbitrarily chosen.

const size_t kMaxIDBMessageSizeInBytes =
    IPC::Channel::kMaximumMessageSize - kMaxIDBMessageOverhead;

}  // namespace content

#endif  // CONTENT_COMMON_INDEXED_DB_INDEXED_DB_CONSTANTS_H_
