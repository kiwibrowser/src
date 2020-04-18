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

package com.android.server.wifi.scanner;

import android.net.wifi.WifiScanner;
import android.util.Log;

import com.android.server.wifi.WifiNative;

/**
 * KnownBandsChannelHelper that uses band to channel mappings retrieved from the HAL.
 * Also supporting updating the channel list from the HAL on demand.
 */
public class HalChannelHelper extends KnownBandsChannelHelper {
    private static final String TAG = "HalChannelHelper";

    private final WifiNative mWifiNative;

    public HalChannelHelper(WifiNative wifiNative) {
        mWifiNative = wifiNative;
        final int[] emptyFreqList = new int[0];
        setBandChannels(emptyFreqList, emptyFreqList, emptyFreqList);
        updateChannels();
    }

    @Override
    public void updateChannels() {
        int[] channels24G = mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_24_GHZ);
        if (channels24G == null) Log.e(TAG, "Failed to get channels for 2.4GHz band");
        int[] channels5G = mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ);
        if (channels5G == null) Log.e(TAG, "Failed to get channels for 5GHz band");
        int[] channelsDfs = mWifiNative.getChannelsForBand(WifiScanner.WIFI_BAND_5_GHZ_DFS_ONLY);
        if (channelsDfs == null) Log.e(TAG, "Failed to get channels for 5GHz DFS only band");
        if (channels24G == null || channels5G == null || channelsDfs == null) {
            Log.e(TAG, "Failed to get all channels for band, not updating band channel lists");
        } else if (channels24G.length > 0 || channels5G.length > 0 || channelsDfs.length > 0) {
            setBandChannels(channels24G, channels5G, channelsDfs);
        } else {
            Log.e(TAG, "Got zero length for all channel lists");
        }
    }
}
