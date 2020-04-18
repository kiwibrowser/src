// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages;

import android.annotation.TargetApi;
import android.app.DownloadManager;
import android.content.Context;
import android.net.Uri;
import android.os.Build;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Since the {@link AndroidDownloadManager} can only be accessed from Java, this bridge will
 * transfer all C++ calls over to Java land for making the call to ADM.  This is a one-way bridge,
 * from C++ to Java only.  The Java side of this bridge is not called by other Java code.
 */
@JNINamespace("offline_pages::android")
public class OfflinePagesDownloadManagerBridge {
    private static final String TAG = "OfflinePagesDMBridge";
    /** Offline pages should not be scanned as for media content. */
    public static final boolean IS_MEDIA_SCANNER_SCANNABLE = false;

    /** We don't want another download notification, since we already made one. */
    public static final boolean SHOW_NOTIFICATION = false;

    /** Mime type to use for Offline Pages. */
    public static final String MIME_TYPE = "multipart/related";

    /** Returns true if DownloadManager is installed on the phone. */
    @CalledByNative
    private static boolean isAndroidDownloadManagerInstalled() {
        DownloadManager downloadManager = getDownloadManager();
        return (downloadManager != null);
    }

    /**
     * This is a pass through to the {@link AndroidDownloadManager} function of the same name.
     * @param title The display name for this download.
     * @param description Long description for this download.
     * @param path File system path for this download.
     * @param length Length in bytes of this downloaded item.
     * @param uri The origin of this download.  Used in API 24+ only.
     * @param referer Where this download was refered from.  Used in API 24+ only.
     * @return the download ID of this item as assigned by the download manager.
     */
    @CalledByNative
    private static long addCompletedDownload(String title, String description, String path,
            long length, String uri, String referer) {
        try {
            // Call the proper version of the pass through based on the supported API level.
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
                return callAddCompletedDownload(title, description, path, length);
            }

            return callAddCompletedDownload(title, description, path, length, uri, referer);
        } catch (Exception e) {
            // In case of exception, we return a download id of 0.
            Log.d(TAG, "ADM threw while trying to add a download. " + e);
            return 0;
        }
    }

    // Use this pass through before API level 24.
    private static long callAddCompletedDownload(
            String title, String description, String path, long length) {
        DownloadManager downloadManager = getDownloadManager();
        if (downloadManager == null) return 0;

        return downloadManager.addCompletedDownload(title, description, IS_MEDIA_SCANNER_SCANNABLE,
                MIME_TYPE, path, length, SHOW_NOTIFICATION);
    }

    // Use this pass through for API levels 24 and higher.
    @TargetApi(Build.VERSION_CODES.N)
    private static long callAddCompletedDownload(String title, String description, String path,
            long length, String uri, String referer) {
        DownloadManager downloadManager = getDownloadManager();
        if (downloadManager == null) return 0;

        return downloadManager.addCompletedDownload(title, description, IS_MEDIA_SCANNER_SCANNABLE,
                MIME_TYPE, path, length, SHOW_NOTIFICATION, Uri.parse(uri), Uri.parse(referer));
    }

    /**
     * This is a pass through to the {@link AndroidDownloadManager} function of the same name.
     * @param ids An array of download IDs to be removed from the download manager.
     * @return the number of IDs that were removed.
     */
    @CalledByNative
    private static int remove(long[] ids) {
        DownloadManager downloadManager = getDownloadManager();
        try {
            if (downloadManager == null) return 0;

            return downloadManager.remove(ids);
        } catch (Exception e) {
            Log.d(TAG, "ADM threw while trying to remove a download. " + e);
            return 0;
        }
    }

    private static DownloadManager getDownloadManager() {
        return (DownloadManager) ContextUtils.getApplicationContext().getSystemService(
                Context.DOWNLOAD_SERVICE);
    }
}
