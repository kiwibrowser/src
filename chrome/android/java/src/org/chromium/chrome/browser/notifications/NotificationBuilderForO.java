// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.text.TextUtils;

import org.chromium.chrome.browser.notifications.channels.ChannelsInitializer;
import org.chromium.chrome.browser.webapps.WebApkServiceClient;

/**
 * Builder to be used on Android O until we target O and these APIs are in the support library.
 */
@TargetApi(Build.VERSION_CODES.O)
public class NotificationBuilderForO extends NotificationBuilder {
    private static final String TAG = "NotifBuilderForO";

    public NotificationBuilderForO(
            Context context, String channelId, ChannelsInitializer channelsInitializer) {
        super(context);
        assert Build.VERSION.SDK_INT >= Build.VERSION_CODES.O;
        if (channelId == null) {
            // The channelId may be null if the notification will be posted by another app that
            // does not target O or sets its own channels. E.g. Web apk notifications.
            return;
        }

        // If the channel ID matches {@link WebApkServiceClient#CHANNEL_ID_WEBAPKS}, we don't create
        // the channel in Chrome. Instead, the channel will be created in WebAPKs.
        if (!TextUtils.equals(channelId, WebApkServiceClient.CHANNEL_ID_WEBAPKS)) {
            channelsInitializer.ensureInitialized(channelId);
        }
        mBuilder.setChannelId(channelId);
    }
}
