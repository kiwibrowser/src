// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_RELEASE_CALLBACK_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_RELEASE_CALLBACK_H_

#include "base/callback.h"

namespace gpu {
struct SyncToken;
}  // namespace gpu

namespace viz {

using ReleaseCallback =
    base::OnceCallback<void(const gpu::SyncToken& sync_token, bool is_lost)>;

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_RELEASE_CALLBACK_H_
