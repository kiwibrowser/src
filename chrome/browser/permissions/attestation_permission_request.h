// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERMISSIONS_ATTESTATION_PERMISSION_REQUEST_H_
#define CHROME_BROWSER_PERMISSIONS_ATTESTATION_PERMISSION_REQUEST_H_

#include "base/callback_forward.h"

class PermissionRequest;

namespace url {
class Origin;
}

// Returns a |PermissionRequest| that asks the user to consent to sending
// identifying information about their security key. The |origin| argument is
// used to identify the origin that is requesting the permission, and only the
// authority part of the URL is used. The caller takes ownership of the returned
// object because the standard pattern for PermissionRequests is that they
// delete themselves once complete.
PermissionRequest* NewAttestationPermissionRequest(
    const url::Origin& origin,
    base::OnceCallback<void(bool)> callback);

#endif  // CHROME_BROWSER_PERMISSIONS_ATTESTATION_PERMISSION_REQUEST_H_
