// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.remote;
import static org.chromium.chrome.browser.media.remote.CastTestRule.CAST_TEST_ROUTE;
import static org.chromium.chrome.browser.media.remote.CastTestRule.DEFAULT_VIDEO;
import static org.chromium.chrome.browser.media.remote.CastTestRule.DEFAULT_VIDEO_PAGE;
import static org.chromium.chrome.browser.media.remote.CastTestRule.TEST_VIDEO_2;
import static org.chromium.chrome.browser.media.remote.CastTestRule.TEST_VIDEO_PAGE_2;
import static org.chromium.chrome.browser.media.remote.CastTestRule.TWO_VIDEO_PAGE;
import static org.chromium.chrome.browser.media.remote.CastTestRule.VIDEO_ELEMENT;

import android.graphics.Rect;
import android.support.test.filters.LargeTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.WebContents;
import org.chromium.media.MediaSwitches;

import java.util.concurrent.TimeoutException;

/**
 * Test that other videos are played locally when casting
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        "disable-features=" + MediaSwitches.USE_MODERN_MEDIA_CONTROLS})
public class CastSwitchVideoTest {
    @Rule
    public CastTestRule mCastTestRule = new CastTestRule();

    private static final String VIDEO_ELEMENT_2 = "video2";

    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure // crbug.com/623526
    public void testPlayNewVideoInNewTab() throws InterruptedException, TimeoutException {
        checkPlaySecondVideo(DEFAULT_VIDEO_PAGE, VIDEO_ELEMENT, new Runnable() {
            @Override
            public void run() {
                try {
                    mCastTestRule.loadUrlInNewTab(
                            mCastTestRule.getTestServer().getURL(TEST_VIDEO_PAGE_2));
                    playVideoFromCurrentTab(VIDEO_ELEMENT);
                } catch (Exception e) {
                    Assert.fail("Failed to start second video; " + e.getMessage());
                }
            }
        });
    }

    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure // crbug.com/623526
    public void testPlayNewVideoNewPageSameTab() throws InterruptedException, TimeoutException {
        checkPlaySecondVideo(DEFAULT_VIDEO_PAGE, VIDEO_ELEMENT, new Runnable() {
            @Override
            public void run() {
                try {
                    mCastTestRule.loadUrl(mCastTestRule.getTestServer().getURL(TEST_VIDEO_PAGE_2));
                    playVideoFromCurrentTab(VIDEO_ELEMENT);
                } catch (Exception e) {
                    Assert.fail("Failed to start second video; " + e.getMessage());
                }
            }
        });
    }

    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure // crbug.com/623526
    public void testPlayTwoVideosSamePage() throws InterruptedException, TimeoutException {
        checkPlaySecondVideo(TWO_VIDEO_PAGE, VIDEO_ELEMENT_2, new Runnable() {
            @Override
            public void run() {
                try {
                    playVideoFromCurrentTab(VIDEO_ELEMENT_2);
                } catch (Exception e) {
                    Assert.fail("Failed to start second video; " + e.getMessage());
                }
            }
        });
    }

    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure // crbug.com/623526
    public void testCastNewVideoInNewTab() throws InterruptedException, TimeoutException {
        checkCastSecondVideo(DEFAULT_VIDEO_PAGE, new Runnable() {
            @Override
            public void run() {
                try {
                    mCastTestRule.loadUrlInNewTab(
                            mCastTestRule.getTestServer().getURL(TEST_VIDEO_PAGE_2));
                    castVideoFromCurrentTab(VIDEO_ELEMENT);
                } catch (Exception e) {
                    Assert.fail("Failed to start second video; " + e.getMessage());
                }
            }
        });
    }

    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure // crbug.com/623526
    public void testCastNewVideoNewPageSameTab() throws InterruptedException, TimeoutException {
        checkCastSecondVideo(DEFAULT_VIDEO_PAGE, new Runnable() {
            @Override
            public void run() {
                try {
                    mCastTestRule.loadUrl(mCastTestRule.getTestServer().getURL(TEST_VIDEO_PAGE_2));
                    castVideoFromCurrentTab(VIDEO_ELEMENT);
                } catch (Exception e) {
                    Assert.fail("Failed to start second video; " + e.getMessage());
                }
            }
        });
    }

    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure // crbug.com/623526
    public void testCastTwoVideosSamePage() throws InterruptedException, TimeoutException {
        checkCastSecondVideo(TWO_VIDEO_PAGE, new Runnable() {
            @Override
            public void run() {
                try {
                    castVideoFromCurrentTab(VIDEO_ELEMENT_2);
                } catch (Exception e) {
                    Assert.fail("Failed to start second video; " + e.getMessage());
                }
            }
        });
    }

    private void checkPlaySecondVideo(
            String firstVideoPage, String secondVideoId, final Runnable startSecondVideo)
                    throws InterruptedException, TimeoutException {
        // TODO(aberent) Checking position is flaky, because it is timing dependent, but probably
        // a good idea in principle. Need to find a way of unflaking it.
        // int position = castAndPauseDefaultVideoFromPage(firstVideoPage);
        mCastTestRule.castAndPauseDefaultVideoFromPage(firstVideoPage);

        startSecondVideo.run();

        // Check that we are still casting the default video
        Assert.assertEquals("The first video is not casting",
                mCastTestRule.getTestServer().getURL(DEFAULT_VIDEO), mCastTestRule.getUriPlaying());

        // Check that the second video is still there and paused
        final Tab tab = mCastTestRule.getActivity().getActivityTab();
        WebContents webContents = tab.getWebContents();
        Assert.assertFalse(
                "Other video is not playing", DOMUtils.isMediaPaused(webContents, secondVideoId));
    }

    private void checkCastSecondVideo(String firstVideoPage,  final Runnable startSecondVideo)
            throws InterruptedException, TimeoutException {
        // TODO(aberent) Checking position is flaky, because it is timing dependent, but probably
        // a good idea in principle. Need to find a way of unflaking it.
        // int position = castAndPauseDefaultVideoFromPage(firstVideoPage);
        mCastTestRule.castAndPauseDefaultVideoFromPage(firstVideoPage);

        startSecondVideo.run();

        // Check that we switch to playing the right video
        mCastTestRule.checkVideoStarted(TEST_VIDEO_2);
    }

    private void castVideoFromCurrentTab(String videoElement) throws InterruptedException,
            TimeoutException {
        final Tab tab = mCastTestRule.getActivity().getActivityTab();
        WebContents webContents = tab.getWebContents();
        mCastTestRule.waitUntilVideoReady(videoElement, webContents);
        Rect videoRect = DOMUtils.getNodeBounds(webContents, videoElement);

        mCastTestRule.castVideoAndWaitUntilPlaying(CAST_TEST_ROUTE, tab, videoRect);
    }

    private void playVideoFromCurrentTab(String videoElement) throws InterruptedException,
            TimeoutException {
        WebContents webContents = mCastTestRule.getWebContents();

        mCastTestRule.waitUntilVideoReady(videoElement, webContents);

        // Need to click on the video first to overcome the user gesture requirement.
        DOMUtils.clickNode(webContents, videoElement);
        DOMUtils.playMedia(webContents, videoElement);
        DOMUtils.waitForMediaPlay(webContents, videoElement);
    }

}
