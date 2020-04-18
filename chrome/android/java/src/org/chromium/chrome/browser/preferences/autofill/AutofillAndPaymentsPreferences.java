// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceFragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.autofill.PersonalDataManager;
import org.chromium.chrome.browser.payments.AndroidPaymentAppFactory;
import org.chromium.chrome.browser.payments.ServiceWorkerPaymentAppBridge;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreference;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

/**
 * Autofill and payments settings fragment, which allows the user to edit autofill and credit card
 * profiles and control payment apps.
 */
public class AutofillAndPaymentsPreferences extends PreferenceFragment {
    public static final String AUTOFILL_GUID = "guid";

    // Needs to be in sync with kSettingsOrigin[] in
    // chrome/browser/ui/webui/options/autofill_options_handler.cc
    public static final String SETTINGS_ORIGIN = "Chrome settings";
    private static final String PREF_AUTOFILL_SWITCH = "autofill_switch";
    private static final String PREF_PAYMENT_APPS = "payment_apps";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.autofill_and_payments_preferences);
        getActivity().setTitle(R.string.prefs_autofill_and_payments);

        ChromeSwitchPreference autofillSwitch =
                (ChromeSwitchPreference) findPreference(PREF_AUTOFILL_SWITCH);
        autofillSwitch.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                PersonalDataManager.setAutofillEnabled((boolean) newValue);
                return true;
            }
        });

        if (ChromeFeatureList.isEnabled(ChromeFeatureList.ANDROID_PAYMENT_APPS)
                || ChromeFeatureList.isEnabled(ChromeFeatureList.SERVICE_WORKER_PAYMENT_APPS)) {
            Preference pref = new Preference(getActivity());
            pref.setTitle(getActivity().getString(R.string.payment_apps_title));
            pref.setFragment(AndroidPaymentAppsFragment.class.getCanonicalName());
            pref.setShouldDisableView(true);
            pref.setKey(PREF_PAYMENT_APPS);
            getPreferenceScreen().addPreference(pref);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        ((ChromeSwitchPreference) findPreference(PREF_AUTOFILL_SWITCH))
                .setChecked(PersonalDataManager.isAutofillEnabled());

        Preference pref = findPreference(PREF_PAYMENT_APPS);
        if (pref != null) {
            refreshPaymentAppsPrefForAndroidPaymentApps(pref);
        }
    }

    private void refreshPaymentAppsPrefForAndroidPaymentApps(Preference pref) {
        if (AndroidPaymentAppFactory.hasAndroidPaymentApps()) {
            setPaymentAppsPrefStatus(pref, true);
        } else {
            refreshPaymentAppsPrefForServiceWorkerPaymentApps(pref);
        }
    }

    private void refreshPaymentAppsPrefForServiceWorkerPaymentApps(Preference pref) {
        ServiceWorkerPaymentAppBridge.hasServiceWorkerPaymentApps(
                new ServiceWorkerPaymentAppBridge.HasServiceWorkerPaymentAppsCallback() {
                    @Override
                    public void onHasServiceWorkerPaymentAppsResponse(boolean hasPaymentApps) {
                        setPaymentAppsPrefStatus(pref, hasPaymentApps);
                    }
                });
    }

    private void setPaymentAppsPrefStatus(Preference pref, boolean enabled) {
        if (enabled) {
            pref.setSummary(null);
            pref.setEnabled(true);
        } else {
            pref.setSummary(getActivity().getString(R.string.payment_no_apps_summary));
            pref.setEnabled(false);
        }
    }
}
