// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;

import static org.chromium.base.test.util.ScalableTimeout.scaleTimeout;

import org.junit.Assert;

import org.chromium.base.ThreadUtils;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer.OnEvaluateJavaScriptResultHelper;
import org.chromium.content_public.browser.WebContents;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Collection of JavaScript utilities.
 */
public class JavaScriptUtils {
    private static final long EVALUATION_TIMEOUT_SECONDS = scaleTimeout(5);

    /**
     * Executes the given snippet of JavaScript code within the given ContentView.
     * Returns the result of its execution in JSON format.
     */
    public static String executeJavaScriptAndWaitForResult(
            WebContents webContents, String code) throws InterruptedException, TimeoutException {
        return executeJavaScriptAndWaitForResult(
                webContents, code, EVALUATION_TIMEOUT_SECONDS, TimeUnit.SECONDS);
    }

    /**
     * Executes the given snippet of JavaScript code within the given ContentViewCore.
     * Does not depend on ContentView and TestCallbackHelperContainer.
     * Returns the result of its execution in JSON format.
     */
    public static String executeJavaScriptAndWaitForResult(
            final WebContents webContents,
            final String code,
            final long timeout,
            final TimeUnit timeoutUnits)
                    throws InterruptedException, TimeoutException {
        final OnEvaluateJavaScriptResultHelper helper = new OnEvaluateJavaScriptResultHelper();
        // Calling this from the UI thread causes it to time-out: the UI thread being blocked won't
        // have a chance to process the JavaScript eval response).
        Assert.assertFalse("Executing JavaScript should be done from the test thread, "
                + " not the UI thread", ThreadUtils.runningOnUiThread());
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                helper.evaluateJavaScriptForTests(webContents, code);
            }
        });
        helper.waitUntilHasValue(timeout, timeoutUnits);
        Assert.assertTrue("Failed to retrieve JavaScript evaluation results.", helper.hasValue());
        return helper.getJsonResultAndClear();
    }

    /**
     * Executes the given snippet of JavaScript code but does not wait for the result.
     */
    public static void executeJavaScript(final WebContents webContents, final String code) {
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                webContents.evaluateJavaScriptForTests(code, null);
            }
        });
    }
}
