// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import static org.chromium.chrome.browser.ChromeFeatureList.SEARCH_ENGINE_PROMO_EXISTING_DEVICE;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.contextual_suggestions.FakeEnabledStateMonitor;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.browser.Features;

/**
 * Tests for ContextualSuggestionsPreference.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@Features.DisableFeatures(SEARCH_ENGINE_PROMO_EXISTING_DEVICE)
public class ContextualSuggestionsPreferenceTest {
    @Rule
    public final ChromeBrowserTestRule mBrowserTestRule = new ChromeBrowserTestRule();

    @Rule
    public TestRule mFeaturesProcessor = new Features.InstrumentationProcessor();

    private ContextualSuggestionsPreference mFragment;
    private FakeEnabledStateMonitor mTestMonitor;
    private ChromeSwitchPreference mSwitch;
    private boolean mInitialSwitchState;

    @Before
    public void setUp() {
        Preferences preferences =
                PreferencesTest.startPreferences(InstrumentationRegistry.getInstrumentation(),
                        ContextualSuggestionsPreference.class.getName());
        mFragment = (ContextualSuggestionsPreference) preferences.getFragmentForTest();
        mTestMonitor = new FakeEnabledStateMonitor();
        ContextualSuggestionsPreference.setEnabledStateMonitorForTesting(mTestMonitor);
        mTestMonitor.setObserver(mFragment);
        mSwitch = (ChromeSwitchPreference) mFragment.findPreference(
                ContextualSuggestionsPreference.PREF_CONTEXTUAL_SUGGESTIONS_SWITCH);
        mInitialSwitchState = mSwitch.isChecked();
    }

    @After
    public void tearDown() {
        ThreadUtils.runOnUiThreadBlocking(() -> setSwitchState(mInitialSwitchState));
        ContextualSuggestionsPreference.setEnabledStateMonitorForTesting(null);
    }

    @Test
    @SmallTest
    @Feature({"ContextualSuggestions"})
    public void testSwitch_Toggle() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mTestMonitor.onSettingsStateChanged(true);

            // Check initial state matches preference.
            setSwitchState(true);
            Assert.assertEquals(mSwitch.isChecked(), true);
            Assert.assertEquals(mSwitch.isChecked(),
                    PrefServiceBridge.getInstance().getBoolean(
                            Pref.CONTEXTUAL_SUGGESTIONS_ENABLED));

            // Toggle and check if preference matches.
            PreferencesTest.clickPreference(mFragment, mSwitch);
            Assert.assertEquals(mSwitch.isChecked(), false);
            Assert.assertEquals(mSwitch.isChecked(),
                    PrefServiceBridge.getInstance().getBoolean(
                            Pref.CONTEXTUAL_SUGGESTIONS_ENABLED));

            // Toggle again check if preference matches.
            PreferencesTest.clickPreference(mFragment, mSwitch);
            Assert.assertEquals(mSwitch.isChecked(), true);
            Assert.assertEquals(mSwitch.isChecked(),
                    PrefServiceBridge.getInstance().getBoolean(
                            Pref.CONTEXTUAL_SUGGESTIONS_ENABLED));
        });
    }

    @Test
    @SmallTest
    @Feature({"ContextualSuggestions"})
    public void testSwitch_SettingsStateChanged() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            // Make sure switch is checked.
            mTestMonitor.onSettingsStateChanged(true);
            setSwitchState(true);
            Assert.assertTrue(mSwitch.isEnabled());
            Assert.assertTrue(mSwitch.isChecked());

            // Simulate settings are disabled.
            mTestMonitor.onSettingsStateChanged(false);
            Assert.assertFalse(mSwitch.isEnabled());
            Assert.assertFalse(mSwitch.isChecked());
        });
    }

    private void setSwitchState(boolean checked) {
        if (mSwitch.isChecked() != checked) PreferencesTest.clickPreference(mFragment, mSwitch);
    }
}
