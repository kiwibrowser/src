// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.MainDex;
import org.chromium.base.library_loader.LibraryLoader;

import java.util.Map;

/**
 * Java accessor for base/feature_list.h state.
 */
@JNINamespace("chrome::android")
@MainDex
public abstract class ChromeFeatureList {
    /** Map that stores substitution feature flags for tests. */
    private static Map<String, Boolean> sTestFeatures;

    // Prevent instantiation.
    private ChromeFeatureList() {}

    /**
     * Sets the feature flags to use in JUnit tests, since native calls are not available there.
     * Do not use directly, prefer using the {@link Features} annotation.
     *
     * @see Features
     * @see Features.Processor
     */
    @VisibleForTesting
    public static void setTestFeatures(Map<String, Boolean> features) {
        sTestFeatures = features;
    }

    /**
     * @return Whether the native FeatureList has been initialized. If this method returns false,
     *         none of the methods in this class that require native access should be called (except
     *         in tests if test features have been set).
     */
    public static boolean isInitialized() {
        if (sTestFeatures != null) return true;
        if (!LibraryLoader.isInitialized()) return false;

        // Even if the native library is loaded, the C++ FeatureList might not be initialized yet.
        // In that case, accessing it will not immediately fail, but instead cause a crash later
        // when it is initialized. Return whether the native FeatureList has been initialized,
        // so the return value can be tested, or asserted for a more actionable stack trace
        // on failure.
        return nativeIsInitialized();
    }

    /**
     * Returns whether the specified feature is enabled or not.
     *
     * Note: Features queried through this API must be added to the array
     * |kFeaturesExposedToJava| in chrome/browser/android/chrome_feature_list.cc
     *
     * @param featureName The name of the feature to query.
     * @return Whether the feature is enabled or not.
     */
    public static boolean isEnabled(String featureName) {
        if (sTestFeatures != null) {
            Boolean enabled = sTestFeatures.get(featureName);
            if (enabled == null) throw new IllegalArgumentException(featureName);
            return enabled;
        }

        assert isInitialized();
        return nativeIsEnabled(featureName);
    }

    /**
     * Returns a field trial param for the specified feature.
     *
     * Note: Features queried through this API must be added to the array
     * |kFeaturesExposedToJava| in chrome/browser/android/chrome_feature_list.cc
     *
     * @param featureName The name of the feature to retrieve a param for.
     * @param paramName The name of the param for which to get as an integer.
     * @return The parameter value as a String. The string is empty if the feature does not exist or
     *   the specified parameter does not exist.
     */
    public static String getFieldTrialParamByFeature(String featureName, String paramName) {
        if (sTestFeatures != null) return "";
        assert isInitialized();
        return nativeGetFieldTrialParamByFeature(featureName, paramName);
    }

    /**
     * Returns a field trial param as an int for the specified feature.
     *
     * Note: Features queried through this API must be added to the array
     * |kFeaturesExposedToJava| in chrome/browser/android/chrome_feature_list.cc
     *
     * @param featureName The name of the feature to retrieve a param for.
     * @param paramName The name of the param for which to get as an integer.
     * @param defaultValue The integer value to use if the param is not available.
     * @return The parameter value as an int. Default value if the feature does not exist or the
     *         specified parameter does not exist or its string value does not represent an int.
     */
    public static int getFieldTrialParamByFeatureAsInt(
            String featureName, String paramName, int defaultValue) {
        if (sTestFeatures != null) return defaultValue;
        assert isInitialized();
        return nativeGetFieldTrialParamByFeatureAsInt(featureName, paramName, defaultValue);
    }

    /**
     * Returns a field trial param as a double for the specified feature.
     *
     * Note: Features queried through this API must be added to the array
     * |kFeaturesExposedToJava| in chrome/browser/android/chrome_feature_list.cc
     *
     * @param featureName The name of the feature to retrieve a param for.
     * @param paramName The name of the param for which to get as an integer.
     * @param defaultValue The double value to use if the param is not available.
     * @return The parameter value as a double. Default value if the feature does not exist or the
     *         specified parameter does not exist or its string value does not represent a double.
     */
    public static double getFieldTrialParamByFeatureAsDouble(
            String featureName, String paramName, double defaultValue) {
        if (sTestFeatures != null) return defaultValue;
        assert isInitialized();
        return nativeGetFieldTrialParamByFeatureAsDouble(featureName, paramName, defaultValue);
    }

    /**
     * Returns a field trial param as a boolean for the specified feature.
     *
     * Note: Features queried through this API must be added to the array
     * |kFeaturesExposedToJava| in chrome/browser/android/chrome_feature_list.cc
     *
     * @param featureName The name of the feature to retrieve a param for.
     * @param paramName The name of the param for which to get as an integer.
     * @param defaultValue The boolean value to use if the param is not available.
     * @return The parameter value as a boolean. Default value if the feature does not exist or the
     *         specified parameter does not exist or its string value is neither "true" nor "false".
     */
    public static boolean getFieldTrialParamByFeatureAsBoolean(
            String featureName, String paramName, boolean defaultValue) {
        if (sTestFeatures != null) return defaultValue;
        assert isInitialized();
        return nativeGetFieldTrialParamByFeatureAsBoolean(featureName, paramName, defaultValue);
    }

    // Alphabetical:
    public static final String ADJUST_WEBAPK_INSTALLATION_SPACE = "AdjustWebApkInstallationSpace";
    public static final String ALLOW_READER_FOR_ACCESSIBILITY = "AllowReaderForAccessibility";
    public static final String ANDROID_PAY_INTEGRATION_V1 = "AndroidPayIntegrationV1";
    public static final String ANDROID_PAY_INTEGRATION_V2 = "AndroidPayIntegrationV2";
    public static final String ANDROID_PAYMENT_APPS = "AndroidPaymentApps";
    public static final String AUTOFILL_KEYBOARD_ACCESSORY = "AutofillKeyboardAccessory";
    public static final String AUTOFILL_SCAN_CARDHOLDER_NAME = "AutofillScanCardholderName";
    public static final String CAF_MEDIA_ROUTER_IMPL = "CafMediaRouterImpl";
    public static final String CAPTIVE_PORTAL_CERTIFICATE_LIST = "CaptivePortalCertificateList";
    public static final String CCT_BACKGROUND_TAB = "CCTBackgroundTab";
    public static final String CCT_EXTERNAL_LINK_HANDLING = "CCTExternalLinkHandling";
    public static final String CCT_PARALLEL_REQUEST = "CCTParallelRequest";
    public static final String CCT_POST_MESSAGE_API = "CCTPostMessageAPI";
    public static final String CCT_REDIRECT_PRECONNECT = "CCTRedirectPreconnect";
    public static final String CHROME_DUPLEX = "ChromeDuplex";
    // TODO(mdjones): Remove CHROME_HOME_SWIPE_VELOCITY_FEATURE or rename.
    public static final String CHROME_HOME_SWIPE_VELOCITY_FEATURE = "ChromeHomeSwipeLogicVelocity";
    public static final String CHROME_MEMEX = "ChromeMemex";
    public static final String CHROME_MODERN_ALTERNATE_CARD_LAYOUT =
            "ChromeModernAlternateCardLayout";
    public static final String CHROME_MODERN_DESIGN = "ChromeModernDesign";
    public static final String CHROME_SMART_SELECTION = "ChromeSmartSelection";
    public static final String CLEAR_OLD_BROWSING_DATA = "ClearOldBrowsingData";
    public static final String CLIPBOARD_CONTENT_SETTING = "ClipboardContentSetting";
    public static final String COMMAND_LINE_ON_NON_ROOTED = "CommandLineOnNonRooted";
    public static final String CONTENT_SUGGESTIONS_FAVICONS_FROM_NEW_SERVER =
            "ContentSuggestionsFaviconsFromNewServer";
    public static final String CONTENT_SUGGESTIONS_NOTIFICATIONS =
            "ContentSuggestionsNotifications";
    public static final String CONTENT_SUGGESTIONS_SCROLL_TO_LOAD =
            "ContentSuggestionsScrollToLoad";
    public static final String CONTENT_SUGGESTIONS_SETTINGS = "ContentSuggestionsSettings";
    public static final String CONTENT_SUGGESTIONS_THUMBNAIL_DOMINANT_COLOR =
            "ContentSuggestionsThumbnailDominantColor";
    public static final String CONTEXTUAL_SEARCH_ML_TAP_SUPPRESSION =
            "ContextualSearchMlTapSuppression";
    public static final String CONTEXTUAL_SEARCH_SECOND_TAP = "ContextualSearchSecondTap";
    public static final String CONTEXTUAL_SEARCH_TAP_DISABLE_OVERRIDE =
            "ContextualSearchTapDisableOverride";
    public static final String CONTEXTUAL_SUGGESTIONS_BOTTOM_SHEET =
            "ContextualSuggestionsBottomSheet";
    public static final String CONTEXTUAL_SUGGESTIONS_SLIM_PEEK_UI =
            "ContextualSuggestionsSlimPeekUI";
    public static final String CUSTOM_CONTEXT_MENU = "CustomContextMenu";
    public static final String CUSTOM_FEEDBACK_UI = "CustomFeedbackUi";
    // Enables the Data Reduction Proxy menu item in the main menu rather than under Settings on
    // Android.
    public static final String DATA_REDUCTION_MAIN_MENU = "DataReductionProxyMainMenu";
    public static final String DONT_PREFETCH_LIBRARIES = "DontPrefetchLibraries";
    public static final String DOWNLOAD_HOME_SHOW_STORAGE_INFO = "DownloadHomeShowStorageInfo";
    public static final String DOWNLOAD_PROGRESS_INFOBAR = "DownloadProgressInfoBar";
    public static final String DOWNLOADS_FOREGROUND = "DownloadsForeground";
    public static final String DOWNLOADS_LOCATION_CHANGE = "DownloadsLocationChange";
    public static final String EXPERIMENTAL_APP_BANNERS = "ExperimentalAppBanners";
    // When enabled, fullscreen WebContents will be moved to a new Activity. Coming soon...
    public static final String FULLSCREEN_ACTIVITY = "FullscreenActivity";
    public static final String GRANT_NOTIFICATIONS_TO_DSE = "GrantNotificationsToDSE";
    public static final String HOME_PAGE_BUTTON_FORCE_ENABLED = "HomePageButtonForceEnabled";
    public static final String HORIZONTAL_TAB_SWITCHER_ANDROID = "HorizontalTabSwitcherAndroid";
    // Whether we show an important sites dialog in the "Clear Browsing Data" flow.
    public static final String IMPORTANT_SITES_IN_CBD = "ImportantSitesInCBD";
    public static final String INTEREST_FEED_CONTENT_SUGGESTIONS = "InterestFeedContentSuggestions";
    public static final String LANGUAGES_PREFERENCE = "LanguagesPreference";
    public static final String LONG_PRESS_BACK_FOR_HISTORY = "LongPressBackForHistory";
    public static final String SEARCH_ENGINE_PROMO_EXISTING_DEVICE =
            "SearchEnginePromo.ExistingDevice";
    public static final String SEARCH_ENGINE_PROMO_NEW_DEVICE = "SearchEnginePromo.NewDevice";
    public static final String MATERIAL_DESIGN_INCOGNITO_NTP = "MaterialDesignIncognitoNTP";
    public static final String MODAL_PERMISSION_PROMPTS = "ModalPermissionPrompts";
    public static final String MODAL_PERMISSION_DIALOG_VIEW = "ModalPermissionDialogView";
    public static final String NEW_PHOTO_PICKER = "NewPhotoPicker";
    public static final String NO_CREDIT_CARD_ABORT = "NoCreditCardAbort";
    public static final String NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER =
            "NTPArticleSuggestionsExpandableHeader";
    public static final String NTP_BUTTON = "NTPButton";
    public static final String NTP_FOREIGN_SESSIONS_SUGGESTIONS = "NTPForeignSessionsSuggestions";
    public static final String NTP_LAUNCH_AFTER_INACTIVITY = "NTPLaunchAfterInactivity";
    public static final String NTP_MODERN_LAYOUT = "NTPModernLayout";
    public static final String NTP_SHOW_GOOGLE_G_IN_OMNIBOX = "NTPShowGoogleGInOmnibox";
    public static final String NTP_SNIPPETS_INCREASED_VISIBILITY = "NTPSnippetsIncreasedVisibility";
    public static final String OFFLINE_PAGES_DESCRIPTIVE_FAIL_STATUS =
            "OfflinePagesDescriptiveFailStatus";
    public static final String OFFLINE_PAGES_DESCRIPTIVE_PENDING_STATUS =
            "OfflinePagesDescriptivePendingStatus";
    public static final String OMNIBOX_HIDE_SCHEME_DOMAIN_IN_STEADY_STATE =
            "OmniboxUIExperimentHideSteadyStateUrlSchemeAndSubdomains";
    public static final String OMNIBOX_SPARE_RENDERER = "OmniboxSpareRenderer";
    public static final String OMNIBOX_VOICE_SEARCH_ALWAYS_VISIBLE =
            "OmniboxVoiceSearchAlwaysVisible";
    public static final String PAY_WITH_GOOGLE_V1 = "PayWithGoogleV1";
    public static final String PASSWORD_SEARCH = "PasswordSearchMobile";
    public static final String PASSWORDS_KEYBOARD_ACCESSORY = "PasswordsKeyboardAccessory";
    public static final String PERMISSION_DELEGATION = "PermissionDelegation";
    public static final String PROGRESS_BAR_THROTTLE = "ProgressBarThrottle";
    public static final String PWA_PERSISTENT_NOTIFICATION = "PwaPersistentNotification";
    public static final String READER_MODE_IN_CCT = "ReaderModeInCCT";
    public static final String REMOVE_NAVIGATION_HISTORY = "RemoveNavigationHistory";
    public static final String SERVICE_WORKER_PAYMENT_APPS = "ServiceWorkerPaymentApps";
    public static final String SHOW_TRUSTED_PUBLISHER_URL = "ShowTrustedPublisherURL";
    public static final String SIMPLIFIED_NTP = "SimplifiedNTP";
    public static final String SITE_NOTIFICATION_CHANNELS = "SiteNotificationChannels";
    public static final String SOLE_INTEGRATION = "SoleIntegration";
    public static final String SOUND_CONTENT_SETTING = "SoundContentSetting";
    public static final String SPANNABLE_INLINE_AUTOCOMPLETE = "SpannableInlineAutocomplete";
    public static final String SUBRESOURCE_FILTER = "SubresourceFilter";
    public static final String QUERY_IN_OMNIBOX = "QueryInOmnibox";
    public static final String TAB_REPARENTING = "TabReparenting";
    public static final String TRUSTED_WEB_ACTIVITY = "TrustedWebActivity";
    public static final String VIDEO_PERSISTENCE = "VideoPersistence";
    public static final String UNIFIED_CONSENT = "UnifiedConsent";
    public static final String VR_BROWSING_FEEDBACK = "VrBrowsingFeedback";
    public static final String VR_BROWSING_IN_CUSTOM_TAB = "VrBrowsingInCustomTab";
    public static final String VR_BROWSING_NATIVE_ANDROID_UI = "VrBrowsingNativeAndroidUi";
    public static final String WEB_AUTH = "WebAuthentication";
    public static final String WEB_PAYMENTS = "WebPayments";
    public static final String WEB_PAYMENTS_METHOD_SECTION_ORDER_V2 =
            "WebPaymentsMethodSectionOrderV2";
    public static final String WEB_PAYMENTS_MODIFIERS = "WebPaymentsModifiers";
    public static final String WEB_PAYMENTS_SINGLE_APP_UI_SKIP = "WebPaymentsSingleAppUiSkip";
    public static final String WEBVR_AUTOPRESENT_FROM_INTENT = "WebVrAutopresentFromIntent";
    public static final String WEBVR_CARDBOARD_SUPPORT = "WebVrCardboardSupport";

    private static native boolean nativeIsInitialized();
    private static native boolean nativeIsEnabled(String featureName);
    private static native String nativeGetFieldTrialParamByFeature(
            String featureName, String paramName);
    private static native int nativeGetFieldTrialParamByFeatureAsInt(
            String featureName, String paramName, int defaultValue);
    private static native double nativeGetFieldTrialParamByFeatureAsDouble(
            String featureName, String paramName, double defaultValue);
    private static native boolean nativeGetFieldTrialParamByFeatureAsBoolean(
            String featureName, String paramName, boolean defaultValue);
}
