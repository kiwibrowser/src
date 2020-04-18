// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/notification_types.h"

namespace extensions {
ExtensionCommandRemovedDetails::ExtensionCommandRemovedDetails(
    const std::string& extension_id,
    const std::string& command_name,
    const std::string& accelerator)
    : extension_id(extension_id),
      command_name(command_name),
      accelerator(accelerator) {
}
}  // namespace extensions
