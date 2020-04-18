// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.NATIVE_URLS_OF_INTEREST;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.util.VrTransitionUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.util.concurrent.TimeoutException;

/**
 * End-to-end tests for native UI presentation in VR Browser mode.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE, "enable-webvr"})
@Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
public class VrShellNativeUiTest {
    // We explicitly instantiate a rule here instead of using parameterization since this class
    // only ever runs in ChromeTabbedActivity.
    @Rule
    public ChromeTabbedActivityVrTestRule mVrTestRule = new ChromeTabbedActivityVrTestRule();

    private static final String TEST_PAGE_2D_URL =
            VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page");

    @Before
    public void setUp() throws Exception {
        VrTransitionUtils.forceEnterVr();
        VrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
    }

    /**
     * Tests that URLs are not shown for native UI.
     */
    @Test
    @MediumTest
    public void testUrlOnNativeUi()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        for (String url : NATIVE_URLS_OF_INTEREST) {
            mVrTestRule.loadUrl(url, PAGE_LOAD_TIMEOUT_S);
            Assert.assertFalse("Should not be showing URL on " + url,
                    TestVrShellDelegate.isDisplayingUrlForTesting());
        }
    }

    /**
     * Tests that URLs are shown for non-native UI.
     */
    @Test
    @MediumTest
    public void testUrlOnNonNativeUi()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        mVrTestRule.loadUrl(TEST_PAGE_2D_URL, PAGE_LOAD_TIMEOUT_S);
        Assert.assertTrue("Should be showing URL", TestVrShellDelegate.isDisplayingUrlForTesting());
    }
}
