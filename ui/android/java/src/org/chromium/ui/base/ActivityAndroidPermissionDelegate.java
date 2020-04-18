// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

import android.app.Activity;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.PermissionInfo;
import android.os.Build;
import android.os.Handler;
import android.os.Process;
import android.text.TextUtils;
import android.util.SparseArray;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ContextUtils;

import java.lang.ref.WeakReference;

/**
 * AndroidPermissionDelegate implementation for Activity.
 */
public class ActivityAndroidPermissionDelegate implements AndroidPermissionDelegate {
    private WeakReference<Activity> mActivity;

    private Handler mHandler;
    private SparseArray<PermissionCallback> mOutstandingPermissionRequests;
    private int mNextRequestCode;

    // Constants used for permission request code bounding.
    private static final int REQUEST_CODE_PREFIX = 1000;
    private static final int REQUEST_CODE_RANGE_SIZE = 100;

    private static final String PERMISSION_QUERIED_KEY_PREFIX = "HasRequestedAndroidPermission::";

    public ActivityAndroidPermissionDelegate(WeakReference<Activity> activity) {
        mActivity = activity;
        mHandler = new Handler();
        mOutstandingPermissionRequests = new SparseArray<PermissionCallback>();
    }

    @Override
    public boolean hasPermission(String permission) {
        return ApiCompatibilityUtils.checkPermission(ContextUtils.getApplicationContext(),
                       permission, Process.myPid(), Process.myUid())
                == PackageManager.PERMISSION_GRANTED;
    }

    @Override
    public boolean canRequestPermission(String permission) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return false;

        Activity activity = mActivity.get();
        if (activity == null) return false;

        if (isPermissionRevokedByPolicy(permission)) {
            return false;
        }

        if (activity.shouldShowRequestPermissionRationale(permission)) {
            return true;
        }

        // Check whether we have ever asked for this permission by checking whether we saved
        // a preference associated with it before.
        String permissionQueriedKey = getHasRequestedPermissionKey(permission);
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        if (!prefs.getBoolean(permissionQueriedKey, false)) return true;

        logUMAOnRequestPermissionDenied(permission);
        return false;
    }

    @Override
    public boolean isPermissionRevokedByPolicy(String permission) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return false;

        Activity activity = mActivity.get();
        if (activity == null) return false;

        return activity.getPackageManager().isPermissionRevokedByPolicy(
                permission, activity.getPackageName());
    }

    @Override
    public void requestPermissions(final String[] permissions, final PermissionCallback callback) {
        if (requestPermissionsInternal(permissions, callback)) return;

        // If the permission request was not sent successfully, just post a response to the
        // callback with whatever the current permission state is for all the requested
        // permissions.  The response is posted to keep the async behavior of this method
        // consistent.
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                int[] results = new int[permissions.length];
                for (int i = 0; i < permissions.length; i++) {
                    results[i] = hasPermission(permissions[i]) ? PackageManager.PERMISSION_GRANTED
                                                               : PackageManager.PERMISSION_DENIED;
                }
                callback.onRequestPermissionsResult(permissions, results);
            }
        });
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, String[] permissions, int[] grantResults) {
        Activity activity = mActivity.get();
        assert activity != null;

        SharedPreferences.Editor editor = ContextUtils.getAppSharedPreferences().edit();
        for (int i = 0; i < permissions.length; i++) {
            editor.putBoolean(getHasRequestedPermissionKey(permissions[i]), true);
        }
        editor.apply();

        PermissionCallback callback = mOutstandingPermissionRequests.get(requestCode);
        mOutstandingPermissionRequests.delete(requestCode);
        if (callback == null) return;
        callback.onRequestPermissionsResult(permissions, grantResults);
    }

    protected void logUMAOnRequestPermissionDenied(String permission) {}

    /**
     * Issues the permission request and returns whether it was sent successfully.
     */
    private boolean requestPermissionsInternal(String[] permissions, PermissionCallback callback) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) return false;
        Activity activity = mActivity.get();
        if (activity == null) return false;

        int requestCode = REQUEST_CODE_PREFIX + mNextRequestCode;
        mNextRequestCode = (mNextRequestCode + 1) % REQUEST_CODE_RANGE_SIZE;
        mOutstandingPermissionRequests.put(requestCode, callback);
        activity.requestPermissions(permissions, requestCode);
        return true;
    }

    private String getHasRequestedPermissionKey(String permission) {
        String permissionQueriedKey = permission;
        // Prior to O, permissions were granted at the group level.  Post O, each permission is
        // granted individually.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            try {
                // Runtime permissions are controlled at the group level.  So when determining
                // whether we have requested a particular permission before, we should check whether
                // we have requested any permission in that group as that mimics the logic in the
                // Android framework.
                //
                // e.g. Requesting first the permission ACCESS_FINE_LOCATION will result in Chrome
                //      treating ACCESS_COARSE_LOCATION as if it had already been requested as well.
                PermissionInfo permissionInfo =
                        ContextUtils.getApplicationContext().getPackageManager().getPermissionInfo(
                                permission, PackageManager.GET_META_DATA);

                if (!TextUtils.isEmpty(permissionInfo.group)) {
                    permissionQueriedKey = permissionInfo.group;
                }
            } catch (NameNotFoundException e) {
                // Unknown permission.  Default back to the permission name instead of the group.
            }
        }

        return PERMISSION_QUERIED_KEY_PREFIX + permissionQueriedKey;
    }
}
