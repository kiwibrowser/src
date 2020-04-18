// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.annotation.TargetApi;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.SharedPreferences;
import android.os.Build;
import android.support.annotation.IntDef;
import android.support.v4.app.NotificationManagerCompat;

import org.chromium.base.ContextUtils;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Helper class to make tracking notification UMA stats easier for various features.  Having a
 * single entry point here to make more complex tracking easier to add in the future.
 */
public class NotificationUmaTracker {
    private static final String TAG = "NotifsUMATracker";
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({DOWNLOAD_FILES, DOWNLOAD_PAGES, CLOSE_INCOGNITO, CONTENT_SUGGESTION, MEDIA_CAPTURE,
            PHYSICAL_WEB, MEDIA, SITES, SYNC, WEBAPK, BROWSER_ACTIONS, WEBAPP_ACTIONS,
            SYSTEM_NOTIFICATION_TYPE_BOUNDARY, OFFLINE_CONTENT_SUGGESTION})
    public @interface SystemNotificationType {}

    /*
     * A list of notification types.  To add a type to this list please update
     * SystemNotificationType in enums.xml and make sure to keep this list in sync.  Additions
     * should be treated as APPEND ONLY to keep the UMA metric semantics the same over time.
     *
     * A SystemNotificationType value can also be saved in shared preferences.
     */
    public static final int DOWNLOAD_FILES = 0;

    public static final int DOWNLOAD_PAGES = 1;

    public static final int CLOSE_INCOGNITO = 2;

    public static final int CONTENT_SUGGESTION = 3;
    public static final int MEDIA_CAPTURE = 4;
    public static final int PHYSICAL_WEB = 5;
    public static final int MEDIA = 6;
    public static final int SITES = 7;
    public static final int SYNC = 8;
    public static final int WEBAPK = 9;
    public static final int BROWSER_ACTIONS = 10;
    public static final int WEBAPP_ACTIONS = 11;
    public static final int OFFLINE_CONTENT_SUGGESTION = 12;

    private static final int SYSTEM_NOTIFICATION_TYPE_BOUNDARY = 13;

    private static final String LAST_SHOWN_NOTIFICATION_TYPE_KEY =
            "NotificationUmaTracker.LastShownNotificationType";

    private static class LazyHolder {
        private static final NotificationUmaTracker INSTANCE = new NotificationUmaTracker();
    }

    /** Cached objects. */
    private final SharedPreferences mSharedPreferences;
    private final NotificationManagerCompat mNotificationManager;

    public static NotificationUmaTracker getInstance() {
        return LazyHolder.INSTANCE;
    }

    private NotificationUmaTracker() {
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
        mNotificationManager = NotificationManagerCompat.from(ContextUtils.getApplicationContext());
    }

    /**
     * Logs {@link android.app.Notification} usage, categorized into {@link SystemNotificationType}
     * types.  Splits the logs by the global enabled state of notifications and also logs the last
     * notification shown prior to the global notifications state being disabled by the user.
     * @param type The type of notification that was shown.
     * @param channelId The id of the notification channel set on the notification.
     * @see SystemNotificationType
     */
    public void onNotificationShown(
            @SystemNotificationType int type, @ChannelDefinitions.ChannelId String channelId) {
        if (!mNotificationManager.areNotificationsEnabled()) {
            logPotentialBlockedCause();
            recordHistogram("Mobile.SystemNotification.Blocked", type);
            return;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O && channelId != null
                && isChannelBlocked(channelId)) {
            recordHistogram("Mobile.SystemNotification.ChannelBlocked", type);
            return;
        }
        saveLastShownNotification(type);
        recordHistogram("Mobile.SystemNotification.Shown", type);
    }

    @TargetApi(26)
    private boolean isChannelBlocked(@ChannelDefinitions.ChannelId String channelId) {
        // Use non-compat notification manager as compat does not have getNotificationChannel (yet).
        NotificationManager notificationManager =
                ContextUtils.getApplicationContext().getSystemService(NotificationManager.class);
        NotificationChannel channel = notificationManager.getNotificationChannel(channelId);
        return channel != null && channel.getImportance() == NotificationManager.IMPORTANCE_NONE;
    }

    private void saveLastShownNotification(@SystemNotificationType int type) {
        mSharedPreferences.edit().putInt(LAST_SHOWN_NOTIFICATION_TYPE_KEY, type).apply();
    }

    private void logPotentialBlockedCause() {
        int lastType = mSharedPreferences.getInt(LAST_SHOWN_NOTIFICATION_TYPE_KEY, -1);
        if (lastType == -1) return;
        mSharedPreferences.edit().remove(LAST_SHOWN_NOTIFICATION_TYPE_KEY).apply();

        recordHistogram("Mobile.SystemNotification.BlockedAfterShown", lastType);
    }

    private static void recordHistogram(String name, @SystemNotificationType int type) {
        if (!LibraryLoader.isInitialized()) return;
        RecordHistogram.recordEnumeratedHistogram(name, type, SYSTEM_NOTIFICATION_TYPE_BOUNDARY);
    }
}
