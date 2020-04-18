// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download;

import static android.app.DownloadManager.ACTION_NOTIFICATION_CLICKED;

import static org.chromium.chrome.browser.download.DownloadNotificationService2.ACTION_DOWNLOAD_CANCEL;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.ACTION_DOWNLOAD_OPEN;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.ACTION_DOWNLOAD_PAUSE;
import static org.chromium.chrome.browser.download.DownloadNotificationService2.ACTION_DOWNLOAD_RESUME;

import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.metrics.RecordHistogram;

import java.util.Arrays;
import java.util.List;

/**
 * Helper to track necessary stats in UMA related to downloads notifications.
 */
public final class DownloadNotificationUmaHelper {
    // The state of a download or offline page request at user-initiated cancel.
    // Keep in sync with enum OfflineItemsStateAtCancel in enums.xml.
    static class StateAtCancel {
        static final int DOWNLOADING = 0;
        static final int PAUSED = 1;
        static final int PENDING_NETWORK = 2;
        static final int PENDING_ANOTHER_DOWNLOAD = 3;
        static final int MAX = 4;
    }

    // NOTE: Keep these lists/classes in sync with DownloadNotification[...] in enums.xml.
    static class ForegroundLifecycle {
        static final int START = 0; // Initial startForeground.
        static final int UPDATE = 1; // Switching pinned notification.
        static final int STOP = 2; // Calling stopForeground.
        static final int MAX = 3;
    }

    private static List<String> sInteractions = Arrays.asList(
            ACTION_NOTIFICATION_CLICKED, // Opening a download where LegacyHelpers.isLegacyDownload.
            ACTION_DOWNLOAD_OPEN, // Opening a download that is not a legacy download.
            ACTION_DOWNLOAD_CANCEL, ACTION_DOWNLOAD_PAUSE, ACTION_DOWNLOAD_RESUME);

    static class LaunchType {
        static final int LAUNCH = 0; // "Denominator" for expected launched notifications.
        static final int RELAUNCH = 1;
        static final int MAX = 2;
    }

    static class ServiceStopped {
        static final int STOPPED = 0; // Expected, intentional stops, serves as a "denominator".
        static final int DESTROYED = 1;
        static final int TASK_REMOVED = 2;
        static final int LOW_MEMORY = 3;
        static final int START_STICKY = 4;
        static final int MAX = 5;
    }

    /**
     * Records an instance where a user interacts with a notification (clicks on, pauses, etc).
     * @param action Notification interaction that was taken (ie. pause, resume).
     */
    static void recordNotificationInteractionHistogram(String action) {
        if (!LibraryLoader.isInitialized()) return;
        int actionType = sInteractions.indexOf(action);
        if (actionType == -1) return;
        RecordHistogram.recordEnumeratedHistogram("Android.DownloadManager.NotificationInteraction",
                actionType, sInteractions.size());
    }

    /**
     * Records an instance where the foreground stops, using expected stops as the denominator to
     * understand the frequency of unexpected stops (low memory, task removed, etc).
     * @param stopType Type of the foreground stop that is being recorded ({@link ServiceStopped}).
     */
    static void recordServiceStoppedHistogram(int stopType, boolean withForeground) {
        if (!LibraryLoader.isInitialized()) return;
        if (withForeground) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Android.DownloadManager.ServiceStopped.DownloadForeground", stopType,
                    ServiceStopped.MAX);
        } else {
            RecordHistogram.recordEnumeratedHistogram(
                    "Android.DownloadManager.ServiceStopped.DownloadNotification", stopType,
                    ServiceStopped.MAX);
        }
    }

    /**
     * Records an instance where the foreground undergoes a lifecycle change (when the foreground
     * starts, changes pinned notification, or stops).
     * @param lifecycleStep The lifecycle step that is being recorded ({@link ForegroundLifecycle}).
     */
    static void recordForegroundServiceLifecycleHistogram(int lifecycleStep) {
        if (!LibraryLoader.isInitialized()) return;
        RecordHistogram.recordEnumeratedHistogram(
                "Android.DownloadManager.ForegroundServiceLifecycle", lifecycleStep,
                ForegroundLifecycle.MAX);
    }

    /**
     * Record the number of existing notifications when a new notification is being launched (more
     * specifically the number of existing shared preference entries when a new shared preference
     * entry is being recorded).
     * @param count The number of existing notifications.
     * @param withForeground Whether this is with foreground enabled or not.
     */
    static void recordExistingNotificationsCountHistogram(int count, boolean withForeground) {
        if (!LibraryLoader.isInitialized()) return;
        if (withForeground) {
            RecordHistogram.recordCountHistogram(
                    "Android.DownloadManager.NotificationsCount.ForegroundEnabled", count);
        } else {
            RecordHistogram.recordCountHistogram(
                    "Android.DownloadManager.NotificationsCount.ForegroundDisabled", count);
        }
    }

    /**
     * Record an instance when a notification is being launched for the first time or relaunched due
     * to the need to dissociate the notification from the foreground (only on API < 24).
     * @param launchType Whether it is a launch or a relaunch ({@link LaunchType}).
     */
    static void recordNotificationFlickerCountHistogram(int launchType) {
        if (!LibraryLoader.isInitialized()) return;
        RecordHistogram.recordEnumeratedHistogram(
                "Android.DownloadManager.NotificationLaunch", launchType, LaunchType.MAX);
    }

    /**
     * Records the state of a request at user-initiated cancel.
     * @param isDownload True if the request is a download, false if it is an offline page.
     * @param state State of a request when cancelled (e.g. downloading, paused).
     */
    static void recordStateAtCancelHistogram(boolean isDownload, int state) {
        if (state == -1) return;
        if (!LibraryLoader.isInitialized()) return;
        if (isDownload) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Android.OfflineItems.StateAtCancel.Downloads", state, StateAtCancel.MAX);
        } else {
            RecordHistogram.recordEnumeratedHistogram(
                    "Android.OfflineItems.StateAtCancel.OfflinePages", state, StateAtCancel.MAX);
        }
    }
}
