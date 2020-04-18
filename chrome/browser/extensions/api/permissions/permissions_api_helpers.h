// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_PERMISSIONS_PERMISSIONS_API_HELPERS_H_
#define CHROME_BROWSER_EXTENSIONS_API_PERMISSIONS_PERMISSIONS_API_HELPERS_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"

namespace extensions {

class PermissionSet;

namespace api {
namespace permissions {
struct Permissions;
}
}

namespace permissions_api_helpers {

// Converts the permission |set| to a permissions object.
std::unique_ptr<api::permissions::Permissions> PackPermissionSet(
    const PermissionSet& set);

// Creates a permission set from |permissions|. Returns NULL if the permissions
// cannot be converted to a permission set, in which case |error| will be set.
std::unique_ptr<const PermissionSet> UnpackPermissionSet(
    const api::permissions::Permissions& permissions,
    bool allow_file_access,
    std::string* error);

}  // namespace permissions_api_helpers
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_PERMISSIONS_PERMISSIONS_API_HELPERS_H_
