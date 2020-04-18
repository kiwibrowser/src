// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.content.browser.test.util.JavaScriptUtils;
import org.chromium.content_public.browser.WebContents;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Test that enabling and attempting to use WebVR neither causes any crashes
 * nor returns any VRDisplays.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class WebViewWebVrTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private TestAwContentsClient mContentsClient;
    private AwTestContainerView mTestContainerView;
    private WebContents mWebContents;

    @Before
    public void setUp() throws Exception {
        mContentsClient = new TestAwContentsClient();
        mTestContainerView = mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        mWebContents = mTestContainerView.getWebContents();
        AwActivityTestRule.enableJavaScriptOnUiThread(mTestContainerView.getAwContents());
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    @CommandLineFlags.Add("enable-webvr")
    public void testWebVrNotFunctional() throws Throwable {
        mActivityTestRule.loadUrlSync(mTestContainerView.getAwContents(),
                mContentsClient.getOnPageFinishedHelper(),
                "file:///android_asset/webvr_not_functional_test.html");
        // Poll the boolean to know when the promise resolves
        AwActivityTestRule.pollInstrumentationThread(() -> {
            String result = "false";
            try {
                result = JavaScriptUtils.executeJavaScriptAndWaitForResult(
                        mWebContents, "promiseResolved", 100, TimeUnit.MILLISECONDS);
            } catch (InterruptedException | TimeoutException e) {
                // Expected to happen regularly, do nothing
            }
            return Boolean.parseBoolean(result);
        });

        // Assert that the promise resolved instead of rejecting, but returned
        // 0 VRDisplays
        Assert.assertTrue(JavaScriptUtils
                                  .executeJavaScriptAndWaitForResult(
                                          mWebContents, "numDisplays", 100, TimeUnit.MILLISECONDS)
                                  .equals("0"));
    }
}
