// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

/**
 * Callback for permission requests.
 */
public interface PermissionCallback {
    /**
     * Called upon completing a permission request.
     * @param permissions The list of permissions in the result.
     * @param grantResults Whether the permissions were granted.
     */
    void onRequestPermissionsResult(String[] permissions, int[] grantResults);
}
