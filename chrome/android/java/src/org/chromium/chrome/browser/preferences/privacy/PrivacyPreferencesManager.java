// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.privacy;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.device.DeviceClassManager;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.survey.SurveyController;
import org.chromium.components.minidump_uploader.util.CrashReportingPermissionManager;
import org.chromium.components.minidump_uploader.util.NetworkPermissionUtil;

/**
 * Reads, writes, and migrates preferences related to network usage and privacy.
 */
public class PrivacyPreferencesManager implements CrashReportingPermissionManager{
    static final String DEPRECATED_PREF_CRASH_DUMP_UPLOAD = "crash_dump_upload";
    static final String DEPRECATED_PREF_CRASH_DUMP_UPLOAD_NO_CELLULAR =
            "crash_dump_upload_no_cellular";
    private static final String DEPRECATED_PREF_CELLULAR_EXPERIMENT = "cellular_experiment";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB = "physical_web";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_SHARING = "physical_web_sharing";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_HAS_DEFERRED_METRICS_KEY =
            "PhysicalWeb.HasDeferredMetrics";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_OPT_IN_DECLINE_BUTTON_PRESS_COUNT =
            "PhysicalWeb.OptIn.DeclineButtonPressed";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_OPT_IN_ENABLE_BUTTON_PRESS_COUNT =
            "PhysicalWeb.OptIn.EnableButtonPressed";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_PREFS_FEATURE_DISABLED_COUNT =
            "PhysicalWeb.Prefs.FeatureDisabled";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_PREFS_FEATURE_ENABLED_COUNT =
            "PhysicalWeb.Prefs.FeatureEnabled";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_PREFS_LOCATION_DENIED_COUNT =
            "PhysicalWeb.Prefs.LocationDenied";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_PREFS_LOCATION_GRANTED_COUNT =
            "PhysicalWeb.Prefs.LocationGranted";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_PWS_BACKGROUND_RESOLVE_TIMES =
            "PhysicalWeb.ResolveTime.Background";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_PWS_FOREGROUND_RESOLVE_TIMES =
            "PhysicalWeb.ResolveTime.Foreground";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_PWS_REFRESH_RESOLVE_TIMES =
            "PhysicalWeb.ResolveTime.Refresh";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_URL_SELECTED_COUNT =
            "PhysicalWeb.UrlSelected";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_TOTAL_URLS_INITIAL_COUNTS =
            "PhysicalWeb.TotalUrls.OnInitialDisplay";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_TOTAL_URLS_REFRESH_COUNTS =
            "PhysicalWeb.TotalUrls.OnRefresh";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_ACTIVITY_REFERRALS =
            "PhysicalWeb.ActivityReferral";
    private static final String DEPRECATED_PREF_PHYSICAL_WEB_PHYSICAL_WEB_STATE =
            "PhysicalWeb.State";

    public static final String PREF_METRICS_REPORTING = "metrics_reporting";
    private static final String PREF_METRICS_IN_SAMPLE = "in_metrics_sample";
    private static final String PREF_NETWORK_PREDICTIONS = "network_predictions";
    private static final String PREF_BANDWIDTH_OLD = "prefetch_bandwidth";
    private static final String PREF_BANDWIDTH_NO_CELLULAR_OLD = "prefetch_bandwidth_no_cellular";
    private static final String ALLOW_PRERENDER_OLD = "allow_prefetch";

    @SuppressLint("StaticFieldLeak")
    private static PrivacyPreferencesManager sInstance;

    private final Context mContext;
    private final SharedPreferences mSharedPreferences;

    @VisibleForTesting
    PrivacyPreferencesManager(Context context) {
        mContext = context;
        mSharedPreferences = ContextUtils.getAppSharedPreferences();

        migratePhysicalWebPreferences();
        migrateUsageAndCrashPreferences();
    }

    public static PrivacyPreferencesManager getInstance() {
        if (sInstance == null) {
            sInstance = new PrivacyPreferencesManager(ContextUtils.getApplicationContext());
        }
        return sInstance;
    }

    // TODO(https://crbug.com/826540): Remove some time after 5/2019.
    public void migratePhysicalWebPreferences() {
        SharedPreferences.Editor editor = mSharedPreferences.edit();
        editor.remove(DEPRECATED_PREF_PHYSICAL_WEB)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_SHARING)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_HAS_DEFERRED_METRICS_KEY)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_OPT_IN_DECLINE_BUTTON_PRESS_COUNT)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_OPT_IN_ENABLE_BUTTON_PRESS_COUNT)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_PREFS_FEATURE_DISABLED_COUNT)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_PREFS_FEATURE_ENABLED_COUNT)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_PREFS_LOCATION_DENIED_COUNT)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_PREFS_LOCATION_GRANTED_COUNT)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_PWS_BACKGROUND_RESOLVE_TIMES)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_PWS_FOREGROUND_RESOLVE_TIMES)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_PWS_REFRESH_RESOLVE_TIMES)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_URL_SELECTED_COUNT)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_TOTAL_URLS_INITIAL_COUNTS)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_TOTAL_URLS_REFRESH_COUNTS)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_ACTIVITY_REFERRALS)
                .remove(DEPRECATED_PREF_PHYSICAL_WEB_PHYSICAL_WEB_STATE)
                .apply();
    }

    public void migrateUsageAndCrashPreferences() {
        SharedPreferences.Editor editor = mSharedPreferences.edit();

        if (mSharedPreferences.contains(DEPRECATED_PREF_CRASH_DUMP_UPLOAD)) {
            String crashDumpNeverUpload = "crash_dump_never_upload";
            setUsageAndCrashReporting(
                    !mSharedPreferences
                             .getString(DEPRECATED_PREF_CRASH_DUMP_UPLOAD, crashDumpNeverUpload)
                             .equals(crashDumpNeverUpload));

            // Remove both this preference and the related one. If the related one is not removed
            // now, later migrations could read from it and clobber the state.
            editor.remove(DEPRECATED_PREF_CRASH_DUMP_UPLOAD);
            if (mSharedPreferences.contains(DEPRECATED_PREF_CRASH_DUMP_UPLOAD_NO_CELLULAR)) {
                editor.remove(DEPRECATED_PREF_CRASH_DUMP_UPLOAD_NO_CELLULAR);
            }
        } else if (mSharedPreferences.contains(DEPRECATED_PREF_CRASH_DUMP_UPLOAD_NO_CELLULAR)) {
            setUsageAndCrashReporting(mSharedPreferences.getBoolean(
                    DEPRECATED_PREF_CRASH_DUMP_UPLOAD_NO_CELLULAR, false));
            editor.remove(DEPRECATED_PREF_CRASH_DUMP_UPLOAD_NO_CELLULAR);
        }

        if (mSharedPreferences.contains(DEPRECATED_PREF_CELLULAR_EXPERIMENT)) {
            editor.remove(DEPRECATED_PREF_CELLULAR_EXPERIMENT);
        }
        editor.apply();
    }

    /**
     * Migrate and delete old preferences.  Note that migration has to happen in Android-specific
     * code because we need to access ALLOW_PRERENDER sharedPreference.
     * TODO(bnc) https://crbug.com/394845. This change is planned for M38. After a year or so, it
     * would be worth considering removing this migration code (also removing accessors in
     * PrefServiceBridge and pref_service_bridge), and reverting to default for users
     * who had set preferences but have not used Chrome for a year. This change would be subject to
     * privacy review.
     */
    public void migrateNetworkPredictionPreferences() {
        PrefServiceBridge prefService = PrefServiceBridge.getInstance();

        // See if PREF_NETWORK_PREDICTIONS is an old boolean value.
        boolean predictionOptionIsBoolean = false;
        try {
            mSharedPreferences.getString(PREF_NETWORK_PREDICTIONS, "");
        } catch (ClassCastException ex) {
            predictionOptionIsBoolean = true;
        }

        // Nothing to do if the user or this migration code has already set the new
        // preference.
        if (!predictionOptionIsBoolean
                && prefService.obsoleteNetworkPredictionOptionsHasUserSetting()) {
            return;
        }

        // Nothing to do if the old preferences are unset.
        if (!predictionOptionIsBoolean
                && !mSharedPreferences.contains(PREF_BANDWIDTH_OLD)
                && !mSharedPreferences.contains(PREF_BANDWIDTH_NO_CELLULAR_OLD)) {
            return;
        }

        // Migrate if the old preferences are at their default values.
        // (Note that for PREF_BANDWIDTH*, if the setting is default, then there is no way to tell
        // whether the user has set it.)
        final String prefBandwidthDefault = BandwidthType.PRERENDER_ON_WIFI.title();
        final String prefBandwidth =
                mSharedPreferences.getString(PREF_BANDWIDTH_OLD, prefBandwidthDefault);
        boolean prefBandwidthNoCellularDefault = true;
        boolean prefBandwidthNoCellular = mSharedPreferences.getBoolean(
                PREF_BANDWIDTH_NO_CELLULAR_OLD, prefBandwidthNoCellularDefault);

        if (!(prefBandwidthDefault.equals(prefBandwidth))
                || (prefBandwidthNoCellular != prefBandwidthNoCellularDefault)) {
            boolean newValue = true;
            // Observe PREF_BANDWIDTH on mobile network capable devices.
            if (isMobileNetworkCapable()) {
                if (mSharedPreferences.contains(PREF_BANDWIDTH_OLD)) {
                    BandwidthType prefetchBandwidthTypePref = BandwidthType.getBandwidthFromTitle(
                            prefBandwidth);
                    if (BandwidthType.NEVER_PRERENDER.equals(prefetchBandwidthTypePref)) {
                        newValue = false;
                    } else if (BandwidthType.PRERENDER_ON_WIFI.equals(prefetchBandwidthTypePref)) {
                        newValue = true;
                    } else if (BandwidthType.ALWAYS_PRERENDER.equals(prefetchBandwidthTypePref)) {
                        newValue = true;
                    }
                }
            // Observe PREF_BANDWIDTH_NO_CELLULAR on devices without mobile network.
            } else {
                if (mSharedPreferences.contains(PREF_BANDWIDTH_NO_CELLULAR_OLD)) {
                    if (prefBandwidthNoCellular) {
                        newValue = true;
                    } else {
                        newValue = false;
                    }
                }
            }
            // Save new value in Chrome PrefService.
            prefService.setNetworkPredictionEnabled(newValue);
        }

        // Delete old sharedPreferences.
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        // Delete PREF_BANDWIDTH and PREF_BANDWIDTH_NO_CELLULAR: just migrated these options.
        if (mSharedPreferences.contains(PREF_BANDWIDTH_OLD)) {
            sharedPreferencesEditor.remove(PREF_BANDWIDTH_OLD);
        }
        if (mSharedPreferences.contains(PREF_BANDWIDTH_NO_CELLULAR_OLD)) {
            sharedPreferencesEditor.remove(PREF_BANDWIDTH_NO_CELLULAR_OLD);
        }
        // Also delete ALLOW_PRERENDER, which was updated based on PREF_BANDWIDTH[_NO_CELLULAR] and
        // network connectivity type, therefore does not carry additional information.
        if (mSharedPreferences.contains(ALLOW_PRERENDER_OLD)) {
            sharedPreferencesEditor.remove(ALLOW_PRERENDER_OLD);
        }
        // Delete bool PREF_NETWORK_PREDICTIONS so that string values can be stored. Note that this
        // SharedPreference carries no information, because it used to be overwritten by
        // kNetworkPredictionEnabled on startup, and now it is overwritten by
        // kNetworkPredictionOptions on startup.
        if (mSharedPreferences.contains(PREF_NETWORK_PREDICTIONS)) {
            sharedPreferencesEditor.remove(PREF_NETWORK_PREDICTIONS);
        }
        sharedPreferencesEditor.apply();
    }

    protected boolean isNetworkAvailable() {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = connectivityManager.getActiveNetworkInfo();
        return (networkInfo != null && networkInfo.isConnected());
    }

    protected boolean isMobileNetworkCapable() {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        // Android telephony team said it is OK to continue using getNetworkInfo() for our purposes.
        // We cannot use ConnectivityManager#getAllNetworks() because that one only reports enabled
        // networks. See crbug.com/532455.
        @SuppressWarnings("deprecation")
        NetworkInfo networkInfo = connectivityManager
                .getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
        return networkInfo != null;
    }

    /**
     * Checks whether prerender should be allowed and updates the preference if it is not set yet.
     * @return Whether prerendering should be allowed.
     */
    public boolean shouldPrerender() {
        if (!DeviceClassManager.enablePrerendering()) return false;
        migrateNetworkPredictionPreferences();
        return PrefServiceBridge.getInstance().canPrefetchAndPrerender();
    }

    /**
     * Sets the usage and crash reporting preference ON or OFF.
     *
     * @param enabled A boolean corresponding whether usage and crash reports uploads are allowed.
     */
    public void setUsageAndCrashReporting(boolean enabled) {
        mSharedPreferences.edit().putBoolean(PREF_METRICS_REPORTING, enabled).apply();
        syncUsageAndCrashReportingPrefs();
        if (!enabled) {
            SurveyController.getInstance().clearCache(ContextUtils.getApplicationContext());
        }
    }

    /**
     * Update usage and crash preferences based on Android preferences if possible in case they are
     * out of sync.
     */
    public void syncUsageAndCrashReportingPrefs() {
        if (PrefServiceBridge.isInitialized()) {
            PrefServiceBridge.getInstance().setMetricsReportingEnabled(
                    isUsageAndCrashReportingPermittedByUser());
        }
    }

    /**
     * Sets whether this client is in-sample for usage metrics and crash reporting. See
     * {@link org.chromium.chrome.browser.metrics.UmaUtils#isClientInMetricsSample} for details.
     */
    public void setClientInMetricsSample(boolean inSample) {
        mSharedPreferences.edit().putBoolean(PREF_METRICS_IN_SAMPLE, inSample).apply();
    }

    /**
     * Checks whether this client is in-sample for usage metrics and crash reporting. See
     * {@link org.chromium.chrome.browser.metrics.UmaUtils#isClientInMetricsSample} for details.
     *
     * @returns boolean Whether client is in-sample.
     */
    @Override
    public boolean isClientInMetricsSample() {
        // The default value is true to avoid sampling out crashes that occur before native code has
        // been initialized on first run. We'd rather have some extra crashes than none from that
        // time.
        return mSharedPreferences.getBoolean(PREF_METRICS_IN_SAMPLE, true);
    }

    /**
     * Checks whether uploading of crash dumps is permitted for the available network(s).
     *
     * @return whether uploading crash dumps is permitted.
     */
    @Override
    public boolean isNetworkAvailableForCrashUploads() {
        ConnectivityManager connectivityManager =
                (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        return NetworkPermissionUtil.isNetworkUnmetered(connectivityManager);
    }

    /**
     * Checks whether uploading of usage metrics and crash dumps is currently permitted, based on
     * user consent only. This doesn't take network condition or experimental state (i.e. disabling
     * upload) into consideration. A crash dump may be retried if this check passes.
     *
     * @return whether the user has consented to reporting usage metrics and crash dumps.
     */
    @Override
    public boolean isUsageAndCrashReportingPermittedByUser() {
        return mSharedPreferences.getBoolean(PREF_METRICS_REPORTING, false);
    }

    /**
     * Check whether the command line switch is used to force uploading if at all possible. Used by
     * test devices to avoid UI manipulation.
     *
     * @return whether uploading should be enabled if at all possible.
     */
    @Override
    public boolean isUploadEnabledForTests() {
        return CommandLine.getInstance().hasSwitch(ChromeSwitches.FORCE_CRASH_DUMP_UPLOAD);
    }

    /**
     * @return Whether uploading usage metrics is currently permitted.
     */
    public boolean isMetricsUploadPermitted() {
        return isNetworkAvailable()
                && (isUsageAndCrashReportingPermittedByUser() || isUploadEnabledForTests());
    }
}
