// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import static android.view.View.SYSTEM_UI_FLAG_LOW_PROFILE;

import android.app.Activity;
import android.app.Instrumentation;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.view.View;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Callback;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.fullscreen.FullscreenOptions;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content_public.browser.WebContents;
import org.chromium.media.MediaSwitches;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.concurrent.TimeoutException;

/**
 * Tests for FullscreenActivity.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.
Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE, MediaSwitches.AUTOPLAY_NO_GESTURE_REQUIRED_POLICY,
        "enable-features=" + ChromeFeatureList.FULLSCREEN_ACTIVITY})
public class FullscreenActivityTest {
    private static final String TEST_PATH = "/content/test/data/media/video-player.html";
    private static final String VIDEO_ID = "video";

    @Rule
    public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeTabbedActivity.class);

    private EmbeddedTestServer mTestServer;
    private ChromeTabbedActivity mActivity;

    @Before
    public void setUp() throws InterruptedException {
        mTestServer = EmbeddedTestServer.createAndStartServer(
                InstrumentationRegistry.getInstrumentation().getContext());
        mActivityTestRule.startMainActivityWithURL(mTestServer.getURL(TEST_PATH));
        mActivity = mActivityTestRule.getActivity();
    }

    @After
    public void tearDown() {
        mTestServer.stopAndDestroyServer();
    }

    /**
     * Tests that the window system visibility has flags set that indicate it is fullscreen. This is
     * useful because there is a slight discrepancy between when the webpage thinks it is fullscreen
     * and when the window thinks it is fullscreen that can lead to a race condition during tests.
     */
    private static boolean hasFullscreenFlags(View view) {
        return (view.getWindowSystemUiVisibility() & SYSTEM_UI_FLAG_LOW_PROFILE)
                == SYSTEM_UI_FLAG_LOW_PROFILE;
    }

    /*
     * Clicks on the fullscreen button in the test page, waits for the FullscreenActivity
     * to be started and for it to go fullscreen.
     */
    private FullscreenActivity enterFullscreen() throws Throwable {
        // Start playback to guarantee it's properly loaded.
        WebContents webContents = mActivity.getCurrentWebContents();
        Assert.assertTrue(DOMUtils.isMediaPaused(webContents, VIDEO_ID));
        DOMUtils.playMedia(webContents, VIDEO_ID);
        DOMUtils.waitForMediaPlay(webContents, VIDEO_ID);

        // Trigger requestFullscreen() via a click on a button.
        Assert.assertTrue(DOMUtils.clickNode(webContents, "fullscreen"));

        final FullscreenActivity fullscreenActivity =
                ChromeActivityTestRule.waitFor(FullscreenActivity.class);

        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                try {
                    Tab tab = fullscreenActivity.getActivityTab();
                    return DOMUtils.isFullscreen(tab.getWebContents())
                            && hasFullscreenFlags(tab.getContentView());
                } catch (InterruptedException | TimeoutException e) {
                    throw new RuntimeException(e);
                }
            }
        });

        return fullscreenActivity;
    }

    /**
     * Enters then exits fullscreen, ensuring that FullscreenActivity is started, that
     * the original ChromeTabbedActivity is brough back to the foreground and that the Tab remains
     * active throughout.
     * @param exit A Callback to exit fullscreen.
     */
    private void testFullscreenAndExit(Callback<ChromeActivity> exit) throws Throwable {
        Activity original = mActivity;
        Tab tab = mActivity.getActivityTab();

        FullscreenActivity fullscreenActivity = enterFullscreen();
        Assert.assertSame(tab, fullscreenActivity.getActivityTab());

        exit.onResult(fullscreenActivity);

        ChromeTabbedActivity activity = ChromeActivityTestRule.waitFor(ChromeTabbedActivity.class);

        // Ensure we haven't started a new ChromeTabbedActivity, https://crbug.com/729805,
        // https://crbug.com/729932.
        Assert.assertSame(original, activity);
        Assert.assertSame(tab, mActivity.getActivityTab());
    }

    /**
     * Toggles fullscreen to check FullscreenActivity has been started.
     */
    @Test
    @MediumTest
    public void testFullscreen() throws Throwable {
        testFullscreenAndExit(
                activity -> DOMUtils.exitFullscreen(activity.getCurrentWebContents()));
    }

    /**
     * Enters fullscreen then presses the back button to exit.
     */
    @Test
    @MediumTest
    public void testExitOnBack() throws Throwable {
        testFullscreenAndExit(activity -> {
            try {
                ThreadUtils.runOnUiThreadBlocking(() -> activity.onBackPressed());
            } catch (Throwable t) {
                throw new RuntimeException(t);
            }
        });
    }

    /**
     * Tests that no flags change on the ChromeTabbedActivity when going fullscreen.
     */
    @Test
    @MediumTest
    public void testFullscreenFlags() throws Throwable {
        int old = mActivity.getTabsView().getSystemUiVisibility();

        FullscreenActivity fullscreenActivity = enterFullscreen();
        DOMUtils.exitFullscreen(fullscreenActivity.getCurrentWebContents());

        ChromeActivityTestRule.waitFor(ChromeTabbedActivity.class);

        Assert.assertEquals(old, mActivity.getTabsView().getSystemUiVisibility());
    }

    /**
     * When a FullscreenActivity goes to the background it exits fullscreen if the video is paused.
     * In this case we want to exit fullscreen normally, not through Intenting back to the CTA,
     * since this will appear to relaunch Chrome.
     */
    @Test
    @MediumTest
    public void testNoIntentWhenInBackground() throws Throwable {
        final Boolean[] isTabFullscreen = new Boolean[1];
        Tab tab = mActivity.getActivityTab();
        tab.addObserver(new EmptyTabObserver() {
            @Override
            public void onEnterFullscreenMode(Tab tab, FullscreenOptions options) {
                isTabFullscreen[0] = true;
            }
            @Override
            public void onExitFullscreenMode(Tab tab) {
                isTabFullscreen[0] = false;
            }
        });

        enterFullscreen();
        Assert.assertTrue(isTabFullscreen[0]);
        DOMUtils.pauseMedia(tab.getWebContents(), VIDEO_ID);

        // Add a monitor to track any intents launched to a ChromeTabbedActivity.
        Instrumentation.ActivityMonitor monitor = new Instrumentation.ActivityMonitor(
                ChromeTabbedActivity.class.getName(), null, true);
        InstrumentationRegistry.getInstrumentation().addMonitor(monitor);

        Assert.assertEquals(0, monitor.getHits());

        // Move Chrome to the background.
        ThreadUtils.runOnUiThreadBlocking(() -> mActivity.moveTaskToBack(true));

        // Wait for the Tab to leave fullscreen.
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return !isTabFullscreen[0];
            }
        });

        Assert.assertEquals(0, monitor.getHits());
    }
}
