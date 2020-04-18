// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications.channels;

import android.annotation.TargetApi;
import android.app.NotificationChannel;
import android.app.NotificationChannelGroup;
import android.app.NotificationManager;
import android.content.res.Resources;
import android.os.Build;

import org.chromium.chrome.browser.notifications.NotificationManagerProxy;

/**
 * Initializes our notification channels.
 */
@TargetApi(Build.VERSION_CODES.O)
public class ChannelsInitializer {
    private final NotificationManagerProxy mNotificationManager;
    private final Resources mResources;

    public ChannelsInitializer(
            NotificationManagerProxy notificationManagerProxy, Resources resources) {
        mNotificationManager = notificationManagerProxy;
        mResources = resources;
    }

    /**
     * Creates all the channels on the notification manager that we want to appear in our
     * channel settings from first launch onwards.
     */
    void initializeStartupChannels() {
        for (String channelId : ChannelDefinitions.getStartupChannelIds()) {
            ensureInitialized(channelId);
        }
    }

    /**
     * Cleans up any old channels that are no longer required from previous versions of the app.
     * It's safe to call this multiple times since deleting an already-deleted channel is a no-op.
     */
    void deleteLegacyChannels() {
        for (String channelId : ChannelDefinitions.getLegacyChannelIds()) {
            mNotificationManager.deleteNotificationChannel(channelId);
        }
    }

    /**
     * Ensures the given channel has been created on the notification manager so a notification
     * can be safely posted to it. This should only be used for channel ids with an entry in
     * {@link ChannelDefinitions.PredefinedChannels}, or that start with a known prefix.
     *
     * Calling this is a (potentially lengthy) no-op if the channel has already been created.
     *
     * @param channelId The ID of the channel to be initialized.
     */
    public void ensureInitialized(String channelId) {
        ensureInitializedWithEnabledState(channelId, true);
    }

    /**
     * As ensureInitialized, but create the channel in disabled mode. The channel's importance will
     * be set to IMPORTANCE_NONE, instead of using the value from
     * {@link ChannelDefinitions.PredefinedChannels}.
     */
    public void ensureInitializedAndDisabled(String channelId) {
        ensureInitializedWithEnabledState(channelId, false);
    }

    private void ensureInitializedWithEnabledState(String channelId, boolean enabled) {
        if (channelId.startsWith(ChannelDefinitions.CHANNEL_ID_PREFIX_SITES)) {
            // If we have a valid site channel ID at this point, it is safe to assume a channel
            // has already been created, since the only way to obtain a site channel ID is by
            // creating a channel.
            assert SiteChannelsManager.isValidSiteChannelId(channelId);
            return;
        }
        ChannelDefinitions.PredefinedChannel predefinedChannel =
                ChannelDefinitions.getChannelFromId(channelId);
        if (predefinedChannel == null) {
            throw new IllegalStateException("Could not initialize channel: " + channelId);
        }
        // Channel group must be created before the channel.
        NotificationChannelGroup channelGroup =
                ChannelDefinitions.getChannelGroupForChannel(predefinedChannel)
                        .toNotificationChannelGroup(mResources);
        mNotificationManager.createNotificationChannelGroup(channelGroup);
        NotificationChannel channel = predefinedChannel.toNotificationChannel(mResources);
        if (!enabled) {
            channel.setImportance(NotificationManager.IMPORTANCE_NONE);
        }
        mNotificationManager.createNotificationChannel(channel);
    }
}
