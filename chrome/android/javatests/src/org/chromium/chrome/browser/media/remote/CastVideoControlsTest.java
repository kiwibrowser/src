// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.remote;
import static org.chromium.base.test.util.Restriction.RESTRICTION_TYPE_NON_LOW_END_DEVICE;
import static org.chromium.chrome.browser.media.remote.CastTestRule.DEFAULT_VIDEO_PAGE;
import static org.chromium.chrome.browser.media.remote.CastTestRule.MAX_VIEW_TIME_MS;
import static org.chromium.chrome.browser.media.remote.CastTestRule.VIEW_RETRY_MS;

import android.graphics.Rect;
import android.support.test.filters.LargeTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.media.MediaSwitches;

import java.util.concurrent.TimeoutException;

/**
 * Instrumentation tests for the fullscreen cast controls.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE,
        "disable-features=" + MediaSwitches.USE_MODERN_MEDIA_CONTROLS})
public class CastVideoControlsTest {
    @Rule
    public CastTestRule mCastTestRule = new CastTestRule();

    private static final long PAUSE_TEST_TIME_MS = 1000;

    /*
     * Test the pause button.
     */
    @Test
    @Feature({"VideoFling"})
    @LargeTest
    @RetryOnFailure
    @Restriction(RESTRICTION_TYPE_NON_LOW_END_DEVICE) // crbug.com/652872
    public void testPauseButton() throws InterruptedException, TimeoutException {
        Rect videoRect = mCastTestRule.castDefaultVideoFromPage(DEFAULT_VIDEO_PAGE);

        final Tab tab = mCastTestRule.getActivity().getActivityTab();

        mCastTestRule.tapPlayPauseButton(tab, videoRect);
        // The new position is sent in a separate message, so we have to wait a bit before
        // fetching it.
        long position = mCastTestRule.getRemotePositionMs();
        boolean paused = false;
        for (int time = 0; time < MAX_VIEW_TIME_MS; time += VIEW_RETRY_MS) {
            Thread.sleep(VIEW_RETRY_MS);
            long newPosition = mCastTestRule.getRemotePositionMs();
            if (newPosition == position) {
                paused = true;
                break;
            }
            position = newPosition;
        }
        // Check we have paused before the end of the video (with a fudge factor for timing
        // variation)
        Assert.assertTrue("Pause didn't stop playback",
                paused || position < mCastTestRule.getRemoteDurationMs() - 100);
        mCastTestRule.tapPlayPauseButton(tab, videoRect);
        Thread.sleep(PAUSE_TEST_TIME_MS);
        Assert.assertTrue(
                "Run didn't restart playback", position < mCastTestRule.getRemotePositionMs());
    }
}
