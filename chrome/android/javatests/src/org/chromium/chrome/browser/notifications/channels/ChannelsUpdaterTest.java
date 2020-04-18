// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications.channels;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.hamcrest.Matchers.greaterThan;
import static org.hamcrest.Matchers.hasSize;
import static org.hamcrest.Matchers.is;

import android.annotation.TargetApi;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.res.Resources;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.InMemorySharedPreferences;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.notifications.NotificationManagerProxy;
import org.chromium.chrome.browser.notifications.NotificationManagerProxyImpl;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.content.browser.test.NativeLibraryTestRule;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * Tests that ChannelsUpdater correctly initializes channels on the notification manager.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@TargetApi(Build.VERSION_CODES.O)
public class ChannelsUpdaterTest {
    private NotificationManagerProxy mNotificationManagerProxy;
    private InMemorySharedPreferences mMockSharedPreferences;
    private ChannelsInitializer mChannelsInitializer;
    private Resources mMockResources;

    @Rule
    public TestRule processor = new Features.JUnitProcessor();

    @Rule
    public NativeLibraryTestRule mNativeLibraryTestRule = new NativeLibraryTestRule();

    @Before
    public void setUp() throws Exception {
        // Not initializing the browser process is safe because
        // UrlFormatter.formatUrlForSecurityDisplay() is stand-alone.
        mNativeLibraryTestRule.loadNativeLibraryNoBrowserProcess();

        Context context = InstrumentationRegistry.getTargetContext();
        mNotificationManagerProxy = new NotificationManagerProxyImpl(
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE));

        mMockResources = context.getResources();

        mChannelsInitializer =
                new ChannelsInitializer(mNotificationManagerProxy, context.getResources());
        mMockSharedPreferences = new InMemorySharedPreferences();

        // Delete any channels that may already have been initialized. Cleaning up here rather than
        // in tearDown in case tests running before these ones caused channels to be created.
        for (NotificationChannel channel : getChannelsIgnoringDefault()) {
            if (!channel.getId().equals(NotificationChannel.DEFAULT_CHANNEL_ID)) {
                mNotificationManagerProxy.deleteNotificationChannel(channel.getId());
            }
        }
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    public void testShouldUpdateChannels_returnsFalsePreO() throws Exception {
        ChannelsUpdater updater = new ChannelsUpdater(
                false /* isAtLeastO */, mMockSharedPreferences, mChannelsInitializer, 0);
        assertThat(updater.shouldUpdateChannels(), is(false));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    public void testShouldUpdateChannels_returnsTrueIfOAndNoSavedVersionInPrefs() throws Exception {
        ChannelsUpdater updater = new ChannelsUpdater(
                true /* isAtLeastO */, mMockSharedPreferences, mChannelsInitializer, 0);
        assertThat(updater.shouldUpdateChannels(), is(true));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    public void testShouldUpdateChannels_returnsTrueIfOAndDifferentVersionInPrefs()
            throws Exception {
        mMockSharedPreferences.edit().putInt(ChannelsUpdater.CHANNELS_VERSION_KEY, 4).apply();
        ChannelsUpdater updater = new ChannelsUpdater(
                true /* isAtLeastO */, mMockSharedPreferences, mChannelsInitializer, 5);
        assertThat(updater.shouldUpdateChannels(), is(true));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    public void testShouldUpdateChannels_returnsFalseIfOAndSameVersionInPrefs() throws Exception {
        mMockSharedPreferences.edit().putInt(ChannelsUpdater.CHANNELS_VERSION_KEY, 3).apply();
        ChannelsUpdater updater = new ChannelsUpdater(
                true /* isAtLeastO */, mMockSharedPreferences, mChannelsInitializer, 3);
        assertThat(updater.shouldUpdateChannels(), is(false));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    public void testUpdateChannels_noopPreO() throws Exception {
        ChannelsUpdater updater = new ChannelsUpdater(
                false /* isAtLeastO */, mMockSharedPreferences, mChannelsInitializer, 21);
        updater.updateChannels();

        assertThat(getChannelsIgnoringDefault(), hasSize(0));
        assertThat(mMockSharedPreferences.getInt(ChannelsUpdater.CHANNELS_VERSION_KEY, -1), is(-1));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @EnableFeatures(ChromeFeatureList.SITE_NOTIFICATION_CHANNELS)
    public void testUpdateChannels_createsExpectedChannelsAndUpdatesPref() throws Exception {
        ChannelsUpdater updater = new ChannelsUpdater(
                true /* isAtLeastO */, mMockSharedPreferences, mChannelsInitializer, 21);
        updater.updateChannels();

        assertThat(getChannelsIgnoringDefault(), hasSize((greaterThan(0))));
        assertThat(getChannelIds(getChannelsIgnoringDefault()),
                containsInAnyOrder(ChannelDefinitions.CHANNEL_ID_BROWSER,
                        ChannelDefinitions.CHANNEL_ID_DOWNLOADS,
                        ChannelDefinitions.CHANNEL_ID_INCOGNITO,
                        ChannelDefinitions.CHANNEL_ID_MEDIA));
        assertThat(mMockSharedPreferences.getInt(ChannelsUpdater.CHANNELS_VERSION_KEY, -1), is(21));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @EnableFeatures(ChromeFeatureList.SITE_NOTIFICATION_CHANNELS)
    public void testUpdateChannels_deletesLegacyChannelsAndCreatesExpectedOnes() throws Exception {
        // Set up any legacy channels.
        for (String id : ChannelDefinitions.getLegacyChannelIds()) {
            NotificationChannel channel =
                    new NotificationChannel(id, id, NotificationManager.IMPORTANCE_LOW);
            channel.setGroup(ChannelDefinitions.CHANNEL_GROUP_ID_GENERAL);
            mNotificationManagerProxy.createNotificationChannel(channel);
        }

        ChannelsUpdater updater = new ChannelsUpdater(true /* isAtLeastO */, mMockSharedPreferences,
                new ChannelsInitializer(mNotificationManagerProxy, mMockResources), 12);
        updater.updateChannels();

        assertThat(getChannelIds(getChannelsIgnoringDefault()),
                containsInAnyOrder(ChannelDefinitions.CHANNEL_ID_BROWSER,
                        ChannelDefinitions.CHANNEL_ID_DOWNLOADS,
                        ChannelDefinitions.CHANNEL_ID_INCOGNITO,
                        ChannelDefinitions.CHANNEL_ID_MEDIA));
    }

    private static List<String> getChannelIds(List<NotificationChannel> channels) {
        List<String> ids = new ArrayList<>();
        for (NotificationChannel ch : channels) {
            ids.add(ch.getId());
        }
        return ids;
    }

    /**
     * Gets the current notification channels from the notification manager, except for any with
     * the default ID, which will be removed from the list before returning.
     *
     * (Android *might* add a default 'Misc' channel on our behalf, but we don't want to tie our
     * tests to its presence, as this could change).
     */
    private List<NotificationChannel> getChannelsIgnoringDefault() {
        List<NotificationChannel> channels = mNotificationManagerProxy.getNotificationChannels();
        for (Iterator<NotificationChannel> it = channels.iterator(); it.hasNext();) {
            NotificationChannel channel = it.next();
            if (channel.getId().equals(NotificationChannel.DEFAULT_CHANNEL_ID)) it.remove();
        }
        return channels;
    }
}