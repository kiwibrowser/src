// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell.util;

import static org.chromium.chrome.browser.vr_shell.XrTestFramework.POLL_TIMEOUT_LONG_MS;

import org.junit.Assert;

import org.chromium.chrome.browser.vr_shell.TestVrShellDelegate;
import org.chromium.chrome.browser.vr_shell.XrTestFramework;
import org.chromium.content_public.browser.WebContents;

/**
 * Class containing utility functions for transitioning between different
 * states in VR, such as fullscreen, WebVR presentation, and the VR browser.
 *
 * All the transitions in this class are performed directly through Chrome,
 * as opposed to NFC tag simulation which involves receiving an intent from
 * an outside application (VR Services).
 */
public class XrTransitionUtils extends TransitionUtils {
    /**
     * WebXR version of enterPresentationOrFail since the condition to check is different between
     * the two APIs.
     */
    public static void enterPresentationOrFail(WebContents webContents) {
        enterPresentation(webContents);
        Assert.assertTrue(XrTestFramework.pollJavaScriptBoolean(
                "exclusiveSession != null", POLL_TIMEOUT_LONG_MS, webContents));
        Assert.assertTrue(TestVrShellDelegate.getVrShellForTesting().getWebVrModeEnabled());
    }
}
