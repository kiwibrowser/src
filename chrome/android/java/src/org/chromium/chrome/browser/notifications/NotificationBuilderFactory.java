// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.annotation.SuppressLint;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Build;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.notifications.channels.ChannelsInitializer;

/**
 * Factory which supplies the appropriate type of notification builder based on Android version.
 * Should be used for all notifications we create, to ensure a notification channel is set on O.
 */
public class NotificationBuilderFactory {
    /**
     * Creates either a Notification.Builder or NotificationCompat.Builder under the hood, wrapped
     * in our own common interface.
     *
     * TODO(crbug.com/704152) Remove this once we've updated to revision 26 of the support library.
     * Then we can use NotificationCompat.Builder and set the channel directly everywhere.
     * Although we will still need to ensure the channel is always initialized first.
     *
     * @param preferCompat true if a NotificationCompat.Builder is preferred.
     *                     A Notification.Builder will be used regardless on Android O.
     * @param channelId The ID of the channel the notification should be posted to. This channel
     *                  will be created if it did not already exist. Must be a known channel within
     *                  {@link ChannelsInitializer#ensureInitialized(String)}.
     */
    public static ChromeNotificationBuilder createChromeNotificationBuilder(
            boolean preferCompat, String channelId) {
        Context context = ContextUtils.getApplicationContext();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            return createNotificationBuilderForO(channelId, context);
        }
        return preferCompat ? new NotificationCompatBuilder(context)
                            : new NotificationBuilder(context);
    }

    @SuppressLint("NewApi") // for Context.getSystemService(Class)
    private static ChromeNotificationBuilder createNotificationBuilderForO(
            String channelId, Context context) {
        return new NotificationBuilderForO(context, channelId,
                new ChannelsInitializer(new NotificationManagerProxyImpl(context.getSystemService(
                                                NotificationManager.class)),
                        context.getResources()));
    }
}
