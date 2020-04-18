// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_PERMISSIONS_PERMISSIONS_PROVIDER_H_
#define EXTENSIONS_COMMON_PERMISSIONS_PERMISSIONS_PROVIDER_H_

#include <memory>
#include <vector>

namespace extensions {

class APIPermissionInfo;

// The PermissionsProvider creates APIPermissions instances. It is only
// needed at startup time. Typically, ExtensionsClient will register
// its PermissionsProviders with the global PermissionsInfo at startup.
// TODO(sashab): Remove all permission messages from this class, moving the
// permission message rules into ChromePermissionMessageProvider.
class PermissionsProvider {
 public:
  // Returns all the known permissions.
  virtual std::vector<std::unique_ptr<APIPermissionInfo>> GetAllPermissions()
      const = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_PERMISSIONS_PERMISSIONS_PROVIDER_H_
