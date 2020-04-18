// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import android.annotation.SuppressLint;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.database.ContentObserver;
import android.os.Build;
import android.os.Handler;
import android.provider.Settings;

import org.chromium.base.Log;

/**
 * Manager for Cast settings.
 */
public final class CastSettingsManager {
    private static final String TAG = "cr_CastSettingsManager";

    private static final String PREFS_FILE_NAME = "CastSettings";

    /** Key for the "send usage stats" boolean setting. */
    private static final String SEND_USAGE_STATS_SETTING = "developer_support";

    /** The default value for the "send usage stats" setting. */
    private static final boolean SEND_USAGE_STATS_SETTING_DEFAULT = true;

    /** The default device name, which is the model name. */
    private static final String DEFAULT_DEVICE_NAME = Build.MODEL;

    // TODO(gunsch): Switch to Settings.Global.DEVICE_NAME once it's in public SDK.
    // private static final String DEVICE_NAME_SETTING_KEY = Settings.Global.DEVICE_NAME;
    private static final String DEVICE_NAME_SETTING_KEY = "device_name";
    private static final String DEVICE_PROVISIONED_SETTING_KEY = Settings.Global.DEVICE_PROVISIONED;

    private final ContentResolver mContentResolver;
    private final SharedPreferences mSettings;
    private SharedPreferences.OnSharedPreferenceChangeListener mSharedPreferenceListener;
    private ContentObserver mDeviceNameObserver;
    private ContentObserver mIsDeviceProvisionedObserver;

    /**
     * Can be implemented to receive notifications from a CastSettingsManager instance when
     * settings have changed.
     */
    public static class OnSettingChangedListener {
        public void onCastEnabledChanged(boolean enabled) {}
        public void onSendUsageStatsChanged(boolean enabled) {}
        public void onDeviceNameChanged(String deviceName) {}
    }

    private OnSettingChangedListener mListener;

    /**
     * Creates a fully-featured CastSettingsManager instance. Will fail if called from a
     * sandboxed process.
     */
    public static CastSettingsManager createCastSettingsManager(
            Context context, OnSettingChangedListener listener) {
        ContentResolver contentResolver = context.getContentResolver();
        SharedPreferences settings = context.getSharedPreferences(PREFS_FILE_NAME, 0);
        return new CastSettingsManager(contentResolver, listener, settings);
    }

    @SuppressLint("NewApi")
    private CastSettingsManager(
            ContentResolver contentResolver,
            OnSettingChangedListener listener,
            SharedPreferences settings) {
        mContentResolver = contentResolver;
        mSettings = settings;
        mListener = listener;

        mSharedPreferenceListener = new SharedPreferences.OnSharedPreferenceChangeListener() {
            @Override
            public void onSharedPreferenceChanged(SharedPreferences prefs, String key) {
                if (mListener == null) {
                    return;
                }

                if (key.equals(SEND_USAGE_STATS_SETTING)) {
                    mListener.onSendUsageStatsChanged(prefs.getBoolean(key,
                                    SEND_USAGE_STATS_SETTING_DEFAULT));
                }
            }
        };
        mSettings.registerOnSharedPreferenceChangeListener(mSharedPreferenceListener);

        mDeviceNameObserver = new ContentObserver(new Handler()) {
            @Override
            public void onChange(boolean selfChange) {
                mListener.onDeviceNameChanged(getDeviceName());
            }
        };
        // TODO(crbug.com/635567): Fix lint properly.
        mContentResolver.registerContentObserver(
                Settings.Global.getUriFor(DEVICE_NAME_SETTING_KEY), true, mDeviceNameObserver);

        if (!isCastEnabled()) {
            mIsDeviceProvisionedObserver = new ContentObserver(new Handler()) {
                @Override
                public void onChange(boolean selfChange) {
                    Log.d(TAG, "Device provisioned");
                    mListener.onCastEnabledChanged(isCastEnabled());
                }
            };
            // TODO(crbug.com/635567): Fix lint properly.
            mContentResolver.registerContentObserver(
                    Settings.Global.getUriFor(DEVICE_PROVISIONED_SETTING_KEY), true,
                    mIsDeviceProvisionedObserver);
        }
    }

    public void dispose() {
        mSettings.unregisterOnSharedPreferenceChangeListener(mSharedPreferenceListener);
        mSharedPreferenceListener = null;
        mContentResolver.unregisterContentObserver(mDeviceNameObserver);
        mDeviceNameObserver = null;

        if (mIsDeviceProvisionedObserver != null) {
            mContentResolver.unregisterContentObserver(mIsDeviceProvisionedObserver);
            mIsDeviceProvisionedObserver = null;
        }
    }

    @SuppressLint("NewApi")
    public boolean isCastEnabled() {
        // However, Cast is disabled until the device is provisioned (see b/18950240).
        // TODO(crbug.com/635567): Fix lint properly.
        return Settings.Global.getInt(
                mContentResolver, DEVICE_PROVISIONED_SETTING_KEY, 0) == 1;
    }

    public boolean isSendUsageStatsEnabled() {
        return mSettings.getBoolean(SEND_USAGE_STATS_SETTING, SEND_USAGE_STATS_SETTING_DEFAULT);
    }

    public void setSendUsageStatsEnabled(boolean enabled) {
        mSettings.edit().putBoolean(SEND_USAGE_STATS_SETTING, enabled).apply();
    }

    @SuppressLint("NewApi")
    public String getDeviceName() {
        // TODO(crbug.com/635567): Fix lint properly.
        String deviceName = Settings.Global.getString(mContentResolver, DEVICE_NAME_SETTING_KEY);
        return (deviceName != null) ? deviceName : DEFAULT_DEVICE_NAME;
    }

}
