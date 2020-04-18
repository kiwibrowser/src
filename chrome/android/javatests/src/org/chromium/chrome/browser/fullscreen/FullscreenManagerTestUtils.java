// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.fullscreen;

import android.os.SystemClock;

import org.junit.Assert;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.fullscreen.ChromeFullscreenManager.FullscreenListener;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TouchCommon;
import org.chromium.content_public.browser.RenderCoordinates;

import java.util.concurrent.Callable;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Utility methods for testing the {@link ChromeFullscreenManager}.
 */
public class FullscreenManagerTestUtils {
    /**
     * Scrolls the underlying web page to show or hide the browser controls.
     * @param testRule The test rule for the currently running test.
     * @param show Whether the browser controls should be shown.
     */
    public static void scrollBrowserControls(ChromeTabbedActivityTestRule testRule, boolean show) {
        ChromeFullscreenManager fullscreenManager = testRule.getActivity().getFullscreenManager();
        int browserControlsHeight = fullscreenManager.getTopControlsHeight();

        waitForPageToBeScrollable(testRule.getActivity().getActivityTab());

        float dragX = 50f;
        // Use a larger scroll range than the height of the browser controls to ensure we overcome
        // the delay in a scroll start being sent.
        float dragStartY = browserControlsHeight * 3;
        float dragEndY = dragStartY - browserControlsHeight * 2;
        float expectedPosition = -browserControlsHeight;
        if (show) {
            expectedPosition = 0f;
            float tempDragStartY = dragStartY;
            dragStartY = dragEndY;
            dragEndY = tempDragStartY;
        }
        long downTime = SystemClock.uptimeMillis();
        TouchCommon.dragStart(testRule.getActivity(), dragX, dragStartY, downTime);
        TouchCommon.dragTo(
                testRule.getActivity(), dragX, dragX, dragStartY, dragEndY, 100, downTime);
        TouchCommon.dragEnd(testRule.getActivity(), dragX, dragEndY, downTime);
        waitForBrowserControlsPosition(testRule, expectedPosition);
    }

    /**
     * Waits for the browser controls to reach the specified position.
     * @param testRule The test rule for the currently running test.
     * @param position The desired top controls offset.
     */
    public static void waitForBrowserControlsPosition(
            ChromeTabbedActivityTestRule testRule, float position) {
        final ChromeFullscreenManager fullscreenManager =
                testRule.getActivity().getFullscreenManager();
        CriteriaHelper.pollUiThread(Criteria.equals(position, new Callable<Float>() {
            @Override
            public Float call() {
                return fullscreenManager.getTopControlOffset();
            }
        }));
    }

    /**
     * Waits for the base page to be scrollable.
     * @param tab The current activity tab.
     */
    public static void waitForPageToBeScrollable(final Tab tab) {
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return RenderCoordinates.fromWebContents(tab.getWebContents())
                               .getContentHeightPixInt()
                        > tab.getContentView().getHeight();
            }
        });
    }

    /**
     * Waits for the browser controls to be moveable by user gesture.
     *
     * This function requires the browser controls to start fully visible. Then it ensures that
     * at some point the controls can be moved by user gesture.  It will then fully cycle the top
     * controls to entirely hidden and back to fully shown.
     *
     * @param testRule The test rule for the currently running test.
     * @param tab The current activity tab.
     */
    public static void waitForBrowserControlsToBeMoveable(
            ChromeTabbedActivityTestRule testRule, final Tab tab) throws InterruptedException {
        waitForBrowserControlsPosition(testRule, 0f);

        final CallbackHelper contentMovedCallback = new CallbackHelper();
        final ChromeFullscreenManager fullscreenManager =
                testRule.getActivity().getFullscreenManager();
        final float initialVisibleContentOffset = fullscreenManager.getTopVisibleContentOffset();

        fullscreenManager.addListener(new FullscreenListener() {
            @Override
            public void onControlsOffsetChanged(
                    float topOffset, float bottomOffset, boolean needsAnimate) {
                if (fullscreenManager.getTopVisibleContentOffset() != initialVisibleContentOffset) {
                    contentMovedCallback.notifyCalled();
                    fullscreenManager.removeListener(this);
                }
            }

            @Override
            public void onToggleOverlayVideoMode(boolean enabled) {}

            @Override
            public void onContentOffsetChanged(float offset) {}

            @Override
            public void onBottomControlsHeightChanged(int bottomControlsHeight) {}
        });

        float dragX = 50f;
        float dragStartY = tab.getView().getHeight() - 50f;

        for (int i = 0; i < 10; i++) {
            float dragEndY = dragStartY - fullscreenManager.getTopControlsHeight();

            long downTime = SystemClock.uptimeMillis();
            TouchCommon.dragStart(testRule.getActivity(), dragX, dragStartY, downTime);
            TouchCommon.dragTo(
                    testRule.getActivity(), dragX, dragX, dragStartY, dragEndY, 100, downTime);
            TouchCommon.dragEnd(testRule.getActivity(), dragX, dragEndY, downTime);

            try {
                contentMovedCallback.waitForCallback(0, 1, 500, TimeUnit.MILLISECONDS);

                scrollBrowserControls(testRule, false);
                scrollBrowserControls(testRule, true);

                return;
            } catch (TimeoutException e) {
                // Ignore and retry
            }
        }

        Assert.fail("Visible content never moved as expected.");
    }

    /**
     * Disable any browser visibility overrides for testing.
     */
    public static void disableBrowserOverrides() {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                BrowserStateBrowserControlsVisibilityDelegate.disableForTesting();
            }
        });
    }
}
