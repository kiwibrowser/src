// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.app.NotificationManager;
import android.content.Context;
import android.support.v4.app.NotificationCompat;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.ChromeNotificationBuilder;
import org.chromium.chrome.browser.notifications.NotificationBuilderFactory;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;

import java.util.HashSet;
import java.util.Set;

/**
 * Manages the notification indicating that a WebApk is backed by chrome code and may share data.
 * It's shown while an Unbound WebApk is displayed in the foreground until the user dismisses it.
 */
public class WebApkDisclosureNotificationManager {
    // We always use the same integer id when showing and closing notifications. The notification
    // tag is always set, which is a safe and sufficient way of identifying a notification, so the
    // integer id is not needed anymore except it must not vary in an uncontrolled way.
    private static final int PLATFORM_ID = 100;

    // Prefix used for generating a unique notification tag.
    private static final String DISMISSAL_NOTIFICATION_TAG_PREFIX =
            "dismissal_notification_tag_prefix.";

    /** Records whether we're currently showing a disclosure notification. */
    private static Set<String> sVisibleNotifications = new HashSet<>();

    /**
     * For Trusted Web Activity show a notification that it's running in Chrome.
     */
    static void maybeShowDisclosure(WebappActivity activity, WebappDataStorage storage) {
        String packageName = activity.getNativeClientPackageName();
        boolean isTWA = (activity.getActivityType() == WebappActivity.ACTIVITY_TYPE_TWA);
        boolean isNotificationAllowed = !storage.hasDismissedDisclosure()
                && !sVisibleNotifications.contains(packageName)
                && !WebappActionsNotificationManager.isEnabled();
        if (!isTWA || !isNotificationAllowed) return;

        int activityState = ApplicationStatus.getStateForActivity(activity);
        if (activityState == ActivityState.STARTED || activityState == ActivityState.RESUMED
                || activityState == ActivityState.PAUSED) {
            sVisibleNotifications.add(packageName);
            WebApkDisclosureNotificationManager.showDisclosure(activity.getWebappInfo());
        }
    }

    /**
     * Shows the privacy disclosure informing the user that Chrome is being used.
     * @param webappInfo Web App this is currently displayed fullscreen.
     */
    private static void showDisclosure(WebappInfo webappInfo) {
        Context context = ContextUtils.getApplicationContext();

        ChromeNotificationBuilder builder =
                NotificationBuilderFactory.createChromeNotificationBuilder(
                        false /* preferCompat */, ChannelDefinitions.CHANNEL_ID_BROWSER);
        builder.setContentTitle(webappInfo.name())
                .setPriorityBeforeO(NotificationCompat.PRIORITY_MIN)
                .setSmallIcon(R.drawable.ic_chrome)
                .setLargeIcon(webappInfo.icon())
                .setDeleteIntent(WebApkDisclosureNotificationService.getDeleteIntent(
                        context, webappInfo.id()))
                .setContentText(context.getResources().getString(
                        R.string.webapk_running_in_chrome_disclosure));

        NotificationManager nm =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        nm.notify(DISMISSAL_NOTIFICATION_TAG_PREFIX + webappInfo.apkPackageName(), PLATFORM_ID,
                builder.build());
        NotificationUmaTracker.getInstance().onNotificationShown(
                NotificationUmaTracker.WEBAPK, ChannelDefinitions.CHANNEL_ID_BROWSER);
    }

    /**
     * Dismisses the notification.
     * @param activity Web App this is currently displayed fullscreen.
     */
    public static void dismissNotification(WebappActivity activity) {
        String packageName = activity.getNativeClientPackageName();
        if (!sVisibleNotifications.contains(packageName)) return;

        Context context = ContextUtils.getApplicationContext();
        NotificationManager nm =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        nm.cancel(DISMISSAL_NOTIFICATION_TAG_PREFIX + packageName, PLATFORM_ID);
        sVisibleNotifications.remove(packageName);
    }
}
