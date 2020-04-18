// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.PAGE_LOAD_TIMEOUT_S;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_CHECK_INTERVAL_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;
import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_SHORT_MS;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_DEVICE_DAYDREAM;
import static org.chromium.chrome.test.util.ChromeRestriction.RESTRICTION_TYPE_VIEWER_DAYDREAM;

import android.support.test.filters.MediumTest;
import android.support.v7.widget.RecyclerView;
import android.view.View;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.history.HistoryPage;
import org.chromium.chrome.browser.vr_shell.rules.ChromeTabbedActivityVrTestRule;
import org.chromium.chrome.browser.vr_shell.util.VrTransitionUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.RenderCoordinates;

import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * End-to-end tests for Daydream controller input while in the VR browser.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@Restriction({RESTRICTION_TYPE_DEVICE_DAYDREAM, RESTRICTION_TYPE_VIEWER_DAYDREAM})
public class VrShellControllerInputTest {
    // We explicitly instantiate a rule here instead of using parameterization since this class
    // only ever runs in ChromeTabbedActivity.
    @Rule
    public ChromeTabbedActivityVrTestRule mVrTestRule = new ChromeTabbedActivityVrTestRule();

    private VrTestFramework mVrTestFramework;
    private EmulatedVrController mController;

    @Before
    public void setUp() throws Exception {
        mVrTestFramework = new VrTestFramework(mVrTestRule);
        VrTransitionUtils.forceEnterVr();
        VrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        mController = new EmulatedVrController(mVrTestRule.getActivity());
        mController.recenterView();
    }

    private void waitForPageToBeScrollable(final RenderCoordinates coord) {
        final View view = mVrTestRule.getActivity().getActivityTab().getContentView();
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return coord.getContentHeightPixInt() > view.getHeight()
                        && coord.getContentWidthPixInt() > view.getWidth();
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_LONG_MS);
    }

    /**
     * Verifies that swiping up/down/left/right on the Daydream controller's
     * touchpad scrolls the webpage while in the VR browser.
     */
    @Test
    @MediumTest
    public void testControllerScrolling() throws InterruptedException {
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("test_controller_scrolling"),
                PAGE_LOAD_TIMEOUT_S);
        final RenderCoordinates coord =
                RenderCoordinates.fromWebContents(mVrTestRule.getWebContents());
        waitForPageToBeScrollable(coord);

        // Test that scrolling down works
        int startScrollPoint = coord.getScrollYPixInt();
        // Arbitrary, but valid values to scroll smoothly
        int scrollSteps = 20;
        int scrollSpeed = 60;
        mController.scroll(EmulatedVrController.ScrollDirection.DOWN, scrollSteps, scrollSpeed,
                /* fling */ false);
        // We need this second scroll down, otherwise the horizontal scrolling becomes flaky
        // This actually seems to not be an issue in this test case anymore, but still occurs in
        // the fling scroll test, so keep around here as an extra precaution.
        // TODO(bsheedy): Figure out why this is the case
        mController.scroll(EmulatedVrController.ScrollDirection.DOWN, scrollSteps, scrollSpeed,
                /* fling */ false);
        int endScrollPoint = coord.getScrollYPixInt();
        Assert.assertTrue("Controller was able to scroll down", startScrollPoint < endScrollPoint);

        // Test that scrolling up works
        startScrollPoint = endScrollPoint;
        mController.scroll(EmulatedVrController.ScrollDirection.UP, scrollSteps, scrollSpeed,
                /* fling */ false);
        endScrollPoint = coord.getScrollYPixInt();
        Assert.assertTrue("Controller was able to scroll up", startScrollPoint > endScrollPoint);

        // Test that scrolling right works
        startScrollPoint = coord.getScrollXPixInt();
        mController.scroll(EmulatedVrController.ScrollDirection.RIGHT, scrollSteps, scrollSpeed,
                /* fling */ false);
        endScrollPoint = coord.getScrollXPixInt();
        Assert.assertTrue("Controller was able to scroll right", startScrollPoint < endScrollPoint);

        // Test that scrolling left works
        startScrollPoint = endScrollPoint;
        mController.scroll(EmulatedVrController.ScrollDirection.LEFT, scrollSteps, scrollSpeed,
                /* fling */ false);
        endScrollPoint = coord.getScrollXPixInt();
        Assert.assertTrue("Controller was able to scroll left", startScrollPoint > endScrollPoint);
    }

    /**
     * Verifies that fling scrolling works on the Daydream controller's touchpad.
     */
    @Test
    @MediumTest
    public void testControllerFlingScrolling() throws InterruptedException {
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("test_controller_scrolling"),
                PAGE_LOAD_TIMEOUT_S);
        final RenderCoordinates coord =
                RenderCoordinates.fromWebContents(mVrTestRule.getWebContents());
        waitForPageToBeScrollable(coord);

        // Arbitrary, but valid values to trigger fling scrolling
        int scrollSteps = 2;
        int scrollSpeed = 40;

        // Test fling scrolling down
        mController.scroll(EmulatedVrController.ScrollDirection.DOWN, scrollSteps, scrollSpeed,
                /* fling */ true);
        final AtomicInteger endScrollPoint = new AtomicInteger(coord.getScrollYPixInt());
        // Check that we continue to scroll past wherever we were when we let go of the touchpad
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return coord.getScrollYPixInt() > endScrollPoint.get();
            }
        }, POLL_TIMEOUT_SHORT_MS, POLL_CHECK_INTERVAL_LONG_MS);
        mController.cancelFlingScroll();

        // Test fling scrolling up
        mController.scroll(EmulatedVrController.ScrollDirection.UP, scrollSteps, scrollSpeed,
                /* fling */ true);
        endScrollPoint.set(coord.getScrollYPixInt());
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return coord.getScrollYPixInt() < endScrollPoint.get();
            }
        }, POLL_TIMEOUT_SHORT_MS, POLL_CHECK_INTERVAL_LONG_MS);
        mController.cancelFlingScroll();
        // Horizontal scrolling becomes flaky if the scroll bar is at the top when we try to scroll
        // horizontally, so scroll down a bit to ensure that isn't the case.
        mController.scroll(EmulatedVrController.ScrollDirection.DOWN, 10, 60,
                /* fling */ false);

        // Test fling scrolling right
        mController.scroll(EmulatedVrController.ScrollDirection.RIGHT, scrollSteps, scrollSpeed,
                /* fling */ true);
        endScrollPoint.set(coord.getScrollXPixInt());
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return coord.getScrollXPixInt() > endScrollPoint.get();
            }
        }, POLL_TIMEOUT_SHORT_MS, POLL_CHECK_INTERVAL_LONG_MS);
        mController.cancelFlingScroll();

        // Test fling scrolling left
        mController.scroll(EmulatedVrController.ScrollDirection.LEFT, scrollSteps, scrollSpeed,
                /* fling */ true);
        endScrollPoint.set(coord.getScrollXPixInt());
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return coord.getScrollXPixInt() < endScrollPoint.get();
            }
        }, POLL_TIMEOUT_SHORT_MS, POLL_CHECK_INTERVAL_LONG_MS);
    }

    /**
     * Verifies that controller clicks in the VR browser are properly registered on the webpage.
     * This is done by clicking on a link on the page and ensuring that it causes a navigation.
     */
    @Test
    @MediumTest
    public void testControllerClicksRegisterOnWebpage() throws InterruptedException {
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile(
                                    "test_controller_clicks_register_on_webpage"),
                PAGE_LOAD_TIMEOUT_S);

        mController.performControllerClick();
        ChromeTabUtils.waitForTabPageLoaded(mVrTestRule.getActivity().getActivityTab(),
                VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page"));
    }

    /*
     * Verifies that swiping up/down on the Daydream controller's touchpad
     * scrolls a native page while in the VR browser.
     */
    @Test
    @MediumTest
    public void testControllerScrollingNative() throws InterruptedException {
        VrTransitionUtils.forceEnterVr();
        VrTransitionUtils.waitForVrEntry(POLL_TIMEOUT_LONG_MS);
        // Fill history with enough items to scroll
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page"),
                PAGE_LOAD_TIMEOUT_S);
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("test_controller_scrolling"),
                PAGE_LOAD_TIMEOUT_S);
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("generic_webvr_page"),
                PAGE_LOAD_TIMEOUT_S);
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_webvr_page"),
                PAGE_LOAD_TIMEOUT_S);
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("test_webvr_autopresent"),
                PAGE_LOAD_TIMEOUT_S);
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("generic_webxr_page"),
                PAGE_LOAD_TIMEOUT_S);
        mVrTestRule.loadUrl(VrTestFramework.getFileUrlForHtmlTestFile("test_gamepad_button"),
                PAGE_LOAD_TIMEOUT_S);

        mVrTestRule.loadUrl("chrome://history", PAGE_LOAD_TIMEOUT_S);

        RecyclerView recyclerView =
                ((HistoryPage) (mVrTestRule.getActivity().getActivityTab().getNativePage()))
                        .getHistoryManagerForTesting()
                        .getRecyclerViewForTests();

        // Test that scrolling down works
        int startScrollPoint = recyclerView.computeVerticalScrollOffset();
        // Arbitrary, but valid values to scroll smoothly
        int scrollSteps = 20;
        int scrollSpeed = 60;
        mController.scroll(EmulatedVrController.ScrollDirection.DOWN, scrollSteps, scrollSpeed,
                /* fling */ false);
        int endScrollPoint = recyclerView.computeVerticalScrollOffset();
        Assert.assertTrue("Controller was able to scroll down", startScrollPoint < endScrollPoint);

        // Test that scrolling up works
        startScrollPoint = endScrollPoint;
        mController.scroll(EmulatedVrController.ScrollDirection.UP, scrollSteps, scrollSpeed,
                /* fling */ false);
        endScrollPoint = recyclerView.computeVerticalScrollOffset();
        Assert.assertTrue("Controller was able to scroll up", startScrollPoint > endScrollPoint);
    }

    /**
     * Verifies that pressing the Daydream controller's 'app' button causes the user to exit
     * fullscreen
     */
    @Test
    @MediumTest
    @RetryOnFailure(message = "Very rarely, button press not registered (race condition?)")
    public void testAppButtonExitsFullscreen() throws InterruptedException, TimeoutException {
        mVrTestFramework.loadUrlAndAwaitInitialization(
                VrTestFramework.getFileUrlForHtmlTestFile("test_navigation_2d_page"),
                PAGE_LOAD_TIMEOUT_S);
        // Enter fullscreen
        DOMUtils.clickNode(mVrTestFramework.getFirstTabWebContents(), "fullscreen",
                false /* goThroughRootAndroidView */);
        VrTestFramework.waitOnJavaScriptStep(mVrTestFramework.getFirstTabWebContents());
        Assert.assertTrue(DOMUtils.isFullscreen(mVrTestFramework.getFirstTabWebContents()));

        mController.pressReleaseAppButton();
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                try {
                    return !DOMUtils.isFullscreen(mVrTestFramework.getFirstTabWebContents());
                } catch (InterruptedException | TimeoutException e) {
                    return false;
                }
            }
        }, POLL_TIMEOUT_LONG_MS, POLL_CHECK_INTERVAL_LONG_MS);
    }
}
