// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Looper;
import android.util.Log;

import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkVersionUtils;

import java.io.File;
import java.util.Scanner;

/**
 * Creates ClassLoader for WebAPK-specific dex file in Chrome APK's assets.
 */
public class HostBrowserClassLoader {
    /** Directory for storing cached dex files. */
    public static final String DEX_DIR_NAME = "dex";

    private static final String TAG = "cr_HostBrowserClassLoader";

    /**
     * Name of the shared preference for Chrome's version code.
     */
    private static final String REMOTE_VERSION_CODE_PREF =
            "org.chromium.webapk.shell_apk.version_code";

    /**
     * Name of the shared preference for the version number of the dynamically loaded dex.
     */
    private static final String RUNTIME_DEX_VERSION_PREF =
            "org.chromium.webapk.shell_apk.dex_version";

    /*
     * ClassLoader for WebAPK dex. Static so that the same ClassLoader is used for app's lifetime.
     * The ClassLoader is re-created if the host browser is upgraded while the WebAPK is still
     * running.
     */
    private static ClassLoader sClassLoader;

    /**
     * Gets / creates ClassLoader for WebAPK dex.
     * @param context WebAPK's context.
     * @param canaryClassname Class to load to check that ClassLoader is valid.
     * @return The ClassLoader.
     */
    public static ClassLoader getClassLoaderInstance(Context context, String canaryClassName) {
        assertRunningOnUiThread();
        Context remoteContext = WebApkUtils.getHostBrowserContext(context);
        if (remoteContext == null) {
            Log.w(TAG, "Failed to get remote context.");
            return null;
        }

        if (sClassLoader == null || !canReuseClassLoaderInstance(context, remoteContext)) {
            sClassLoader =
                    createClassLoader(context, remoteContext, new DexLoader(), canaryClassName);
        }
        return sClassLoader;
    }

    /**
     * Creates ClassLoader for WebAPK dex.
     * @param context WebAPK's context.
     * @param remoteContext Host browser's context.
     * @param canaryClassName Class to load to check that ClassLoader is valid.
     * @param dexLoader DexLoader for creating ClassLoader.
     * @return The ClassLoader.
     */
    public static ClassLoader createClassLoader(
            Context context, Context remoteContext, DexLoader dexLoader, String canaryClassName) {
        SharedPreferences preferences =
                context.getSharedPreferences(WebApkConstants.PREF_PACKAGE, Context.MODE_PRIVATE);

        int runtimeDexVersion = preferences.getInt(RUNTIME_DEX_VERSION_PREF, -1);
        int newRuntimeDexVersion = checkForNewRuntimeDexVersion(preferences, remoteContext);
        if (newRuntimeDexVersion == -1) {
            newRuntimeDexVersion = runtimeDexVersion;
        }
        File localDexDir = context.getDir(DEX_DIR_NAME, Context.MODE_PRIVATE);
        if (newRuntimeDexVersion != runtimeDexVersion) {
            Log.w(TAG, "Delete cached dex files.");
            dexLoader.deleteCachedDexes(localDexDir);
        }

        String dexAssetName = WebApkVersionUtils.getRuntimeDexName(newRuntimeDexVersion);
        File remoteDexFile =
                new File(remoteContext.getDir(DEX_DIR_NAME, Context.MODE_PRIVATE), dexAssetName);
        return dexLoader.load(
                remoteContext, dexAssetName, canaryClassName, remoteDexFile, localDexDir);
    }

    /**
     * Returns whether {@link sClassLoader} can be reused.
     */
    public static boolean canReuseClassLoaderInstance(Context context, Context remoteContext) {
        // WebAPK may still be running when the host browser gets upgraded. Prevent ClassLoader from
        // getting reused in this scenario.
        SharedPreferences preferences =
                context.getSharedPreferences(WebApkConstants.PREF_PACKAGE, Context.MODE_PRIVATE);
        int cachedRemoteVersionCode = preferences.getInt(REMOTE_VERSION_CODE_PREF, -1);
        int remoteVersionCode = getVersionCode(remoteContext);
        return remoteVersionCode == cachedRemoteVersionCode;
    }

    /**
     * Checks if there is a new "runtime dex" version number. If there is a new version number,
     * updates SharedPreferences.
     * @param preferences WebAPK's SharedPreferences.
     * @param remoteContext
     * @return The new "runtime dex" version number. -1 if there is no new version number.
     */
    private static int checkForNewRuntimeDexVersion(
            SharedPreferences preferences, Context remoteContext) {
        // The "runtime dex" version only changes when {@link remoteContext}'s APK version code
        // changes. Checking the APK's version code is less expensive than reading from the APK's
        // assets.
        int remoteVersionCode = getVersionCode(remoteContext);
        int cachedRemoteVersionCode = preferences.getInt(REMOTE_VERSION_CODE_PREF, -1);
        if (cachedRemoteVersionCode == remoteVersionCode) {
            return -1;
        }

        int runtimeDexVersion = readAssetContentsToInt(remoteContext, "webapk_dex_version.txt");
        SharedPreferences.Editor editor = preferences.edit();
        editor.putInt(REMOTE_VERSION_CODE_PREF, remoteVersionCode);
        editor.putInt(RUNTIME_DEX_VERSION_PREF, runtimeDexVersion);
        editor.apply();
        return runtimeDexVersion;
    }

    /**
     * Returns version code of {@link context}'s APK.
     */
    private static int getVersionCode(Context context) {
        try {
            return context.getPackageManager()
                    .getPackageInfo(context.getPackageName(), 0)
                    .versionCode;
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Failed to get remote package info.");
        }
        return -1;
    }

    /**
     * Returns the first integer in an asset file's contents.
     * @param context
     * @param assetName The name of the asset.
     * @return The first integer.
     */
    private static int readAssetContentsToInt(Context context, String assetName) {
        Scanner scanner = null;
        int value = -1;
        try {
            scanner = new Scanner(context.getAssets().open(assetName));
            value = scanner.nextInt();
            scanner.close();
        } catch (Exception e) {
        } finally {
            if (scanner != null) {
                try {
                    scanner.close();
                } catch (Exception e) {
                }
            }
        }
        return value;
    }

    /**
     * Asserts that current thread is the UI thread.
     */
    private static void assertRunningOnUiThread() {
        assert Looper.getMainLooper().equals(Looper.myLooper());
    }
}
