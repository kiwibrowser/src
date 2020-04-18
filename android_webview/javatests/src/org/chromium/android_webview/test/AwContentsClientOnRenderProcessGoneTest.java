// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.AwRenderProcessGoneDetail;
import org.chromium.android_webview.AwSwitches;
import org.chromium.android_webview.renderer_priority.RendererPriority;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.parameter.SkipCommandLineParameterization;

import java.util.concurrent.TimeUnit;

/**
 * Tests for AwContentsClient.onRenderProcessGone callback.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class AwContentsClientOnRenderProcessGoneTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private static final String TAG = "AwRendererGone";
    private static class GetRenderProcessGoneHelper extends CallbackHelper {
        private AwRenderProcessGoneDetail mDetail;

        public AwRenderProcessGoneDetail getAwRenderProcessGoneDetail() {
            assert getCallCount() > 0;
            return mDetail;
        }

        public void notifyCalled(AwRenderProcessGoneDetail detail) {
            mDetail = detail;
            notifyCalled();
        }
    }

    private static class RenderProcessGoneTestAwContentsClient extends TestAwContentsClient {

        private GetRenderProcessGoneHelper mGetRenderProcessGoneHelper;

        public RenderProcessGoneTestAwContentsClient() {
            mGetRenderProcessGoneHelper = new GetRenderProcessGoneHelper();
        }

        public GetRenderProcessGoneHelper getGetRenderProcessGoneHelper() {
            return mGetRenderProcessGoneHelper;
        }

        @Override
        public boolean onRenderProcessGone(AwRenderProcessGoneDetail detail) {
            mGetRenderProcessGoneHelper.notifyCalled(detail);
            return true;
        }
    }

    @Test
    @DisabledTest // http://crbug.com/689292
    @Feature({"AndroidWebView"})
    @SmallTest
    @CommandLineFlags.Add(AwSwitches.WEBVIEW_SANDBOXED_RENDERER)
    @SkipCommandLineParameterization
    public void testOnRenderProcessCrash() throws Throwable {
        RenderProcessGoneTestAwContentsClient contentsClient =
                new RenderProcessGoneTestAwContentsClient();
        AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(contentsClient);
        AwContents awContents = testView.getAwContents();
        GetRenderProcessGoneHelper helper = contentsClient.getGetRenderProcessGoneHelper();
        mActivityTestRule.loadUrlAsync(awContents, "chrome://crash");
        int callCount = helper.getCallCount();
        helper.waitForCallback(callCount, 1, CallbackHelper.WAIT_TIMEOUT_SECONDS * 5,
                TimeUnit.SECONDS);
        Assert.assertEquals(callCount + 1, helper.getCallCount());
        Assert.assertTrue(helper.getAwRenderProcessGoneDetail().didCrash());
        Assert.assertEquals(
                RendererPriority.HIGH, helper.getAwRenderProcessGoneDetail().rendererPriority());
    }

    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    @CommandLineFlags.Add(AwSwitches.WEBVIEW_SANDBOXED_RENDERER)
    @SkipCommandLineParameterization
    public void testOnRenderProcessKill() throws Throwable {
        RenderProcessGoneTestAwContentsClient contentsClient =
                new RenderProcessGoneTestAwContentsClient();
        AwTestContainerView testView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(contentsClient);
        AwContents awContents = testView.getAwContents();
        GetRenderProcessGoneHelper helper = contentsClient.getGetRenderProcessGoneHelper();
        mActivityTestRule.loadUrlAsync(awContents, "chrome://kill");
        int callCount = helper.getCallCount();
        helper.waitForCallback(callCount);

        Assert.assertEquals(callCount + 1, helper.getCallCount());
        Assert.assertFalse(helper.getAwRenderProcessGoneDetail().didCrash());
        Assert.assertEquals(
                RendererPriority.HIGH, helper.getAwRenderProcessGoneDetail().rendererPriority());
    }
}
