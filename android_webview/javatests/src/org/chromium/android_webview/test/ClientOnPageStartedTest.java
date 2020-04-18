// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwContents;
import org.chromium.base.test.util.Feature;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer;
import org.chromium.content_public.common.ContentUrlConstants;

/**
 * Tests for the ContentViewClient.onPageStarted() method.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class ClientOnPageStartedTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private TestAwContentsClient mContentsClient;
    private AwContents mAwContents;

    @Before
    public void setUp() throws Exception {
        setTestAwContentsClient(new TestAwContentsClient());
    }

    private void setTestAwContentsClient(TestAwContentsClient contentsClient) throws Exception {
        mContentsClient = contentsClient;
        final AwTestContainerView testContainerView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(mContentsClient);
        mAwContents = testContainerView.getAwContents();
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testOnPageStartedPassesCorrectUrl() throws Throwable {
        TestCallbackHelperContainer.OnPageStartedHelper onPageStartedHelper =
                mContentsClient.getOnPageStartedHelper();

        String html = "<html><body>Simple page.</body></html>";
        int currentCallCount = onPageStartedHelper.getCallCount();
        mActivityTestRule.loadDataAsync(mAwContents, html, "text/html", false);

        onPageStartedHelper.waitForCallback(currentCallCount);
        Assert.assertEquals("data:text/html," + html, onPageStartedHelper.getUrl());
    }

    @Test
    @MediumTest
    @Feature({"AndroidWebView"})
    public void testOnPageStartedCalledOnceOnError() throws Throwable {
        class LocalTestClient extends TestAwContentsClient {
            private boolean mIsOnReceivedErrorCalled;
            private boolean mIsOnPageStartedCalled;
            private boolean mAllowAboutBlank;

            @Override
            public void onReceivedError(int errorCode, String description, String failingUrl) {
                Assert.assertEquals("onReceivedError called twice for " + failingUrl, false,
                        mIsOnReceivedErrorCalled);
                mIsOnReceivedErrorCalled = true;
                Assert.assertEquals(
                        "onPageStarted not called before onReceivedError for " + failingUrl, true,
                        mIsOnPageStartedCalled);
                super.onReceivedError(errorCode, description, failingUrl);
            }

            @Override
            public void onPageStarted(String url) {
                if (mAllowAboutBlank && ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL.equals(url)) {
                    super.onPageStarted(url);
                    return;
                }
                Assert.assertEquals(
                        "onPageStarted called twice for " + url, false, mIsOnPageStartedCalled);
                mIsOnPageStartedCalled = true;
                Assert.assertEquals("onReceivedError called before onPageStarted for " + url, false,
                        mIsOnReceivedErrorCalled);
                super.onPageStarted(url);
            }

            void setAllowAboutBlank() {
                mAllowAboutBlank = true;
            }
        }

        LocalTestClient testContentsClient = new LocalTestClient();
        setTestAwContentsClient(testContentsClient);

        TestCallbackHelperContainer.OnReceivedErrorHelper onReceivedErrorHelper =
                mContentsClient.getOnReceivedErrorHelper();
        TestCallbackHelperContainer.OnPageStartedHelper onPageStartedHelper =
                mContentsClient.getOnPageStartedHelper();
        TestCallbackHelperContainer.OnPageFinishedHelper onPageFinishedHelper =
                mContentsClient.getOnPageFinishedHelper();

        String invalidUrl = "http://localhost:7/non_existent";
        mActivityTestRule.loadUrlSync(mAwContents, onPageFinishedHelper, invalidUrl);

        Assert.assertEquals(invalidUrl, onReceivedErrorHelper.getFailingUrl());
        Assert.assertEquals(invalidUrl, onPageStartedHelper.getUrl());

        // Rather than wait a fixed time to see that another onPageStarted callback isn't issued
        // we load a valid page. Since callbacks arrive sequentially, this will ensure that
        // any extra calls of onPageStarted / onReceivedError will arrive to our client.
        testContentsClient.setAllowAboutBlank();
        mActivityTestRule.loadUrlSync(
                mAwContents, onPageFinishedHelper, ContentUrlConstants.ABOUT_BLANK_DISPLAY_URL);
    }
}
