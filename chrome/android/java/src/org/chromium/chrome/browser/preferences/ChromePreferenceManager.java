// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.content.SharedPreferences;
import android.os.StrictMode;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.crash.MinidumpUploadService.ProcessType;

import java.util.Collections;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

/**
 * ChromePreferenceManager stores and retrieves various values in Android shared preferences.
 */
public class ChromePreferenceManager {
    private static final String TAG = "preferences";

    private static final String PROMOS_SKIPPED_ON_FIRST_START = "promos_skipped_on_first_start";
    private static final String SIGNIN_PROMO_LAST_SHOWN_MAJOR_VERSION =
            "signin_promo_last_shown_chrome_version";
    private static final String SIGNIN_PROMO_LAST_SHOWN_ACCOUNT_NAMES =
            "signin_promo_last_shown_account_names";
    private static final String ALLOW_LOW_END_DEVICE_UI = "allow_low_end_device_ui";
    private static final String PREF_WEBSITE_SETTINGS_FILTER = "website_settings_filter";
    private static final String CONTEXTUAL_SEARCH_PROMO_OPEN_COUNT =
            "contextual_search_promo_open_count";
    private static final String CONTEXTUAL_SEARCH_TAP_TRIGGERED_PROMO_COUNT =
            "contextual_search_tap_triggered_promo_count";
    private static final String CONTEXTUAL_SEARCH_TAP_COUNT = "contextual_search_tap_count";
    private static final String CONTEXTUAL_SEARCH_LAST_ANIMATION_TIME =
            "contextual_search_last_animation_time";
    private static final String CONTEXTUAL_SEARCH_TAP_QUICK_ANSWER_COUNT =
            "contextual_search_tap_quick_answer_count";
    private static final String CONTEXTUAL_SEARCH_CURRENT_WEEK_NUMBER =
            "contextual_search_current_week_number";
    private static final String CHROME_HOME_ENABLED_KEY = "chrome_home_enabled";
    private static final String CHROME_HOME_USER_ENABLED_KEY = "chrome_home_user_enabled";
    private static final String CHROME_HOME_OPT_OUT_SNACKBAR_SHOWN =
            "chrome_home_opt_out_snackbar_shown";

    private static final String CHROME_DEFAULT_BROWSER = "applink.chrome_default_browser";
    private static final String CHROME_MODERN_DESIGN_ENABLED_KEY = "chrome_modern_design_enabled";

    private static final String HOME_PAGE_BUTTON_FORCE_ENABLED_KEY =
            "home_page_button_force_enabled";

    private static final String NTP_BUTTON_ENABLED_KEY = "ntp_button_enabled";

    private static final String CONTENT_SUGGESTIONS_SHOWN_KEY = "content_suggestions_shown";

    private static final String SETTINGS_PERSONALIZED_SIGNIN_PROMO_DISMISSED =
            "settings_personalized_signin_promo_dismissed";

    private static final String NTP_SIGNIN_PROMO_DISMISSED =
            "ntp.personalized_signin_promo_dismissed";
    private static final String NTP_SIGNIN_PROMO_SUPPRESSION_PERIOD_START =
            "ntp.signin_promo_suppression_period_start";

    private static final String SUCCESS_UPLOAD_SUFFIX = "_crash_success_upload";
    private static final String FAILURE_UPLOAD_SUFFIX = "_crash_failure_upload";

    public static final String CHROME_HOME_SHARED_PREFERENCES_KEY = "chrome_home_enabled_date";

    public static final String CHROME_HOME_INFO_PROMO_SHOWN_KEY = "chrome_home_info_promo_shown";

    private static final String SOLE_INTEGRATION_ENABLED_KEY = "sole_integration_enabled";

    private static final String COMMAND_LINE_ON_NON_ROOTED_ENABLED_KEY =
            "command_line_on_non_rooted_enabled";

    private static final String VERIFIED_DIGITAL_ASSET_LINKS =
            "verified_digital_asset_links";

    private static class LazyHolder {
        static final ChromePreferenceManager INSTANCE = new ChromePreferenceManager();
    }

    private final SharedPreferences mSharedPreferences;

    private ChromePreferenceManager() {
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
    }

    /**
     * Get the static instance of ChromePreferenceManager if exists else create it.
     * @return the ChromePreferenceManager singleton
     */
    public static ChromePreferenceManager getInstance() {
        return LazyHolder.INSTANCE;
    }

    /**
     * @return Number of times of successful crash upload.
     */
    public int getCrashSuccessUploadCount(@ProcessType String process) {
        // Convention to keep all the key in preference lower case.
        return mSharedPreferences.getInt(successUploadKey(process), 0);
    }

    public void setCrashSuccessUploadCount(@ProcessType String process, int count) {
        SharedPreferences.Editor sharedPreferencesEditor;

        sharedPreferencesEditor = mSharedPreferences.edit();
        // Convention to keep all the key in preference lower case.
        sharedPreferencesEditor.putInt(successUploadKey(process), count);
        sharedPreferencesEditor.apply();
    }

    public void incrementCrashSuccessUploadCount(@ProcessType String process) {
        setCrashSuccessUploadCount(process, getCrashSuccessUploadCount(process) + 1);
    }

    private String successUploadKey(@ProcessType String process) {
        return process.toLowerCase(Locale.US) + SUCCESS_UPLOAD_SUFFIX;
    }

    /**
     * @return Number of times of failure crash upload after reaching the max number of tries.
     */
    public int getCrashFailureUploadCount(@ProcessType String process) {
        return mSharedPreferences.getInt(failureUploadKey(process), 0);
    }

    public void setCrashFailureUploadCount(@ProcessType String process, int count) {
        SharedPreferences.Editor sharedPreferencesEditor;

        sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putInt(failureUploadKey(process), count);
        sharedPreferencesEditor.apply();
    }

    public void incrementCrashFailureUploadCount(@ProcessType String process) {
        setCrashFailureUploadCount(process, getCrashFailureUploadCount(process) + 1);
    }

    private String failureUploadKey(@ProcessType String process) {
        return process.toLowerCase(Locale.US) + FAILURE_UPLOAD_SUFFIX;
    }

    /**
     * @return Whether the promotion for data reduction has been skipped on first invocation.
     */
    public boolean getPromosSkippedOnFirstStart() {
        return mSharedPreferences.getBoolean(PROMOS_SKIPPED_ON_FIRST_START, false);
    }

    /**
     * Marks whether the data reduction promotion was skipped on first
     * invocation.
     * @param displayed Whether the promotion was shown.
     */
    public void setPromosSkippedOnFirstStart(boolean displayed) {
        writeBoolean(PROMOS_SKIPPED_ON_FIRST_START, displayed);
    }

    /**
     * @return The value for the website settings filter (the one that specifies
     * which sites to show in the list).
     */
    public String getWebsiteSettingsFilterPreference() {
        return mSharedPreferences.getString(
                ChromePreferenceManager.PREF_WEBSITE_SETTINGS_FILTER, "");
    }

    /**
     * Sets the filter value for website settings (which websites to show in the list).
     * @param prefValue The type to restrict the filter to.
     */
    public void setWebsiteSettingsFilterPreference(String prefValue) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putString(
                ChromePreferenceManager.PREF_WEBSITE_SETTINGS_FILTER, prefValue);
        sharedPreferencesEditor.apply();
    }

    /**
     * This value may have been explicitly set to false when we used to keep existing low-end
     * devices on the normal UI rather than the simplified UI. We want to keep the existing device
     * settings. For all new low-end devices they should get the simplified UI by default.
     * @return Whether low end device UI was allowed.
     */
    public boolean getAllowLowEndDeviceUi() {
        return mSharedPreferences.getBoolean(ALLOW_LOW_END_DEVICE_UI, true);
    }

    /**
     * Returns Chrome major version number when signin promo was last shown, or 0 if version number
     * isn't known.
     */
    public int getSigninPromoLastShownVersion() {
        return mSharedPreferences.getInt(SIGNIN_PROMO_LAST_SHOWN_MAJOR_VERSION, 0);
    }

    /**
     * Sets Chrome major version number when signin promo was last shown.
     */
    public void setSigninPromoLastShownVersion(int majorVersion) {
        SharedPreferences.Editor editor = mSharedPreferences.edit();
        editor.putInt(SIGNIN_PROMO_LAST_SHOWN_MAJOR_VERSION, majorVersion).apply();
    }

    /**
     * Returns a set of account names on the device when signin promo was last shown,
     * or null if promo hasn't been shown yet.
     */
    public Set<String> getSigninPromoLastAccountNames() {
        return mSharedPreferences.getStringSet(SIGNIN_PROMO_LAST_SHOWN_ACCOUNT_NAMES, null);
    }

    /**
     * Stores a set of account names on the device when signin promo is shown.
     */
    public void setSigninPromoLastAccountNames(Set<String> accountNames) {
        SharedPreferences.Editor editor = mSharedPreferences.edit();
        editor.putStringSet(SIGNIN_PROMO_LAST_SHOWN_ACCOUNT_NAMES, accountNames).apply();
    }

    /**
     * @return Number of times the panel was opened with the promo visible.
     */
    public int getContextualSearchPromoOpenCount() {
        return mSharedPreferences.getInt(CONTEXTUAL_SEARCH_PROMO_OPEN_COUNT, 0);
    }

    /**
     * Sets the number of times the panel was opened with the promo visible.
     * @param count Number of times the panel was opened with a promo visible.
     */
    public void setContextualSearchPromoOpenCount(int count) {
        writeInt(CONTEXTUAL_SEARCH_PROMO_OPEN_COUNT, count);
    }

    /**
     * @return The last time the search provider icon was animated on tap.
     */
    public long getContextualSearchLastAnimationTime() {
        return mSharedPreferences.getLong(CONTEXTUAL_SEARCH_LAST_ANIMATION_TIME, 0);
    }

    /**
     * Sets the last time the search provider icon was animated on tap.
     * @param time The last time the search provider icon was animated on tap.
     */
    public void setContextualSearchLastAnimationTime(long time) {
        SharedPreferences.Editor ed = mSharedPreferences.edit();
        ed.putLong(CONTEXTUAL_SEARCH_LAST_ANIMATION_TIME, time);
        ed.apply();
    }

    /**
     * @return Number of times the promo was triggered to peek by a tap gesture, or a negative value
     *         if in the disabled state.
     */
    public int getContextualSearchTapTriggeredPromoCount() {
        return mSharedPreferences.getInt(CONTEXTUAL_SEARCH_TAP_TRIGGERED_PROMO_COUNT, 0);
    }

    /**
     * Sets the number of times the promo was triggered to peek by a tap gesture.
     * Use a negative value to record that the counter has been disabled.
     * @param count Number of times the promo was triggered by a tap gesture, or a negative value
     *        to record that the counter has been disabled.
     */
    public void setContextualSearchTapTriggeredPromoCount(int count) {
        writeInt(CONTEXTUAL_SEARCH_TAP_TRIGGERED_PROMO_COUNT, count);
    }

    /**
     * @return Number of tap gestures that have been received since the last time the panel was
     *         opened.
     */
    public int getContextualSearchTapCount() {
        return mSharedPreferences.getInt(CONTEXTUAL_SEARCH_TAP_COUNT, 0);
    }

    /**
     * Sets the number of tap gestures that have been received since the last time the panel was
     * opened.
     * @param count Number of taps that have been received since the last time the panel was opened.
     */
    public void setContextualSearchTapCount(int count) {
        writeInt(CONTEXTUAL_SEARCH_TAP_COUNT, count);
    }

    /**
     * @return Number of Tap triggered Quick Answers (that "do answer") that have been shown since
     *         the last time the panel was opened.
     */
    public int getContextualSearchTapQuickAnswerCount() {
        return mSharedPreferences.getInt(CONTEXTUAL_SEARCH_TAP_QUICK_ANSWER_COUNT, 0);
    }

    /**
     * Sets the number of tap triggered Quick Answers (that "do answer") that have been shown since
     * the last time the panel was opened.
     * @param count Number of Tap triggered Quick Answers (that "do answer") that have been shown
     *              since the last time the panel was opened.
     */
    public void setContextualSearchTapQuickAnswerCount(int count) {
        writeInt(CONTEXTUAL_SEARCH_TAP_QUICK_ANSWER_COUNT, count);
    }

    /**
     * @return The current week number, persisted for weekly CTR recording.
     */
    public int getContextualSearchCurrentWeekNumber() {
        return mSharedPreferences.getInt(CONTEXTUAL_SEARCH_CURRENT_WEEK_NUMBER, 0);
    }

    /**
     * Sets the current week number to persist.  Used for weekly CTR recording.
     * @param weekNumber The week number to store.
     */
    public void setContextualSearchCurrentWeekNumber(int weekNumber) {
        writeInt(CONTEXTUAL_SEARCH_CURRENT_WEEK_NUMBER, weekNumber);
    }

    public boolean getCachedChromeDefaultBrowser() {
        return mSharedPreferences.getBoolean(CHROME_DEFAULT_BROWSER, false);
    }

    public void setCachedChromeDefaultBrowser(boolean isDefault) {
        writeBoolean(CHROME_DEFAULT_BROWSER, isDefault);
    }

    /** Set whether the user dismissed the personalized sign in promo from the Settings. */
    public void setSettingsPersonalizedSigninPromoDismissed(boolean isPromoDismissed) {
        writeBoolean(SETTINGS_PERSONALIZED_SIGNIN_PROMO_DISMISSED, isPromoDismissed);
    }

    /** Checks if the user dismissed the personalized sign in promo from the Settings. */
    public boolean getSettingsPersonalizedSigninPromoDismissed() {
        return mSharedPreferences.getBoolean(SETTINGS_PERSONALIZED_SIGNIN_PROMO_DISMISSED, false);
    }

    /** Checks if the user dismissed the personalized sign in promo from the new tab page. */
    public boolean getNewTabPageSigninPromoDismissed() {
        return mSharedPreferences.getBoolean(NTP_SIGNIN_PROMO_DISMISSED, false);
    }

    /** Set whether the user dismissed the personalized sign in promo from the new tab page. */
    public void setNewTabPageSigninPromoDismissed(boolean isPromoDismissed) {
        writeBoolean(NTP_SIGNIN_PROMO_DISMISSED, isPromoDismissed);
    }

    /**
     * Returns timestamp of the suppression period start if signin promos in the New Tab Page are
     * temporarily suppressed; zero otherwise.
     * @return the epoch time in milliseconds (see {@link System#currentTimeMillis()}).
     */
    public long getNewTabPageSigninPromoSuppressionPeriodStart() {
        return readLong(NTP_SIGNIN_PROMO_SUPPRESSION_PERIOD_START, 0);
    }

    /**
     * Sets timestamp of the suppression period start if signin promos in the New Tab Page are
     * temporarily suppressed.
     * @param timeMillis the epoch time in milliseconds (see {@link System#currentTimeMillis()}).
     */
    public void setNewTabPageSigninPromoSuppressionPeriodStart(long timeMillis) {
        writeLong(NTP_SIGNIN_PROMO_SUPPRESSION_PERIOD_START, timeMillis);
    }

    /**
     * Removes the stored timestamp of the suppression period start when signin promos in the New
     * Tab Page are no longer suppressed.
     */
    public void clearNewTabPageSigninPromoSuppressionPeriodStart() {
        removeKey(NTP_SIGNIN_PROMO_SUPPRESSION_PERIOD_START);
    }

    /**
     * Set whether or not the new tab page button is enabled.
     * @param isEnabled If the new tab page button is enabled.
     */
    public void setNewTabPageButtonEnabled(boolean isEnabled) {
        writeBoolean(NTP_BUTTON_ENABLED_KEY, isEnabled);
    }

    /**
     * Get whether or not the new tab page button is enabled.
     * @return True if the new tab page button is enabled.
     */
    public boolean isNewTabPageButtonEnabled() {
        return mSharedPreferences.getBoolean(NTP_BUTTON_ENABLED_KEY, false);
    }

    /**
     * Set whether or not Chrome modern design is enabled.
     * @param isEnabled Whether the feature is enabled.
     */
    public void setChromeModernDesignEnabled(boolean isEnabled) {
        writeBoolean(CHROME_MODERN_DESIGN_ENABLED_KEY, isEnabled);
    }

    /**
     * @return Whether Chrome modern design is enabled.
     */
    public boolean isChromeModernDesignEnabled() {
        return mSharedPreferences.getBoolean(CHROME_MODERN_DESIGN_ENABLED_KEY, false);
    }

    /**
     * Set whether or not Chrome Home is enabled.
     * @param isEnabled If Chrome Home is enabled.
     */
    public void setChromeHomeEnabled(boolean isEnabled) {
        writeBoolean(CHROME_HOME_ENABLED_KEY, isEnabled);
    }

    /**
     * Get whether or not Chrome Home is enabled.
     * @return True if Chrome Home is enabled.
     */
    public boolean isChromeHomeEnabled() {
        return mSharedPreferences.getBoolean(CHROME_HOME_ENABLED_KEY, false);
    }

    /**
     * Set whether or not the home page button is force enabled.
     * @param isEnabled If the home page button is force enabled.
     */
    public void setHomePageButtonForceEnabled(boolean isEnabled) {
        writeBoolean(HOME_PAGE_BUTTON_FORCE_ENABLED_KEY, isEnabled);
    }

    /**
     * Get whether or not the home page button is force enabled.
     * @return True if the home page button is force enabled.
     */
    public boolean isHomePageButtonForceEnabled() {
        return mSharedPreferences.getBoolean(HOME_PAGE_BUTTON_FORCE_ENABLED_KEY, false);
    }

    /**
     * Clean up unused Chrome Home preferences.
     */
    public void clearObsoleteChromeHomePrefs() {
        removeKey(CHROME_HOME_USER_ENABLED_KEY);
        removeKey(CHROME_HOME_INFO_PROMO_SHOWN_KEY);
        removeKey(CHROME_HOME_OPT_OUT_SNACKBAR_SHOWN);
    }

    /** Marks that the content suggestions surface has been shown. */
    public void setSuggestionsSurfaceShown() {
        writeBoolean(CONTENT_SUGGESTIONS_SHOWN_KEY, true);
    }

    /**
     * Set whether or not command line on non-rooted devices is enabled.
     * @param isEnabled If command line on non-rooted devices is enabled.
     */
    public void setCommandLineOnNonRootedEnabled(boolean isEnabled) {
        writeBoolean(COMMAND_LINE_ON_NON_ROOTED_ENABLED_KEY, isEnabled);
    }

    /** Retunr whether command line on non-rooted devices is enabled. */
    public boolean getCommandLineOnNonRootedEnabled() {
        return mSharedPreferences.getBoolean(COMMAND_LINE_ON_NON_ROOTED_ENABLED_KEY, false);
    }

    /** Returns whether the content suggestions surface has ever been shown. */
    public boolean getSuggestionsSurfaceShown() {
        return mSharedPreferences.getBoolean(CONTENT_SUGGESTIONS_SHOWN_KEY, false);
    }

    /**
     * Get whether or not Sole integration is enabled.
     * @return True if Sole integration is enabled.
     */
    public boolean isSoleEnabled() {
        return mSharedPreferences.getBoolean(SOLE_INTEGRATION_ENABLED_KEY, true);
    }

    /**
     * Set whether or not Sole integration is enabled.
     * @param isEnabled If Sole integration is enabled.
     */
    public void setSoleEnabled(boolean isEnabled) {
        writeBoolean(SOLE_INTEGRATION_ENABLED_KEY, isEnabled);
    }

    /**
     * Gets a set of Strings representing digital asset links that have been verified.
     * Set by {@link #setVerifiedDigitalAssetLinks(Set)}.
     */
    public Set<String> getVerifiedDigitalAssetLinks() {
        // From the official docs, modifying the result of a SharedPreferences.getStringSet can
        // cause bad things to happen including exceptions or ruining the data.
        return new HashSet<>(mSharedPreferences.getStringSet(VERIFIED_DIGITAL_ASSET_LINKS,
                Collections.emptySet()));
    }

    /**
     * Sets a set of digital asset links (represented a strings) that have been verified.
     * Can be retrieved by {@link #getVerifiedDigitalAssetLinks()}.
     */
    public void setVerifiedDigitalAssetLinks(Set<String> links) {
        mSharedPreferences.edit().putStringSet(VERIFIED_DIGITAL_ASSET_LINKS, links).apply();
    }

    /**
     * Writes the given int value to the named shared preference.
     * @param key The name of the preference to modify.
     * @param value The new value for the preference.
     */
    public void writeInt(String key, int value) {
        SharedPreferences.Editor ed = mSharedPreferences.edit();
        ed.putInt(key, value);
        ed.apply();
    }

    /**
     * Reads the given int value from the named shared preference.
     * @param key The name of the preference to return.
     * @return The value of the preference.
     */
    public int readInt(String key) {
        return mSharedPreferences.getInt(key, 0);
    }

    /**
     * Writes the given long to the named shared preference.
     *
     * @param key The name of the preference to modify.
     * @param value The new value for the preference.
     */
    private void writeLong(String key, long value) {
        SharedPreferences.Editor ed = mSharedPreferences.edit();
        ed.putLong(key, value);
        ed.apply();
    }

    /**
     * Reads the given long value from the named shared preference.
     *
     * @param key The name of the preference to return.
     * @param defaultValue The default value to return if there's no value stored.
     * @return The value of the preference if stored; defaultValue otherwise.
     */
    private long readLong(String key, long defaultValue) {
        return mSharedPreferences.getLong(key, defaultValue);
    }

    /**
     * Writes the given String to the named shared preference.
     *
     * @param key The name of the preference to modify.
     * @param value The new value for the preference.
     */
    private void writeString(String key, String value) {
        SharedPreferences.Editor ed = mSharedPreferences.edit();
        ed.putString(key, value);
        ed.apply();
    }

    /**
     * Writes the given boolean to the named shared preference.
     *
     * @param key The name of the preference to modify.
     * @param value The new value for the preference.
     */
    private void writeBoolean(String key, boolean value) {
        SharedPreferences.Editor ed = mSharedPreferences.edit();
        ed.putBoolean(key, value);
        ed.apply();
    }

    /**
     * Removes the shared preference entry.
     *
     * @param key The key of the preference to remove.
     */
    private void removeKey(String key) {
        SharedPreferences.Editor ed = mSharedPreferences.edit();
        ed.remove(key);
        ed.apply();
    }

    /**
     * Logs the most recent date that Chrome Home was enabled.
     * Removes the entry if Chrome Home is disabled.
     *
     * @param isChromeHomeEnabled Whether or not Chrome Home is currently enabled.
     */
    public static void setChromeHomeEnabledDate(boolean isChromeHomeEnabled) {
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            SharedPreferences sharedPreferences = ContextUtils.getAppSharedPreferences();
            long earliestLoggedDate =
                    sharedPreferences.getLong(CHROME_HOME_SHARED_PREFERENCES_KEY, 0L);
            if (isChromeHomeEnabled && earliestLoggedDate == 0L) {
                sharedPreferences.edit()
                        .putLong(CHROME_HOME_SHARED_PREFERENCES_KEY, System.currentTimeMillis())
                        .apply();
            } else if (!isChromeHomeEnabled && earliestLoggedDate != 0L) {
                sharedPreferences.edit().remove(CHROME_HOME_SHARED_PREFERENCES_KEY).apply();
            }
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
    }
}
