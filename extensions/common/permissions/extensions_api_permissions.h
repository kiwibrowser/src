// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_PERMISSIONS_EXTENSIONS_API_PERMISSIONS_H_
#define EXTENSIONS_COMMON_PERMISSIONS_EXTENSIONS_API_PERMISSIONS_H_

#include "base/compiler_specific.h"
#include "extensions/common/permissions/permissions_provider.h"

namespace extensions {

class ExtensionsAPIPermissions : public PermissionsProvider {
 public:
  std::vector<std::unique_ptr<APIPermissionInfo>> GetAllPermissions()
      const override;
};

}  // namespace extensions

#endif  // EXTENSIONS_COMMON_PERMISSIONS_EXTENSIONS_API_PERMISSIONS_H_
