// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import static org.chromium.android_webview.test.AwActivityTestRule.WAIT_TIMEOUT_MS;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.view.View;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwContents;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.media.MediaSwitches;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Test for AwContentClient.GetVideoLoadingProgressView.
 *
 * This test takes advantage of onViewAttachedToWindow, and assume our progress view
 * is shown once it attached to window.
 *
 * As it needs user gesture to switch to the full screen mode video, A large button
 * used to trigger switch occupies almost the whole WebView so the simulated click event
 * can't miss it.
 */
@RunWith(AwJUnit4ClassRunner.class)
@CommandLineFlags.Add({"disable-features=" + MediaSwitches.USE_MODERN_MEDIA_CONTROLS})
public class AwContentsClientGetVideoLoadingProgressViewTest
        implements View.OnAttachStateChangeListener {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private static final String VIDEO_TEST_URL =
            "file:///android_asset/full_screen_video_test_not_preloaded.html";
    // These values must be kept in sync with the element ids in the above file.
    private static final String CUSTOM_PLAY_CONTROL_ID = "playControl";
    private static final String CUSTOM_FULLSCREEN_CONTROL_ID = "fullscreenControl";

    private CallbackHelper mViewAttachedCallbackHelper = new CallbackHelper();

    @Override
    public void onViewAttachedToWindow(View view) {
        mViewAttachedCallbackHelper.notifyCalled();
        view.removeOnAttachStateChangeListener(this);
    }

    @Override
    public void onViewDetachedFromWindow(View arg0) {
    }

    private void waitForViewAttached() throws InterruptedException, TimeoutException {
        mViewAttachedCallbackHelper.waitForCallback(0, 1, WAIT_TIMEOUT_MS,
                TimeUnit.MILLISECONDS);
    }

    @Test
    @Feature({"AndroidWebView"})
    @SmallTest
    public void testGetVideoLoadingProgressView() throws Throwable {
        TestAwContentsClient contentsClient =
                new FullScreenVideoTestAwContentsClient(mActivityTestRule.getActivity(),
                        mActivityTestRule.isHardwareAcceleratedTest()) {
                    @Override
                    protected View getVideoLoadingProgressView() {
                        View view = new View(
                                InstrumentationRegistry.getInstrumentation().getTargetContext());
                        view.addOnAttachStateChangeListener(
                                AwContentsClientGetVideoLoadingProgressViewTest.this);
                        return view;
                    }
                };
        final AwTestContainerView testContainerView =
                mActivityTestRule.createAwTestContainerViewOnMainSync(contentsClient);
        final AwContents awContents = testContainerView.getAwContents();
        awContents.getSettings().setFullscreenSupported(true);
        AwActivityTestRule.enableJavaScriptOnUiThread(awContents);
        mActivityTestRule.loadUrlSync(
                awContents, contentsClient.getOnPageFinishedHelper(), VIDEO_TEST_URL);
        Thread.sleep(5 * 1000);
        DOMUtils.clickNode(awContents.getWebContents(), CUSTOM_FULLSCREEN_CONTROL_ID);
        Thread.sleep(1 * 1000);
        DOMUtils.clickNode(awContents.getWebContents(), CUSTOM_PLAY_CONTROL_ID);
        waitForViewAttached();
    }
}
