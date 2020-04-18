// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.remote;
import static org.chromium.base.test.util.Restriction.RESTRICTION_TYPE_NON_LOW_END_DEVICE;
import static org.chromium.chrome.browser.media.remote.CastTestRule.DEFAULT_VIDEO;
import static org.chromium.chrome.browser.media.remote.CastTestRule.DEFAULT_VIDEO_PAGE;
import static org.chromium.chrome.browser.media.remote.CastTestRule.STABILIZE_TIME_MS;

import android.graphics.Rect;
import android.support.test.filters.LargeTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisableIf;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.FlakyTest;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.media.ui.MediaNotificationListener;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.media.MediaSwitches;

import java.util.concurrent.TimeoutException;

/**
 * Simple tests of casting videos. These tests all use the same page, containing the same non-YT
 * video.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        "disable-features=" + MediaSwitches.USE_MODERN_MEDIA_CONTROLS})
public class CastStartStopTest {
    @Rule
    public CastTestRule mCastTestRule = new CastTestRule();

    /*
     * Test that we can cast a video, and that we get the ExpandedControllerActivity when we do.
     */
    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE) // crbug.com/652872
    public void testCastingGenericVideo() throws InterruptedException, TimeoutException {
        mCastTestRule.castDefaultVideoFromPage(DEFAULT_VIDEO_PAGE);
        mCastTestRule.checkVideoStarted(DEFAULT_VIDEO);
    }

    /*
     * Test that we can disconnect a cast session from the expanded controller activity overlay.
     */
    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE) // crbug.com/652872
    public void testStopFromVideoControls() throws InterruptedException, TimeoutException {
        Rect videoRect = mCastTestRule.castDefaultVideoFromPage(DEFAULT_VIDEO_PAGE);

        final Tab tab = mCastTestRule.getActivity().getActivityTab();

        mCastTestRule.clickDisconnectFromRoute(tab, videoRect);

        mCastTestRule.checkDisconnected();
    }

    /*
     * Test that we can stop a cast session from the notification.
     */
    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE) // crbug.com/652872
    public void testStopFromNotification() throws InterruptedException, TimeoutException {
        mCastTestRule.castDefaultVideoFromPage(DEFAULT_VIDEO_PAGE);

        // Get the notification
        final CastNotificationControl notificationControl = mCastTestRule.waitForCastNotification();

        // Send play
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                notificationControl.onStop(MediaNotificationListener.ACTION_SOURCE_MEDIA_SESSION);
            }
        });
        mCastTestRule.checkDisconnected();
    }

    /*
     * Test that a cast session disconnects when the video ends
     */
    @Test
    @DisableIf.Build(sdk_is_less_than = 19, message = "crbug.com/582067")
    @Feature({"VideoFling"})
    @LargeTest
    @FlakyTest
    public void testStopWhenVideoEnds() throws InterruptedException, TimeoutException {
        mCastTestRule.castDefaultVideoFromPage(DEFAULT_VIDEO_PAGE);
        // Wait for the video to finish (this assumes the video is short, the test video
        // is 8 seconds).
        mCastTestRule.sleepNoThrow(STABILIZE_TIME_MS);

        Thread.sleep(mCastTestRule.getRemoteDurationMs());

        // Everything should now have disconnected
        mCastTestRule.checkDisconnected();
    }
}
