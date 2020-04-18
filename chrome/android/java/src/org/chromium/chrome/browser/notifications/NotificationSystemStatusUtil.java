// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.support.v4.app.NotificationManagerCompat;

import org.chromium.base.metrics.RecordHistogram;

/**
 * Utility for determining whether the user has disabled all of Chrome's notifications using the
 * system's per-application settings.
 *
 * Enabling developers to show notifications with their own content creates a significant product
 * risk: one spammy notification too many and the user might disable notifications for all of
 * Chrome, which is obviously very bad. While we have a strong focus on providing clear attribution
 * and ways of revoking notifications for a particular website, measuring this is still important.
 */
public class NotificationSystemStatusUtil {
    /** Status codes returned by {@link determineAppNotificationsEnabled}. **/
    private static final int APP_NOTIFICATIONS_STATUS_UNDETERMINABLE = 0;
    private static final int APP_NOTIFICATIONS_STATUS_ENABLED = 2;
    private static final int APP_NOTIFICATIONS_STATUS_DISABLED = 3;

    /** Must be set to the maximum value of the above values, plus one. **/
    private static final int APP_NOTIFICATIONS_STATUS_BOUNDARY = 4;

    /**
     * Determines whether notifications are enabled for the app represented by |context| and updates
     * the histogram "Notifications.AppNotiicationStatus".
     * Notifications may be disabled because either the user, or a management tool, has explicitly
     * disallowed the Chrome App to display notifications.
     *
     * This check requires Android KitKat or later. Earlier versions will log an INDETERMINABLE
     * status.
     *
     * @param context The context to check of whether it can show notifications.
     */
    @TargetApi(Build.VERSION_CODES.KITKAT)
    static void recordAppNotificationStatusHistogram(Context context) {
        int histogramValue;

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
            histogramValue = APP_NOTIFICATIONS_STATUS_UNDETERMINABLE;
        } else {
            NotificationManagerCompat manager = NotificationManagerCompat.from(context);
            histogramValue = manager.areNotificationsEnabled() ? APP_NOTIFICATIONS_STATUS_ENABLED
                                                               : APP_NOTIFICATIONS_STATUS_DISABLED;
        }

        RecordHistogram.recordEnumeratedHistogram("Notifications.AppNotificationStatus",
                histogramValue, APP_NOTIFICATIONS_STATUS_BOUNDARY);
    }
}
