// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_COMMON_EXTENSIONS_API_CAST_API_PERMISSIONS_H_
#define CHROMECAST_COMMON_EXTENSIONS_API_CAST_API_PERMISSIONS_H_

#include <vector>

#include "base/compiler_specific.h"
#include "extensions/common/permissions/permissions_provider.h"

namespace extensions {

// Registers the permissions used in Cast with the PermissionsInfo global.
class CastAPIPermissions : public PermissionsProvider {
 public:
  std::vector<std::unique_ptr<APIPermissionInfo>> GetAllPermissions()
      const override;
};

}  // namespace extensions

#endif  // CHROMECAST_COMMON_EXTENSIONS_API_CAST_API_PERMISSIONS_H_
