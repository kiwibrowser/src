// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.annotation.Nullable;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DownloadPromptStatus;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreference;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

/**
 * Fragment to keep track of all downloads related preferences.
 */
public class DownloadPreferences
        extends PreferenceFragment implements Preference.OnPreferenceChangeListener {
    public static final String PREF_LOCATION_CHANGE = "location_change";
    private static final String PREF_LOCATION_PROMPT_ENABLED = "location_prompt_enabled";

    private DownloadLocationPreference mLocationChangePref;
    private ChromeSwitchPreference mLocationPromptEnabledPref;

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getActivity().setTitle(R.string.menu_downloads);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.download_preferences);

        mLocationPromptEnabledPref =
                (ChromeSwitchPreference) findPreference(PREF_LOCATION_PROMPT_ENABLED);
        mLocationPromptEnabledPref.setOnPreferenceChangeListener(this);

        mLocationChangePref = (DownloadLocationPreference) findPreference(PREF_LOCATION_CHANGE);

        updateData();
    }

    @Override
    public void onResume() {
        super.onResume();
        updateData();
    }

    private void updateData() {
        if (mLocationChangePref != null) {
            mLocationChangePref.updateSummary();
        }

        if (mLocationPromptEnabledPref != null) {
            // Location prompt is marked enabled if the prompt status is not DONT_SHOW.
            boolean isLocationPromptEnabled =
                    PrefServiceBridge.getInstance().getPromptForDownloadAndroid()
                    != DownloadPromptStatus.DONT_SHOW;
            mLocationPromptEnabledPref.setChecked(isLocationPromptEnabled);
        }
    }

    // Preference.OnPreferenceChangeListener implementation.

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (PREF_LOCATION_PROMPT_ENABLED.equals(preference.getKey())) {
            if ((boolean) newValue) {
                // Only update if the interstitial has been shown before.
                if (PrefServiceBridge.getInstance().getPromptForDownloadAndroid()
                        != DownloadPromptStatus.SHOW_INITIAL) {
                    PrefServiceBridge.getInstance().setPromptForDownloadAndroid(
                            DownloadPromptStatus.SHOW_PREFERENCE);
                }
            } else {
                PrefServiceBridge.getInstance().setPromptForDownloadAndroid(
                        DownloadPromptStatus.DONT_SHOW);
            }
        }
        return true;
    }
}
