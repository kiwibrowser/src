// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.accessibility;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;

import org.chromium.base.ContextUtils;
import org.chromium.base.ObserverList;
import org.chromium.base.ThreadUtils;
import org.chromium.base.SysUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.util.MathUtils;

/**
 * Singleton class for accessing these font size-related preferences:
 *  - User Font Scale Factor: the font scale value that the user sees and can set. This is a value
 *        between 50% and 200% (i.e. 0.5 and 2).
 *  - Font Scale Factor: the font scale factor applied to webpage text during font boosting. This
 *        equals the user font scale factor times the Android system font scale factor, which
 *        reflects the font size indicated in Android settings > Display > Font size.
 *  - Force Enable Zoom: whether force enable zoom is on or off
 *  - User Set Force Enable Zoom: whether the user has manually set the force enable zoom button
 */
public class NightModePrefs {
    @SuppressLint("StaticFieldLeak")
    private static NightModePrefs sNightModePrefs;

    private final long mNightModePrefsAndroidPtr;
    private final Context mApplicationContext;
    private final SharedPreferences mSharedPreferences;

    private final ObserverList<NightModePrefsObserver> mObserverList;

    static final String PREF_USER_NIGHT_MODE_FACTOR = "user_night_mode_factor";
    static final String PREF_USER_NIGHT_MODE_ENABLED = "user_night_mode_enabled";
    static final String PREF_USER_NIGHT_MODE_GRAYSCALE_ENABLED = "user_night_mode_grayscale_enabled";

    /**
     * Interface for observing changes in font size-related preferences.
     */
    public interface NightModePrefsObserver {
        void onNightScaleFactorChanged(float nightScaleFactor, float userNightScaleFactor);
    }

    private NightModePrefs(Context context) {
        mNightModePrefsAndroidPtr = nativeInit();
        mApplicationContext = context.getApplicationContext();
        mSharedPreferences = ContextUtils.getAppSharedPreferences();
        mObserverList = new ObserverList<NightModePrefsObserver>();
    }

    /**
     * Adds an observer to listen for changes to font scale-related preferences.
     */
    public void addObserver(NightModePrefsObserver observer) {
        mObserverList.addObserver(observer);
    }

    /**
     * Removes an observer so it will no longer receive updates for changes to font scale-related
     * preferences.
     */
    public void removeObserver(NightModePrefsObserver observer) {
        mObserverList.removeObserver(observer);
    }

    /**
     * Returns the singleton NightModePrefs, constructing it if it doesn't already exist.
     */
    public static NightModePrefs getInstance(Context context) {
        ThreadUtils.assertOnUiThread();
        if (sNightModePrefs == null) {
            sNightModePrefs = new NightModePrefs(context);
        }
        return sNightModePrefs;
    }

    public void setUserNightModeFactor(float userNightModeFactor) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putFloat(PREF_USER_NIGHT_MODE_FACTOR, userNightModeFactor);
        sharedPreferencesEditor.apply();
        nativeSetNightModeFactor(mNightModePrefsAndroidPtr, userNightModeFactor);
        onNightScaleFactorChanged(userNightModeFactor);
    }

    public void setUserNightModeEnabled(boolean userNightModeEnabled) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_USER_NIGHT_MODE_ENABLED, userNightModeEnabled);
        sharedPreferencesEditor.apply();
        nativeSetNightModeEnabled(mNightModePrefsAndroidPtr, userNightModeEnabled);
    }

    public void setUserNightModeGrayscaleEnabled(boolean userNightModeGrayscaleEnabled) {
        SharedPreferences.Editor sharedPreferencesEditor = mSharedPreferences.edit();
        sharedPreferencesEditor.putBoolean(PREF_USER_NIGHT_MODE_GRAYSCALE_ENABLED, userNightModeGrayscaleEnabled);
        sharedPreferencesEditor.apply();
        nativeSetNightModeGrayscaleEnabled(mNightModePrefsAndroidPtr, userNightModeGrayscaleEnabled);
    }

    private void onNightScaleFactorChanged(float nightScaleFactor) {
        float userNightScaleFactor = getUserNightModeFactor();
        for (NightModePrefsObserver observer : mObserverList) {
            observer.onNightScaleFactorChanged(nightScaleFactor, userNightScaleFactor);
        }
    }

    public float getUserNightModeFactor() {
        float nightFactor = mSharedPreferences.getFloat(PREF_USER_NIGHT_MODE_FACTOR, 0.0f);
        if (nightFactor == 0.0f && SysUtils.firstInstallDate() >= 1549047600)
            return 1.0f;
        if (nightFactor == 0.0f)
            return 0.99f;
        return nightFactor;
    }

    public boolean getUserNightModeEnabled() {
        return mSharedPreferences.getBoolean(PREF_USER_NIGHT_MODE_ENABLED, false);
    }

    private native long nativeInit();
    private native void nativeSetNightModeFactor(long nativeNightModePrefsAndroid,
            float nightModeFactor);
    private native void nativeSetNightModeEnabled(long nativeNightModePrefsAndroid,
            boolean nightModeEnabled);
    private native void nativeSetNightModeGrayscaleEnabled(long nativeNightModePrefsAndroid,
            boolean nightModeGrayscaleEnabled);
}
