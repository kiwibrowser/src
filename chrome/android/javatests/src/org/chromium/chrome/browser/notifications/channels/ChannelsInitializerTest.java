// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.notifications.channels;

import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.hamcrest.Matchers.empty;
import static org.hamcrest.Matchers.hasSize;
import static org.hamcrest.Matchers.is;
import static org.hamcrest.Matchers.not;
import static org.junit.Assert.assertThat;

import android.annotation.TargetApi;
import android.app.NotificationChannel;
import android.app.NotificationChannelGroup;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.notifications.NotificationManagerProxy;
import org.chromium.chrome.browser.notifications.NotificationManagerProxyImpl;
import org.chromium.chrome.browser.notifications.NotificationSettingsBridge;
import org.chromium.chrome.test.util.browser.Features;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;
import org.chromium.content.browser.test.NativeLibraryTestRule;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * Instrumentation tests for ChannelsInitializer.
 *
 * These are Android instrumentation tests so that resource strings can be accessed, and so that
 * we can test against the real NotificationManager implementation.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class ChannelsInitializerTest {
    private ChannelsInitializer mChannelsInitializer;
    private NotificationManagerProxy mNotificationManagerProxy;
    private Context mContext;

    @Rule
    public NativeLibraryTestRule mNativeLibraryTestRule = new NativeLibraryTestRule();

    @Rule
    public TestRule mProcessor = new Features.InstrumentationProcessor();

    @Before
    @TargetApi(Build.VERSION_CODES.O)
    public void setUp() throws Exception {
        // Not initializing the browser process is safe because
        // UrlFormatter.formatUrlForSecurityDisplay() is stand-alone.
        mNativeLibraryTestRule.loadNativeLibraryNoBrowserProcess();

        mContext = InstrumentationRegistry.getTargetContext();
        mNotificationManagerProxy = new NotificationManagerProxyImpl(
                (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE));
        mChannelsInitializer =
                new ChannelsInitializer(mNotificationManagerProxy, mContext.getResources());

        // Delete any channels and channel groups that may already have been initialized. Cleaning
        // up here rather than in tearDown in case tests running before these ones caused channels
        // to be created.
        for (NotificationChannel channel : mNotificationManagerProxy.getNotificationChannels()) {
            if (!channel.getId().equals(NotificationChannel.DEFAULT_CHANNEL_ID)) {
                mNotificationManagerProxy.deleteNotificationChannel(channel.getId());
            }
        }
        for (NotificationChannelGroup group :
                mNotificationManagerProxy.getNotificationChannelGroups()) {
            mNotificationManagerProxy.deleteNotificationChannelGroup(group.getId());
        }
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    @EnableFeatures(ChromeFeatureList.SITE_NOTIFICATION_CHANNELS)
    public void testDeleteLegacyChannels_noopOnCurrentDefinitions() throws Exception {
        assertThat(getChannelsIgnoringDefault(), is(empty()));

        mChannelsInitializer.deleteLegacyChannels();
        assertThat(getChannelsIgnoringDefault(), is(empty()));

        mChannelsInitializer.initializeStartupChannels();
        assertThat(getChannelsIgnoringDefault(), is(not(empty())));

        int nChannels = getChannelsIgnoringDefault().size();
        mChannelsInitializer.deleteLegacyChannels();
        assertThat(getChannelsIgnoringDefault(), hasSize(nChannels));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testInitializeStartupChannels() throws Exception {
        mChannelsInitializer.initializeStartupChannels();
        List<String> notificationChannelIds = new ArrayList<>();
        for (NotificationChannel channel : getChannelsIgnoringDefault()) {
            notificationChannelIds.add(channel.getId());
        }
        assertThat(notificationChannelIds,
                containsInAnyOrder(ChannelDefinitions.CHANNEL_ID_BROWSER,
                        ChannelDefinitions.CHANNEL_ID_DOWNLOADS,
                        ChannelDefinitions.CHANNEL_ID_INCOGNITO,
                        ChannelDefinitions.CHANNEL_ID_MEDIA));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testInitializeStartupChannels_groupCreated() throws Exception {
        mChannelsInitializer.initializeStartupChannels();
        assertThat(mNotificationManagerProxy.getNotificationChannelGroups(), hasSize(1));
        assertThat(mNotificationManagerProxy.getNotificationChannelGroups().get(0).getId(),
                is(ChannelDefinitions.CHANNEL_GROUP_ID_GENERAL));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testEnsureInitialized_browserChannel() throws Exception {
        mChannelsInitializer.ensureInitialized(ChannelDefinitions.CHANNEL_ID_BROWSER);

        assertThat(getChannelsIgnoringDefault(), hasSize(1));
        NotificationChannel channel = getChannelsIgnoringDefault().get(0);
        assertThat(channel.getId(), is(ChannelDefinitions.CHANNEL_ID_BROWSER));
        assertThat(channel.getName().toString(),
                is(mContext.getString(org.chromium.chrome.R.string.notification_category_browser)));
        assertThat(channel.getImportance(), is(NotificationManager.IMPORTANCE_LOW));
        assertThat(channel.getGroup(), is(ChannelDefinitions.CHANNEL_GROUP_ID_GENERAL));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testEnsureInitialized_downloadsChannel() throws Exception {
        mChannelsInitializer.ensureInitialized(ChannelDefinitions.CHANNEL_ID_DOWNLOADS);

        assertThat(getChannelsIgnoringDefault(), hasSize(1));
        NotificationChannel channel = getChannelsIgnoringDefault().get(0);
        assertThat(channel.getId(), is(ChannelDefinitions.CHANNEL_ID_DOWNLOADS));
        assertThat(channel.getName().toString(),
                is(mContext.getString(
                        org.chromium.chrome.R.string.notification_category_downloads)));
        assertThat(channel.getImportance(), is(NotificationManager.IMPORTANCE_LOW));
        assertThat(channel.getGroup(), is(ChannelDefinitions.CHANNEL_GROUP_ID_GENERAL));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testEnsureInitialized_incognitoChannel() throws Exception {
        mChannelsInitializer.ensureInitialized(ChannelDefinitions.CHANNEL_ID_INCOGNITO);

        assertThat(getChannelsIgnoringDefault(), hasSize(1));
        NotificationChannel channel = getChannelsIgnoringDefault().get(0);
        assertThat(channel.getId(), is(ChannelDefinitions.CHANNEL_ID_INCOGNITO));
        assertThat(channel.getName().toString(),
                is(mContext.getString(
                        org.chromium.chrome.R.string.notification_category_incognito)));
        assertThat(channel.getImportance(), is(NotificationManager.IMPORTANCE_LOW));
        assertThat(channel.getGroup(), is(ChannelDefinitions.CHANNEL_GROUP_ID_GENERAL));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testEnsureInitialized_mediaChannel() throws Exception {
        mChannelsInitializer.ensureInitialized(ChannelDefinitions.CHANNEL_ID_MEDIA);

        assertThat(getChannelsIgnoringDefault(), hasSize(1));
        NotificationChannel channel = getChannelsIgnoringDefault().get(0);
        assertThat(channel.getId(), is(ChannelDefinitions.CHANNEL_ID_MEDIA));
        assertThat(channel.getName().toString(),
                is(mContext.getString(org.chromium.chrome.R.string.notification_category_media)));
        assertThat(channel.getImportance(), is(NotificationManager.IMPORTANCE_LOW));
        assertThat(channel.getGroup(), is(ChannelDefinitions.CHANNEL_GROUP_ID_GENERAL));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testEnsureInitialized_sitesChannel() throws Exception {
        mChannelsInitializer.ensureInitialized(ChannelDefinitions.CHANNEL_ID_SITES);

        assertThat(getChannelsIgnoringDefault(), hasSize(1));

        NotificationChannel channel = getChannelsIgnoringDefault().get(0);
        assertThat(channel.getId(), is(ChannelDefinitions.CHANNEL_ID_SITES));
        assertThat(channel.getName().toString(),
                is(mContext.getString(org.chromium.chrome.R.string.notification_category_sites)));
        assertThat(channel.getImportance(), is(NotificationManager.IMPORTANCE_DEFAULT));
        assertThat(channel.getGroup(), is(ChannelDefinitions.CHANNEL_GROUP_ID_GENERAL));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testEnsureInitialized_contentSuggestionsDisabled() throws Exception {
        // This test does not cover ensureInitialized() with CHANNEL_ID_CONTENT_SUGGESTIONS, because
        // channels ignore construction parameters when re-created. If one test created the channel
        // enabled, and the other disabled, the second test would fail.
        mChannelsInitializer.ensureInitializedAndDisabled(
                ChannelDefinitions.CHANNEL_ID_CONTENT_SUGGESTIONS);

        assertThat(getChannelsIgnoringDefault(), hasSize(1));

        NotificationChannel channel = getChannelsIgnoringDefault().get(0);
        assertThat(channel.getId(), is(ChannelDefinitions.CHANNEL_ID_CONTENT_SUGGESTIONS));
        assertThat(channel.getName().toString(),
                is(mContext.getString(
                        org.chromium.chrome.R.string.notification_category_content_suggestions)));
        assertThat(channel.getImportance(), is(NotificationManager.IMPORTANCE_NONE));
        assertThat(channel.getGroup(), is(ChannelDefinitions.CHANNEL_GROUP_ID_GENERAL));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testEnsureInitialized_webappActions() throws Exception {
        mChannelsInitializer.ensureInitialized(ChannelDefinitions.CHANNEL_ID_WEBAPP_ACTIONS);

        assertThat(getChannelsIgnoringDefault(), hasSize(1));

        NotificationChannel channel = getChannelsIgnoringDefault().get(0);
        assertThat(channel.getId(), is(ChannelDefinitions.CHANNEL_ID_WEBAPP_ACTIONS));
        assertThat(channel.getName().toString(),
                is(mContext.getString(
                        org.chromium.chrome.R.string.notification_category_fullscreen_controls)));
        assertThat(channel.getImportance(), is(NotificationManager.IMPORTANCE_MIN));
        assertThat(channel.getGroup(), is(ChannelDefinitions.CHANNEL_GROUP_ID_GENERAL));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testEnsureInitialized_singleOriginSiteChannel() throws Exception {
        String origin = "https://example.com";
        long creationTime = 621046800000L;
        NotificationSettingsBridge.SiteChannel siteChannel =
                SiteChannelsManager.getInstance().createSiteChannel(origin, creationTime, true);
        mChannelsInitializer.ensureInitialized(siteChannel.getId());

        assertThat(getChannelsIgnoringDefault(), hasSize(1));

        NotificationChannel channel = getChannelsIgnoringDefault().get(0);
        assertThat(channel.getName().toString(), is("example.com"));
        assertThat(channel.getImportance(), is(NotificationManager.IMPORTANCE_DEFAULT));
        assertThat(channel.getGroup(), is(ChannelDefinitions.CHANNEL_GROUP_ID_SITES));
    }

    @Test
    @SmallTest
    @MinAndroidSdkLevel(Build.VERSION_CODES.O)
    @TargetApi(Build.VERSION_CODES.O)
    @Feature({"Browser", "Notifications"})
    public void testEnsureInitialized_multipleCalls() throws Exception {
        mChannelsInitializer.ensureInitialized(ChannelDefinitions.CHANNEL_ID_SITES);
        mChannelsInitializer.ensureInitialized(ChannelDefinitions.CHANNEL_ID_BROWSER);
        assertThat(getChannelsIgnoringDefault(), hasSize(2));
    }

    /**
     * Gets the current notification channels from the notification manager, except for any with
     * the default ID, which will be removed from the list before returning.
     *
     * (Android *might* add a default 'Misc' channel on our behalf, but we don't want to tie our
     * tests to its presence, as this could change).
     */
    @TargetApi(Build.VERSION_CODES.O)
    private List<NotificationChannel> getChannelsIgnoringDefault() {
        List<NotificationChannel> channels = mNotificationManagerProxy.getNotificationChannels();
        for (Iterator<NotificationChannel> it = channels.iterator(); it.hasNext();) {
            NotificationChannel channel = it.next();
            if (channel.getId().equals(NotificationChannel.DEFAULT_CHANNEL_ID)) it.remove();
        }
        return channels;
    }
}
