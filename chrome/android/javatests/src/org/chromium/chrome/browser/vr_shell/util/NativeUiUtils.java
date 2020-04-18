// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.util;

import android.graphics.PointF;
import android.view.Choreographer;
import android.view.View;

import org.junit.Assert;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.vr_shell.TestVrShellDelegate;
import org.chromium.chrome.browser.vr_shell.UserFriendlyElementName;
import org.chromium.chrome.browser.vr_shell.VrDialog;
import org.chromium.chrome.browser.vr_shell.VrShellImpl;
import org.chromium.chrome.browser.vr_shell.VrUiTestAction;
import org.chromium.chrome.browser.vr_shell.VrUiTestActivityResult;
import org.chromium.chrome.browser.vr_shell.VrViewContainer;

import java.util.concurrent.CountDownLatch;

/**
 * Class containing utility functions for interacting with the VR browser native UI, e.g. the
 * omnibox or back button.
 */
public class NativeUiUtils {
    // Arbitrary but reasonable amount of time to expect the UI to stop updating after interacting
    // with an element.
    private static final int DEFAULT_UI_QUIESCENCE_TIMEOUT_MS = 1000;

    /**
     * Clicks on a UI element as if done via a controller click.
     * @param elementName The UserFriendlyElementName that will be clicked on.
     * @param position The position within the element to click on.
     */
    public static void clickElement(int elementName, PointF position) {
        TestVrShellDelegate instance = TestVrShellDelegate.getInstance();
        instance.performUiActionForTesting(elementName, VrUiTestAction.HOVER_ENTER, position);
        instance.performUiActionForTesting(elementName, VrUiTestAction.BUTTON_DOWN, position);
        instance.performUiActionForTesting(elementName, VrUiTestAction.BUTTON_UP, position);
    }

    /**
     * Clicks on a UI element as if done via a controller click and waits until all resulting
     * animations have finished and propogated to the point of being visible in screenshots.
     * @param elementName The UserFriendlyElementName that will be clicked on.
     * @param position The position within the element to click on.
     */
    public static void clickElementAndWaitForUiQuiescence(int elementName, PointF position)
            throws InterruptedException {
        final TestVrShellDelegate instance = TestVrShellDelegate.getInstance();
        final CountDownLatch resultLatch = new CountDownLatch(1);
        instance.setUiExpectingActivityForTesting(
                DEFAULT_UI_QUIESCENCE_TIMEOUT_MS, () -> { resultLatch.countDown(); });
        clickElement(elementName, position);

        // Wait for any outstanding animations to finish.
        resultLatch.await();
        Assert.assertEquals(
                VrUiTestActivityResult.QUIESCENT, instance.getLastUiActivityResultForTesting());
    }

    /**
     * Clicks on a fallback UI element's positive button, e.g. "Allow" or "Confirm".
     */
    public static void clickFallbackUiPositiveButton() throws InterruptedException {
        clickFallbackUiButton(R.id.positive_button);
    }

    /**
     * Clicks on a fallback UI element's negative button, e.g. "Deny" or "Cancel".
     */
    public static void clickFallbackUiNegativeButton() throws InterruptedException {
        clickFallbackUiButton(R.id.negative_button);
    }

    /**
     * Blocks until the specified number of frames have been triggered by the Choreographer.
     * @param numFrames The number of frames to wait for.
     */
    public static void waitNumFrames(int numFrames) throws InterruptedException {
        final CountDownLatch frameLatch = new CountDownLatch(numFrames);
        ThreadUtils.runOnUiThread(() -> {
            final Choreographer.FrameCallback callback = new Choreographer.FrameCallback() {
                @Override
                public void doFrame(long frameTimeNanos) {
                    if (frameLatch.getCount() == 0) return;
                    Choreographer.getInstance().postFrameCallback(this);
                    frameLatch.countDown();
                };
            };
            Choreographer.getInstance().postFrameCallback(callback);
        });
        frameLatch.await();
    }

    private static void clickFallbackUiButton(int buttonId) throws InterruptedException {
        VrShellImpl vrShell = (VrShellImpl) (TestVrShellDelegate.getVrShellForTesting());
        VrViewContainer viewContainer = vrShell.getVrViewContainerForTesting();
        Assert.assertTrue(
                "VrViewContainer actually has children", viewContainer.getChildCount() > 0);
        // Click on whatever dialog was most recently added
        VrDialog vrDialog = (VrDialog) viewContainer.getChildAt(viewContainer.getChildCount() - 1);
        View button = vrDialog.findViewById(buttonId);
        Assert.assertNotNull("Found a View with matching ID", button);
        // We need to flip the Y axis since this calculates from the bottom left, but Android seems
        // to process input events from the top left?
        // We also need to scale by the VrDialog's width/height since the native code will later
        // multiply by the same amount.
        float x = (button.getX() + button.getWidth() / 2) / vrDialog.getWidth();
        float y = (vrDialog.getHeight() - (button.getY() + button.getHeight() / 2))
                / vrDialog.getHeight();
        PointF buttonCenter = new PointF(x, y);
        clickElementAndWaitForUiQuiescence(UserFriendlyElementName.BROWSING_DIALOG, buttonCenter);
    }
}