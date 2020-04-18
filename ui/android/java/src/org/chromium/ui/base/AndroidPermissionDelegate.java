// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

/**
 * Contains the functionality for interacting with the android permissions system.
 */
public interface AndroidPermissionDelegate {
    /**
     * Determine whether access to a particular permission is granted.
     * @param permission The permission whose access is to be checked.
     * @return Whether access to the permission is granted.
     */
    boolean hasPermission(String permission);

    /**
     * Determine whether the specified permission can be requested.
     *
     * <p>
     * A permission can be requested in the following states:
     * 1.) Default un-granted state, permission can be requested
     * 2.) Permission previously requested but denied by the user, but the user did not select
     *     "Never ask again".
     *
     * @param permission The permission name.
     * @return Whether the requesting the permission is allowed.
     */
    boolean canRequestPermission(String permission);

    /**
     * Determine whether the specified permission is revoked by policy.
     *
     * @param permission The permission name.
     * @return Whether the permission is revoked by policy and the user has no ability to change it.
     */
    boolean isPermissionRevokedByPolicy(String permission);

    /**
     * Requests the specified permissions are granted for further use.
     * @param permissions The list of permissions to request access to.
     * @param callback The callback to be notified whether the permissions were granted.
     */
    void requestPermissions(String[] permissions, PermissionCallback callback);

    /**
     * Callback for the result from requesting permissions.
     * @param requestCode The request code passed in requestPermissions.
     * @param permissions The list of requested permissions.
     * @param grantResults The grant results for the corresponding permissions.
     */
    void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults);
}
