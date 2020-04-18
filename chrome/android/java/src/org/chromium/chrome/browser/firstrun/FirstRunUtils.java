// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.firstrun;

import android.content.SharedPreferences;
import android.support.v4.app.Fragment;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.metrics.UmaSessionStats;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.signin.AccountAdder;

/** Provides first run related utility functions. */
public class FirstRunUtils {
    public static final String CACHED_TOS_ACCEPTED_PREF = "first_run_tos_accepted";

    /**
     * Synchronizes first run native and Java preferences.
     * Must be called after native initialization.
     */
    public static void cacheFirstRunPrefs() {
        SharedPreferences javaPrefs = ContextUtils.getAppSharedPreferences();
        PrefServiceBridge prefsBridge = PrefServiceBridge.getInstance();
        // Set both Java and native prefs if any of the three indicators indicate ToS has been
        // accepted. This needed because:
        //   - Old versions only set native pref, so this syncs Java pref.
        //   - Backup & restore does not restore native pref, so this needs to update it.
        //   - checkAnyUserHasSeenToS() may be true which needs to sync its state to the prefs.
        boolean javaPrefValue = javaPrefs.getBoolean(CACHED_TOS_ACCEPTED_PREF, false);
        boolean nativePrefValue = prefsBridge.isFirstRunEulaAccepted();
        boolean userHasSeenTos =
                ToSAckedReceiver.checkAnyUserHasSeenToS();
        boolean isFirstRunComplete = FirstRunStatus.getFirstRunFlowComplete();
        if (javaPrefValue || nativePrefValue || userHasSeenTos || isFirstRunComplete) {
            if (!javaPrefValue) {
                javaPrefs.edit().putBoolean(CACHED_TOS_ACCEPTED_PREF, true).apply();
            }
            if (!nativePrefValue) {
                prefsBridge.setEulaAccepted();
            }
        }
    }

    /**
     * @return Whether the user has accepted Chrome Terms of Service.
     */
    public static boolean didAcceptTermsOfService() {
        // Note: Does not check PrefServiceBridge.getInstance().isFirstRunEulaAccepted()
        // because this may be called before native is initialized.
        return ContextUtils.getAppSharedPreferences().getBoolean(CACHED_TOS_ACCEPTED_PREF, false)
                || ToSAckedReceiver.checkAnyUserHasSeenToS();
    }

    /**
     * Sets the EULA/Terms of Services state as "ACCEPTED".
     * @param allowCrashUpload True if the user allows to upload crash dumps and collect stats.
     */
    public static void acceptTermsOfService(boolean allowCrashUpload) {
        UmaSessionStats.changeMetricsReportingConsent(allowCrashUpload);
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putBoolean(CACHED_TOS_ACCEPTED_PREF, true)
                .apply();
        PrefServiceBridge.getInstance().setEulaAccepted();
    }

    /**
     * Opens the Android account adder UI.
     * @param fragment A fragment that requested the service.
     */
    public static void openAccountAdder(Fragment fragment) {
        AccountAdder.getInstance().addAccount(fragment, AccountAdder.ADD_ACCOUNT_RESULT);
    }
}
