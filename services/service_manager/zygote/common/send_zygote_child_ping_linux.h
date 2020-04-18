// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_ZYGOTE_COMMON_SEND_ZYGOTE_CHILD_PING_LINUX_H_
#define SERVICES_SERVICE_MANAGER_ZYGOTE_COMMON_SEND_ZYGOTE_CHILD_PING_LINUX_H_

#include "base/component_export.h"

namespace service_manager {

// Sends a zygote child "ping" message to browser process via socket |fd|.
// Returns true on success.
COMPONENT_EXPORT(SERVICE_MANAGER_ZYGOTE) bool SendZygoteChildPing(int fd);

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_ZYGOTE_COMMON_SEND_ZYGOTE_CHILD_PING_LINUX_H_
