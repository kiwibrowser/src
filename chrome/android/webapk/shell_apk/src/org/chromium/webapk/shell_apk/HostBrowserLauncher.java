// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webapk.shell_apk;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;

import org.chromium.webapk.lib.common.WebApkConstants;
import org.chromium.webapk.lib.common.WebApkMetaDataKeys;

import java.util.List;

/**
 * Selects host browser to launch, showing dialog to select browser if necessary, and launches
 * browser.
 */
class HostBrowserLauncher {
    private static final String LAST_RESORT_HOST_BROWSER = "com.android.chrome";
    private static final String LAST_RESORT_HOST_BROWSER_APPLICATION_NAME = "Google Chrome";
    private static final String TAG = "cr_HostBrowserLauncher";

    // Action for launching {@link WebappLauncherActivity}.
    // TODO(hanxi): crbug.com/737556. Replaces this string with the new WebAPK launch action after
    // it is propagated to all the Chrome's channels.
    public static final String ACTION_START_WEBAPK =
            "com.google.android.apps.chrome.webapps.WebappManager.ACTION_START_WEBAPP";

    // Must stay in sync with
    // {@link org.chromium.chrome.browser.ShortcutHelper#REUSE_URL_MATCHING_TAB_ELSE_NEW_TAB}.
    private static final String REUSE_URL_MATCHING_TAB_ELSE_NEW_TAB =
            "REUSE_URL_MATCHING_TAB_ELSE_NEW_TAB";

    /** Parent activity for any dialogs. */
    private Activity mParentActivity;

    private Context mContext;

    /** Intent used to launch WebAPK. */
    private Intent mIntent;

    /** URL to launch WebAPK at. */
    private String mStartUrl;

    /** The source which is launching/navigating the WebAPK. */
    private int mSource;

    /** Whether the WebAPK should be navigated to {@link mStartUrl} if it is already running. */
    private boolean mForceNavigation;

    private long mWebApkLaunchTime;

    public HostBrowserLauncher(Activity parentActivity, Intent intent, String startUrl, int source,
            boolean forceNavigation, long webApkLaunchTime) {
        mParentActivity = parentActivity;
        mContext = parentActivity.getApplicationContext();
        mIntent = intent;
        mStartUrl = startUrl;
        mSource = source;
        mForceNavigation = forceNavigation;
        mWebApkLaunchTime = webApkLaunchTime;
    }

    /**
     * Creates install Intent.
     * @param packageName Package to install.
     * @return The intent.
     */
    private static Intent createInstallIntent(String packageName) {
        String marketUrl = "market://details?id=" + packageName;
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(marketUrl));
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return intent;
    }

    /**
     * Selects host browser to launch, showing dialog to select browser if necessary, and launches
     * browser.
     */
    public void selectHostBrowserAndLaunch(Runnable finishCallback) {
        Bundle metadata = WebApkUtils.readMetaData(mContext);
        if (metadata == null) {
            finishCallback.run();
            return;
        }

        Log.v(TAG, "Url of the WebAPK: " + mStartUrl);
        String packageName = mContext.getPackageName();
        Log.v(TAG, "Package name of the WebAPK:" + packageName);

        String runtimeHostInPreferences = WebApkUtils.getHostBrowserFromSharedPreference(mContext);
        String runtimeHost = WebApkUtils.getHostBrowserPackageName(mContext);
        if (!TextUtils.isEmpty(runtimeHostInPreferences)
                && !runtimeHostInPreferences.equals(runtimeHost)) {
            deleteSharedPref();
            deleteInternalStorage();
        }

        if (!TextUtils.isEmpty(runtimeHost)) {
            launchInHostBrowser(runtimeHost, false);
            finishCallback.run();
            return;
        }

        List<ResolveInfo> infos =
                WebApkUtils.getInstalledBrowserResolveInfos(mContext.getPackageManager());
        if (hasBrowserSupportingWebApks(infos)) {
            showChooseHostBrowserDialog(infos, finishCallback);
        } else {
            showInstallHostBrowserDialog(metadata, finishCallback);
        }
    }

    /** Deletes the SharedPreferences. */
    private void deleteSharedPref() {
        SharedPreferences sharedPref =
                mContext.getSharedPreferences(WebApkConstants.PREF_PACKAGE, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPref.edit();
        editor.clear();
        editor.apply();
    }

    /** Deletes the internal storage. */
    private void deleteInternalStorage() {
        DexLoader.deletePath(mContext.getCacheDir());
        DexLoader.deletePath(mContext.getFilesDir());
        DexLoader.deletePath(
                mContext.getDir(HostBrowserClassLoader.DEX_DIR_NAME, Context.MODE_PRIVATE));
    }

    private void launchInHostBrowser(String runtimeHost, boolean hostBrowserDialogShown) {
        PackageInfo info;
        try {
            info = mContext.getPackageManager().getPackageInfo(runtimeHost, 0);
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Unable to get the host browser's package info.");
            return;
        }

        if (WebApkUtils.shouldLaunchInTab(info.versionName)) {
            launchInTab(runtimeHost);
            return;
        }

        Intent intent = new Intent();
        intent.setAction(ACTION_START_WEBAPK);
        intent.setPackage(runtimeHost);
        Bundle copiedExtras = mIntent.getExtras();
        if (copiedExtras != null) {
            intent.putExtras(copiedExtras);
        }
        intent.putExtra(WebApkConstants.EXTRA_URL, mStartUrl)
                .putExtra(WebApkConstants.EXTRA_SOURCE, mSource)
                .putExtra(WebApkConstants.EXTRA_WEBAPK_PACKAGE_NAME, mContext.getPackageName())
                .putExtra(WebApkConstants.EXTRA_FORCE_NAVIGATION, mForceNavigation);

        // Only pass on the start time if no user action was required between launching the webapk
        // and chrome starting up. See https://crbug.com/842023
        if (!hostBrowserDialogShown) {
            intent.putExtra(WebApkConstants.EXTRA_WEBAPK_LAUNCH_TIME, mWebApkLaunchTime);
        }

        try {
            mParentActivity.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Log.w(TAG, "Unable to launch browser in WebAPK mode.");
            e.printStackTrace();
        }
    }

    /** Launches a WebAPK in its runtime host browser as a tab. */
    private void launchInTab(String runtimeHost) {
        Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(mStartUrl));
        intent.setPackage(runtimeHost);
        intent.putExtra(REUSE_URL_MATCHING_TAB_ELSE_NEW_TAB, true)
                .putExtra(WebApkConstants.EXTRA_SOURCE, mSource);
        try {
            mParentActivity.startActivity(intent);
        } catch (ActivityNotFoundException e) {
        }
    }

    /**
     * Launches the Play Store with the host browser's page.
     */
    private void installBrowser(String hostBrowserPackageName) {
        try {
            mParentActivity.startActivity(createInstallIntent(hostBrowserPackageName));
        } catch (ActivityNotFoundException e) {
        }
    }

    /** Returns whether there is any installed browser supporting WebAPKs. */
    private static boolean hasBrowserSupportingWebApks(List<ResolveInfo> resolveInfos) {
        List<String> browsersSupportingWebApk = WebApkUtils.getBrowsersSupportingWebApk();
        for (ResolveInfo info : resolveInfos) {
            if (browsersSupportingWebApk.contains(info.activityInfo.packageName)) {
                return true;
            }
        }
        return false;
    }

    /** Shows a dialog to choose the host browser. */
    private void showChooseHostBrowserDialog(List<ResolveInfo> infos, Runnable finishCallback) {
        ChooseHostBrowserDialog.DialogListener listener =
                new ChooseHostBrowserDialog.DialogListener() {
                    @Override
                    public void onHostBrowserSelected(String selectedHostBrowser) {
                        launchInHostBrowser(selectedHostBrowser, true);
                        WebApkUtils.writeHostBrowserToSharedPref(mContext, selectedHostBrowser);
                        finishCallback.run();
                    }
                    @Override
                    public void onQuit() {
                        finishCallback.run();
                    }
                };
        ChooseHostBrowserDialog.show(
                mParentActivity, listener, infos, mContext.getString(R.string.name));
    }

    /** Shows a dialog to install the host browser. */
    private void showInstallHostBrowserDialog(Bundle metadata, Runnable finishCallback) {
        String lastResortHostBrowserPackageName =
                metadata.getString(WebApkMetaDataKeys.RUNTIME_HOST);
        String lastResortHostBrowserApplicationName =
                metadata.getString(WebApkMetaDataKeys.RUNTIME_HOST_APPLICATION_NAME);

        if (TextUtils.isEmpty(lastResortHostBrowserPackageName)) {
            // WebAPKs without runtime host specified in the AndroidManifest.xml always install
            // Google Chrome as the default host browser.
            lastResortHostBrowserPackageName = LAST_RESORT_HOST_BROWSER;
            lastResortHostBrowserApplicationName = LAST_RESORT_HOST_BROWSER_APPLICATION_NAME;
        }

        InstallHostBrowserDialog.DialogListener listener =
                new InstallHostBrowserDialog.DialogListener() {
                    @Override
                    public void onConfirmInstall(String packageName) {
                        installBrowser(packageName);
                        WebApkUtils.writeHostBrowserToSharedPref(mContext, packageName);
                        finishCallback.run();
                    }
                    @Override
                    public void onConfirmQuit() {
                        finishCallback.run();
                    }
                };

        InstallHostBrowserDialog.show(mParentActivity, listener, mContext.getString(R.string.name),
                lastResortHostBrowserPackageName, lastResortHostBrowserApplicationName,
                R.drawable.last_resort_runtime_host_logo);
    }
}
