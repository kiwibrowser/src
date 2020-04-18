// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.StrictMode;

import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.DaydreamApi;
import com.google.vr.ndk.base.GvrUiLayout;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.UsedByReflection;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

import java.lang.reflect.Method;

/**
 * Builder class to create all VR related classes. These VR classes are behind the same build time
 * flag as this class. So no reflection is necessary when create them.
 */
@UsedByReflection("VrShellDelegate.java")
public class VrClassesWrapperImpl implements VrClassesWrapper {
    private static final String TAG = "VrClassesWrapperImpl";
    private static final String VR_BOOT_SYSTEM_PROPERTY = "ro.boot.vr";

    private Boolean mBootsToVr = null;

    @UsedByReflection("VrShellDelegate.java")
    public VrClassesWrapperImpl() {}

    @Override
    public VrShell createVrShell(
            ChromeActivity activity, VrShellDelegate delegate, TabModelSelector tabModelSelector) {
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            return new VrShellImpl(activity, delegate, tabModelSelector);
        } catch (Exception ex) {
            Log.e(TAG, "Unable to instantiate VrShellImpl", ex);
            return null;
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
    }

    @Override
    public VrDaydreamApi createVrDaydreamApi() {
        return new VrDaydreamApiImpl();
    }

    @Override
    public VrCoreVersionChecker createVrCoreVersionChecker() {
        return new VrCoreVersionCheckerImpl();
    }

    @Override
    public void setVrModeEnabled(Activity activity, boolean enabled) {
        AndroidCompat.setVrModeEnabled(activity, enabled);
    }

    @Override
    public boolean isDaydreamReadyDevice() {
        return DaydreamApi.isDaydreamReadyPlatform(ContextUtils.getApplicationContext());
    }

    @Override
    public boolean isInVrSession() {
        Context context = ContextUtils.getApplicationContext();
        // The call to isInVrSession crashes when called on a non-Daydream ready device, so we add
        // the device check (b/77268533).
        try {
          return isDaydreamReadyDevice() && DaydreamApi.isInVrSession(context);
        } catch (Exception ex) {
          Log.e(TAG, "Unable to check if in vr session", ex);
          return false;
        }
    }

    @Override
    public boolean supports2dInVr() {
        Context context = ContextUtils.getApplicationContext();
        return isDaydreamReadyDevice() && DaydreamApi.supports2dInVr(context);
    }

    @Override
    public boolean bootsToVr() {
        if (mBootsToVr == null) {
            // TODO(mthiesse): Replace this with a Daydream API call when supported.
            // Note that System.GetProperty is unable to read system ro properties, so we have to
            // resort to reflection as seen below. This method of reading system properties has been
            // available since API level 1.
            mBootsToVr = getIntSystemProperty(VR_BOOT_SYSTEM_PROPERTY, 0) == 1;
        }
        return mBootsToVr;
    }

    @Override
    public void launchGvrSettings(Activity activity) {
        GvrUiLayout.launchOrInstallGvrApp(activity);
    }

    @Override
    public Intent createVrIntent(final ComponentName componentName) {
        return DaydreamApi.createVrIntent(componentName);
    }

    @Override
    public Intent setupVrIntent(Intent intent) {
        return DaydreamApi.setupVrIntent(intent);
    }

    private int getIntSystemProperty(String key, int defaultValue) {
        try {
            final Class<?> systemProperties = Class.forName("android.os.SystemProperties");
            final Method getInt = systemProperties.getMethod("getInt", String.class, int.class);
            return (Integer) getInt.invoke(null, key, defaultValue);
        } catch (Exception e) {
            Log.e("Exception while getting system property %s. Using default %s.", key,
                    defaultValue, e);
            return defaultValue;
        }
    }
}
