// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/scoped_access_permission_media_to_mojo_adapter.h"

namespace video_capture {

ScopedAccessPermissionMediaToMojoAdapter::
    ScopedAccessPermissionMediaToMojoAdapter(
        std::unique_ptr<
            media::VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>
            access_permission)
    : access_permission_(std::move(access_permission)) {}

ScopedAccessPermissionMediaToMojoAdapter::
    ~ScopedAccessPermissionMediaToMojoAdapter() {}

}  // namespace video_capture
