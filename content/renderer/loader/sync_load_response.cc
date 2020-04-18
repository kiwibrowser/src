// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/loader/sync_load_response.h"

namespace content {

SyncLoadResponse::SyncLoadResponse() {}

SyncLoadResponse::SyncLoadResponse(SyncLoadResponse&& other) = default;

SyncLoadResponse::~SyncLoadResponse() {}

SyncLoadResponse& SyncLoadResponse::operator=(SyncLoadResponse&& other) =
    default;

}  // namespace content
