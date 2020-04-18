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

package com.android.server.wifi.util;

import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiScanner;
import android.util.Log;

import com.android.server.wifi.WifiNative;

import java.util.ArrayList;
import java.util.Random;

/**
 * Provide utility functions for updating soft AP related configuration.
 */
public class ApConfigUtil {
    private static final String TAG = "ApConfigUtil";

    public static final int DEFAULT_AP_BAND = WifiConfiguration.AP_BAND_2GHZ;
    public static final int DEFAULT_AP_CHANNEL = 6;

    /* Return code for updateConfiguration. */
    public static final int SUCCESS = 0;
    public static final int ERROR_NO_CHANNEL = 1;
    public static final int ERROR_GENERIC = 2;

    /* Random number generator used for AP channel selection. */
    private static final Random sRandom = new Random();

    /**
     * Convert frequency to channel.
     * @param frequency frequency to convert
     * @return channel number associated with given frequency, -1 if no match
     */
    public static int convertFrequencyToChannel(int frequency) {
        if (frequency >= 2412 && frequency <= 2472) {
            return (frequency - 2412) / 5 + 1;
        } else if (frequency == 2484) {
            return 14;
        } else if (frequency >= 5170  &&  frequency <= 5825) {
            /* DFS is included. */
            return (frequency - 5170) / 5 + 34;
        }

        return -1;
    }

    /**
     * Return a channel number for AP setup based on the frequency band.
     * @param apBand 0 for 2GHz, 1 for 5GHz
     * @param allowed2GChannels list of allowed 2GHz channels
     * @param allowed5GFreqList list of allowed 5GHz frequencies
     * @return a valid channel number on success, -1 on failure.
     */
    public static int chooseApChannel(int apBand,
                                      ArrayList<Integer> allowed2GChannels,
                                      int[] allowed5GFreqList) {
        if (apBand != WifiConfiguration.AP_BAND_2GHZ
                && apBand != WifiConfiguration.AP_BAND_5GHZ) {
            Log.e(TAG, "Invalid band: " + apBand);
            return -1;
        }

        if (apBand == WifiConfiguration.AP_BAND_2GHZ)  {
            /* Select a channel from 2GHz band. */
            if (allowed2GChannels == null || allowed2GChannels.size() == 0) {
                Log.d(TAG, "2GHz allowed channel list not specified");
                /* Use default channel. */
                return DEFAULT_AP_CHANNEL;
            }

            /* Pick a random channel. */
            int index = sRandom.nextInt(allowed2GChannels.size());
            return allowed2GChannels.get(index).intValue();
        }

        /* 5G without DFS. */
        if (allowed5GFreqList != null && allowed5GFreqList.length > 0) {
            /* Pick a random channel from the list of supported channels. */
            return convertFrequencyToChannel(
                    allowed5GFreqList[sRandom.nextInt(allowed5GFreqList.length)]);
        }

        Log.e(TAG, "No available channels on 5GHz band");
        return -1;
    }

    /**
     * Update AP band and channel based on the provided country code and band.
     * This will also set
     * @param wifiNative reference to WifiNative
     * @param countryCode country code
     * @param allowed2GChannels list of allowed 2GHz channels
     * @param config configuration to update
     * @return an integer result code
     */
    public static int updateApChannelConfig(WifiNative wifiNative,
                                            String countryCode,
                                            ArrayList<Integer> allowed2GChannels,
                                            WifiConfiguration config) {
        /* Use default band and channel for device without HAL. */
        if (!wifiNative.isHalStarted()) {
            config.apBand = DEFAULT_AP_BAND;
            config.apChannel = DEFAULT_AP_CHANNEL;
            return SUCCESS;
        }

        /* Country code is mandatory for 5GHz band. */
        if (config.apBand == WifiConfiguration.AP_BAND_5GHZ
                && countryCode == null) {
            Log.e(TAG, "5GHz band is not allowed without country code");
            return ERROR_GENERIC;
        }

        /* Select a channel if it is not specified. */
        if (config.apChannel == 0) {
            config.apChannel = chooseApChannel(
                    config.apBand, allowed2GChannels,
                    wifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ));
            if (config.apChannel == -1) {
                if (wifiNative.isGetChannelsForBandSupported()) {
                    /* We're not able to get channel when it is supported by HAL. */
                    Log.e(TAG, "Failed to get available channel.");
                    return ERROR_NO_CHANNEL;
                }

                /* Use the default for HAL without get channel support. */
                config.apBand = DEFAULT_AP_BAND;
                config.apChannel = DEFAULT_AP_CHANNEL;
            }
        }

        return SUCCESS;
    }
}
