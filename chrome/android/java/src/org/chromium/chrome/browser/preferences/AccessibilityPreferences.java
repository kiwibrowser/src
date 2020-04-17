// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences;

import android.os.Bundle;
import android.content.SharedPreferences;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceFragment;
import android.content.Context;
import android.widget.ListView;
import android.content.pm.PackageManager;
import android.content.Intent;
import android.content.Context;
import android.content.pm.ResolveInfo;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import android.util.Log;
import android.net.Uri;
import org.chromium.ui.base.DeviceFormFactor;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.RestartWorker;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.accessibility.FontSizePrefs;
import org.chromium.chrome.browser.accessibility.NightModePrefs;
import org.chromium.chrome.browser.accessibility.FontSizePrefs.FontSizePrefsObserver;
import org.chromium.chrome.browser.accessibility.NightModePrefs.NightModePrefsObserver;
import org.chromium.base.ContextUtils;
import org.chromium.ui.widget.Toast;
import org.chromium.chrome.browser.util.AccessibilityUtil;

import android.view.View;
import android.content.DialogInterface;
import android.support.v7.app.AlertDialog;
import java.text.NumberFormat;

import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;

import android.widget.ListView;
import org.chromium.base.ContextUtils;

/**
 * Fragment to keep track of all the accessibility related preferences.
 */
public class AccessibilityPreferences extends PreferenceFragment
        implements OnPreferenceChangeListener {

    static final String PREF_TEXT_SCALE = "text_scale";
    static final String PREF_NIGHT_MODE = "night_scale";
    static final String PREF_FORCE_ENABLE_ZOOM = "force_enable_zoom";
    static final String PREF_READER_FOR_ACCESSIBILITY = "reader_for_accessibility";
    static final String PREF_OPEN_IN_EXTERNAL_APP = "open_in_external_app";
    static final String PREF_ENABLE_BOTTOM_TOOLBAR = "enable_bottom_toolbar";

    private NumberFormat mFormat;
    private FontSizePrefs mFontSizePrefs;
    private NightModePrefs mNightModePrefs;

    private TextScalePreference mTextScalePref;
    private NightScalePreference mNightScalePref;
    private ChromeBaseCheckBoxPreference mNightGrayscalePref;
    private SeekBarLinkedCheckBoxPreference mForceEnableZoomPref;
    private ChromeBaseCheckBoxPreference mReaderForAccessibilityPref;
    private ChromeBaseCheckBoxPreference mBottomToolbar;
    private ChromeBaseCheckBoxPreference mOverscrollButton;
    private ChromeBaseCheckBoxPreference mTextRewrap;
    private ChromeBaseCheckBoxPreference mShowExtensionsFirst;
    private ChromeBaseCheckBoxPreference mAccessibilityTabSwitcherPref;
    private ChromeBaseCheckBoxPreference mDesktopMode;
    private ChromeBaseCheckBoxPreference mKeepToolbar;

    private FontSizePrefsObserver mFontSizePrefsObserver = new FontSizePrefsObserver() {
        @Override
        public void onFontScaleFactorChanged(float fontScaleFactor, float userFontScaleFactor) {
            updateTextScaleSummary(userFontScaleFactor);
        }

        @Override
        public void onForceEnableZoomChanged(boolean enabled) {
            mForceEnableZoomPref.setChecked(enabled);
        }
    };

    private NightModePrefsObserver mNightModePrefsObserver = new NightModePrefsObserver() {
        @Override
        public void onNightScaleFactorChanged(float nightScaleFactor, float userNightScaleFactor) {
            Context context = ContextUtils.getApplicationContext();

            Toast.makeText(context, "Night Mode contrast set to " + (userNightScaleFactor * 100), Toast.LENGTH_SHORT).show();

            updateNightScaleSummary(userNightScaleFactor);
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.prefs_accessibility);
        PreferenceUtils.addPreferencesFromResource(this, R.xml.accessibility_preferences);

        mFormat = NumberFormat.getPercentInstance();
        mFontSizePrefs = FontSizePrefs.getInstance(getActivity());
        mNightModePrefs = NightModePrefs.getInstance(getActivity());

        mTextScalePref = (TextScalePreference) findPreference(PREF_TEXT_SCALE);
        mTextScalePref.setOnPreferenceChangeListener(this);

        mNightScalePref = (NightScalePreference) findPreference(PREF_NIGHT_MODE);
        mNightScalePref.setOnPreferenceChangeListener(this);

        mNightGrayscalePref = (ChromeBaseCheckBoxPreference) findPreference("nightmode_grayscale");
        mNightGrayscalePref.setOnPreferenceChangeListener(this);

        ((ChromeBaseCheckBoxPreference) findPreference("side_swipe_mode_enabled")).setOnPreferenceChangeListener(this);
        ((ChromeBaseCheckBoxPreference) findPreference("side_swipe_mode_enabled")).setChecked(ContextUtils.getAppSharedPreferences().getBoolean("side_swipe_mode_enabled", true));

        ((ChromeBaseCheckBoxPreference) findPreference("up_swipe_mode_enabled")).setOnPreferenceChangeListener(this);

        mForceEnableZoomPref = (SeekBarLinkedCheckBoxPreference) findPreference(
                PREF_FORCE_ENABLE_ZOOM);
        mForceEnableZoomPref.setOnPreferenceChangeListener(this);
        mForceEnableZoomPref.setLinkedSeekBarPreference(mTextScalePref);

        mReaderForAccessibilityPref =
                (ChromeBaseCheckBoxPreference) findPreference(PREF_READER_FOR_ACCESSIBILITY);
        if (ChromeFeatureList.isEnabled(ChromeFeatureList.ALLOW_READER_FOR_ACCESSIBILITY)) {
            mReaderForAccessibilityPref.setChecked(PrefServiceBridge.getInstance().getBoolean(
                    Pref.READER_FOR_ACCESSIBILITY_ENABLED));
            mReaderForAccessibilityPref.setOnPreferenceChangeListener(this);
        } else {
            getPreferenceScreen().removePreference(mReaderForAccessibilityPref);
        }

        mAccessibilityTabSwitcherPref = (ChromeBaseCheckBoxPreference) findPreference(
                ChromePreferenceManager.ACCESSIBILITY_TAB_SWITCHER);
        if (AccessibilityUtil.isAccessibilityEnabled()) {
            mAccessibilityTabSwitcherPref.setChecked(
                    ChromePreferenceManager.getInstance().readBoolean(
                            ChromePreferenceManager.ACCESSIBILITY_TAB_SWITCHER, true));
        }
        mBottomToolbar = (ChromeBaseCheckBoxPreference) findPreference("enable_bottom_toolbar");
        mBottomToolbar.setOnPreferenceChangeListener(this);
        if (DeviceFormFactor.isTablet()) {
             this.getPreferenceScreen().removePreference(mBottomToolbar);
        }
        mOverscrollButton = (ChromeBaseCheckBoxPreference) findPreference("enable_overscroll_button");
        mOverscrollButton.setOnPreferenceChangeListener(this);
        mOverscrollButton.setChecked(ContextUtils.getAppSharedPreferences().getBoolean("enable_overscroll_button", true));
        if (DeviceFormFactor.isTablet()) {
             this.getPreferenceScreen().removePreference(mOverscrollButton);
        }

        mTextRewrap = (ChromeBaseCheckBoxPreference) findPreference("text_rewrap");
        mTextRewrap.setOnPreferenceChangeListener(this);
        mTextRewrap.setChecked(ContextUtils.getAppSharedPreferences().getBoolean("text_rewrap", false));

        mShowExtensionsFirst = (ChromeBaseCheckBoxPreference) findPreference("show_extensions_first");
        mShowExtensionsFirst.setOnPreferenceChangeListener(this);
        mShowExtensionsFirst.setChecked(ContextUtils.getAppSharedPreferences().getBoolean("show_extensions_first", false));

        mDesktopMode = (ChromeBaseCheckBoxPreference) findPreference("desktop_mode");
        mDesktopMode.setOnPreferenceChangeListener(this);
        mDesktopMode.setChecked(PrefServiceBridge.getInstance().desktopModeEnabled());

        mKeepToolbar = (ChromeBaseCheckBoxPreference) findPreference("keep_toolbar_visible");
        mKeepToolbar.setOnPreferenceChangeListener(this);
        String KeepToolbarSetting = ContextUtils.getAppSharedPreferences().getString("keep_toolbar_visible_configuration", "unknown");
        if (KeepToolbarSetting.equals("unknown")) {
          if (AccessibilityUtil.isAccessibilityEnabled())
            mKeepToolbar.setChecked(true);
          else
            mKeepToolbar.setChecked(false);
        } else if (KeepToolbarSetting.equals("on")) {
            mKeepToolbar.setChecked(true);
        } else {
            mKeepToolbar.setChecked(false);
        }
        if (DeviceFormFactor.isTablet()) {
             this.getPreferenceScreen().removePreference(mKeepToolbar);
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        ((ListView) getView().findViewById(android.R.id.list)).setItemsCanFocus(true);
        ((ListView) getView().findViewById(android.R.id.list)).setDivider(null);
    }

    @Override
    public void onStart() {
        super.onStart();
        updateValues();
        mTextScalePref.startObservingFontPrefs();
        mFontSizePrefs.addObserver(mFontSizePrefsObserver);
        mNightModePrefs.addObserver(mNightModePrefsObserver);
    }

    @Override
    public void onStop() {
        mTextScalePref.stopObservingFontPrefs();
        mFontSizePrefs.removeObserver(mFontSizePrefsObserver);
        mNightModePrefs.removeObserver(mNightModePrefsObserver);
        super.onStop();
    }

    private void updateValues() {
        float userFontScaleFactor = mFontSizePrefs.getUserFontScaleFactor();
        mTextScalePref.setValue(userFontScaleFactor);
        updateTextScaleSummary(userFontScaleFactor);

        float userNightScaleFactor = mNightModePrefs.getUserNightModeFactor();
        mNightScalePref.setValue(userNightScaleFactor);
        updateNightScaleSummary(userNightScaleFactor);

        mForceEnableZoomPref.setChecked(mFontSizePrefs.getForceEnableZoom());
    }

    private void updateTextScaleSummary(float userFontScaleFactor) {
        mTextScalePref.setSummary(mFormat.format(userFontScaleFactor));
    }

    private void updateNightScaleSummary(float userNightScaleFactor) {
        mNightScalePref.setSummary(mFormat.format(userNightScaleFactor));
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

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (PREF_TEXT_SCALE.equals(preference.getKey())) {
            mFontSizePrefs.setUserFontScaleFactor((Float) newValue);
        } else if (PREF_NIGHT_MODE.equals(preference.getKey())) {
            mNightModePrefs.setUserNightModeFactor((Float) newValue);
        } else if ("nightmode_grayscale".equals(preference.getKey())) {
            mNightModePrefs.setUserNightModeGrayscaleEnabled((Boolean) newValue);
        } else if ("side_swipe_mode_enabled".equals(preference.getKey())) {
            SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean("side_swipe_mode_enabled", (boolean)newValue);
            sharedPreferencesEditor.apply();
            AskForRelaunch();
        } else if ("up_swipe_mode_enabled".equals(preference.getKey())) {
            SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean("up_swipe_mode_enabled", (boolean)newValue);
            sharedPreferencesEditor.apply();
        } else if ("enable_overscroll_button".equals(preference.getKey())) {
            SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean("enable_overscroll_button", (boolean)newValue);
            sharedPreferencesEditor.apply();
            AskForRelaunch();
        } else if ("keep_toolbar_visible".equals(preference.getKey())) {
            SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
            if ((boolean)newValue)
              sharedPreferencesEditor.putString("keep_toolbar_visible_configuration", "on");
            else
              sharedPreferencesEditor.putString("keep_toolbar_visible_configuration", "off");
            sharedPreferencesEditor.apply();
        } else if ("text_rewrap".equals(preference.getKey())) {
            SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean("text_rewrap", (boolean)newValue);
            sharedPreferencesEditor.apply();
            AskForRelaunch();
        } else if ("desktop_mode".equals(preference.getKey())) {
            PrefServiceBridge.getInstance().setDesktopModeEnabled((boolean)newValue);
        } else if ("show_extensions_first".equals(preference.getKey())) {
            SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean("show_extensions_first", (boolean)newValue);
            sharedPreferencesEditor.apply();
        } else if (PREF_FORCE_ENABLE_ZOOM.equals(preference.getKey())) {
            mFontSizePrefs.setForceEnableZoomFromUser((Boolean) newValue);
        } else if (PREF_READER_FOR_ACCESSIBILITY.equals(preference.getKey())) {
            PrefServiceBridge.getInstance().setBoolean(
                    Pref.READER_FOR_ACCESSIBILITY_ENABLED, (Boolean) newValue);
        } else if (PREF_OPEN_IN_EXTERNAL_APP.equals(preference.getKey())) {
            SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean(PREF_OPEN_IN_EXTERNAL_APP, (boolean)newValue);
            sharedPreferencesEditor.apply();
        } else if (PREF_ENABLE_BOTTOM_TOOLBAR.equals(preference.getKey())) {
            SharedPreferences.Editor sharedPreferencesEditor = ContextUtils.getAppSharedPreferences().edit();
            sharedPreferencesEditor.putBoolean(PREF_ENABLE_BOTTOM_TOOLBAR, (boolean)newValue);
            sharedPreferencesEditor.apply();
            AskForRelaunch();
        }
        return true;
    }

    private void AskForRelaunch() {
        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this.getActivity());
         alertDialogBuilder
            .setMessage(R.string.preferences_restart_is_needed)
            .setCancelable(true)
            .setPositiveButton(R.string.preferences_restart_now, new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dialog,int id) {
                  RestartWorker restartWorker = new RestartWorker();
                  restartWorker.Restart();
                  dialog.cancel();
              }
            })
            .setNegativeButton(R.string.preferences_restart_later,new DialogInterface.OnClickListener() {
              @Override
              public void onClick(DialogInterface dialog,int id) {
                  dialog.cancel();
              }
            });
            AlertDialog alertDialog = alertDialogBuilder.create();
            alertDialog.show();
    }
}
