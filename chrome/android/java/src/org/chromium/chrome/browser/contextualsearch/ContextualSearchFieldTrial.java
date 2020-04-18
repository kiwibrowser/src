// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.text.TextUtils;

import org.chromium.base.CommandLine;
import org.chromium.base.SysUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.components.variations.VariationsAssociatedData;

/**
 * Provides Field Trial support for the Contextual Search application within Chrome for Android.
 */
public class ContextualSearchFieldTrial {
    private static final String FIELD_TRIAL_NAME = "ContextualSearch";
    private static final String DISABLED_PARAM = "disabled";
    private static final String ENABLED_VALUE = "true";

    static final String MANDATORY_PROMO_ENABLED = "mandatory_promo_enabled";
    static final String MANDATORY_PROMO_LIMIT = "mandatory_promo_limit";
    static final int MANDATORY_PROMO_DEFAULT_LIMIT = 10;

    private static final String DISABLE_SEARCH_TERM_RESOLUTION = "disable_search_term_resolution";
    private static final String WAIT_AFTER_TAP_DELAY_MS = "wait_after_tap_delay_ms";

    // ------------
    // Translation.
    // ------------
    // All these members are private, except for usage by testing.
    // Master switch, needed to disable all translate code for Contextual Search in case of an
    // emergency.
    @VisibleForTesting
    static final String DISABLE_TRANSLATION = "disable_translation";
    // Enables usage of English as the target language even when it's the primary UI language.
    @VisibleForTesting
    static final String ENABLE_ENGLISH_TARGET_TRANSLATION =
            "enable_english_target_translation";

    // ---------------------------------------------
    // Features for suppression or machine learning.
    // ---------------------------------------------
    // TODO(donnd): remove all supporting code once short-lived data collection is done.
    private static final String SCREEN_TOP_SUPPRESSION_DPS = "screen_top_suppression_dps";
    private static final String ENABLE_BAR_OVERLAP_COLLECTION = "enable_bar_overlap_collection";
    private static final String BAR_OVERLAP_SUPPRESSION_ENABLED = "enable_bar_overlap_suppression";
    private static final String WORD_EDGE_SUPPRESSION_ENABLED = "enable_word_edge_suppression";
    private static final String SHORT_WORD_SUPPRESSION_ENABLED = "enable_short_word_suppression";
    private static final String NOT_LONG_WORD_SUPPRESSION_ENABLED =
            "enable_not_long_word_suppression";
    private static final String SHORT_TEXT_RUN_SUPPRESSION_ENABLED =
            "enable_short_text_run_suppression";
    private static final String SMALL_TEXT_SUPPRESSION_ENABLED = "enable_small_text_suppression";
    @VisibleForTesting
    static final String NOT_AN_ENTITY_SUPPRESSION_ENABLED = "enable_not_an_entity_suppression";
    // The threshold for tap suppression based on duration.
    private static final String TAP_DURATION_THRESHOLD_MS = "tap_duration_threshold_ms";
    // The threshold for tap suppression based on a recent scroll.
    private static final String RECENT_SCROLL_DURATION_MS = "recent_scroll_duration_ms";

    private static final String MINIMUM_SELECTION_LENGTH = "minimum_selection_length";

    // -----------------
    // Disable switches.
    // -----------------
    // Safety switch for disabling online-detection.  Also used to disable detection when running
    // tests.
    @VisibleForTesting
    static final String ONLINE_DETECTION_DISABLED = "disable_online_detection";
    private static final String DISABLE_AMP_AS_SEPARATE_TAB = "disable_amp_as_separate_tab";
    // Disable logging for Machine Learning
    private static final String DISABLE_UKM_RANKER_LOGGING = "disable_ukm_ranker_logging";
    private static final String DISABLE_SUPPRESS_FOR_SMART_SELECTION =
            "disable_suppress_for_smart_selection";

    // ----------------------
    // Privacy-related flags.
    // ----------------------
    private static final String DISABLE_SEND_HOME_COUNTRY = "disable_send_home_country";
    private static final String DISABLE_PAGE_CONTENT_NOTIFICATION =
            "disable_page_content_notification";

    // Cached values to avoid repeated and redundant JNI operations.
    // TODO(donnd): consider creating a single Map to cache these static values.
    private static Boolean sEnabled;
    private static Boolean sDisableSearchTermResolution;
    private static Boolean sIsMandatoryPromoEnabled;
    private static Integer sMandatoryPromoLimit;
    private static Boolean sIsTranslationDisabled;
    private static Boolean sIsEnglishTargetTranslationEnabled;
    private static Integer sScreenTopSuppressionDps;
    private static Boolean sIsBarOverlapCollectionEnabled;
    private static Boolean sIsBarOverlapSuppressionEnabled;
    private static Boolean sIsWordEdgeSuppressionEnabled;
    private static Boolean sIsShortWordSuppressionEnabled;
    private static Boolean sIsNotLongWordSuppressionEnabled;
    private static Boolean sIsNotAnEntitySuppressionEnabled;
    private static Boolean sIsShortTextRunSuppressionEnabled;
    private static Boolean sIsSmallTextSuppressionEnabled;
    private static Integer sMinimumSelectionLength;
    private static Boolean sIsOnlineDetectionDisabled;
    private static Boolean sIsAmpAsSeparateTabDisabled;
    private static Boolean sContextualSearchMlTapSuppressionEnabled;
    private static Boolean sContextualSearchSecondTapMlOverrideEnabled;
    private static Boolean sContextualSearchTapDisableOverrideEnabled;
    private static Boolean sIsSendHomeCountryDisabled;
    private static Boolean sIsPageContentNotificationDisabled;
    private static Boolean sIsUkmRankerLoggingDisabled;
    private static Boolean sIsSuppressForSmartSelectionDisabled;
    private static Integer sWaitAfterTapDelayMs;
    private static Integer sTapDurationThresholdMs;
    private static Integer sRecentScrollDurationMs;

    /**
     * Don't instantiate.
     */
    private ContextualSearchFieldTrial() {}

    /**
     * Checks the current Variations parameters associated with the active group as well as the
     * Chrome preference to determine if the service is enabled.
     * @return Whether Contextual Search is enabled or not.
     */
    public static boolean isEnabled() {
        if (sEnabled == null) {
            sEnabled = detectEnabled();
        }
        return sEnabled.booleanValue();
    }

    private static boolean detectEnabled() {
        if (SysUtils.isLowEndDevice()) {
            return false;
        }

        // Allow this user-flippable flag to disable the feature.
        if (CommandLine.getInstance().hasSwitch(ChromeSwitches.DISABLE_CONTEXTUAL_SEARCH)) {
            return false;
        }

        // Allow this user-flippable flag to enable the feature.
        if (CommandLine.getInstance().hasSwitch(ChromeSwitches.ENABLE_CONTEXTUAL_SEARCH)) {
            return true;
        }

        // Allow disabling the feature remotely.
        if (getBooleanParam(DISABLED_PARAM)) {
            return false;
        }

        return true;
    }

    /**
     * @return Whether the search term resolution is disabled.
     */
    static boolean isSearchTermResolutionDisabled() {
        if (sDisableSearchTermResolution == null) {
            sDisableSearchTermResolution = getBooleanParam(DISABLE_SEARCH_TERM_RESOLUTION);
        }
        return sDisableSearchTermResolution.booleanValue();
    }

    /**
     * @return Whether the Mandatory Promo is enabled.
     */
    static boolean isMandatoryPromoEnabled() {
        if (sIsMandatoryPromoEnabled == null) {
            sIsMandatoryPromoEnabled = getBooleanParam(MANDATORY_PROMO_ENABLED);
        }
        return sIsMandatoryPromoEnabled.booleanValue();
    }

    /**
     * @return The number of times the Promo should be seen before it becomes mandatory.
     */
    static int getMandatoryPromoLimit() {
        if (sMandatoryPromoLimit == null) {
            sMandatoryPromoLimit = getIntParamValueOrDefault(
                    MANDATORY_PROMO_LIMIT,
                    MANDATORY_PROMO_DEFAULT_LIMIT);
        }
        return sMandatoryPromoLimit.intValue();
    }

    /**
     * @return Whether all translate code is disabled.
     */
    static boolean isTranslationDisabled() {
        if (sIsTranslationDisabled == null) {
            sIsTranslationDisabled = getBooleanParam(DISABLE_TRANSLATION);
        }
        return sIsTranslationDisabled.booleanValue();
    }

    /**
     * @return Whether English-target translation should be enabled (default is disabled for 'en').
     */
    static boolean isEnglishTargetTranslationEnabled() {
        if (sIsEnglishTargetTranslationEnabled == null) {
            sIsEnglishTargetTranslationEnabled = getBooleanParam(ENABLE_ENGLISH_TARGET_TRANSLATION);
        }
        return sIsEnglishTargetTranslationEnabled.booleanValue();
    }

    /**
     * Gets a Y value limit that will suppress a Tap near the top of the screen.
     * Any Y value less than the limit will suppress the Tap trigger.
     * @return The Y value triggering limit in DPs, a value of zero will not limit.
     */
    static int getScreenTopSuppressionDps() {
        if (sScreenTopSuppressionDps == null) {
            sScreenTopSuppressionDps = getIntParamValueOrDefault(SCREEN_TOP_SUPPRESSION_DPS, 0);
        }
        return sScreenTopSuppressionDps.intValue();
    }

    /**
     * @return Whether collecting data on Bar overlap is enabled.
     */
    static boolean isBarOverlapCollectionEnabled() {
        if (sIsBarOverlapCollectionEnabled == null) {
            sIsBarOverlapCollectionEnabled = getBooleanParam(ENABLE_BAR_OVERLAP_COLLECTION);
        }
        return sIsBarOverlapCollectionEnabled.booleanValue();
    }

    /**
     * @return Whether triggering is suppressed by a selection nearly overlapping the normal
     *         Bar peeking location.
     */
    static boolean isBarOverlapSuppressionEnabled() {
        if (sIsBarOverlapSuppressionEnabled == null) {
            sIsBarOverlapSuppressionEnabled = getBooleanParam(BAR_OVERLAP_SUPPRESSION_ENABLED);
        }
        return sIsBarOverlapSuppressionEnabled.booleanValue();
    }

    /**
     * @return Whether triggering is suppressed by a tap that's near the edge of a word.
     */
    static boolean isWordEdgeSuppressionEnabled() {
        if (sIsWordEdgeSuppressionEnabled == null) {
            sIsWordEdgeSuppressionEnabled = getBooleanParam(WORD_EDGE_SUPPRESSION_ENABLED);
        }
        return sIsWordEdgeSuppressionEnabled.booleanValue();
    }

    /**
     * @return Whether triggering is suppressed by a tap that's in a short word.
     */
    static boolean isShortWordSuppressionEnabled() {
        if (sIsShortWordSuppressionEnabled == null) {
            sIsShortWordSuppressionEnabled = getBooleanParam(SHORT_WORD_SUPPRESSION_ENABLED);
        }
        return sIsShortWordSuppressionEnabled.booleanValue();
    }

    /**
     * @return Whether triggering is suppressed by a tap that's not in a long word.
     */
    static boolean isNotLongWordSuppressionEnabled() {
        if (sIsNotLongWordSuppressionEnabled == null) {
            sIsNotLongWordSuppressionEnabled = getBooleanParam(NOT_LONG_WORD_SUPPRESSION_ENABLED);
        }
        return sIsNotLongWordSuppressionEnabled.booleanValue();
    }

    /**
     * @return Whether triggering is suppressed for a tap that's not on an entity.
     */
    static boolean isNotAnEntitySuppressionEnabled() {
        if (sIsNotAnEntitySuppressionEnabled == null) {
            sIsNotAnEntitySuppressionEnabled = getBooleanParam(NOT_AN_ENTITY_SUPPRESSION_ENABLED);
        }
        return sIsNotAnEntitySuppressionEnabled.booleanValue();
    }

    /**
     * @return Whether triggering is suppressed for a tap that has a short element run-length.
     */
    static boolean isShortTextRunSuppressionEnabled() {
        if (sIsShortTextRunSuppressionEnabled == null) {
            sIsShortTextRunSuppressionEnabled = getBooleanParam(SHORT_TEXT_RUN_SUPPRESSION_ENABLED);
        }
        return sIsShortTextRunSuppressionEnabled.booleanValue();
    }

    /**
     * @return Whether triggering is suppressed for a tap on small-looking text.
     */
    static boolean isSmallTextSuppressionEnabled() {
        if (sIsSmallTextSuppressionEnabled == null) {
            sIsSmallTextSuppressionEnabled = getBooleanParam(SMALL_TEXT_SUPPRESSION_ENABLED);
        }
        return sIsSmallTextSuppressionEnabled.booleanValue();
    }

    /**
     * @return The minimum valid selection length.
     */
    static int getMinimumSelectionLength() {
        if (sMinimumSelectionLength == null) {
            sMinimumSelectionLength = getIntParamValueOrDefault(MINIMUM_SELECTION_LENGTH, 0);
        }
        return sMinimumSelectionLength.intValue();
    }

    /**
     * @return Whether to disable auto-promotion of clicks in the AMP carousel into a separate Tab.
     */
    static boolean isAmpAsSeparateTabDisabled() {
        if (sIsAmpAsSeparateTabDisabled == null) {
            sIsAmpAsSeparateTabDisabled = getBooleanParam(DISABLE_AMP_AS_SEPARATE_TAB);
        }
        return sIsAmpAsSeparateTabDisabled;
    }

    /**
     * @return Whether detection of device-online should be disabled (default false).
     */
    static boolean isOnlineDetectionDisabled() {
        // TODO(donnd): Convert to test-only after launch and we have confidence it's robust.
        if (sIsOnlineDetectionDisabled == null) {
            sIsOnlineDetectionDisabled = getBooleanParam(ONLINE_DETECTION_DISABLED);
        }
        return sIsOnlineDetectionDisabled;
    }

    /**
     * @return Whether sending the "home country" to Google is disabled.
     */
    static boolean isSendHomeCountryDisabled() {
        if (sIsSendHomeCountryDisabled == null) {
            sIsSendHomeCountryDisabled = getBooleanParam(DISABLE_SEND_HOME_COUNTRY);
        }
        return sIsSendHomeCountryDisabled.booleanValue();
    }

    /**
     * @return Whether sending the page content notifications to observers (e.g. icing for
     *         conversational search) is disabled.
     */
    static boolean isPageContentNotificationDisabled() {
        if (sIsPageContentNotificationDisabled == null) {
            sIsPageContentNotificationDisabled = getBooleanParam(DISABLE_PAGE_CONTENT_NOTIFICATION);
        }
        return sIsPageContentNotificationDisabled.booleanValue();
    }

    /**
     * @return Whether or not logging to Ranker via UKM is disabled.
     */
    static boolean isUkmRankerLoggingDisabled() {
        if (sIsUkmRankerLoggingDisabled == null) {
            sIsUkmRankerLoggingDisabled = getBooleanParam(DISABLE_UKM_RANKER_LOGGING);
        }
        return sIsUkmRankerLoggingDisabled;
    }

    /**
     * Determines whether the Contextual Search UI is suppressed when Smart Select is active.
     * This applies to long-press activation of a UI for Smart Select and/or Contextual Search. If
     * this returns true, the Contextual Search Bar will be allowed to show in response to a
     * long-press gesture on Android O even when the Smart Select UI may be active.
     * @return Whether suppression our UI when Smart Select is active has been disabled.
     */
    static boolean isSuppressForSmartSelectionDisabled() {
        if (sIsSuppressForSmartSelectionDisabled == null) {
            sIsSuppressForSmartSelectionDisabled =
                    getBooleanParam(DISABLE_SUPPRESS_FOR_SMART_SELECTION);
        }
        return sIsSuppressForSmartSelectionDisabled;
    }

    /**
     * Gets an amount to delay after a Tap gesture is recognized, in case some user gesture
     * immediately follows that would prevent the UI from showing.
     * The classic example is a scroll, which might be a signal that the previous tap was
     * accidental.
     * @return The delay in MS after the Tap before showing any UI.
     */
    static int getWaitAfterTapDelayMs() {
        if (sWaitAfterTapDelayMs == null) {
            sWaitAfterTapDelayMs = getIntParamValueOrDefault(WAIT_AFTER_TAP_DELAY_MS, 0);
        }
        return sWaitAfterTapDelayMs.intValue();
    }

    /**
     * Gets a threshold for the duration of a tap gesture for categorization as brief or lengthy.
     * @return The maximum amount of time in milliseconds for a tap gesture that's still considered
     *         a very brief duration tap.
     */
    static int getTapDurationThresholdMs() {
        if (sTapDurationThresholdMs == null) {
            sTapDurationThresholdMs = getIntParamValueOrDefault(TAP_DURATION_THRESHOLD_MS, 0);
        }
        return sTapDurationThresholdMs.intValue();
    }

    /**
     * Gets the duration to use for suppressing Taps after a recent scroll, or {@code 0} if no
     * suppression is configured.
     * @return The period of time after a scroll when tap triggering is suppressed.
     */
    static int getRecentScrollDurationMs() {
        if (sRecentScrollDurationMs == null) {
            sRecentScrollDurationMs = getIntParamValueOrDefault(RECENT_SCROLL_DURATION_MS, 0);
        }
        return sRecentScrollDurationMs.intValue();
    }

    // ---------------------------
    // Feature-controlled Switches
    // ---------------------------

    /**
     * @return Whether or not ML-based Tap suppression is enabled.
     */
    static boolean isContextualSearchMlTapSuppressionEnabled() {
        if (sContextualSearchMlTapSuppressionEnabled == null) {
            sContextualSearchMlTapSuppressionEnabled = ChromeFeatureList.isEnabled(
                    ChromeFeatureList.CONTEXTUAL_SEARCH_ML_TAP_SUPPRESSION);
        }
        return sContextualSearchMlTapSuppressionEnabled;
    }

    /**
     * @return Whether or not to override an ML-based Tap suppression on a second tap.
     */
    static boolean isContextualSearchSecondTapMlOverrideEnabled() {
        if (sContextualSearchSecondTapMlOverrideEnabled == null) {
            sContextualSearchSecondTapMlOverrideEnabled =
                    ChromeFeatureList.isEnabled(ChromeFeatureList.CONTEXTUAL_SEARCH_SECOND_TAP);
        }
        return sContextualSearchSecondTapMlOverrideEnabled;
    }

    /**
     * @return Whether or not to override tap-disable for users that have never opened the panel.
     */
    static boolean isContextualSearchTapDisableOverrideEnabled() {
        if (sContextualSearchTapDisableOverrideEnabled == null) {
            sContextualSearchTapDisableOverrideEnabled = ChromeFeatureList.isEnabled(
                    ChromeFeatureList.CONTEXTUAL_SEARCH_TAP_DISABLE_OVERRIDE);
        }
        return sContextualSearchTapDisableOverrideEnabled;
    }

    // --------------------------------------------------------------------------------------------
    // Helpers.
    // --------------------------------------------------------------------------------------------

    /**
     * Gets a boolean Finch parameter, assuming the <paramName>="true" format.  Also checks for a
     * command-line switch with the same name, for easy local testing.
     * @param paramName The name of the Finch parameter (or command-line switch) to get a value for.
     * @return Whether the Finch param is defined with a value "true", if there's a command-line
     *         flag present with any value.
     */
    private static boolean getBooleanParam(String paramName) {
        if (CommandLine.getInstance().hasSwitch(paramName)) {
            return true;
        }
        return TextUtils.equals(ENABLED_VALUE,
                VariationsAssociatedData.getVariationParamValue(FIELD_TRIAL_NAME, paramName));
    }

    /**
     * Returns an integer value for a Finch parameter, or the default value if no parameter exists
     * in the current configuration.  Also checks for a command-line switch with the same name.
     * @param paramName The name of the Finch parameter (or command-line switch) to get a value for.
     * @param defaultValue The default value to return when there's no param or switch.
     * @return An integer value -- either the param or the default.
     */
    private static int getIntParamValueOrDefault(String paramName, int defaultValue) {
        String value = CommandLine.getInstance().getSwitchValue(paramName);
        if (TextUtils.isEmpty(value)) {
            value = VariationsAssociatedData.getVariationParamValue(FIELD_TRIAL_NAME, paramName);
        }
        if (!TextUtils.isEmpty(value)) {
            try {
                return Integer.parseInt(value);
            } catch (NumberFormatException e) {
                return defaultValue;
            }
        }

        return defaultValue;
    }
}
