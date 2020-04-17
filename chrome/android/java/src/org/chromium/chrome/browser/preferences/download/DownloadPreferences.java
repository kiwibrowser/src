// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.download;

import org.chromium.chrome.browser.preferences.ChromeBaseCheckBoxPreference;
import org.chromium.base.Log;
import android.content.ComponentName;
import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.support.annotation.Nullable;
import android.content.Context;
import android.widget.ListView;
import android.content.pm.PackageManager;
import android.content.Intent;
import android.content.Context;
import android.content.pm.ResolveInfo;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import android.net.Uri;
import android.text.TextUtils;
import org.chromium.ui.base.DeviceFormFactor;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.download.DownloadPromptStatus;
import org.chromium.chrome.browser.preferences.ChromeSwitchPreference;
import org.chromium.chrome.browser.preferences.PrefServiceBridge;
import org.chromium.chrome.browser.preferences.PreferenceUtils;

import android.view.View;
import org.chromium.base.ContextUtils;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.widget.ListView;

/**
 * Fragment to keep track of all downloads related preferences.
 */
public class DownloadPreferences
        extends PreferenceFragment implements Preference.OnPreferenceChangeListener {
    public static final String PREF_LOCATION_CHANGE = "location_change";
    private static final String PREF_LOCATION_PROMPT_ENABLED = "location_prompt_enabled";

    private DownloadLocationPreference mLocationChangePref;
    private ChromeBaseCheckBoxPreference mExternalDownloadManager;
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

        mExternalDownloadManager = (ChromeBaseCheckBoxPreference) findPreference("enable_external_download_manager");

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

        if (mExternalDownloadManager != null) {
            mExternalDownloadManager.setOnPreferenceChangeListener(this);
            mExternalDownloadManager.setChecked(ContextUtils.getAppSharedPreferences().getBoolean("enable_external_download_manager", false));
            if (ContextUtils.getAppSharedPreferences().getBoolean("enable_external_download_manager", false)
                  && !TextUtils.isEmpty(ContextUtils.getAppSharedPreferences().getString("selected_external_download_manager_package_name", ""))) {
                mExternalDownloadManager.setSummary(ContextUtils.getAppSharedPreferences().getString("selected_external_download_manager_package_name", ""));
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
            Log.i("Kiwi", "[DownloadPreferences] Received activity result, RQ: " + requestCode);
            if (requestCode == 4242 && resultCode == Activity.RESULT_OK && data != null) {
                 ComponentName componentName = data.getComponent();
                 final String packageName = componentName.getPackageName();
                 final String activityName = componentName.getClassName();
                 Log.i("Kiwi", "[DownloadPreferences] Received activity result, PN: " + packageName + " - AN: " + activityName);
                 SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
                 sharedPreferencesEditor.putString("selected_external_download_manager_package_name", packageName);
                 sharedPreferencesEditor.putString("selected_external_download_manager_activity_name", activityName);
                 sharedPreferencesEditor.apply();
                 updateData();
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
        else if ("enable_external_download_manager".equals(preference.getKey())) {
            SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean("enable_external_download_manager", (boolean)newValue);
            sharedPreferencesEditor.apply();
            if ((boolean)newValue == true) {
                    List<Intent> targetedShareIntents = new ArrayList<Intent>();
                    Intent shareIntent = new Intent(android.content.Intent.ACTION_VIEW, Uri.parse("http://test.com/file.rar"));
                    // Set title and text to share when the user selects an option.
                    shareIntent.putExtra(Intent.EXTRA_TEXT, "http://test.com/file.rar");
                    List<ResolveInfo> resInfo = getActivity().getPackageManager().queryIntentActivities(shareIntent, 0);
                    if (!resInfo.isEmpty()) {
                        for (ResolveInfo info : resInfo) {
                            if (!"com.kiwibrowser.browser".equalsIgnoreCase(info.activityInfo.packageName)) {
                                Intent targetedShare = new Intent(android.content.Intent.ACTION_VIEW);
                                targetedShare.setPackage(info.activityInfo.packageName.toLowerCase(Locale.ROOT));
                                targetedShareIntents.add(targetedShare);
                            }
                        }
                        // Then show the ACTION_PICK_ACTIVITY to let the user select it
                        Intent intentPick = new Intent();
                        intentPick.setAction(Intent.ACTION_PICK_ACTIVITY);
                        // Set the title of the dialog
                        intentPick.putExtra(Intent.EXTRA_TITLE, "Download manager");
                        intentPick.putExtra(Intent.EXTRA_INTENT, shareIntent);
                        intentPick.putExtra(Intent.EXTRA_INITIAL_INTENTS, targetedShareIntents.toArray());
                        // Call StartActivityForResult so we can get the app name selected by the user
                        this.startActivityForResult(intentPick, /* REQUEST_CODE_MY_PICK */ 4242);
                    }
            }
        }
        return true;
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
