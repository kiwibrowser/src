// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.view.View;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.textbubble.TextBubble;
import org.chromium.ui.widget.ViewRectProvider;

/**
 * Draws a bubble pointing upward at the tab switcher button.
 * TODO(dtrainor, nyquist): Migrate this to an IPH message if it doesn't go away.
 */
public class TabSwitcherCallout {
    public static final String PREF_NEED_TO_SHOW_TAB_SWITCHER_CALLOUT =
            "org.chromium.chrome.browser.toolbar.NEED_TO_SHOW_TAB_SWITCHER_CALLOUT";

    private static final int TAB_SWITCHER_CALLOUT_DISMISS_MS = 10000;
    private static final float Y_OVERLAP_DP = 18.f;

    /**
     * Show the TabSwitcherCallout, if necessary.
     * @param context           {@link Context} to draw resources from.
     * @param tabSwitcherButton Button that triggers the tab switcher.
     * @return                  {@link TextBubble} if one was shown, {@code null} otherwise.
     */
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static TextBubble showIfNecessary(Context context, View tabSwitcherButton) {
        if (!isTabSwitcherCalloutNecessary()) return null;
        setIsTabSwitcherCalloutNecessary(false);

        ViewRectProvider rectProvider = new ViewRectProvider(tabSwitcherButton);
        int yInsetPx = (int) (Y_OVERLAP_DP * context.getResources().getDisplayMetrics().density);
        rectProvider.setInsetPx(0, yInsetPx, 0, yInsetPx);

        TextBubble bubble =
                new TextBubble(context, tabSwitcherButton, R.string.tab_switcher_callout_body,
                        R.string.tab_switcher_callout_body, rectProvider);
        bubble.setDismissOnTouchInteraction(true);
        bubble.setAutoDismissTimeout(TAB_SWITCHER_CALLOUT_DISMISS_MS);
        bubble.show();
        return bubble;
    }

    /** @return Whether or not the tab switcher button callout needs to be shown. */
    public static boolean isTabSwitcherCalloutNecessary() {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        return prefs.getBoolean(PREF_NEED_TO_SHOW_TAB_SWITCHER_CALLOUT, false);
    }

    /** Sets whether the tab switcher callout should be shown when the browser starts up. */
    public static void setIsTabSwitcherCalloutNecessary(boolean shouldShow) {
        SharedPreferences prefs = ContextUtils.getAppSharedPreferences();
        prefs.edit().putBoolean(PREF_NEED_TO_SHOW_TAB_SWITCHER_CALLOUT, shouldShow).apply();
    }
}
