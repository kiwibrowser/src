// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.website;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.Features.DisableFeatures;
import org.chromium.chrome.test.util.browser.Features.EnableFeatures;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;

/** Tests for the PermissionInfoTest. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PermissionInfoTest {
    private static final String DSE_ORIGIN = "https://www.google.com";
    private static final String OTHER_ORIGIN = "https://www.other.com";

    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    private void setGeolocation(
            String origin, String embedder, ContentSetting setting, boolean incognito) {
        GeolocationInfo info = new GeolocationInfo(origin, embedder, incognito);
        ThreadUtils.runOnUiThreadBlocking(() -> info.setContentSetting(setting));
    }

    private ContentSetting getGeolocation(String origin, String embedder, boolean incognito)
            throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(() -> {
            GeolocationInfo info = new GeolocationInfo(origin, embedder, incognito);
            return info.getContentSetting();
        });
    }

    private void setNotifications(
            String origin, String embedder, ContentSetting setting, boolean incognito) {
        NotificationInfo info = new NotificationInfo(origin, embedder, incognito);
        ThreadUtils.runOnUiThreadBlocking(() -> info.setContentSetting(setting));
    }

    private ContentSetting getNotifications(String origin, String embedder, boolean incognito)
            throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(new Callable<ContentSetting>() {
            @Override
            public ContentSetting call() {
                NotificationInfo info = new NotificationInfo(origin, embedder, incognito);
                return info.getContentSetting();
            }
        });
    }

    @Test
    @SmallTest
    @Feature({"Preferences"})
    public void testResetDSEGeolocation() throws Throwable {
        // Resetting the DSE geolocation permission should change it to ALLOW.
        boolean incognito = false;
        setGeolocation(DSE_ORIGIN, null, ContentSetting.BLOCK, incognito);
        Assert.assertEquals(ContentSetting.BLOCK, getGeolocation(DSE_ORIGIN, null, incognito));
        setGeolocation(DSE_ORIGIN, null, ContentSetting.DEFAULT, incognito);
        Assert.assertEquals(ContentSetting.ALLOW, getGeolocation(DSE_ORIGIN, null, incognito));

        // Resetting in incognito should not have the same behavior.
        incognito = true;
        setGeolocation(DSE_ORIGIN, null, ContentSetting.BLOCK, incognito);
        Assert.assertEquals(ContentSetting.BLOCK, getGeolocation(DSE_ORIGIN, null, incognito));
        setGeolocation(DSE_ORIGIN, null, ContentSetting.DEFAULT, incognito);
        Assert.assertEquals(ContentSetting.ASK, getGeolocation(DSE_ORIGIN, null, incognito));

        // Resetting a different top level origin should not have the same behavior
        incognito = false;
        setGeolocation(OTHER_ORIGIN, null, ContentSetting.BLOCK, incognito);
        Assert.assertEquals(ContentSetting.BLOCK, getGeolocation(OTHER_ORIGIN, null, incognito));
        setGeolocation(OTHER_ORIGIN, null, ContentSetting.DEFAULT, incognito);
        Assert.assertEquals(ContentSetting.ASK, getGeolocation(OTHER_ORIGIN, null, incognito));
    }

    @Test
    @SmallTest
    @Feature({"Preferences"})
    @DisableFeatures(ChromeFeatureList.PERMISSION_DELEGATION)
    public void testResetDSEGeolocationEmbeddedOrigin() throws Throwable {
        // It's not possible to set a permission for an embedded origin when permission delegation
        // is enabled. This code can be deleted when the feature is enabled by default.
        // Resetting an embedded DSE origin should not have the same behavior.
        boolean incognito = false;
        setGeolocation(DSE_ORIGIN, OTHER_ORIGIN, ContentSetting.BLOCK, incognito);
        Assert.assertEquals(
                ContentSetting.BLOCK, getGeolocation(DSE_ORIGIN, OTHER_ORIGIN, incognito));
        setGeolocation(DSE_ORIGIN, OTHER_ORIGIN, ContentSetting.DEFAULT, incognito);
        Assert.assertEquals(
                ContentSetting.ASK, getGeolocation(DSE_ORIGIN, OTHER_ORIGIN, incognito));
    }

    @Test
    @SmallTest
    @Feature({"Preferences"})
    @EnableFeatures(ChromeFeatureList.GRANT_NOTIFICATIONS_TO_DSE)
    public void testResetDSENotifications() throws Throwable {
        // On Android O+ we need to clear notification channels so they don't interfere with the
        // test.
        ThreadUtils.runOnUiThreadBlocking(
                () -> WebsitePreferenceBridge.nativeResetNotificationsSettingsForTest());

        // Resetting the DSE notifications permission should change it to ALLOW.
        boolean incognito = false;
        setNotifications(DSE_ORIGIN, null, ContentSetting.BLOCK, incognito);
        Assert.assertEquals(ContentSetting.BLOCK, getNotifications(DSE_ORIGIN, null, incognito));
        setNotifications(DSE_ORIGIN, null, ContentSetting.DEFAULT, incognito);
        Assert.assertEquals(ContentSetting.ALLOW, getNotifications(DSE_ORIGIN, null, incognito));

        // Resetting in incognito should not have the same behavior.
        ThreadUtils.runOnUiThreadBlocking(
                () -> WebsitePreferenceBridge.nativeResetNotificationsSettingsForTest());
        incognito = true;
        setNotifications(DSE_ORIGIN, null, ContentSetting.BLOCK, incognito);
        Assert.assertEquals(ContentSetting.BLOCK, getNotifications(DSE_ORIGIN, null, incognito));
        setNotifications(DSE_ORIGIN, null, ContentSetting.DEFAULT, incognito);
        Assert.assertEquals(ContentSetting.ASK, getNotifications(DSE_ORIGIN, null, incognito));

        // // Resetting a different top level origin should not have the same behavior
        ThreadUtils.runOnUiThreadBlocking(
                () -> WebsitePreferenceBridge.nativeResetNotificationsSettingsForTest());
        incognito = false;
        setNotifications(OTHER_ORIGIN, null, ContentSetting.BLOCK, incognito);
        Assert.assertEquals(ContentSetting.BLOCK, getNotifications(OTHER_ORIGIN, null, incognito));
        setNotifications(OTHER_ORIGIN, null, ContentSetting.DEFAULT, incognito);
        Assert.assertEquals(ContentSetting.ASK, getNotifications(OTHER_ORIGIN, null, incognito));
    }
}
