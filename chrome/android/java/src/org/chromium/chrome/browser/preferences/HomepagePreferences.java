// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceFragment;

import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.ChromeBaseCheckBoxPreference;
import android.preference.CheckBoxPreference;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.partnercustomizations.HomepageManager;
import org.chromium.chrome.browser.util.FeatureUtilities;

import org.chromium.base.ContextUtils;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.View;
import android.widget.ListView;

/**
 * Fragment that allows the user to configure homepage related preferences.
 */
public class HomepagePreferences extends PreferenceFragment {
    private static final String PREF_HOMEPAGE_SWITCH = "homepage_switch";
    private static final String PREF_HOMEPAGE_EDIT = "homepage_edit";

    private HomepageManager mHomepageManager;
    private ChromeSwitchPreference mHomepageSwitch;
    private Preference mHomepageEdit;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mHomepageManager = HomepageManager.getInstance();

        if (FeatureUtilities.isNewTabPageButtonEnabled()) {
            getActivity().setTitle(R.string.options_startup_page_title);
        } else {
            getActivity().setTitle(R.string.options_homepage_title);
        }
        PreferenceUtils.addPreferencesFromResource(this, R.xml.homepage_preferences);

        mHomepageSwitch = (ChromeSwitchPreference) findPreference(PREF_HOMEPAGE_SWITCH);
        boolean isHomepageEnabled = mHomepageManager.getPrefHomepageEnabled();
        mHomepageSwitch.setChecked(isHomepageEnabled);
        mHomepageSwitch.setOnPreferenceChangeListener(new OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                mHomepageManager.setPrefHomepageEnabled((boolean) newValue);
                return true;
            }
        });

        mHomepageEdit = findPreference(PREF_HOMEPAGE_EDIT);
        updateCurrentHomepageUrl();

    }
    private void updateCurrentHomepageUrl() {
        mHomepageEdit.setSummary(mHomepageManager.getPrefHomepageUseDefaultUri()
                        ? HomepageManager.getDefaultHomepageUri()
                        : mHomepageManager.getPrefHomepageCustomUri());
    }
    @Override
    public void onResume() {
        super.onResume();
        updateCurrentHomepageUrl();
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        if (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")) {
            view.setBackgroundColor(Color.BLACK);
            ListView list = (ListView) view.findViewById(android.R.id.list);
            if (list != null)
                list.setDivider(new ColorDrawable(Color.GRAY));
                list.setDividerHeight((int) getResources().getDisplayMetrics().density);
        }
    }
}
