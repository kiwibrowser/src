// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.app.Notification;
import android.app.PendingIntent;
import android.graphics.Bitmap;
import android.graphics.drawable.Icon;
import android.os.Bundle;
import android.support.v4.media.session.MediaSessionCompat;
import android.widget.RemoteViews;

/**
 * Abstraction over Notification.Builder and NotificationCompat.Builder interfaces.
 *
 * TODO(awdf) Remove this once we've updated to revision 26 of the support library.
 */
public interface ChromeNotificationBuilder {
    ChromeNotificationBuilder setAutoCancel(boolean autoCancel);

    ChromeNotificationBuilder setContentIntent(PendingIntent contentIntent);

    ChromeNotificationBuilder setContentTitle(CharSequence title);

    ChromeNotificationBuilder setContentText(CharSequence text);

    ChromeNotificationBuilder setSmallIcon(int icon);

    ChromeNotificationBuilder setSmallIcon(Icon icon);

    ChromeNotificationBuilder setTicker(CharSequence text);

    ChromeNotificationBuilder setLocalOnly(boolean localOnly);

    ChromeNotificationBuilder setGroup(String group);

    ChromeNotificationBuilder setGroupSummary(boolean isGroupSummary);

    ChromeNotificationBuilder addExtras(Bundle extras);

    ChromeNotificationBuilder setOngoing(boolean ongoing);

    ChromeNotificationBuilder setVisibility(int visibility);

    ChromeNotificationBuilder setShowWhen(boolean showWhen);

    ChromeNotificationBuilder addAction(int icon, CharSequence title, PendingIntent intent);

    ChromeNotificationBuilder addAction(Notification.Action action);

    ChromeNotificationBuilder setDeleteIntent(PendingIntent intent);

    /**
     * Sets the priority of single notification on Android versions prior to Oreo.
     * (From Oreo onwards, priority is instead determined by channel importance.)
     */
    ChromeNotificationBuilder setPriorityBeforeO(int pri);

    ChromeNotificationBuilder setProgress(int max, int percentage, boolean indeterminate);

    ChromeNotificationBuilder setSubText(CharSequence text);

    ChromeNotificationBuilder setContentInfo(String info);

    ChromeNotificationBuilder setWhen(long time);

    ChromeNotificationBuilder setLargeIcon(Bitmap icon);

    ChromeNotificationBuilder setVibrate(long[] vibratePattern);

    ChromeNotificationBuilder setDefaults(int defaults);

    ChromeNotificationBuilder setOnlyAlertOnce(boolean onlyAlertOnce);

    ChromeNotificationBuilder setPublicVersion(Notification publicNotification);

    ChromeNotificationBuilder setContent(RemoteViews views);

    ChromeNotificationBuilder setStyle(Notification.BigPictureStyle style);

    ChromeNotificationBuilder setStyle(Notification.BigTextStyle bigTextStyle);

    ChromeNotificationBuilder setMediaStyle(MediaSessionCompat session, int[] actions,
            PendingIntent intent, boolean showCancelButton);

    Notification buildWithBigContentView(RemoteViews bigView);

    Notification buildWithBigTextStyle(String bigText);

    Notification build();
}
