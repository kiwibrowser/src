// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.content_public.browser.WebContents;

/**
 * Extension of TestFramework containing WebVR-specific functionality.
 */
public class VrTestFramework extends TestFramework {
    public VrTestFramework(ChromeActivityTestRule rule) {
        super(rule);
    }

    /**
     * Checks whether a VRDisplay was actually found.
     * @param webContents The WebContents to run the JavaScript through.
     * @return Whether a VRDisplay was found.
     */
    public static boolean vrDisplayFound(WebContents webContents) {
        return !runJavaScriptOrFail("vrDisplay", POLL_TIMEOUT_SHORT_MS, webContents).equals("null");
    }
}
