// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import static org.chromium.base.test.util.ScalableTimeout.scaleTimeout;

import android.os.SystemClock;
import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.content_shell_apk.ContentShellActivity;
import org.chromium.content_shell_apk.ContentShellActivityTestRule;

import java.io.File;

/**
 * Test suite for TracingControllerAndroid.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class TracingControllerAndroidTest {
    @Rule
    public ContentShellActivityTestRule mActivityTestRule = new ContentShellActivityTestRule();

    private static final long TIMEOUT_MILLIS = scaleTimeout(30 * 1000);

    @Test
    @MediumTest
    @Feature({"GPU"})
    @DisabledTest(message = "crbug.com/621956")
    public void testTraceFileCreation() throws Exception {
        ContentShellActivity activity = mActivityTestRule.launchContentShellWithUrl("about:blank");
        mActivityTestRule.waitForActiveShellToBeDoneLoading();

        final TracingControllerAndroid tracingController = new TracingControllerAndroid(activity);
        Assert.assertFalse(tracingController.isTracing());
        Assert.assertNull(tracingController.getOutputPath());

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Assert.assertTrue(tracingController.startTracing(true, "*", "record-until-full"));
            }
        });

        Assert.assertTrue(tracingController.isTracing());
        File file = new File(tracingController.getOutputPath());
        Assert.assertTrue(file.getName().startsWith("chrome-profile-results"));

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                tracingController.stopTracing();
            }
        });

        // The tracer stops asynchronously, because it needs to wait for native code to flush and
        // close the output file. Give it a little time.
        long startTime = SystemClock.uptimeMillis();
        while (tracingController.isTracing()) {
            if (SystemClock.uptimeMillis() > startTime + TIMEOUT_MILLIS) {
                Assert.fail("Timed out waiting for tracing to stop.");
            }
            Thread.sleep(1000);
        }

        // It says it stopped, so it should have written the output file.
        Assert.assertTrue(file.exists());
        Assert.assertTrue(file.delete());
        tracingController.destroy();
    }
}
