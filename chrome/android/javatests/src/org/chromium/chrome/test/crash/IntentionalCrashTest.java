// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.crash;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/** Tests that intentionally crash in different ways.
 *
 *  These are all purposefully disabled and should only be run manually.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add(ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE)
public class IntentionalCrashTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    @DisabledTest
    @SmallTest
    @Test
    public void testRendererCrash() {
        try {
            mActivityTestRule.startMainActivityFromLauncher();
            mActivityTestRule.loadUrl("chrome://crash");
        } catch (InterruptedException e) {
            Assert.fail(e.toString());
        }
    }

    @DisabledTest
    @SmallTest
    @Test
    public void testBrowserCrash() {
        try {
            mActivityTestRule.startMainActivityFromLauncher();
            mActivityTestRule.loadUrl("chrome://inducebrowsercrashforrealz");
        } catch (InterruptedException e) {
            Assert.fail(e.toString());
        }
    }

    @DisabledTest
    @SmallTest
    @Test
    public void testJavaCrash() {
        try {
            mActivityTestRule.startMainActivityFromLauncher();
            mActivityTestRule.loadUrl("chrome://java-crash/");
        } catch (InterruptedException e) {
            Assert.fail(e.toString());
        }
    }

    @DisabledTest
    @SmallTest
    @Test
    public void testGpuCrash() {
        try {
            mActivityTestRule.startMainActivityFromLauncher();
            mActivityTestRule.loadUrl("chrome://gpucrash");
        } catch (InterruptedException e) {
            Assert.fail(e.toString());
        }
    }
}
