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
import android.support.annotation.StringDef;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Contains the properties of all our pre-definable notification channels on Android O+. In
 * practice this is all our channels except site channels, which are defined dynamically by the
 * {@link SiteChannelsManager}.
 * <br/><br/>
 * PLEASE NOTE: Notification channels appear in system UI and their properties are persisted forever
 * by Android, so should not be added or removed lightly, and the proper deprecation and versioning
 * steps must be taken when doing so.
 * <br/><br/>
 * See the README.md in this directory for more information before adding or changing any channels.
 */
@TargetApi(Build.VERSION_CODES.O)
public class ChannelDefinitions {
    public static final String CHANNEL_ID_BROWSER = "browser";
    public static final String CHANNEL_ID_DOWNLOADS = "downloads";
    public static final String CHANNEL_ID_INCOGNITO = "incognito";
    public static final String CHANNEL_ID_MEDIA = "media";
    public static final String CHANNEL_ID_SCREEN_CAPTURE = "screen_capture";
    public static final String CHANNEL_ID_CONTENT_SUGGESTIONS = "content_suggestions";
    public static final String CHANNEL_ID_WEBAPP_ACTIONS = "webapp_actions";
    // TODO(crbug.com/700377): Deprecate the 'sites' channel.
    public static final String CHANNEL_ID_SITES = "sites";
    public static final String CHANNEL_ID_PREFIX_SITES = "web:";
    public static final String CHANNEL_GROUP_ID_SITES = "sites";
    static final String CHANNEL_GROUP_ID_GENERAL = "general";
    /**
     * Version number identifying the current set of channels. This must be incremented whenever
     * the set of channels returned by {@link #getStartupChannelIds()} or
     * {@link #getLegacyChannelIds()} changes.
     */
    static final int CHANNELS_VERSION = 2;

    /**
     * To define a new channel, add the channel ID to this StringDef and add a new entry to
     * PredefinedChannels.MAP below with the appropriate channel parameters.
     * To remove an existing channel, remove the ID from this StringDef, remove its entry from
     * Predefined Channels.MAP, and add the ID to the LEGACY_CHANNELS_ID array below.
     * See the README in this directory for more detailed instructions.
     */
    @StringDef({CHANNEL_ID_BROWSER, CHANNEL_ID_DOWNLOADS, CHANNEL_ID_INCOGNITO, CHANNEL_ID_MEDIA,
            CHANNEL_ID_SCREEN_CAPTURE, CHANNEL_ID_CONTENT_SUGGESTIONS, CHANNEL_ID_WEBAPP_ACTIONS,
            CHANNEL_ID_SITES})
    @Retention(RetentionPolicy.SOURCE)
    public @interface ChannelId {}

    @StringDef({CHANNEL_GROUP_ID_GENERAL, CHANNEL_GROUP_ID_SITES})
    @Retention(RetentionPolicy.SOURCE)
    @interface ChannelGroupId {}

    // Map defined in static inner class so it's only initialized lazily.
    @TargetApi(Build.VERSION_CODES.N) // for NotificationManager.IMPORTANCE_* constants
    private static class PredefinedChannels {
        /**
         * Definitions for predefined channels. Any channel listed in STARTUP must have an entry in
         * this map; it may also contain channels that are enabled conditionally.
         */
        static final Map<String, PredefinedChannel> MAP;

        /**
         * The set of predefined channels to be initialized on startup.
         *
         * <p>CHANNELS_VERSION must be incremented every time an entry is added to or removed from
         * this set, or when the definition of one of a channel in this set is changed. If an entry
         * is removed from here then it must be added to the LEGACY_CHANNEL_IDs array.
         */
        static final Set<String> STARTUP;

        static {
            Map<String, PredefinedChannel> map = new HashMap<>();
            Set<String> startup = new HashSet<>();

            map.put(CHANNEL_ID_BROWSER,
                    new PredefinedChannel(CHANNEL_ID_BROWSER,
                            R.string.notification_category_browser,
                            NotificationManager.IMPORTANCE_LOW, CHANNEL_GROUP_ID_GENERAL));
            startup.add(CHANNEL_ID_BROWSER);

            map.put(CHANNEL_ID_DOWNLOADS,
                    new PredefinedChannel(CHANNEL_ID_DOWNLOADS,
                            R.string.notification_category_downloads,
                            NotificationManager.IMPORTANCE_LOW, CHANNEL_GROUP_ID_GENERAL));
            startup.add(CHANNEL_ID_DOWNLOADS);

            map.put(CHANNEL_ID_INCOGNITO,
                    new PredefinedChannel(CHANNEL_ID_INCOGNITO,
                            R.string.notification_category_incognito,
                            NotificationManager.IMPORTANCE_LOW, CHANNEL_GROUP_ID_GENERAL));
            startup.add(CHANNEL_ID_INCOGNITO);

            map.put(CHANNEL_ID_MEDIA,
                    new PredefinedChannel(CHANNEL_ID_MEDIA, R.string.notification_category_media,
                            NotificationManager.IMPORTANCE_LOW, CHANNEL_GROUP_ID_GENERAL));
            startup.add(CHANNEL_ID_MEDIA);

            // CHANNEL_ID_SCREEN_CAPTURE will be created on first use, instead of on startup,
            // so that it doesn't clutter the list for users who don't use this feature.
            map.put(CHANNEL_ID_SCREEN_CAPTURE,
                    new PredefinedChannel(CHANNEL_ID_SCREEN_CAPTURE,
                            R.string.notification_category_screen_capture,
                            NotificationManager.IMPORTANCE_HIGH, CHANNEL_GROUP_ID_GENERAL));

            // Not adding sites channel to startup channels because we now use dynamic site
            // channels by default, so notifications will only be posted to this channel if the
            // SiteNotificationChannels flag is disabled. In that case, it's fine for the channel
            // to only appear when the first notification is posted when the flag's disabled.
            // TODO(crbug.com/758553) Deprecate this channel properly when the flag is removed.
            map.put(CHANNEL_ID_SITES,
                    new PredefinedChannel(CHANNEL_ID_SITES, R.string.notification_category_sites,
                            NotificationManager.IMPORTANCE_DEFAULT, CHANNEL_GROUP_ID_GENERAL));

            // Not adding to startup channels because this channel is experimental and enabled only
            // through the associated feature (see
            // org.chromium.chrome.browser.ntp.ContentSuggestionsNotificationHelper).
            map.put(CHANNEL_ID_CONTENT_SUGGESTIONS,
                    new PredefinedChannel(CHANNEL_ID_CONTENT_SUGGESTIONS,
                            R.string.notification_category_content_suggestions,
                            NotificationManager.IMPORTANCE_LOW, CHANNEL_GROUP_ID_GENERAL));

            // Not adding to startup channels because we want CHANNEL_ID_WEBAPP_ACTIONS to be
            // created on the first use, as not all users use installed web apps.
            map.put(CHANNEL_ID_WEBAPP_ACTIONS,
                    new PredefinedChannel(CHANNEL_ID_WEBAPP_ACTIONS,
                            R.string.notification_category_fullscreen_controls,
                            NotificationManager.IMPORTANCE_MIN, CHANNEL_GROUP_ID_GENERAL));

            MAP = Collections.unmodifiableMap(map);
            STARTUP = Collections.unmodifiableSet(startup);
        }
    }

    /**
     * When channels become deprecated they should be removed from PredefinedChannels and their ids
     * added to this array so they can be deleted on upgrade.
     * We also want to keep track of old channel ids so they aren't accidentally reused.
     */
    private static final String[] LEGACY_CHANNEL_IDS = {};

    // Map defined in static inner class so it's only initialized lazily.
    private static class PredefinedChannelGroups {
        static final Map<String, PredefinedChannelGroup> MAP;
        static {
            Map<String, PredefinedChannelGroup> map = new HashMap<>();
            map.put(CHANNEL_GROUP_ID_GENERAL,
                    new PredefinedChannelGroup(CHANNEL_GROUP_ID_GENERAL,
                            R.string.notification_category_group_general));
            map.put(CHANNEL_GROUP_ID_SITES,
                    new PredefinedChannelGroup(
                            CHANNEL_GROUP_ID_SITES, R.string.notification_category_sites));
            MAP = Collections.unmodifiableMap(map);
        }
    }

    /**
     * @return A set of channel ids of channels that should be initialized on startup.
     */
    static Set<String> getStartupChannelIds() {
        // CHANNELS_VERSION must be incremented if the set of channels returned here changes.
        return PredefinedChannels.STARTUP;
    }

    /**
     * @return An array of old ChannelIds that may have been returned by
     * {@link #getStartupChannelIds} in the past, but are no longer in use.
     */
    static List<String> getLegacyChannelIds() {
        List<String> legacyChannels = new ArrayList<>(Arrays.asList(LEGACY_CHANNEL_IDS));
        // When the SiteNotificationChannels feature is enabled, we use dynamically-created channels
        // for different sites, so we no longer need the generic predefined 'Sites' channel.
        // Err on the side of deleting it if we can't tell if the flag is enabled, because it will
        // always be recreated if it is actually required.
        // TODO(crbug.com/758553) Put CHANNEL_ID_SITES in LEGACY_CHANNEL_IDS once flag is gone.
        if (!ChromeFeatureList.isInitialized()
                || ChromeFeatureList.isEnabled(ChromeFeatureList.SITE_NOTIFICATION_CHANNELS)) {
            legacyChannels.add(ChannelDefinitions.CHANNEL_ID_SITES);
        }
        return legacyChannels;
    }

    static PredefinedChannelGroup getChannelGroupForChannel(PredefinedChannel channel) {
        return getChannelGroup(channel.mGroupId);
    }

    static PredefinedChannelGroup getChannelGroup(@ChannelGroupId String groupId) {
        return PredefinedChannelGroups.MAP.get(groupId);
    }

    static PredefinedChannel getChannelFromId(@ChannelId String channelId) {
        return PredefinedChannels.MAP.get(channelId);
    }

    /**
     * Helper class for storing predefined channel properties while allowing the channel name to be
     * lazily evaluated only when it is converted to an actual NotificationChannel.
     */
    static class PredefinedChannel {
        @ChannelId
        private final String mId;
        private final int mNameResId;
        private final int mImportance;
        @ChannelGroupId
        private final String mGroupId;

        PredefinedChannel(@ChannelId String id, int nameResId, int importance,
                @ChannelGroupId String groupId) {
            this.mId = id;
            this.mNameResId = nameResId;
            this.mImportance = importance;
            this.mGroupId = groupId;
        }

        NotificationChannel toNotificationChannel(Resources resources) {
            String name = resources.getString(mNameResId);
            NotificationChannel channel = new NotificationChannel(mId, name, mImportance);
            channel.setGroup(mGroupId);
            return channel;
        }
    }

    /**
     * Helper class for storing predefined channel group properties while allowing the group name
     * to be lazily evaluated only when it is converted to an actual NotificationChannelGroup.
     */
    public static class PredefinedChannelGroup {
        @ChannelGroupId
        public final String mId;
        public final int mNameResId;

        PredefinedChannelGroup(@ChannelGroupId String id, int nameResId) {
            this.mId = id;
            this.mNameResId = nameResId;
        }

        NotificationChannelGroup toNotificationChannelGroup(Resources resources) {
            String name = resources.getString(mNameResId);
            return new NotificationChannelGroup(mId, name);
        }
    }
}
