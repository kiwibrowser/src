// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import android.support.annotation.IntDef;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.download.ui.DownloadFilter;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Records download related metrics on Android.
 */
public class DownloadMetrics {
    // Tracks where the users interact with download files on Android. Used in histogram.
    // See AndroidDownloadOpenSource in enums.xml. The values used by this enum will be persisted
    // to server logs and should not be deleted, changed or reused.
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({UNKNOWN, ANDROID_DOWNLOAD_MANAGER, DOWNLOAD_HOME, NOTIFICATION, NEW_TAP_PAGE, INFO_BAR,
            SNACK_BAR, AUTO_OPEN, DOWNLOAD_PROGRESS_INFO_BAR, DOWNLOAD_SOURCE_BOUNDARY})
    public @interface DownloadOpenSource {}

    public static final int UNKNOWN = 0;
    public static final int ANDROID_DOWNLOAD_MANAGER = 1;
    public static final int DOWNLOAD_HOME = 2;
    public static final int NOTIFICATION = 3;
    public static final int NEW_TAP_PAGE = 4;
    public static final int INFO_BAR = 5;
    public static final int SNACK_BAR = 6;
    public static final int AUTO_OPEN = 7;
    public static final int DOWNLOAD_PROGRESS_INFO_BAR = 8;
    private static final int DOWNLOAD_SOURCE_BOUNDARY = 9;

    private static final String TAG = "DownloadMetrics";
    private static final int MAX_VIEW_RETENTION_MINUTES = 30 * 24 * 60;

    /**
     * Records download open source.
     * @param source The source where the user opened the download media file.
     * @param mimeType The mime type of the download.
     */
    public static void recordDownloadOpen(@DownloadOpenSource int source, String mimeType) {
        if (!isNativeLoaded()) {
            Log.w(TAG, "Native is not loaded, dropping download open metrics.");
            return;
        }

        int type = DownloadFilter.fromMimeType(mimeType);
        if (type == DownloadFilter.FILTER_VIDEO) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Android.DownloadManager.OpenSource.Video", source, DOWNLOAD_SOURCE_BOUNDARY);
        } else if (type == DownloadFilter.FILTER_AUDIO) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Android.DownloadManager.OpenSource.Audio", source, DOWNLOAD_SOURCE_BOUNDARY);
        }
    }

    /**
     * Records how long does the user keep the download file on disk when the user tries to open
     * the file.
     * @param mimeType The mime type of the download.
     * @param startTime The start time of the download.
     */
    public static void recordDownloadViewRetentionTime(String mimeType, long startTime) {
        if (!isNativeLoaded()) {
            Log.w(TAG, "Native is not loaded, dropping download view retention metrics.");
            return;
        }

        int type = DownloadFilter.fromMimeType(mimeType);
        int viewRetentionTimeMinutes = (int) ((System.currentTimeMillis() - startTime) / 60000);

        if (type == DownloadFilter.FILTER_VIDEO) {
            RecordHistogram.recordCustomCountHistogram(
                    "Android.DownloadManager.ViewRetentionTime.Video", viewRetentionTimeMinutes, 1,
                    MAX_VIEW_RETENTION_MINUTES, 50);
        } else if (type == DownloadFilter.FILTER_AUDIO) {
            RecordHistogram.recordCustomCountHistogram(
                    "Android.DownloadManager.ViewRetentionTime.Audio", viewRetentionTimeMinutes, 1,
                    MAX_VIEW_RETENTION_MINUTES, 50);
        }
    }

    private static boolean isNativeLoaded() {
        return ChromeBrowserInitializer.getInstance(ContextUtils.getApplicationContext())
                .hasNativeInitializationCompleted();
    }
}
