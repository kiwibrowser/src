// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browserservices;

import android.content.Context;
import android.content.res.Resources;
import android.net.Uri;
import android.support.customtabs.trusted.TrustedWebActivityServiceConnectionManager;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.NotificationBuilderBase;
import org.chromium.chrome.browser.notifications.NotificationUmaTracker;

/**
 * Uses a Trusted Web Activity client to display notifications.
 */
public class TrustedWebActivityClient {
    private final TrustedWebActivityServiceConnectionManager mConnection;
    private final Context mContext;

    /**
     * Creates a TrustedWebActivityService.
     */
    public TrustedWebActivityClient(Context context) {
        mConnection = new TrustedWebActivityServiceConnectionManager(context);
        mContext = context.getApplicationContext();
    }

    /**
     * Whether a Trusted Web Activity client is available to display notifications of the given
     * scope.
     * @param scope The scope of the Service Worker that triggered the notification.
     * @return Whether a Trusted Web Activity client was found to show the notification.
     */
    public boolean twaExistsForScope(Uri scope) {
        return mConnection.serviceExistsForScope(scope, new Origin(scope).toString());
    }

    /**
     * Displays a notification through a Trusted Web Activity client.
     * @param scope The scope of the Service Worker that triggered the notification.
     * @param platformTag A notification tag.
     * @param platformId A notification id.
     * @param builder A builder for the notification to display.
     *                The Trusted Web Activity client may override the small icon.
     */
    public void notifyNotification(Uri scope, String platformTag, int platformId,
            NotificationBuilderBase builder) {
        Resources res = mContext.getResources();
        String channelDisplayName = res.getString(R.string.notification_category_group_general);

        mConnection.execute(scope, new Origin(scope).toString(), service -> {
            int smallIconId = service.getSmallIconId();

            if (smallIconId != -1) {
                builder.setSmallIconForRemoteApp(smallIconId,
                        service.getComponentName().getPackageName());
            }

            boolean success =
                    service.notify(platformTag, platformId, builder.build(), channelDisplayName);

            if (success) {
                NotificationUmaTracker.getInstance().onNotificationShown(
                        NotificationUmaTracker.SITES, null);
            }
        });
    }

    /**
     * Cancels a notification through a Trusted Web Activity client.
     * @param scope The scope of the Service Worker that triggered the notification.
     * @param platformTag The tag of the notification to cancel.
     * @param platformId The id of the notification to cancel.
     */
    public void cancelNotification(Uri scope, String platformTag, int platformId) {
        mConnection.execute(scope, new Origin(scope).toString(),
                service -> service.cancel(platformTag, platformId));
    }

    /**
     * Registers the package of a Trusted Web Activity client app to be used to deal with
     * notifications from the given origin. This can be called on any thread, but may hit the disk
     * so should be called on a background thread if possible.
     * @param context A context used to access shared preferences.
     * @param origin The origin to use the client app for.
     * @param clientPackage The package of the client app.
     */
    public static void registerClient(Context context, Origin origin, String clientPackage) {
        TrustedWebActivityServiceConnectionManager
                .registerClient(context, origin.toString(), clientPackage);
    }
}
