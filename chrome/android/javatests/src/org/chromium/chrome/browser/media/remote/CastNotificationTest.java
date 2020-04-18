// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.remote;
import static org.chromium.base.test.util.Restriction.RESTRICTION_TYPE_NON_LOW_END_DEVICE;
import static org.chromium.chrome.browser.media.remote.CastTestRule.DEFAULT_VIDEO_PAGE;
import static org.chromium.chrome.browser.media.remote.CastTestRule.MAX_VIEW_TIME_MS;
import static org.chromium.chrome.browser.media.remote.CastTestRule.STABILIZE_TIME_MS;

import android.support.test.filters.LargeTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.media.remote.RemoteVideoInfo.PlayerState;
import org.chromium.chrome.browser.media.ui.MediaNotificationListener;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.media.MediaSwitches;

import java.util.concurrent.TimeoutException;

/**
 * Tests of the notification.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        "disable-features=" + MediaSwitches.USE_MODERN_MEDIA_CONTROLS})
public class CastNotificationTest {
    @Rule
    public CastTestRule mCastTestRule = new CastTestRule();

    private static final long PAUSE_TEST_TIME_MS = 1000;

    /**
     * Test the pause button on the notification.
     */
    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE) // crbug.com/652872
    public void testNotificationPause() throws InterruptedException, TimeoutException {
        mCastTestRule.castDefaultVideoFromPage(DEFAULT_VIDEO_PAGE);

        // Get the notification
        final CastNotificationControl notificationControl = mCastTestRule.waitForCastNotification();
        Assert.assertNotNull("No notificationTransportControl", notificationControl);
        // Send pause
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                notificationControl.onPause(MediaNotificationListener.ACTION_SOURCE_MEDIA_SESSION);
            }
        });
        Assert.assertTrue(
                "Not paused", mCastTestRule.waitForState(PlayerState.PAUSED, MAX_VIEW_TIME_MS));

        // The new position is sent in a separate message, so we have to wait a bit before
        // fetching it.
        Thread.sleep(STABILIZE_TIME_MS);
        long position = mCastTestRule.getRemotePositionMs();
        // Position should not change while paused
        Thread.sleep(PAUSE_TEST_TIME_MS);
        Assert.assertEquals(
                "Pause didn't stop playback", position, mCastTestRule.getRemotePositionMs());
        // Send play
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                notificationControl.onPlay(MediaNotificationListener.ACTION_SOURCE_MEDIA_SESSION);
            }
        });
        Assert.assertTrue(
                "Not playing", mCastTestRule.waitForState(PlayerState.PLAYING, MAX_VIEW_TIME_MS));

        // Should now be running again.
        Thread.sleep(PAUSE_TEST_TIME_MS);
        Assert.assertTrue(
                "Run didn't restart playback", position < mCastTestRule.getRemotePositionMs());
    }
}
