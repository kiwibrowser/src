// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.text.TextUtils;
import android.view.View;

import org.chromium.base.Callback;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.appmenu.AppMenuHandler;
import org.chromium.chrome.browser.feature_engagement.TrackerFactory;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.chrome.browser.widget.ViewHighlighter;
import org.chromium.chrome.browser.widget.textbubble.TextBubble;
import org.chromium.components.feature_engagement.FeatureConstants;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.ui.widget.ViewRectProvider;

/**
 * A helper class for IPH shown on the toolbar.
 */
public class ToolbarButtonInProductHelpController {
    private ToolbarButtonInProductHelpController() {}

    /**
     * Attempts to show an IPH text bubble for those that trigger on a cold start.
     * @param activity The activity to use for the IPH.
     */
    public static void maybeShowColdStartIPH(ChromeTabbedActivity activity) {
        maybeShowDownloadHomeIPH(activity);
        maybeShowNTPButtonIPH(activity);
    }

    private static void maybeShowDownloadHomeIPH(ChromeTabbedActivity activity) {
        setupAndMaybeShowIPHForFeature(FeatureConstants.DOWNLOAD_HOME_FEATURE,
                R.id.downloads_menu_id, R.string.iph_download_home_text,
                R.string.iph_download_home_accessibility_text,
                activity.getToolbarManager().getMenuButton(), activity.getAppMenuHandler(),
                Profile.getLastUsedProfile(), activity);
    }

    private static void maybeShowNTPButtonIPH(ChromeTabbedActivity activity) {
        if (!canShowNTPButtonIPH(activity)) return;

        String variation = ToolbarLayout.getNTPButtonVariation();
        if (TextUtils.isEmpty(variation)) return;

        int iphText = 0;
        int iphTextForAccessibility = 0;
        switch (variation) {
            case ToolbarLayout.NTP_BUTTON_HOME_VARIATION:
                iphText = R.string.iph_ntp_button_text_home_text;
                iphTextForAccessibility = R.string.iph_ntp_button_text_home_accessibility_text;
                break;
            case ToolbarLayout.NTP_BUTTON_NEWS_FEED_VARIATION:
                iphText = R.string.iph_ntp_button_text_news_feed_text;
                iphTextForAccessibility = R.string.iph_ntp_button_text_news_feed_accessibility_text;
                break;
            case ToolbarLayout.NTP_BUTTON_CHROME_VARIATION:
                iphText = R.string.iph_ntp_button_text_chrome_text;
                iphTextForAccessibility = R.string.iph_ntp_button_text_chrome_accessibility_text;
                break;
            default:
                break;
        }

        setupAndMaybeShowIPHForFeature(FeatureConstants.NTP_BUTTON_FEATURE, null, iphText,
                iphTextForAccessibility, activity.findViewById(R.id.home_button), null,
                Profile.getLastUsedProfile(), activity);
    }

    /**
     * Attempts to show an IPH text bubble for download continuing.
     * @param activity The activity to use for the IPH.
     * @param profile The profile to use for the tracker.
     */
    public static void maybeShowDownloadContinuingIPH(
            ChromeTabbedActivity activity, Profile profile) {
        setupAndMaybeShowIPHForFeature(
                FeatureConstants.DOWNLOAD_INFOBAR_DOWNLOAD_CONTINUING_FEATURE,
                R.id.downloads_menu_id, R.string.iph_download_infobar_download_continuing_text,
                R.string.iph_download_infobar_download_continuing_text,
                activity.getToolbarManager().getMenuButton(), activity.getAppMenuHandler(), profile,
                activity);
    }

    private static void setupAndMaybeShowIPHForFeature(String featureName,
            Integer highlightMenuItemId, @StringRes int stringId,
            @StringRes int accessibilityStringId, View anchorView,
            @Nullable AppMenuHandler appMenuHandler, Profile profile,
            ChromeTabbedActivity activity) {
        final Tracker tracker = TrackerFactory.getTrackerForProfile(profile);
        tracker.addOnInitializedCallback((Callback<Boolean>) success
                -> maybeShowIPH(tracker, featureName, highlightMenuItemId, stringId,
                        accessibilityStringId, anchorView, appMenuHandler, activity));
    }

    private static void maybeShowIPH(Tracker tracker, String featureName,
            Integer highlightMenuItemId, @StringRes int stringId,
            @StringRes int accessibilityStringId, View anchorView, AppMenuHandler appMenuHandler,
            ChromeTabbedActivity activity) {
        // Activity was destroyed; don't show IPH.
        if (activity.isActivityDestroyed()) return;

        assert(stringId != 0 && accessibilityStringId != 0);
        if (!tracker.shouldTriggerHelpUI(featureName)) return;

        ViewRectProvider rectProvider = new ViewRectProvider(anchorView);

        TextBubble textBubble =
                new TextBubble(activity, anchorView, stringId, accessibilityStringId, rectProvider);
        textBubble.setDismissOnTouchInteraction(true);
        textBubble.addOnDismissListener(() -> anchorView.getHandler().postDelayed(() -> {
            tracker.dismissed(featureName);
            turnOffHighlightForTextBubble(appMenuHandler, anchorView);
        }, ViewHighlighter.IPH_MIN_DELAY_BETWEEN_TWO_HIGHLIGHTS));

        turnOnHighlightForTextBubble(appMenuHandler, highlightMenuItemId, anchorView);

        int yInsetPx = activity.getResources().getDimensionPixelOffset(
                R.dimen.text_bubble_menu_anchor_y_inset);
        rectProvider.setInsetPx(0, 0, 0, yInsetPx);
        textBubble.show();
    }

    private static void turnOnHighlightForTextBubble(
            AppMenuHandler handler, Integer highlightMenuItemId, View anchorView) {
        if (handler != null) {
            handler.setMenuHighlight(highlightMenuItemId);
        } else {
            ViewHighlighter.turnOnHighlight(anchorView, true);
        }
    }

    private static void turnOffHighlightForTextBubble(AppMenuHandler handler, View anchorView) {
        if (handler != null) {
            handler.setMenuHighlight(null);
        } else {
            ViewHighlighter.turnOffHighlight(anchorView);
        }
    }

    private static boolean canShowNTPButtonIPH(ChromeTabbedActivity activity) {
        return FeatureUtilities.isNewTabPageButtonEnabled()
                && !activity.getCurrentTabModel().isIncognito()
                && activity.findViewById(R.id.home_button).getVisibility() == View.VISIBLE;
    }
}
