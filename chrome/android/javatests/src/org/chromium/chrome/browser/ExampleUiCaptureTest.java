// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.support.test.espresso.Espresso;
import android.support.test.espresso.action.ViewActions;
import android.support.test.espresso.matcher.ViewMatchers;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.test.ScreenShooter;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.ui.test.util.UiRestriction;

import java.io.IOException;

/**
 * Simple test to demonstrate use of ScreenShooter rule.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Restriction(UiRestriction.RESTRICTION_TYPE_PHONE) // Tab switcher button only exists on phones.
public class ExampleUiCaptureTest {
    @Rule
    public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeTabbedActivity.class);

    @Rule
    public ScreenShooter mScreenShooter = new ScreenShooter();

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityFromLauncher();
    }

    /**
     * Capture the New Tab Page and the tab switcher.
     * @throws IOException
     * @throws InterruptedException
     */
    @Test
    @SmallTest
    @Feature({"UiCatalogue"})
    public void testCaptureTabSwitcher() throws IOException, InterruptedException {
        mScreenShooter.shoot("NTP", ScreenShooter.TagsEnum.UiCatalogueExample);
        Espresso.onView(ViewMatchers.withId(R.id.tab_switcher_button)).perform(ViewActions.click());
        mScreenShooter.shoot("Tab switcher", ScreenShooter.TagsEnum.UiCatalogueExample);
    }
}
