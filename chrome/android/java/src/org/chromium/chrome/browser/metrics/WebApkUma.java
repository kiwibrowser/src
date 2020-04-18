// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.metrics;

import android.Manifest;
import android.content.ContentResolver;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Environment;
import android.os.StatFs;
import android.provider.Settings;
import android.text.TextUtils;

import org.chromium.base.ContextUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.website.SiteSettingsCategory;
import org.chromium.chrome.browser.preferences.website.Website;
import org.chromium.chrome.browser.preferences.website.WebsitePermissionsFetcher;
import org.chromium.webapk.lib.common.WebApkConstants;

import java.io.File;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Centralizes UMA data collection for WebAPKs. NOTE: Histogram names and values are defined in
 * tools/metrics/histograms/histograms.xml. Please update that file if any change is made.
 */
public class WebApkUma {
    // This enum is used to back UMA histograms, and should therefore be treated as append-only.
    // Deprecated: UPDATE_REQUEST_SENT_FIRST_TRY = 0;
    // Deprecated: UPDATE_REQUEST_SENT_ONSTOP = 1;
    // Deprecated: UPDATE_REQUEST_SENT_WHILE_WEBAPK_IN_FOREGROUND = 2;
    public static final int UPDATE_REQUEST_SENT_WHILE_WEBAPK_CLOSED = 3;
    public static final int UPDATE_REQUEST_SENT_MAX = 4;

    // This enum is used to back UMA histograms, and should therefore be treated as append-only.
    // The queued request times shouldn't exceed three.
    public static final int UPDATE_REQUEST_QUEUED_ONCE = 0;
    public static final int UPDATE_REQUEST_QUEUED_TWICE = 1;
    public static final int UPDATE_REQUEST_QUEUED_THREE_TIMES = 2;
    public static final int UPDATE_REQUEST_QUEUED_MAX = 3;

    // This enum is used to back UMA histograms, and should therefore be treated as append-only.
    public static final int GOOGLE_PLAY_INSTALL_SUCCESS = 0;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_NO_DELEGATE = 1;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_TO_CONNECT_TO_SERVICE = 2;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_CALLER_VERIFICATION_FAILURE = 3;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_POLICY_VIOLATION = 4;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_API_DISABLED = 5;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_REQUEST_FAILED = 6;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_DOWNLOAD_CANCELLED = 7;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_DOWNLOAD_ERROR = 8;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_INSTALL_ERROR = 9;
    public static final int GOOGLE_PLAY_INSTALL_FAILED_INSTALL_TIMEOUT = 10;
    // GOOGLE_PLAY_INSTALL_REQUEST_FAILED_* errors are the error codes shown in the "reason" of
    // the returned Bundle when calling installPackage() API returns false.
    public static final int GOOGLE_PLAY_INSTALL_REQUEST_FAILED_POLICY_DISABLED = 11;
    public static final int GOOGLE_PLAY_INSTALL_REQUEST_FAILED_UNKNOWN_ACCOUNT = 12;
    public static final int GOOGLE_PLAY_INSTALL_REQUEST_FAILED_NETWORK_ERROR = 13;
    public static final int GOOGLE_PLAY_INSTALL_REQUSET_FAILED_RESOLVE_ERROR = 14;
    public static final int GOOGLE_PLAY_INSTALL_REQUEST_FAILED_NOT_GOOGLE_SIGNED = 15;
    public static final int GOOGLE_PLAY_INSTALL_RESULT_MAX = 16;

    // This enum is used to back UMA histograms, and should therefore be treated as append-only.
    private static final int PERMISSION_OTHER = 0;
    private static final int PERMISSION_LOCATION = 1;
    private static final int PERMISSION_MICROPHONE = 2;
    private static final int PERMISSION_CAMERA = 3;
    private static final int PERMISSION_STORAGE = 4;
    private static final int PERMISSION_COUNT = 5;

    public static final String HISTOGRAM_UPDATE_REQUEST_SENT =
            "WebApk.Update.RequestSent";

    public static final String HISTOGRAM_UPDATE_REQUEST_QUEUED = "WebApk.Update.RequestQueued";

    private static final int WEBAPK_OPEN_MAX = 3;
    public static final int WEBAPK_OPEN_LAUNCH_SUCCESS = 0;
    // Obsolete: WEBAPK_OPEN_NO_LAUNCH_INTENT = 1;
    public static final int WEBAPK_OPEN_ACTIVITY_NOT_FOUND = 2;

    private static final String ADJUST_WEBAPK_INSTALLATION_SPACE_PARAM =
            "webapk_extra_installation_space_mb";

    /**
     * Records the time point when a request to update a WebAPK is sent to the WebAPK Server.
     * @param type representing when the update request is sent to the WebAPK server.
     */
    public static void recordUpdateRequestSent(int type) {
        assert type >= 0 && type < UPDATE_REQUEST_SENT_MAX;
        RecordHistogram.recordEnumeratedHistogram(HISTOGRAM_UPDATE_REQUEST_SENT,
                type, UPDATE_REQUEST_SENT_MAX);
    }

    /**
     * Records the times that an update request has been queued once, twice and three times before
     * sending to WebAPK server.
     * @param times representing the times that an update has been queued.
     */
    public static void recordUpdateRequestQueued(int times) {
        RecordHistogram.recordEnumeratedHistogram(HISTOGRAM_UPDATE_REQUEST_QUEUED, times,
                UPDATE_REQUEST_QUEUED_MAX);
    }

    /**
     * When a user presses on the "Open WebAPK" menu item, this records whether the WebAPK was
     * opened successfully.
     * @param type Result of trying to open WebAPK.
     */
    public static void recordWebApkOpenAttempt(int type) {
        assert type >= 0 && type < WEBAPK_OPEN_MAX;
        RecordHistogram.recordEnumeratedHistogram("WebApk.OpenFromMenu", type, WEBAPK_OPEN_MAX);
    }

    /** Records whether a WebAPK has permission to display notifications. */
    public static void recordNotificationPermissionStatus(boolean permissionEnabled) {
        RecordHistogram.recordBooleanHistogram(
                "WebApk.Notification.Permission.Status", permissionEnabled);
    }

    /**
     * Records whether installing a WebAPK from Google Play succeeded. If not, records the reason
     * that the install failed.
     */
    public static void recordGooglePlayInstallResult(int result) {
        assert result >= 0 && result < GOOGLE_PLAY_INSTALL_RESULT_MAX;
        RecordHistogram.recordEnumeratedHistogram(
                "WebApk.Install.GooglePlayInstallResult", result, GOOGLE_PLAY_INSTALL_RESULT_MAX);
    }

    /** Records the error code if installing a WebAPK via Google Play fails. */
    public static void recordGooglePlayInstallErrorCode(int errorCode) {
        // Don't use an enumerated histogram as there are > 30 potential error codes. In practice,
        // a given client will always get the same error code.
        RecordHistogram.recordSparseSlowlyHistogram(
                "WebApk.Install.GooglePlayErrorCode", Math.min(errorCode, 1000));
    }

    /**
     * Records whether updating a WebAPK from Google Play succeeded. If not, records the reason
     * that the update failed.
     */
    public static void recordGooglePlayUpdateResult(int result) {
        assert result >= 0 && result < GOOGLE_PLAY_INSTALL_RESULT_MAX;
        RecordHistogram.recordEnumeratedHistogram(
                "WebApk.Update.GooglePlayUpdateResult", result, GOOGLE_PLAY_INSTALL_RESULT_MAX);
    }

    /** Records the duration of a WebAPK session (from launch/foreground to background). */
    public static void recordWebApkSessionDuration(long duration) {
        RecordHistogram.recordLongTimesHistogram(
                "WebApk.Session.TotalDuration", duration, TimeUnit.MILLISECONDS);
    }

    /** Records the amount of time that it takes to bind to the play install service. */
    public static void recordGooglePlayBindDuration(long durationMs) {
        RecordHistogram.recordTimesHistogram(
                "WebApk.Install.GooglePlayBindDuration", durationMs, TimeUnit.MILLISECONDS);
    }

    /** Records the current Shell APK version. */
    public static void recordShellApkVersion(int shellApkVersion, String packageName) {
        String name = packageName.startsWith(WebApkConstants.WEBAPK_PACKAGE_PREFIX)
                ? "WebApk.ShellApkVersion.BrowserApk"
                : "WebApk.ShellApkVersion.UnboundApk";
        RecordHistogram.recordSparseSlowlyHistogram(name, shellApkVersion);
    }

    /**
     * Records the requests of Android runtime permissions which haven't been granted to Chrome when
     * Chrome is running in WebAPK runtime.
     */
    public static void recordAndroidRuntimePermissionPromptInWebApk(final String[] permissions) {
        recordPermissionUma("WebApk.Permission.ChromeWithoutPermission", permissions);
    }

    /**
     * Records the amount of requests that WekAPK runtime permissions access is deined because
     * Chrome does not have that permission.
     */
    public static void recordAndroidRuntimePermissionDeniedInWebApk(final String[] permissions) {
        recordPermissionUma("WebApk.Permission.ChromePermissionDenied2", permissions);
    }

    private static void recordPermissionUma(String permissionUmaName, final String[] permissions) {
        Set<Integer> permissionGroups = new HashSet<Integer>();
        for (String permission : permissions) {
            permissionGroups.add(getPermissionGroup(permission));
        }
        for (Integer permission : permissionGroups) {
            RecordHistogram.recordEnumeratedHistogram(
                    permissionUmaName, permission, PERMISSION_COUNT);
        }
    }

    private static int getPermissionGroup(String permission) {
        if (TextUtils.equals(permission, Manifest.permission.ACCESS_COARSE_LOCATION)
                || TextUtils.equals(permission, Manifest.permission.ACCESS_FINE_LOCATION)) {
            return PERMISSION_LOCATION;
        }
        if (TextUtils.equals(permission, Manifest.permission.RECORD_AUDIO)) {
            return PERMISSION_MICROPHONE;
        }
        if (TextUtils.equals(permission, Manifest.permission.CAMERA)) {
            return PERMISSION_CAMERA;
        }
        if (TextUtils.equals(permission, Manifest.permission.READ_EXTERNAL_STORAGE)
                || TextUtils.equals(permission, Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
            return PERMISSION_STORAGE;
        }
        return PERMISSION_OTHER;
    }

    /**
     * Recorded when a WebAPK is launched from the homescreen. Records the time elapsed since the
     * previous WebAPK launch. Not recorded the first time that a WebAPK is launched.
     */
    public static void recordLaunchInterval(long intervalMs) {
        RecordHistogram.recordCustomCountHistogram("WebApk.LaunchInterval2",
                (int) TimeUnit.MILLISECONDS.toMinutes(intervalMs), 30,
                (int) TimeUnit.DAYS.toMinutes(90), 50);
    }

    /** Records to UMA the count of old "WebAPK update request" files. */
    public static void recordNumberOfStaleWebApkUpdateRequestFiles(int count) {
        RecordHistogram.recordCountHistogram("WebApk.Update.NumStaleUpdateRequestFiles", count);
    }

    /** Records whether Chrome could bind to the WebAPK service. */
    public static void recordBindToWebApkServiceSucceeded(boolean bindSucceeded) {
        RecordHistogram.recordBooleanHistogram("WebApk.WebApkService.BindSuccess", bindSucceeded);
    }

    /** Records the network error code caught when a WebAPK is launched. */
    public static void recordNetworkErrorWhenLaunch(int errorCode) {
        RecordHistogram.recordSparseSlowlyHistogram("WebApk.Launch.NetworkError", -errorCode);
    }

    /**
     * Log necessary disk usage and cache size UMAs when WebAPK installation fails.
     */
    public static void logSpaceUsageUMAWhenInstallationFails() {
        new AsyncTask<Void, Void, Void>() {
            long mAvailableSpaceInByte = 0;
            long mCacheSizeInByte = 0;
            @Override
            protected Void doInBackground(Void... params) {
                mAvailableSpaceInByte = getAvailableSpaceAboveLowSpaceLimit();
                mCacheSizeInByte = getCacheDirSize();
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                logSpaceUsageUMA(mAvailableSpaceInByte, mCacheSizeInByte);
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private static void logSpaceUsageUMA(long availableSpaceInByte, long cacheSizeInByte) {
        WebsitePermissionsFetcher fetcher = new WebsitePermissionsFetcher(
                new UnimportantStorageSizeCalculator(availableSpaceInByte, cacheSizeInByte));
        fetcher.fetchPreferencesForCategory(
                SiteSettingsCategory.fromString(SiteSettingsCategory.CATEGORY_USE_STORAGE));
    }

    private static void logSpaceUsageUMAOnDataAvailable(
            long spaceSize, long cacheSize, long unimportantSiteSize) {
        RecordHistogram.recordSparseSlowlyHistogram(
                "WebApk.Install.AvailableSpace.Fail", roundByteToMb(spaceSize));

        RecordHistogram.recordSparseSlowlyHistogram(
                "WebApk.Install.ChromeCacheSize.Fail", roundByteToMb(cacheSize));

        RecordHistogram.recordSparseSlowlyHistogram(
                "WebApk.Install.ChromeUnimportantStorage.Fail", roundByteToMb(unimportantSiteSize));

        RecordHistogram.recordSparseSlowlyHistogram(
                "WebApk.Install.AvailableSpaceAfterFreeUpCache.Fail",
                roundByteToMb(spaceSize + cacheSize));

        RecordHistogram.recordSparseSlowlyHistogram(
                "WebApk.Install.AvailableSpaceAfterFreeUpUnimportantStorage.Fail",
                roundByteToMb(spaceSize + unimportantSiteSize));

        RecordHistogram.recordSparseSlowlyHistogram(
                "WebApk.Install.AvailableSpaceAfterFreeUpAll.Fail",
                roundByteToMb(spaceSize + cacheSize + unimportantSiteSize));
    }

    private static int roundByteToMb(long bytes) {
        int mbs = (int) (bytes / 1024L / 1024L / 10L * 10L);
        return Math.min(1000, Math.max(-1000, mbs));
    }

    private static long getDirectorySizeInByte(File dir) {
        if (dir == null) return 0;
        if (!dir.isDirectory()) return dir.length();

        long sizeInByte = 0;
        try {
            File[] files = dir.listFiles();
            if (files == null) return 0;

            for (File file : files) {
                sizeInByte += getDirectorySizeInByte(file);
            }
        } catch (SecurityException e) {
            return 0;
        }
        return sizeInByte;
    }

    /**
     * @return The available space that can be used to install WebAPK. Negative value means there is
     * less free space available than the system's minimum by the given amount.
     */
    @SuppressWarnings("deprecation")
    public static long getAvailableSpaceAboveLowSpaceLimit() {
        long partitionAvailableBytes;
        long partitionTotalBytes;
        StatFs partitionStats = new StatFs(Environment.getDataDirectory().getAbsolutePath());
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            partitionAvailableBytes = partitionStats.getAvailableBytes();
            partitionTotalBytes = partitionStats.getTotalBytes();
        } else {
            // these APIs were deprecated in API level 18.
            long blockSize = partitionStats.getBlockSize();
            partitionAvailableBytes = blockSize * (long) partitionStats.getAvailableBlocks();
            partitionTotalBytes = blockSize * (long) partitionStats.getBlockCount();
        }
        long minimumFreeBytes = getLowSpaceLimitBytes(partitionTotalBytes);

        long webApkExtraSpaceBytes = 0;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            // Extra installation space is only allowed >= Android L
            webApkExtraSpaceBytes = ChromeFeatureList.getFieldTrialParamByFeatureAsInt(
                                            ChromeFeatureList.ADJUST_WEBAPK_INSTALLATION_SPACE,
                                            ADJUST_WEBAPK_INSTALLATION_SPACE_PARAM, 0)
                    * 1024L * 1024L;
        }

        return partitionAvailableBytes - minimumFreeBytes + webApkExtraSpaceBytes;
    }

    /**
     * @return Size of the cache directory.
     */
    public static long getCacheDirSize() {
        return getDirectorySizeInByte(ContextUtils.getApplicationContext().getCacheDir());
    }

    /**
     * Mirror the system-derived calculation of reserved bytes and return that value.
     */
    private static long getLowSpaceLimitBytes(long partitionTotalBytes) {
        // Copied from android/os/storage/StorageManager.java
        final int defaultThresholdPercentage = 10;
        // Copied from android/os/storage/StorageManager.java
        final long defaultThresholdMaxBytes = 500 * 1024 * 1024;
        // Copied from android/provider/Settings.java
        final String sysStorageThresholdPercentage = "sys_storage_threshold_percentage";
        // Copied from android/provider/Settings.java
        final String sysStorageThresholdMaxBytes = "sys_storage_threshold_max_bytes";

        ContentResolver resolver = ContextUtils.getApplicationContext().getContentResolver();
        int minFreePercent = 0;
        long minFreeBytes = 0;

        // Retrieve platform-appropriate values first
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            minFreePercent = Settings.Global.getInt(
                    resolver, sysStorageThresholdPercentage, defaultThresholdPercentage);
            minFreeBytes = Settings.Global.getLong(
                    resolver, sysStorageThresholdMaxBytes, defaultThresholdMaxBytes);
        } else {
            minFreePercent = Settings.Secure.getInt(
                    resolver, sysStorageThresholdPercentage, defaultThresholdPercentage);
            minFreeBytes = Settings.Secure.getLong(
                    resolver, sysStorageThresholdMaxBytes, defaultThresholdMaxBytes);
        }

        long minFreePercentInBytes = (partitionTotalBytes * minFreePercent) / 100;

        return Math.min(minFreeBytes, minFreePercentInBytes);
    }

    private static class UnimportantStorageSizeCalculator
            implements WebsitePermissionsFetcher.WebsitePermissionsCallback {
        private long mAvailableSpaceInByte;
        private long mCacheSizeInByte;

        UnimportantStorageSizeCalculator(long availableSpaceInByte, long cacheSizeInByte) {
            mAvailableSpaceInByte = availableSpaceInByte;
            mCacheSizeInByte = cacheSizeInByte;
        }
        @Override
        public void onWebsitePermissionsAvailable(Collection<Website> sites) {
            long siteStorageSize = 0;
            long importantSiteStorageTotal = 0;
            for (Website site : sites) {
                siteStorageSize += site.getTotalUsage();
                if (site.getLocalStorageInfo() != null
                        && site.getLocalStorageInfo().isDomainImportant()) {
                    importantSiteStorageTotal += site.getTotalUsage();
                }
            }

            long unimportantSiteStorageTotal = siteStorageSize - importantSiteStorageTotal;
            logSpaceUsageUMAOnDataAvailable(
                    mAvailableSpaceInByte, mCacheSizeInByte, unimportantSiteStorageTotal);
        }
    }
}
