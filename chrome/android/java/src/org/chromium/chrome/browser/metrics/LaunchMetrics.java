// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.metrics;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.metrics.CachedMetrics;
import org.chromium.blink_public.platform.WebDisplayMode;
import org.chromium.content_public.browser.WebContents;

import java.util.ArrayList;
import java.util.List;

/**
 * Used for recording metrics about Chrome launches that need to be recorded before the native
 * library may have been loaded.  Metrics are cached until the library is known to be loaded, then
 * committed to the MetricsService all at once.
 */
@JNINamespace("metrics")
public class LaunchMetrics {
    private static class HomeScreenLaunch {
        public final String mUrl;
        public final boolean mIsShortcut;
        // Corresponds to C++ ShortcutInfo::Source
        public final int mSource;
        @WebDisplayMode
        public final int mDisplayMode;

        public HomeScreenLaunch(
                String url, boolean isShortcut, int source, @WebDisplayMode int displayMode) {
            mUrl = url;
            mIsShortcut = isShortcut;
            mSource = source;
            mDisplayMode = displayMode;
        }
    }

    private static final List<HomeScreenLaunch> sHomeScreenLaunches = new ArrayList<>();
    private static final List<Long> sWebappHistogramTimes = new ArrayList<>();

    /**
     * Records the launch of a standalone Activity for a URL (i.e. a WebappActivity)
     * added from a specific source.
     * @param url URL that kicked off the Activity's creation.
     * @param source integer id of the source from where the URL was added.
     * @param displayMode integer id of the {@link WebDisplayMode} of the web app.
     */
    public static void recordHomeScreenLaunchIntoStandaloneActivity(
            String url, int source, @WebDisplayMode int displayMode) {
        sHomeScreenLaunches.add(new HomeScreenLaunch(url, false, source, displayMode));
    }

    /**
     * Records the launch of a Tab for a URL (i.e. a Home screen shortcut).
     * @param url URL that kicked off the Tab's creation.
     * @param source integer id of the source from where the URL was added.
     */
    public static void recordHomeScreenLaunchIntoTab(String url, int source) {
        sHomeScreenLaunches.add(new HomeScreenLaunch(url, true, source, WebDisplayMode.UNDEFINED));
    }

    /**
     * Records the time it took to look up from disk whether a MAC is valid during webapp startup.
     * @param time the number of milliseconds it took to finish.
     */
    public static void recordWebappHistogramTimes(long time) {
        sWebappHistogramTimes.add(time);
    }

    /**
     * Calls out to native code to record URLs that have been launched via the Home screen.
     * This intermediate step is necessary because Activity.onCreate() may be called when
     * the native library has not yet been loaded.
     * @param webContents WebContents for the current Tab.
     */
    public static void commitLaunchMetrics(WebContents webContents) {
        for (HomeScreenLaunch launch : sHomeScreenLaunches) {
            nativeRecordLaunch(launch.mIsShortcut, launch.mUrl, launch.mSource, launch.mDisplayMode,
                    webContents);
        }
        sHomeScreenLaunches.clear();

        // Record generic cached events.
        CachedMetrics.commitCachedMetrics();
    }

    /**
     * Records metrics about the state of the homepage on launch.
     * @param showHomeButton Whether the home button is shown.
     * @param homepageIsNtp Whether the homepage is set to the NTP.
     * @param homepageUrl The value of the homepage URL.
     */
    public static void recordHomePageLaunchMetrics(
            boolean showHomeButton, boolean homepageIsNtp, String homepageUrl) {
        if (homepageUrl == null) {
            homepageUrl = "";
            assert !showHomeButton : "Homepage should be disabled for a null URL";
        }
        nativeRecordHomePageLaunchMetrics(showHomeButton, homepageIsNtp, homepageUrl);
    }

    private static native void nativeRecordLaunch(boolean isShortcut, String url, int source,
            @WebDisplayMode int displayMode, WebContents webContents);
    private static native void nativeRecordHomePageLaunchMetrics(
            boolean showHomeButton, boolean homepageIsNtp, String homepageUrl);
}
