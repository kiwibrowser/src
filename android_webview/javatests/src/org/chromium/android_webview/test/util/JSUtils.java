// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test.util;

import static org.chromium.base.test.util.ScalableTimeout.scaleTimeout;

import android.app.Instrumentation;

import org.junit.Assert;

import org.chromium.android_webview.AwContents;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer
        .OnEvaluateJavaScriptResultHelper;

/**
 * Collection of functions for JavaScript-based interactions with a page.
 */
public class JSUtils {
    private static final long WAIT_TIMEOUT_MS = scaleTimeout(2000);
    private static final int CHECK_INTERVAL = 100;

    public static void clickOnLinkUsingJs(final Instrumentation instrumentation,
            final AwContents awContents,
            final OnEvaluateJavaScriptResultHelper onEvaluateJavaScriptResultHelper,
            final String linkId) throws Exception {
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                try {
                    String linkIsNotNull = executeJavaScriptAndWaitForResult(instrumentation,
                            awContents, onEvaluateJavaScriptResultHelper,
                            "document.getElementById('" + linkId + "') != null");
                    return linkIsNotNull.equals("true");
                } catch (Throwable t) {
                    t.printStackTrace();
                    Assert.fail("Failed to check if DOM is loaded: " + t.toString());
                    return false;
                }
            }
        }, WAIT_TIMEOUT_MS, CHECK_INTERVAL);

        instrumentation.runOnMainSync(
                () -> awContents.getWebContents().evaluateJavaScriptForTests(
                        "var evObj = new MouseEvent('click', {bubbles: true});"
                                + "document.getElementById('" + linkId + "').dispatchEvent(evObj);"
                                + "console.log('element with id [" + linkId + "] clicked');",
                        null));
    }

    public static String executeJavaScriptAndWaitForResult(Instrumentation instrumentation,
            final AwContents awContents,
            final OnEvaluateJavaScriptResultHelper onEvaluateJavaScriptResultHelper,
            final String code) throws Exception {
        instrumentation.runOnMainSync(
                () -> onEvaluateJavaScriptResultHelper.evaluateJavaScriptForTests(
                        awContents.getWebContents(), code));
        onEvaluateJavaScriptResultHelper.waitUntilHasValue();
        Assert.assertTrue("Failed to retrieve JavaScript evaluation results.",
                onEvaluateJavaScriptResultHelper.hasValue());
        return onEvaluateJavaScriptResultHelper.getJsonResultAndClear();
    }
}
