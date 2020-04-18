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
import android.util.ArraySet;

import com.android.server.wifi.WifiNative;

import java.util.Set;

/**
 * ChannelHelper that offers channel manipulation utilities when the channels in a band are not
 * known. Operations performed may simplify any band to include all channels.
 */
public class NoBandChannelHelper extends ChannelHelper {

    /**
     * These parameters are used to estimate the scan duration.
     * This is a guess at the number of channels the device supports for use when a ScanSettings
     * specifies a band instead of a list of channels.
     */
    private static final int ALL_BAND_CHANNEL_COUNT_ESTIMATE = 36;

    @Override
    public boolean settingsContainChannel(WifiScanner.ScanSettings settings, int channel) {
        if (settings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
            for (int i = 0; i < settings.channels.length; ++i) {
                if (settings.channels[i].frequency == channel) {
                    return true;
                }
            }
            return false;
        } else {
            return true;
        }
    }

    @Override
    public WifiScanner.ChannelSpec[] getAvailableScanChannels(int band) {
        return NO_CHANNELS; // not supported
    }

    @Override
    public int estimateScanDuration(WifiScanner.ScanSettings settings) {
        if (settings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
            return settings.channels.length * SCAN_PERIOD_PER_CHANNEL_MS;
        } else {
            return ALL_BAND_CHANNEL_COUNT_ESTIMATE * SCAN_PERIOD_PER_CHANNEL_MS;
        }
    }

    /**
     * ChannelCollection that merges channels without knowing which channels are in each band. In
     * order to do this if any band is added or the maxChannels is exceeded then all channels will
     * be included.
     */
    public class NoBandChannelCollection extends ChannelCollection {
        private final ArraySet<Integer> mChannels = new ArraySet<Integer>();
        private boolean mAllChannels = false;

        @Override
        public void addChannel(int frequency) {
            mChannels.add(frequency);
        }

        @Override
        public void addBand(int band) {
            if (band != WifiScanner.WIFI_BAND_UNSPECIFIED) {
                mAllChannels = true;
            }
        }

        @Override
        public boolean containsChannel(int channel) {
            return mAllChannels || mChannels.contains(channel);
        }

        @Override
        public boolean containsBand(int band) {
            if (band != WifiScanner.WIFI_BAND_UNSPECIFIED) {
                return mAllChannels;
            }
            return false;
        }

        @Override
        public boolean partiallyContainsBand(int band) {
            // We don't need to partially collapse settings in supplicant scanner because we
            // don't have any limitation on the number of channels that can be scanned. We also
            // don't currently keep track of bands very well in NoBandChannelHelper.
            return false;
        }

        @Override
        public boolean isEmpty() {
            return !mAllChannels && mChannels.isEmpty();
        }

        @Override
        public boolean isAllChannels() {
            return mAllChannels;
        }

        @Override
        public void clear() {
            mAllChannels = false;
            mChannels.clear();
        }

        @Override
        public Set<Integer> getMissingChannelsFromBand(int band) {
            // We don't need to partially collapse settings in supplicant scanner because we
            // don't have any limitation on the number of channels that can be scanned. We also
            // don't currently keep track of bands very well in NoBandChannelHelper.
            return new ArraySet<Integer>();
        }

        @Override
        public Set<Integer> getContainingChannelsFromBand(int band) {
            // We don't need to partially collapse settings in supplicant scanner because we
            // don't have any limitation on the number of channels that can be scanned. We also
            // don't currently keep track of bands very well in NoBandChannelHelper.
            return new ArraySet<Integer>();
        }

        @Override
        public Set<Integer> getChannelSet() {
            if (!isEmpty() && !mAllChannels) {
                return mChannels;
            } else {
                return new ArraySet<>();
            }
        }

        @Override
        public void fillBucketSettings(WifiNative.BucketSettings bucketSettings, int maxChannels) {
            if (mAllChannels || mChannels.size() > maxChannels) {
                bucketSettings.band = WifiScanner.WIFI_BAND_BOTH_WITH_DFS;
                bucketSettings.num_channels = 0;
                bucketSettings.channels = null;
            } else {
                bucketSettings.band = WifiScanner.WIFI_BAND_UNSPECIFIED;
                bucketSettings.num_channels = mChannels.size();
                bucketSettings.channels = new WifiNative.ChannelSettings[mChannels.size()];
                for (int i = 0; i < mChannels.size(); ++i) {
                    WifiNative.ChannelSettings channelSettings = new WifiNative.ChannelSettings();
                    channelSettings.frequency = mChannels.valueAt(i);
                    bucketSettings.channels[i] = channelSettings;
                }
            }
        }

        @Override
        public Set<Integer> getSupplicantScanFreqs() {
            if (mAllChannels) {
                return null;
            } else {
                return new ArraySet<Integer>(mChannels);
            }
        }
    }

    @Override
    public ChannelCollection createChannelCollection() {
        return new NoBandChannelCollection();
    }
}
