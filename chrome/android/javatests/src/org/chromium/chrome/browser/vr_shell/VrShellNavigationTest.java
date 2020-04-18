// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.NATIVE_URLS_OF_INTEREST;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_SHORT_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.history.HistoryItemView;
import org.chromium.chrome.browser.history.HistoryPage;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.util.TransitionUtils;
import org.chromium.chrome.browser.vr_shell.util.VrInfoBarUtils;
import org.chromium.chrome.browser.vr_shell.util.VrTransitionUtils;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.content.browser.test.util.ClickUtils;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.WebContents;

import java.util.ArrayList;
import java.util.concurrent.TimeoutException;

/**
 * End-to-end tests for testing navigation transitions (e.g. link clicking) in VR Browser mode, aka
 * "VR Shell".
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE, "enable-webvr"})
@Restriction(RESTRICTION_TYPE_VIEWER_DAYDREAM)
public class VrShellNavigationTest {
    // We explicitly instantiate a rule here instead of using parameterization since this class
    // only ever runs in ChromeTabbedActivity.
    @Rule
    public ChromeTabbedActivityVrTestRule mTestRule = new ChromeTabbedActivityVrTestRule();

    private VrTestFramework mVrTestFramework;
    private XrTestFramework mXrTestFramework;

    private static final String TEST_PAGE_2D_URL =
            VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page");
    private static final String TEST_PAGE_2D_2_URL =
            VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page2");
    private static final String TEST_PAGE_WEBVR_URL =
            VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_webvr_page");
    private static final String TEST_PAGE_WEBXR_URL =
            XrTestFramework.getFileUrlForHtmlTestFile("test_navigation_webxr_page");

    private enum Page { PAGE_2D, PAGE_2D_2, PAGE_WEBVR, PAGE_WEBXR }
    private enum PresentationMode { NON_PRESENTING, PRESENTING }
    private enum FullscreenMode { NON_FULLSCREENED, FULLSCREENED }

    @Before
    public void setUp() throws Exception {
        mVrTestFramework = new VrTestFramework(mTestRule);
        mXrTestFramework = new XrTestFramework(mTestRule);
        TransitionUtils.forceEnterVr();
        TransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
    }

    private String getUrl(Page page) {
        switch (page) {
            case PAGE_2D:
                return TEST_PAGE_2D_URL;
            case PAGE_2D_2:
                return TEST_PAGE_2D_2_URL;
            case PAGE_WEBVR:
                return TEST_PAGE_WEBVR_URL;
            case PAGE_WEBXR:
                return TEST_PAGE_WEBXR_URL;
            default:
                throw new UnsupportedOperationException("Don't know page type " + page);
        }
    }

    /**
     * Triggers navigation to either a 2D or WebVR page. Similar to
     * {@link ChromeActivityTestRule#loadUrl loadUrl} but makes sure page initiates the
     * navigation. This is desirable since we are testing navigation transitions end-to-end.
     */
    private void navigateTo(final Page to) throws InterruptedException {
        ChromeTabUtils.waitForTabPageLoaded(
                mTestRule.getActivity().getActivityTab(), new Runnable() {
                    @Override
                    public void run() {
                        VrTestFramework.runJavaScriptOrFail(
                                "window.location.href = '" + getUrl(to) + "';",
                                POLL_TIMEOUT_SHORT_MS, mVrTestFramework.getFirstTabWebContents());
                    }
                }, POLL_TIMEOUT_LONG_MS);
    }

    private void enterFullscreenOrFail(WebContents webContents)
            throws InterruptedException, TimeoutException {
        DOMUtils.clickNode(webContents, "fullscreen", false /* goThroughRootAndroidView */);
        TestFramework.waitOnJavaScriptStep(webContents);
        Assert.assertTrue(DOMUtils.isFullscreen(webContents));
    }

    private void assertState(WebContents wc, Page page, PresentationMode presentationMode,
            FullscreenMode fullscreenMode) throws InterruptedException, TimeoutException {
        Assert.assertTrue("Browser is in VR", VrShellDelegate.isInVr());
        Assert.assertEquals("Browser is on correct web site", getUrl(page), wc.getVisibleUrl());
        Assert.assertEquals("Browser is in VR Presentation Mode",
                presentationMode == PresentationMode.PRESENTING,
                TestVrShellDelegate.getVrShellForTesting().getWebVrModeEnabled());
        Assert.assertEquals("Browser is in fullscreen",
                fullscreenMode == FullscreenMode.FULLSCREENED, DOMUtils.isFullscreen(wc));
        // Feedback infobar should never show up during navigations.
        VrInfoBarUtils.expectInfoBarPresent(mVrTestFramework, false);
    }

    /**
     * Tests navigation from a 2D to a 2D page. Also tests that this navigation is
     * properly added to Chrome's history and is usable.
     */
    @Test
    @MediumTest
    public void test2dTo2d() throws InterruptedException, TimeoutException {
        mVrTestFramework.loadUrlAndAwaitInitialization(TEST_PAGE_2D_URL, PAGE_LOAD_TIMEOUT_S);

        navigateTo(Page.PAGE_2D_2);

        assertState(mVrTestFramework.getFirstTabWebContents(), Page.PAGE_2D_2,
                PresentationMode.NON_PRESENTING, FullscreenMode.NON_FULLSCREENED);

        // Test that the navigations were added to history
        mTestRule.loadUrl(UrlConstants.HISTORY_URL, PAGE_LOAD_TIMEOUT_S);
        HistoryPage historyPage =
                (HistoryPage) mTestRule.getActivity().getActivityTab().getNativePage();
        ArrayList<HistoryItemView> itemViews = historyPage.getHistoryManagerForTesting()
                                                       .getAdapterForTests()
                                                       .getItemViewsForTests();
        Assert.assertEquals("Two navigations showed up in history", 2, itemViews.size());
        // History is in reverse chronological order, so the first navigation should actually be
        // after the second in the list
        Assert.assertEquals("First navigation is correct", getUrl(Page.PAGE_2D),
                itemViews.get(1).getItem().getUrl());
        Assert.assertEquals("Second navigation is correct", getUrl(Page.PAGE_2D_2),
                itemViews.get(0).getItem().getUrl());

        // Test that clicking on history items in VR works
        ThreadUtils.runOnUiThreadBlocking(() -> itemViews.get(0).onClick());
        ChromeTabUtils.waitForTabPageLoaded(
                mTestRule.getActivity().getActivityTab(), getUrl(Page.PAGE_2D_2));
        assertState(mVrTestFramework.getFirstTabWebContents(), Page.PAGE_2D_2,
                PresentationMode.NON_PRESENTING, FullscreenMode.NON_FULLSCREENED);
    }

    /**
     * Tests navigation from a 2D to a WebVR page.
     */
    @Test
    @MediumTest
    public void test2dToWebVr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        impl2dToWeb(Page.PAGE_WEBVR, mVrTestFramework);
    }

    /**
     * Tests navigation from a 2D to a WebXR page.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void test2dToWebXr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        impl2dToWeb(Page.PAGE_WEBXR, mXrTestFramework);
    }

    private void impl2dToWeb(Page page, TestFramework framework)
            throws InterruptedException, TimeoutException {
        framework.loadUrlAndAwaitInitialization(TEST_PAGE_2D_URL, PAGE_LOAD_TIMEOUT_S);

        navigateTo(page);

        assertState(framework.getFirstTabWebContents(), page, PresentationMode.NON_PRESENTING,
                FullscreenMode.NON_FULLSCREENED);
    }

    /**
     * Tests navigation from a fullscreened 2D to a WebVR page.
     */
    @Test
    @MediumTest
    public void test2dFullscreenToWebVr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        impl2dFullscreenToWeb(Page.PAGE_WEBVR, mVrTestFramework);
    }

    /**
     * Tests navigation from a fullscreened 2D to a WebXR page.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void test2dFullscreenToWebXr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        impl2dFullscreenToWeb(Page.PAGE_WEBXR, mXrTestFramework);
    }

    private void impl2dFullscreenToWeb(Page page, TestFramework framework)
            throws InterruptedException, TimeoutException {
        framework.loadUrlAndAwaitInitialization(TEST_PAGE_2D_URL, PAGE_LOAD_TIMEOUT_S);
        enterFullscreenOrFail(framework.getFirstTabWebContents());

        navigateTo(page);

        assertState(framework.getFirstTabWebContents(), page, PresentationMode.NON_PRESENTING,
                FullscreenMode.NON_FULLSCREENED);
    }

    /**
     * Tests navigation from a WebVR to a 2D page.
     */
    @Test
    @MediumTest
    public void testWebVrTo2d()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webTo2dImpl(Page.PAGE_WEBVR, mVrTestFramework);
    }

    /**
     * Tests navigation from a WebXR to a 2D page.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testWebXrTo2d()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webTo2dImpl(Page.PAGE_WEBXR, mXrTestFramework);
    }

    private void webTo2dImpl(Page page, TestFramework framework)
            throws InterruptedException, TimeoutException {
        framework.loadUrlAndAwaitInitialization(getUrl(page), PAGE_LOAD_TIMEOUT_S);

        navigateTo(Page.PAGE_2D);

        assertState(framework.getFirstTabWebContents(), Page.PAGE_2D,
                PresentationMode.NON_PRESENTING, FullscreenMode.NON_FULLSCREENED);
    }

    /**
     * Tests navigation from a WebVR to a WebVR page.
     */
    @Test
    @MediumTest
    public void testWebVrToWebVr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webToWebImpl(Page.PAGE_WEBVR, mVrTestFramework);
    }

    /**
     * Tests navigation from a WebXR to a WebXR page.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testWebXrToWebXr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webToWebImpl(Page.PAGE_WEBXR, mXrTestFramework);
    }

    private void webToWebImpl(Page page, TestFramework framework)
            throws InterruptedException, TimeoutException {
        framework.loadUrlAndAwaitInitialization(getUrl(page), PAGE_LOAD_TIMEOUT_S);

        navigateTo(page);

        assertState(framework.getFirstTabWebContents(), page, PresentationMode.NON_PRESENTING,
                FullscreenMode.NON_FULLSCREENED);
    }

    /**
     * Tests navigation from a presenting WebVR to a 2D page.
     */
    @Test
    @MediumTest
    public void testWebVrPresentingTo2d()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webPresentingTo2dImpl(Page.PAGE_WEBVR, mVrTestFramework);
    }

    /**
     * Tests navigation from a presenting WebXR to a 2D page.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testWebXrPresentingTo2d()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webPresentingTo2dImpl(Page.PAGE_WEBXR, mXrTestFramework);
    }

    private void webPresentingTo2dImpl(Page page, TestFramework framework)
            throws InterruptedException, TimeoutException {
        framework.loadUrlAndAwaitInitialization(getUrl(page), PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);

        navigateTo(Page.PAGE_2D);

        assertState(framework.getFirstTabWebContents(), Page.PAGE_2D,
                PresentationMode.NON_PRESENTING, FullscreenMode.NON_FULLSCREENED);
    }

    /**
     * Tests navigation from a presenting WebVR to a WebVR page.
     */
    @Test
    @MediumTest
    public void testWebVrPresentingToWebVr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webPresentingToWebImpl(Page.PAGE_WEBVR, mVrTestFramework);
    }

    /**
     * Tests navigation from a presenting WebXR to a WebXR page.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testWebXrPresentingToWebXr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webPresentingToWebImpl(Page.PAGE_WEBXR, mXrTestFramework);
    }

    private void webPresentingToWebImpl(Page page, TestFramework framework)
            throws InterruptedException, TimeoutException {
        framework.loadUrlAndAwaitInitialization(getUrl(page), PAGE_LOAD_TIMEOUT_S);
        TransitionUtils.enterPresentationOrFail(framework);

        navigateTo(page);

        assertState(framework.getFirstTabWebContents(), page, PresentationMode.NON_PRESENTING,
                FullscreenMode.NON_FULLSCREENED);
    }

    /**
     * Tests navigation from a fullscreened WebVR to a 2D page.
     */
    @Test
    @MediumTest
    public void testWebVrFullscreenTo2d()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webFullscreenTo2dImpl(Page.PAGE_WEBVR, mVrTestFramework);
    }

    /**
     * Tests navigation from a fullscreened WebXR to a 2D page.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testWebXrFullscreenTo2d()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webFullscreenTo2dImpl(Page.PAGE_WEBXR, mXrTestFramework);
    }

    private void webFullscreenTo2dImpl(Page page, TestFramework framework)
            throws InterruptedException, TimeoutException {
        framework.loadUrlAndAwaitInitialization(getUrl(page), PAGE_LOAD_TIMEOUT_S);
        enterFullscreenOrFail(framework.getFirstTabWebContents());

        navigateTo(Page.PAGE_2D);

        assertState(framework.getFirstTabWebContents(), Page.PAGE_2D,
                PresentationMode.NON_PRESENTING, FullscreenMode.NON_FULLSCREENED);
    }

    /**
     * Tests navigation from a fullscreened WebVR to a WebVR page.
     */
    @Test
    @MediumTest
    public void testWebVrFullscreenToWebVr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webFullscreenToWebImpl(Page.PAGE_WEBVR, mVrTestFramework);
    }

    /**
     * Tests navigation from a fullscreened WebXR to a WebXR page.
     */
    @Test
    @MediumTest
    @CommandLineFlags.Remove({"enable-webvr"})
    @CommandLineFlags.Add({"enable-features=WebXR"})
    public void testWebXrFullscreenToWebXr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        webFullscreenToWebImpl(Page.PAGE_WEBXR, mXrTestFramework);
    }

    private void webFullscreenToWebImpl(Page page, TestFramework framework)
            throws InterruptedException, TimeoutException {
        framework.loadUrlAndAwaitInitialization(getUrl(page), PAGE_LOAD_TIMEOUT_S);
        enterFullscreenOrFail(framework.getFirstTabWebContents());

        navigateTo(page);

        assertState(framework.getFirstTabWebContents(), page, PresentationMode.NON_PRESENTING,
                FullscreenMode.NON_FULLSCREENED);
    }

    /**
     * Tests that the back button is disabled in VR if pressing it in regular 2D Chrome would result
     * in Chrome being backgrounded.
     */
    @Test
    @MediumTest
    public void testBackDoesntBackgroundChrome()
            throws IllegalArgumentException, InterruptedException {
        Assert.assertFalse("Back button isn't disabled.", VrTransitionUtils.isBackButtonEnabled());
        mTestRule.loadUrlInNewTab(getUrl(Page.PAGE_2D), false, TabLaunchType.FROM_CHROME_UI);
        Assert.assertFalse("Back button isn't disabled.", VrTransitionUtils.isBackButtonEnabled());
        final Tab tab =
                mTestRule.loadUrlInNewTab(getUrl(Page.PAGE_2D), false, TabLaunchType.FROM_LINK);
        Assert.assertTrue("Back button isn't enabled.", VrTransitionUtils.isBackButtonEnabled());
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestRule.getActivity().getTabModelSelector().closeTab(tab);
            }
        });
        Assert.assertFalse("Back button isn't disabled.", VrTransitionUtils.isBackButtonEnabled());
    }

    /**
     * Tests navigation from a fullscreened WebVR to a WebVR page.
     */
    @Test
    @MediumTest
    public void testNavigationButtons() throws IllegalArgumentException, InterruptedException {
        Assert.assertFalse("Back button isn't disabled.", VrTransitionUtils.isBackButtonEnabled());
        Assert.assertFalse(
                "Forward button isn't disabled.", VrTransitionUtils.isForwardButtonEnabled());
        // Opening a new tab shouldn't enable the back button
        mTestRule.loadUrlInNewTab(getUrl(Page.PAGE_2D), false, TabLaunchType.FROM_CHROME_UI);
        Assert.assertFalse("Back button isn't disabled.", VrTransitionUtils.isBackButtonEnabled());
        Assert.assertFalse(
                "Forward button isn't disabled.", VrTransitionUtils.isForwardButtonEnabled());
        // Navigating to a new page should enable the back button
        mTestRule.loadUrl(getUrl(Page.PAGE_WEBVR));
        Assert.assertTrue("Back button isn't enabled.", VrTransitionUtils.isBackButtonEnabled());
        Assert.assertFalse(
                "Forward button isn't disabled.", VrTransitionUtils.isForwardButtonEnabled());
        // Navigating back should disable the back button and enable the forward button
        VrTransitionUtils.navigateBack();
        ChromeTabUtils.waitForTabPageLoaded(
                mTestRule.getActivity().getActivityTab(), getUrl(Page.PAGE_2D));
        Assert.assertFalse("Back button isn't disabled.", VrTransitionUtils.isBackButtonEnabled());
        Assert.assertTrue(
                "Forward button isn't enabled.", VrTransitionUtils.isForwardButtonEnabled());
        // Navigating forward should disable the forward button and enable the back button
        VrTransitionUtils.navigateForward();
        ChromeTabUtils.waitForTabPageLoaded(
                mTestRule.getActivity().getActivityTab(), getUrl(Page.PAGE_WEBVR));
        Assert.assertTrue("Back button isn't enabled.", VrTransitionUtils.isBackButtonEnabled());
        Assert.assertFalse(
                "Forward button isn't disabled.", VrTransitionUtils.isForwardButtonEnabled());
    }

    /**
     * Tests that navigation to/from native pages works properly and that interacting with the
     * screen doesn't cause issues. See crbug.com/737167.
     */
    @Test
    @MediumTest
    public void testNativeNavigationAndInteraction()
            throws IllegalArgumentException, InterruptedException {
        for (String url : NATIVE_URLS_OF_INTEREST) {
            mTestRule.loadUrl(TEST_PAGE_2D_URL, PAGE_LOAD_TIMEOUT_S);
            mTestRule.loadUrl(url, PAGE_LOAD_TIMEOUT_S);
            ClickUtils.mouseSingleClickView(InstrumentationRegistry.getInstrumentation(),
                    mTestRule.getActivity().getWindow().getDecorView().getRootView());
        }
        mTestRule.loadUrl(TEST_PAGE_2D_URL, PAGE_LOAD_TIMEOUT_S);
    }

    /**
     * Tests that renderer crashes while in (fullscreen) 2D browsing don't exit VR.
     */
    @Test
    @MediumTest
    public void testRendererKilledInFullscreenStaysInVr()
            throws IllegalArgumentException, InterruptedException, TimeoutException {
        mVrTestFramework.loadUrlAndAwaitInitialization(TEST_PAGE_2D_URL, PAGE_LOAD_TIMEOUT_S);
        enterFullscreenOrFail(mVrTestFramework.getFirstTabWebContents());

        final Tab tab = mTestRule.getActivity().getActivityTab();
        ThreadUtils.runOnUiThreadBlocking(() -> tab.simulateRendererKilledForTesting(true));

        mVrTestFramework.simulateRendererKilled();

        ThreadUtils.runOnUiThreadBlocking(() -> tab.reload());
        ChromeTabUtils.waitForTabPageLoaded(tab, TEST_PAGE_2D_URL);
        ChromeTabUtils.waitForInteractable(tab);

        assertState(mVrTestFramework.getFirstTabWebContents(), Page.PAGE_2D,
                PresentationMode.NON_PRESENTING, FullscreenMode.NON_FULLSCREENED);
    }
}
