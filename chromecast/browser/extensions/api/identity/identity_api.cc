// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/extensions/api/identity/identity_api.h"

namespace extensions {
namespace cast {

namespace {
const char kErrorNoUserAccount[] = "No user account.";
}  // namespace

///////////////////////////////////////////////////////////////////////////////

IdentityGetAuthTokenFunction::IdentityGetAuthTokenFunction() {}

IdentityGetAuthTokenFunction::~IdentityGetAuthTokenFunction() {}

ExtensionFunction::ResponseAction IdentityGetAuthTokenFunction::Run() {
  return RespondNow(Error(kErrorNoUserAccount));
}

///////////////////////////////////////////////////////////////////////////////

IdentityRemoveCachedAuthTokenFunction::IdentityRemoveCachedAuthTokenFunction() {
}

IdentityRemoveCachedAuthTokenFunction::
    ~IdentityRemoveCachedAuthTokenFunction() {}

ExtensionFunction::ResponseAction IdentityRemoveCachedAuthTokenFunction::Run() {
  // This stub identity API does not maintain a token cache, so there is nothing
  // to remove.
  return RespondNow(NoArguments());
}

}  // namespace cast
}  // namespace extensions
