// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwContentsStatics;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.Feature;

/**
 * AwContentsStatics tests.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class AwContentsStaticsTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private static class ClearClientCertCallbackHelper extends CallbackHelper
            implements Runnable {
        @Override
        public void run() {
            notifyCalled();
        }
    }

    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    public void testClearClientCertPreferences() throws Throwable {
        final ClearClientCertCallbackHelper callbackHelper = new ClearClientCertCallbackHelper();
        int currentCallCount = callbackHelper.getCallCount();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
            // Make sure calling clearClientCertPreferences with null callback does not
            // cause a crash.
            AwContentsStatics.clearClientCertPreferences(null);
            AwContentsStatics.clearClientCertPreferences(callbackHelper);
        });
        callbackHelper.waitForCallback(currentCallCount);
    }
}
