/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.wifi;

import android.text.TextUtils;
import android.util.Log;

/**
 * Provide functions for making changes to WiFi country code.
 * This Country Code is from MCC or phone default setting. This class sends Country Code
 * to driver through wpa_supplicant when WifiStateMachine marks current state as ready
 * using setReadyForChange(true).
 */
public class WifiCountryCode {
    private static final String TAG = "WifiCountryCode";
    private final WifiNative mWifiNative;
    private boolean DBG = false;
    private boolean mReady = false;

    /** config option that indicate whether or not to reset country code to default when
     * cellular radio indicates country code loss
     */
    private boolean mRevertCountryCodeOnCellularLoss;
    private String mDefaultCountryCode = null;
    private String mTelephonyCountryCode = null;
    private String mCurrentCountryCode = null;

    public WifiCountryCode(
            WifiNative wifiNative,
            String oemDefaultCountryCode,
            String persistentCountryCode,
            boolean revertCountryCodeOnCellularLoss) {

        mWifiNative = wifiNative;
        mRevertCountryCodeOnCellularLoss = revertCountryCodeOnCellularLoss;

        if (!TextUtils.isEmpty(persistentCountryCode)) {
            mDefaultCountryCode = persistentCountryCode.toUpperCase();
        } else if (!TextUtils.isEmpty(oemDefaultCountryCode)) {
            mDefaultCountryCode = oemDefaultCountryCode.toUpperCase();
        } else {
            if (mRevertCountryCodeOnCellularLoss) {
                Log.w(TAG, "config_wifi_revert_country_code_on_cellular_loss is set, "
                         + "but there is no default country code.");
                mRevertCountryCodeOnCellularLoss = false;
                return;
            }
        }

        if (mRevertCountryCodeOnCellularLoss) {
            Log.d(TAG, "Country code will be reverted to " + mDefaultCountryCode
                    + " on MCC loss");
        }
    }

    /**
     * Enable verbose logging for WifiCountryCode.
     */
    public void enableVerboseLogging(int verbose) {
        if (verbose > 0) {
            DBG = true;
        } else {
            DBG = false;
        }
    }

    /**
     * This is called when sim card is removed.
     * In this case we should invalid all other country codes except the
     * phone default one.
     */
    public synchronized void simCardRemoved() {
        if (DBG) Log.d(TAG, "SIM Card Removed");
        // SIM card is removed, we need to reset the country code to phone default.
        if (mRevertCountryCodeOnCellularLoss) {
            mTelephonyCountryCode = null;
            if (mReady) {
                updateCountryCode();
            }
        }
    }

    /**
     * This is called when airplane mode is enabled.
     * In this case we should invalidate all other country code except the
     * phone default one.
     */
    public synchronized void airplaneModeEnabled() {
        if (DBG) Log.d(TAG, "Airplane Mode Enabled");
        mTelephonyCountryCode = null;
        // Airplane mode is enabled, we need to reset the country code to phone default.
        if (mRevertCountryCodeOnCellularLoss) {
            mTelephonyCountryCode = null;
            // Country code will be set upon when wpa_supplicant starts next time.
        }
    }

    /**
     * Change the state to indicates if wpa_supplicant is ready to handle country code changing
     * request or not.
     * We call native code to request country code changes only when wpa_supplicant is
     * started but not yet L2 connected.
     */
    public synchronized void setReadyForChange(boolean ready) {
        if (DBG) Log.d(TAG, "Set ready: " + ready);
        mReady = ready;
        // We are ready to set country code now.
        // We need to post pending country code request.
        if (mReady) {
            updateCountryCode();
        }
    }

    /**
     * Handle country code change request.
     * @param countryCode The country code intended to set.
     * This is supposed to be from Telephony service.
     * otherwise we think it is from other applications.
     * @return Returns true if the country code passed in is acceptable.
     */
    public synchronized boolean setCountryCode(String countryCode, boolean persist) {
        if (DBG) Log.d(TAG, "Receive set country code request: " + countryCode);
        // Ignore empty country code.
        if (TextUtils.isEmpty(countryCode)) {
            if (DBG) Log.d(TAG, "Ignore empty country code");
            return false;
        }
        if (persist) {
            mDefaultCountryCode = countryCode;
        }
        mTelephonyCountryCode = countryCode.toUpperCase();
        // If wpa_supplicant is ready we set the country code now, otherwise it will be
        // set once wpa_supplicant is ready.
        if (mReady) {
            updateCountryCode();
        }
        return true;
    }

    /**
     * Method to get the Country Code that was sent to wpa_supplicant.
     *
     * @return Returns the local copy of the Country Code that was sent to the driver upon
     * setReadyForChange(true).
     * If wpa_supplicant was never started, this may be null even if a SIM reported a valid
     * country code.
     * Returns null if no Country Code was sent to driver.
     */
    public synchronized String getCountryCodeSentToDriver() {
        return mCurrentCountryCode;
    }

    /**
     * Method to return the currently reported Country Code from the SIM or phone default setting.
     *
     * @return The currently reported Country Code from the SIM. If there is no Country Code
     * reported from SIM, a phone default Country Code will be returned.
     * Returns null when there is no Country Code available.
     */
    public synchronized String getCountryCode() {
        return pickCountryCode();
    }

    private void updateCountryCode() {
        if (DBG) Log.d(TAG, "Update country code");
        String country = pickCountryCode();
        // We do not check if the country code equals the current one.
        // There are two reasons:
        // 1. Wpa supplicant may silently modify the country code.
        // 2. If Wifi restarted therefoere wpa_supplicant also restarted,
        // the country code counld be reset to '00' by wpa_supplicant.
        if (country != null) {
            setCountryCodeNative(country);
        }
        // We do not set country code if there is no candidate. This is reasonable
        // because wpa_supplicant usually starts with an international safe country
        // code setting: '00'.
    }

    private String pickCountryCode() {
        if (mTelephonyCountryCode != null) {
            return mTelephonyCountryCode;
        }
        if (mDefaultCountryCode != null) {
            return mDefaultCountryCode;
        }
        // If there is no candidate country code we will return null.
        return null;
    }

    private boolean setCountryCodeNative(String country) {
        if (mWifiNative.setCountryCode(country)) {
            Log.d(TAG, "Succeeded to set country code to: " + country);
            mCurrentCountryCode = country;
            return true;
        }
        Log.d(TAG, "Failed to set country code to: " + country);
        return false;
    }
}

