// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.permissions;

import android.app.Activity;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.support.v7.app.AlertDialog;
import android.util.SparseArray;
import android.view.View;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.metrics.WebApkUma;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.webapps.WebApkActivity;
import org.chromium.ui.base.PermissionCallback;
import org.chromium.ui.base.WindowAndroid;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Methods to handle requesting native permissions from Android when the user grants a website a
 * permission.
 */
public class AndroidPermissionRequester {
    /**
    * An interface for classes which need to be informed of the outcome of asking a user to grant an
    * Android permission.
    */
    public interface RequestDelegate {
        void onAndroidPermissionAccepted();
        void onAndroidPermissionCanceled();
    }

    private static SparseArray<String[]> generatePermissionsMapping(
            WindowAndroid windowAndroid, int[] contentSettingsTypes) {
        SparseArray<String[]> permissionsToRequest = new SparseArray<>();
        for (int i = 0; i < contentSettingsTypes.length; i++) {
            String[] permissions = PrefServiceBridge.getAndroidPermissionsForContentSetting(
                    contentSettingsTypes[i]);
            if (permissions == null) continue;
            List<String> missingPermissions = new ArrayList<>();
            for (int j = 0; j < permissions.length; j++) {
                String permission = permissions[j];
                if (!windowAndroid.hasPermission(permission)) missingPermissions.add(permission);
            }
            if (!missingPermissions.isEmpty()) {
                permissionsToRequest.append(contentSettingsTypes[i],
                        missingPermissions.toArray(new String[missingPermissions.size()]));
            }
        }
        return permissionsToRequest;
    }

    private static int getContentSettingType(
            SparseArray<String[]> contentSettingsTypesToPermissionsMap, String permission) {
        // SparseArray#indexOfValue uses == instead of .equals, so we need to manually iterate
        // over the list.
        for (int i = 0; i < contentSettingsTypesToPermissionsMap.size(); i++) {
            String[] contentSettingPermissions = contentSettingsTypesToPermissionsMap.valueAt(i);
            for (int j = 0; j < contentSettingPermissions.length; j++) {
                if (permission.equals(contentSettingPermissions[j])) {
                    return contentSettingsTypesToPermissionsMap.keyAt(i);
                }
            }
        }

        return -1;
    }

    /**
     * Returns true if any of the permissions in contentSettingsTypes must be requested from the
     * system. Otherwise returns false.
     *
     * If true is returned, this method will asynchronously request the necessary permissions using
     * a dialog, running methods on the RequestDelegate when the user has made a decision.
     */
    public static boolean requestAndroidPermissions(
            final Tab tab, final int[] contentSettingsTypes, final RequestDelegate delegate) {
        final WindowAndroid windowAndroid = tab.getWindowAndroid();
        if (windowAndroid == null) return false;

        final SparseArray<String[]> contentSettingsTypesToPermissionsMap =
                generatePermissionsMapping(windowAndroid, contentSettingsTypes);

        if (contentSettingsTypesToPermissionsMap.size() == 0) return false;

        PermissionCallback callback = new PermissionCallback() {
            @Override
            public void onRequestPermissionsResult(String[] permissions, int[] grantResults) {
                boolean allRequestable = true;
                Set<Integer> deniedContentSettings = new HashSet<Integer>();
                List<String> deniedPermissions = new ArrayList<String>();

                for (int i = 0; i < grantResults.length; i++) {
                    if (grantResults[i] == PackageManager.PERMISSION_DENIED) {
                        deniedPermissions.add(permissions[i]);
                        deniedContentSettings.add(getContentSettingType(
                                contentSettingsTypesToPermissionsMap, permissions[i]));

                        if (!windowAndroid.canRequestPermission(permissions[i])) {
                            allRequestable = false;
                        }
                    }
                }

                Activity activity = windowAndroid.getActivity().get();
                if (activity instanceof WebApkActivity && deniedPermissions.size() > 0) {
                    WebApkUma.recordAndroidRuntimePermissionDeniedInWebApk(
                            deniedPermissions.toArray(new String[deniedPermissions.size()]));
                }

                if (allRequestable && !deniedContentSettings.isEmpty() && activity != null) {
                    int deniedStringId = -1;
                    if (deniedContentSettings.size() == 2
                            && deniedContentSettings.contains(
                                       ContentSettingsType.CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC)
                            && deniedContentSettings.contains(
                                       ContentSettingsType
                                               .CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA)) {
                        deniedStringId =
                                R.string.infobar_missing_microphone_camera_permissions_text;
                    } else if (deniedContentSettings.size() == 1) {
                        if (deniedContentSettings.contains(
                                    ContentSettingsType.CONTENT_SETTINGS_TYPE_GEOLOCATION)) {
                            deniedStringId = R.string.infobar_missing_location_permission_text;
                        } else if (deniedContentSettings.contains(
                                           ContentSettingsType
                                                   .CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC)) {
                            deniedStringId = R.string.infobar_missing_microphone_permission_text;
                        } else if (deniedContentSettings.contains(
                                           ContentSettingsType
                                                   .CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA)) {
                            deniedStringId = R.string.infobar_missing_camera_permission_text;
                        }
                    }

                    assert deniedStringId
                            != -1 : "Invalid combination of missing content settings: "
                                    + deniedContentSettings;

                    View view = activity.getLayoutInflater().inflate(
                            R.layout.update_permissions_dialog, null);
                    TextView dialogText = (TextView) view.findViewById(R.id.text);
                    dialogText.setText(deniedStringId);

                    AlertDialog.Builder builder =
                            new AlertDialog.Builder(activity, R.style.AlertDialogTheme);
                    builder.setView(view);
                    builder.setPositiveButton(R.string.infobar_update_permissions_button_text,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int id) {
                                    requestAndroidPermissions(tab, contentSettingsTypes, delegate);
                                }
                            });
                    builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
                        @Override
                        public void onCancel(DialogInterface dialog) {
                            delegate.onAndroidPermissionCanceled();
                        }
                    });
                    builder.create().show();
                } else if (deniedContentSettings.isEmpty()) {
                    delegate.onAndroidPermissionAccepted();
                } else {
                    delegate.onAndroidPermissionCanceled();
                }
            }
        };

        Set<String> permissionsToRequest = new HashSet<>();
        for (int i = 0; i < contentSettingsTypesToPermissionsMap.size(); i++) {
            Collections.addAll(
                    permissionsToRequest, contentSettingsTypesToPermissionsMap.valueAt(i));
        }
        String[] permissions =
                permissionsToRequest.toArray(new String[permissionsToRequest.size()]);
        windowAndroid.requestPermissions(permissions, callback);
        if (windowAndroid.getActivity().get() instanceof WebApkActivity) {
            WebApkUma.recordAndroidRuntimePermissionPromptInWebApk(permissions);
        }
        return true;
    }
}
