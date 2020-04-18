// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/permissions/permission_message.h"

namespace extensions {

PermissionMessage::PermissionMessage(const base::string16& message,
                                     const PermissionIDSet& permissions)
    : message_(message), permissions_(permissions) {}

PermissionMessage::PermissionMessage(
    const base::string16& message,
    const PermissionIDSet& permissions,
    const std::vector<base::string16>& submessages)
    : message_(message), permissions_(permissions), submessages_(submessages) {}

PermissionMessage::PermissionMessage(const PermissionMessage& other) = default;

PermissionMessage::~PermissionMessage() {}

}  // namespace extensions
