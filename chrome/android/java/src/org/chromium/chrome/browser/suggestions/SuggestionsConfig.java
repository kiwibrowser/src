// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.res.Resources;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.util.AccessibilityUtil;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

/**
 * Provides configuration details for suggestions.
 */
public final class SuggestionsConfig {
    private SuggestionsConfig() {}

    /**
     * @return Whether scrolling to the bottom of suggestions triggers a load.
     */
    public static boolean scrollToLoad() {
        // The scroll to load feature does not work well for users who require accessibility mode.
        if (AccessibilityUtil.isAccessibilityEnabled()) return false;

        return ChromeFeatureList.isEnabled(ChromeFeatureList.SIMPLIFIED_NTP)
                && ChromeFeatureList.isEnabled(
                           ChromeFeatureList.CONTENT_SUGGESTIONS_SCROLL_TO_LOAD);
    }

    /**
     * @param resources The resources to fetch the color from.
     * @return The background color for the suggestions sheet content.
     */
    public static int getBackgroundColor(Resources resources) {
        return useModernLayout()
                ? ApiCompatibilityUtils.getColor(resources, R.color.suggestions_modern_bg)
                : ApiCompatibilityUtils.getColor(resources, R.color.ntp_bg);
    }

    /**
     * Returns the current tile style, that depends on the enabled features and the screen size.
     */
    @TileView.Style
    public static int getTileStyle(UiConfig uiConfig) {
        boolean small = uiConfig.getCurrentDisplayStyle().isSmall();
        if (useModernLayout()) {
            return small ? TileView.Style.MODERN_CONDENSED : TileView.Style.MODERN;
        }
        if (useCondensedTileLayout(small)) return TileView.Style.CLASSIC_CONDENSED;
        return TileView.Style.CLASSIC;
    }

    private static boolean useCondensedTileLayout(boolean isScreenSmall) {
        if (isScreenSmall) return true;

        return false;
    }

    /**
     * @return Whether the modern layout should be used for suggestions.
     */
    public static boolean useModernLayout() {
        return FeatureUtilities.isChromeModernDesignEnabled()
                || ChromeFeatureList.isEnabled(ChromeFeatureList.NTP_MODERN_LAYOUT);
    }
}
