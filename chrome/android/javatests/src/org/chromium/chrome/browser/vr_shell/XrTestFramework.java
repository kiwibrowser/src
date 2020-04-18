// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.content_public.browser.WebContents;

/**
 * Extension of TestFramework containing WebXR-specific functionality.
 */
public class XrTestFramework extends TestFramework {
    public XrTestFramework(ChromeActivityTestRule rule) {
        super(rule);
    }

    /**
     * Checks whether an XRDevice was actually found.
     * @param webContents The WebContents to run the JavaScript through.
     * @return Whether an XRDevice was found.
     */
    public static boolean xrDeviceFound(WebContents webContents) {
        return !runJavaScriptOrFail("xrDevice", POLL_TIMEOUT_SHORT_MS, webContents).equals("null");
    }
}
