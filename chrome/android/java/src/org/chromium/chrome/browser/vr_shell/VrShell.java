// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.graphics.PointF;
import android.widget.FrameLayout;

/**
 * Abstracts away the VrShell class, which may or may not be present at runtime depending on
 * compile flags.
 */
public interface VrShell extends VrDialogManager, VrToastManager {
    /**
     * Performs native VrShell initialization.
     */
    void initializeNative(boolean forWebVr, boolean webVrAutopresentationExpected, boolean inCct,
            boolean isStandaloneVrDevice);

    /**
     * Pauses VrShell.
     */
    void pause();

    /**
     * Resumes VrShell.
     */
    void resume();

    /**
     * Destroys VrShell.
     */
    void teardown();

    /**
     * Sets whether we're presenting WebVR content or not.
     */
    void setWebVrModeEnabled(boolean enabled);

    /**
     * Returns true if we're presenting WebVR content.
     */
    boolean getWebVrModeEnabled();

    /**
     * Returns true if our URL bar is showing a string.
     */
    boolean isDisplayingUrlForTesting();

    /**
     * Returns the GVRLayout as a FrameLayout.
     */
    FrameLayout getContainer();

    /**
     * Returns whether the back button is enabled.
     */
    Boolean isBackButtonEnabled();

    /**
     * Returns whether the forward button is enabled.
     */
    Boolean isForwardButtonEnabled();

    /**
     * Requests to exit VR.
     */
    void requestToExitVr(@UiUnsupportedMode int reason, boolean showExitPromptBeforeDoff);

    /**
     *  Triggers VrShell to navigate forward.
     */
    void navigateForward();

    /**
     *  Triggers VrShell to navigate backward.
     */
    void navigateBack();

    /**
     * Simulates a user accepting the currently visible DOFF prompt.
     */
    void acceptDoffPromptForTesting();

    /**
     * Performs a UI action that doesn't require a position argument on a UI element.
     * @param elementName The UserFriendlyElementName to perform the UI action on.
     * @param actionType The VrUiTestAction to perform on the specified element.
     * @param position The position on the element to perform the action.
     */
    void performUiActionForTesting(int elementName, int actionType, PointF position);

    /**
     * Notifies the native UI that it should start tracking UI activity, reporting a result
     * when either the UI has activity but reaches a stable state, or the specified timeout is
     * reached.
     * @param quiescenceTimeoutMs The maximum amount of time spent waiting for UI quiescence before
     *        reporting a timeout.
     * @param resultCallback A Runnable that will be run once the UI reports a result.
     */
    void setUiExpectingActivityForTesting(int quiescenceTimeoutMs, Runnable resultCallback);

    /**
     * Returns the last result set by the native UI about UI quiescence.
     * @return The VrUiTestActivityResult value last set by the native UI.
     */
    int getLastUiActivityResultForTesting();

    /**
     * @param topContentOffset The content offset (usually applied by the omnibox).
     */
    void rawTopContentOffsetChanged(float topContentOffset);
}
