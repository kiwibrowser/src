// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.base;

import android.content.Context;
import android.os.Build;
import android.support.annotation.UiThread;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.ui.R;
import org.chromium.ui.display.DisplayAndroid;
import org.chromium.ui.display.DisplayUtil;

/**
 * UI utilities for accessing form factor information.
 */
public class DeviceFormFactor {
    /**
     * Miniumum screen size in dp to be considered a tablet. Matches the value
     * used by res/ directories. E.g.: res/values-sw600dp/values.xml
     */
    public static final int MINIMUM_TABLET_WIDTH_DP = 600;

    /**
     * Matches the value set in res/values-sw600dp/values.xml
     */
    private static final int SCREEN_BUCKET_TABLET = 2;

    /**
     * Matches the value set in res/values-sw720dp/values.xml
     */
    private static final int SCREEN_BUCKET_LARGET_TABLET = 3;

    /**
     * Each activity could be on a different display, and this will just tell you whether the
     * display associated with the application context is "tablet sized".
     * Use {@link #isNonMultiDisplayContextOnTablet} or {@link #isWindowOnTablet} instead.
     */
    @CalledByNative
    @Deprecated
    public static boolean isTablet() {
        return detectScreenWidthBucket(ContextUtils.getApplicationContext())
                >= SCREEN_BUCKET_TABLET;
    }

    /**
     * See {@link DisplayAndroid#getNonMultiDisplay}} for what "NonMultiDisplay" means.
     * When possible, it is generally more correct to use {@link #isWindowOnTablet}.
     * Only Activity instances and Contexts that wrap Activities are meaningfully associated with
     * displays, so care should be taken to pass a context that makes sense.
     *
     * @return Whether the display associated with the given context is large enough to be
     *         considered a tablet and will thus load tablet-specific resources (those in the config
     *         -sw600).
     *         Not affected by Android N multi-window, but can change for external displays.
     *         E.g. http://developer.samsung.com/samsung-dex/testing
     */
    public static boolean isNonMultiDisplayContextOnTablet(Context context) {
        return detectScreenWidthBucket(context) >= SCREEN_BUCKET_TABLET;
    }

    /**
     * @return Whether the display associated with the window is large enough to be
     *         considered a tablet and will thus load tablet-specific resources (those in the config
     *         -sw600).
     *         Not affected by Android N multi-window, but can change for external displays.
     *         E.g. http://developer.samsung.com/samsung-dex/testing
     */
    @UiThread
    public static boolean isWindowOnTablet(WindowAndroid windowAndroid) {
        return detectScreenWidthBucket(windowAndroid) >= SCREEN_BUCKET_TABLET;
    }

    /**
     * @return Whether the display associated with the given context is large enough to be
     *         considered a large tablet and will thus load large-tablet-specific resources (those
     *         in the config -sw720).
     *         Not affected by Android N multi-window, but can change for external displays.
     *         E.g. http://developer.samsung.com/samsung-dex/testing
     */
    public static boolean isNonMultiDisplayContextOnLargeTablet(Context context) {
        return detectScreenWidthBucket(context) == SCREEN_BUCKET_LARGET_TABLET;
    }

    private static int detectScreenWidthBucket(Context context) {
        // Pre-JB MR1, Display.getSize() is used rather than Display.getRealSize().
        // For our query, getSize() is not always correct. https://crbug.com/829318
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1
                // TODO(agrieve): Remove thread check and audit for background usages.
                //     https://crbug.com/669974
                && ThreadUtils.runningOnUiThread()
                && !isTabletDisplay(DisplayAndroid.getNonMultiDisplay(context))) {
            // There have been no cases where tablet resources end up being used on phone-sized
            // displays. Short-circuit this common-case since checking resources is slower (and
            // triggers a strict-mode violation when value is not cached).
            return 0;
        }
        return context.getResources().getInteger(R.integer.min_screen_width_bucket);
    }

    private static int detectScreenWidthBucket(WindowAndroid windowAndroid) {
        ThreadUtils.assertOnUiThread();
        Context context = windowAndroid.getContext().get();
        if (context == null || !isTabletDisplay(windowAndroid.getDisplay())) {
            return 0;
        }
        return context.getResources().getInteger(R.integer.min_screen_width_bucket);
    }

    /**
     * @return The minimum width in px at which the display should be treated like a tablet for
     *         layout.
     */
    @UiThread
    public static int getNonMultiDisplayMinimumTabletWidthPx(Context context) {
        return getMinimumTabletWidthPx(DisplayAndroid.getNonMultiDisplay(context));
    }

    /**
     * @return The minimum width in px at which the display should be treated like a tablet for
     *         layout.
     */
    public static int getMinimumTabletWidthPx(DisplayAndroid display) {
        return DisplayUtil.dpToPx(display, DeviceFormFactor.MINIMUM_TABLET_WIDTH_DP);
    }

    // Function is private to ensure that Context is also consulted when answering this query.
    private static boolean isTabletDisplay(DisplayAndroid display) {
        return DisplayUtil.getSmallestWidth(display) >= getMinimumTabletWidthPx(display);
    }
}
