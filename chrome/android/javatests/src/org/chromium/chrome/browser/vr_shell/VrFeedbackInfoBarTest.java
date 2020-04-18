// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_SHORT_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_DEVICE_DAYDREAM;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.util.TransitionUtils;
import org.chromium.chrome.browser.vr_shell.util.VrInfoBarUtils;
import org.chromium.chrome.browser.vr_shell.util.VrShellDelegateUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ChromeTabUtils;

import java.util.concurrent.TimeoutException;

/**
 * Tests for the infobar that prompts the user to enter feedback on their VR browsing experience.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE, "enable-webvr"})
@Restriction(RESTRICTION_TYPE_DEVICE_DAYDREAM)
@RetryOnFailure(message = "These tests have a habit of hitting a race condition, preventing "
                + "VR entry. See crbug.com/762724")
public class VrFeedbackInfoBarTest {
    // We explicitly instantiate a rule here instead of using parameterization since this class
    // only ever runs in ChromeTabbedActivity.
    @Rule
    public ChromeTabbedActivityVrTestRule mTestRule = new ChromeTabbedActivityVrTestRule();

    private VrTestFramework mVrTestFramework;
    private XrTestFramework mXrTestFramework;

    private static final String TEST_PAGE_2D_URL =
            VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page");
    private static final String TEST_PAGE_WEBVR_URL =
            VrTestFramework.getFileUrlForHtmlTestFile("generic_webvr_page");
    private static final String TEST_PAGE_WEBXR_URL =
            XrTestFramework.getFileUrlForHtmlTestFile("generic_webxr_page");

    @Before
    public void setUp() throws Exception {
        mVrTestFramework = new VrTestFramework(mTestRule);
        mXrTestFramework = new XrTestFramework(mTestRule);
        Assert.assertFalse(VrFeedbackStatus.getFeedbackOptOut());
    }

    private void assertState(boolean isInVr, boolean isInfobarVisible) {
        Assert.assertEquals("Browser is in VR", isInVr, VrShellDelegate.isInVr());
        VrInfoBarUtils.expectInfoBarPresent(mVrTestFramework, isInfobarVisible);
    }

    private void enterThenExitVr() {
        TransitionUtils.forceEnterVr();
        TransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        assertState(true /* isInVr */, false /* isInfobarVisible  */);
        TransitionUtils.forceExitVr();
    }

    /**
     * Tests that we respect the feedback frequency when showing the feedback prompt.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    public void testFeedbackFrequency() throws InterruptedException, TimeoutException {
        mVrTestFramework.loadUrlAndAwaitInitialization(TEST_PAGE_2D_URL, PAGE_LOAD_TIMEOUT_S);
        // Set frequency of infobar to every 2nd time.
        VrShellDelegateUtils.getDelegateInstance().setFeedbackFrequencyForTesting(2);

        // Verify that the Feedback infobar is visible when exiting VR.
        enterThenExitVr();
        assertState(false /* isInVr */, true /* isInfobarVisible  */);
        VrInfoBarUtils.clickInfobarCloseButton(mVrTestFramework);

        // Feedback infobar shouldn't show up this time.
        enterThenExitVr();
        assertState(false /* isInVr */, false /* isInfobarVisible  */);

        // Feedback infobar should show up again.
        enterThenExitVr();
        assertState(false /* isInVr */, true /* isInfobarVisible  */);
        VrInfoBarUtils.clickInfobarCloseButton(mVrTestFramework);
    }

    /**
     * Tests that we don't show the feedback prompt when the user has opted-out.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    public void testFeedbackOptOut() throws InterruptedException, TimeoutException {
        mVrTestFramework.loadUrlAndAwaitInitialization(TEST_PAGE_2D_URL, PAGE_LOAD_TIMEOUT_S);

        // Show infobar every time.
        VrShellDelegateUtils.getDelegateInstance().setFeedbackFrequencyForTesting(1);

        // The infobar should show the first time.
        enterThenExitVr();
        assertState(false /* isInVr */, true /* isInfobarVisible  */);

        // Opt-out of seeing the infobar.
        VrInfoBarUtils.clickInfoBarButton(VrInfoBarUtils.Button.SECONDARY, mVrTestFramework);
        Assert.assertTrue(VrFeedbackStatus.getFeedbackOptOut());

        // The infobar should not show because the user opted out.
        enterThenExitVr();
        assertState(false /* isInVr */, false /* isInfobarVisible  */);
    }

    /**
     * Tests that we only show the feedback prompt when the user has actually used the VR browser.
     */
    @Test
    @MediumTest
    public void testFeedbackOnlyOnVrBrowsing() throws InterruptedException, TimeoutException {
        feedbackOnlyOnVrBrowsingImpl(TEST_PAGE_WEBVR_URL, mVrTestFramework);
    }

    /**
     * Tests that we only show the feedback prompt when the user has actually used the VR browser.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testFeedbackOnlyOnVrBrowsing_WebXr() throws InterruptedException, TimeoutException {
        feedbackOnlyOnVrBrowsingImpl(TEST_PAGE_WEBXR_URL, mXrTestFramework);
    }

    private void feedbackOnlyOnVrBrowsingImpl(String url, TestFramework framework)
            throws InterruptedException {
        // Enter VR presentation mode.
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);
        assertState(true /* isInVr */, false /* isInfobarVisible  */);
        Assert.assertTrue(TestVrShellDelegate.getVrShellForTesting().getWebVrModeEnabled());

        // Exiting VR should not prompt for feedback since the no VR browsing was performed.
        TransitionUtils.forceExitVr();
        assertState(false /* isInVr */, false /* isInfobarVisible  */);
    }

    /**
     * Tests that we show the prompt if the VR browser is used after exiting presentation mode.
     */
    @Test
    @MediumTest
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    public void testExitPresentationInVr() throws InterruptedException, TimeoutException {
        // Enter VR presentation mode.
        exitPresentationInVrImpl(TEST_PAGE_WEBVR_URL, mVrTestFramework);
    }

    /**
     * Tests that we show the prompt if the VR browser is used after exiting a WebXR exclusive
     * session.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    @Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
    public void testExitPresentationInVr_WebXr() throws InterruptedException, TimeoutException {
        exitPresentationInVrImpl(TEST_PAGE_WEBXR_URL, mXrTestFramework);
    }

    private void exitPresentationInVrImpl(String url, final TestFramework framework)
            throws InterruptedException {
        // Enter VR presentation mode.
        framework.loadUrlAndAwaitInitialization(url, PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);
        assertState(true /* isInVr */, false /* isInfobarVisible  */);
        Assert.assertTrue(TestVrShellDelegate.getVrShellForTesting().getWebVrModeEnabled());

        // Exit presentation mode by navigating to a different url.
        ChromeTabUtils.waitForTabPageLoaded(
                mTestRule.getActivity().getActivityTab(), new Runnable() {
                    @Override
                    public void run() {
                        TestFramework.runJavaScriptOrFail(
                                "window.location.href = '" + TEST_PAGE_2D_URL + "';",
                                POLL_TIMEOUT_SHORT_MS, framework.getFirstTabWebContents());
                    }
                }, POLL_TIMEOUT_LONG_MS);

        // Exiting VR should prompt for feedback since 2D browsing was performed after.
        TransitionUtils.forceExitVr();
        assertState(false /* isInVr */, true /* isInfobarVisible  */);
    }
}
