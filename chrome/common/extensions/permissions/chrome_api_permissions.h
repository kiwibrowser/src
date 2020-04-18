// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_PERMISSIONS_CHROME_API_PERMISSIONS_H_
#define CHROME_COMMON_EXTENSIONS_PERMISSIONS_CHROME_API_PERMISSIONS_H_

#include <vector>

#include "base/compiler_specific.h"
#include "extensions/common/permissions/permissions_provider.h"

namespace extensions {

// Registers the permissions used in Chrome with the PermissionsInfo global.
class ChromeAPIPermissions : public PermissionsProvider {
 public:
  std::vector<std::unique_ptr<APIPermissionInfo>> GetAllPermissions()
      const override;
};

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_PERMISSIONS_CHROME_API_PERMISSIONS_H_
