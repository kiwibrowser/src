// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.util;

import static org.chromium.chrome.browser.vr_shell.VrTestFramework.POLL_TIMEOUT_LONG_MS;

import org.junit.Assert;

import org.chromium.chrome.browser.vr_shell.TestVrShellDelegate;
import org.chromium.chrome.browser.vr_shell.VrTestFramework;
import org.chromium.content_public.browser.WebContents;

/**
 * Class containing utility functions for transitioning between different
 * states in VR, such as fullscreen, WebVR presentation, and the VR browser.
 *
 * All the transitions in this class are performed directly through Chrome,
 * as opposed to NFC tag simulation which involves receiving an intent from
 * an outside application (VR Services).
 */
public class VrTransitionUtils extends TransitionUtils {
    /**
     * Sends a click event directly to the WebGL canvas then waits for WebVR to
     * think that it is presenting, failing if this does not occur within the
     * allotted time.
     *
     * @param cvc The ContentViewCore for the tab the canvas is in.
     */
    public static void enterPresentationOrFail(WebContents webContents) {
        enterPresentation(webContents);
        Assert.assertTrue(VrTestFramework.pollJavaScriptBoolean(
                "vrDisplay.isPresenting", POLL_TIMEOUT_LONG_MS, webContents));
        Assert.assertTrue(TestVrShellDelegate.getVrShellForTesting().getWebVrModeEnabled());
    }
}
