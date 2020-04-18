// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.app.Fragment;
import android.os.Build;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisableIf;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ContentSettingsType;
import org.chromium.chrome.browser.ntp.snippets.SnippetsBridge;
import org.chromium.chrome.browser.preferences.website.ContentSettingsResources;
import org.chromium.chrome.browser.preferences.website.SingleCategoryPreferences;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

/**
 * Tests for the NotificationsPreferences.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class NotificationsPreferencesTest {
    // TODO(peconn): Add UI Catalogue entries for NotificationsPreferences.
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();
    private Preferences mActivity;

    @Before
    public void setUp() {
        mActivity = PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                NotificationsPreferences.class.getName());
    }

    @Test
    @SmallTest
    @Feature({"Preferences"})
    @DisableIf.Build(sdk_is_greater_than = Build.VERSION_CODES.N)
    @CommandLineFlags.Add("enable-features=ContentSuggestionsNotifications")
    public void testContentSuggestionsToggle() {
        final PreferenceFragment fragment = (PreferenceFragment) mActivity.getFragmentForTest();
        final ChromeSwitchPreference toggle = (ChromeSwitchPreference) fragment.findPreference(
                NotificationsPreferences.PREF_SUGGESTIONS);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                // Make sure the toggle reflects the state correctly.
                boolean initiallyChecked = toggle.isChecked();
                Assert.assertEquals(toggle.isChecked(),
                        SnippetsBridge.areContentSuggestionsNotificationsEnabled());

                // Make sure we can change the state.
                PreferencesTest.clickPreference(fragment, toggle);
                Assert.assertEquals(toggle.isChecked(), !initiallyChecked);
                Assert.assertEquals(toggle.isChecked(),
                        SnippetsBridge.areContentSuggestionsNotificationsEnabled());

                // Make sure we can change it back.
                PreferencesTest.clickPreference(fragment, toggle);
                Assert.assertEquals(toggle.isChecked(), initiallyChecked);
                Assert.assertEquals(toggle.isChecked(),
                        SnippetsBridge.areContentSuggestionsNotificationsEnabled());

                // Click it one last time so we're in a toggled state for the UI Capture.
                PreferencesTest.clickPreference(fragment, toggle);
            }
        });
    }

    @Test
    @SmallTest
    @Feature({"Preferences"})
    @DisableIf.Build(sdk_is_greater_than = Build.VERSION_CODES.N)
    @CommandLineFlags.Add("disable-features=NTPArticleSuggestions")
    public void testToggleDisabledWhenSuggestionsDisabled() {
        PreferenceFragment fragment = (PreferenceFragment) mActivity.getFragmentForTest();
        ChromeSwitchPreference toggle = (ChromeSwitchPreference) fragment.findPreference(
                NotificationsPreferences.PREF_SUGGESTIONS);

        Assert.assertFalse(toggle.isEnabled());
        Assert.assertFalse(toggle.isChecked());
    }


    @Test
    @SmallTest
    @Feature({"Preferences"})
    @DisableIf.Build(sdk_is_greater_than = Build.VERSION_CODES.N)
    public void testLinkToWebsiteNotifications() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PreferenceFragment fragment = (PreferenceFragment) mActivity.getFragmentForTest();
                Preference fromWebsites =
                        fragment.findPreference(NotificationsPreferences.PREF_FROM_WEBSITES);

                PreferencesTest.clickPreference(fragment, fromWebsites);
            }
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return getTopFragment() instanceof SingleCategoryPreferences;
            }
        });

        SingleCategoryPreferences fragment = (SingleCategoryPreferences) getTopFragment();
        Assert.assertTrue(fragment.getCategoryForTest().showNotificationsSites());
    }

    /** Gets the fragment of the top Activity. Assumes the top Activity is a Preferences. */
    private static Fragment getTopFragment() {
        Preferences preferences = (Preferences) ApplicationStatus.getLastTrackedFocusedActivity();
        return preferences.getFragmentForTest();
    }

    @Test
    @SmallTest
    @Feature({"Preferences"})
    @DisableIf.Build(sdk_is_greater_than = Build.VERSION_CODES.N)
    public void testWebsiteNotificationsSummary() {
        final PreferenceFragment fragment = (PreferenceFragment) mActivity.getFragmentForTest();
        final Preference fromWebsites =
                fragment.findPreference(NotificationsPreferences.PREF_FROM_WEBSITES);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                PrefServiceBridge.getInstance().setNotificationsEnabled(false);
                fragment.onResume();
                Assert.assertEquals(fromWebsites.getSummary(), getNotificationsSummary(false));

                PrefServiceBridge.getInstance().setNotificationsEnabled(true);
                fragment.onResume();
                Assert.assertEquals(fromWebsites.getSummary(), getNotificationsSummary(true));
            }
        });
    }

    /** Gets the summary text that should be used for site specific notifications. */
    private String getNotificationsSummary(boolean enabled) {
        return mActivity.getResources().getString(ContentSettingsResources.getCategorySummary(
                ContentSettingsType.CONTENT_SETTINGS_TYPE_NOTIFICATIONS, enabled));
    }
}
