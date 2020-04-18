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
 * ChannelHelper that offers channel manipulation utilities when the channels in a band are known.
 * This allows more fine operations on channels than if band channels are not known.
 */
public class KnownBandsChannelHelper extends ChannelHelper {

    private WifiScanner.ChannelSpec[][] mBandsToChannels;

    protected void setBandChannels(int[] channels2G, int[] channels5G, int[] channelsDfs) {
        mBandsToChannels = new WifiScanner.ChannelSpec[8][];

        mBandsToChannels[0] = NO_CHANNELS;

        mBandsToChannels[1] = new WifiScanner.ChannelSpec[channels2G.length];
        copyChannels(mBandsToChannels[1], 0, channels2G);

        mBandsToChannels[2] = new WifiScanner.ChannelSpec[channels5G.length];
        copyChannels(mBandsToChannels[2], 0, channels5G);

        mBandsToChannels[3] = new WifiScanner.ChannelSpec[channels2G.length + channels5G.length];
        copyChannels(mBandsToChannels[3], 0, channels2G);
        copyChannels(mBandsToChannels[3], channels2G.length, channels5G);

        mBandsToChannels[4] = new WifiScanner.ChannelSpec[channelsDfs.length];
        copyChannels(mBandsToChannels[4], 0, channelsDfs);

        mBandsToChannels[5] = new WifiScanner.ChannelSpec[channels2G.length + channelsDfs.length];
        copyChannels(mBandsToChannels[5], 0, channels2G);
        copyChannels(mBandsToChannels[5], channels2G.length, channelsDfs);

        mBandsToChannels[6] = new WifiScanner.ChannelSpec[channels5G.length + channelsDfs.length];
        copyChannels(mBandsToChannels[6], 0, channels5G);
        copyChannels(mBandsToChannels[6], channels5G.length, channelsDfs);

        mBandsToChannels[7] = new WifiScanner.ChannelSpec[
                channels2G.length + channels5G.length + channelsDfs.length];
        copyChannels(mBandsToChannels[7], 0, channels2G);
        copyChannels(mBandsToChannels[7], channels2G.length, channels5G);
        copyChannels(mBandsToChannels[7], channels2G.length + channels5G.length, channelsDfs);
    }

    private static void copyChannels(
            WifiScanner.ChannelSpec[] channelSpec, int offset, int[] channels) {
        for (int i = 0; i < channels.length; i++) {
            channelSpec[offset + i] = new WifiScanner.ChannelSpec(channels[i]);
        }
    }

    @Override
    public WifiScanner.ChannelSpec[] getAvailableScanChannels(int band) {
        if (band < WifiScanner.WIFI_BAND_24_GHZ || band > WifiScanner.WIFI_BAND_BOTH_WITH_DFS) {
            // invalid value for band
            return NO_CHANNELS;
        } else {
            return mBandsToChannels[band];
        }
    }

    @Override
    public int estimateScanDuration(WifiScanner.ScanSettings settings) {
        if (settings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
            return settings.channels.length * SCAN_PERIOD_PER_CHANNEL_MS;
        } else {
            return getAvailableScanChannels(settings.band).length * SCAN_PERIOD_PER_CHANNEL_MS;
        }
    }

    private boolean isDfsChannel(int frequency) {
        for (WifiScanner.ChannelSpec dfsChannel :
                mBandsToChannels[WifiScanner.WIFI_BAND_5_GHZ_DFS_ONLY]) {
            if (frequency == dfsChannel.frequency) {
                return true;
            }
        }
        return false;
    }

    // TODO this should be rewritten to be based on the input data instead of hardcoded ranges
    private int getBandFromChannel(int frequency) {
        if (2400 <= frequency && frequency < 2500) {
            return WifiScanner.WIFI_BAND_24_GHZ;
        } else if (isDfsChannel(frequency)) {
            return WifiScanner.WIFI_BAND_5_GHZ_DFS_ONLY;
        } else if (5100 <= frequency && frequency < 6000) {
            return WifiScanner.WIFI_BAND_5_GHZ;
        } else {
            return WifiScanner.WIFI_BAND_UNSPECIFIED;
        }
    }

    @Override
    public boolean settingsContainChannel(WifiScanner.ScanSettings settings, int channel) {
        WifiScanner.ChannelSpec[] settingsChannels;
        if (settings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
            settingsChannels = settings.channels;
        } else {
            settingsChannels = getAvailableScanChannels(settings.band);
        }
        for (int i = 0; i < settingsChannels.length; ++i) {
            if (settingsChannels[i].frequency == channel) {
                return true;
            }
        }
        return false;
    }

    /**
     * ChannelCollection that merges channels so that the optimal schedule will be generated.
     * When the max channels value is satisfied this implementation will always create a channel
     * list that includes no more than the added channels.
     */
    public class KnownBandsChannelCollection extends ChannelCollection {
        /**
         * Stores all channels, including those that belong to added bands.
         */
        private final ArraySet<Integer> mChannels = new ArraySet<Integer>();
        /**
         * Contains only the bands that were explicitly added as bands.
         */
        private int mExactBands = 0;
        /**
         * Contains all bands, including those that were added because an added channel was in that
         * band.
         */
        private int mAllBands = 0;

        @Override
        public void addChannel(int frequency) {
            mChannels.add(frequency);
            mAllBands |= getBandFromChannel(frequency);
        }

        @Override
        public void addBand(int band) {
            mExactBands |= band;
            mAllBands |= band;
            WifiScanner.ChannelSpec[] bandChannels = getAvailableScanChannels(band);
            for (int i = 0; i < bandChannels.length; ++i) {
                mChannels.add(bandChannels[i].frequency);
            }
        }

        @Override
        public boolean containsChannel(int channel) {
            return mChannels.contains(channel);
        }

        @Override
        public boolean containsBand(int band) {
            WifiScanner.ChannelSpec[] bandChannels = getAvailableScanChannels(band);
            for (int i = 0; i < bandChannels.length; ++i) {
                if (!mChannels.contains(bandChannels[i].frequency)) {
                    return false;
                }
            }
            return true;
        }

        @Override
        public boolean partiallyContainsBand(int band) {
            WifiScanner.ChannelSpec[] bandChannels = getAvailableScanChannels(band);
            for (int i = 0; i < bandChannels.length; ++i) {
                if (mChannels.contains(bandChannels[i].frequency)) {
                    return true;
                }
            }
            return false;
        }

        @Override
        public boolean isEmpty() {
            return mChannels.isEmpty();
        }

        @Override
        public boolean isAllChannels() {
            return getAvailableScanChannels(WifiScanner.WIFI_BAND_BOTH_WITH_DFS).length ==
                    mChannels.size();
        }

        @Override
        public void clear() {
            mAllBands = 0;
            mExactBands = 0;
            mChannels.clear();
        }

        @Override
        public Set<Integer> getMissingChannelsFromBand(int band) {
            ArraySet<Integer> missingChannels = new ArraySet<>();
            WifiScanner.ChannelSpec[] bandChannels = getAvailableScanChannels(band);
            for (int i = 0; i < bandChannels.length; ++i) {
                if (!mChannels.contains(bandChannels[i].frequency)) {
                    missingChannels.add(bandChannels[i].frequency);
                }
            }
            return missingChannels;
        }

        @Override
        public Set<Integer> getContainingChannelsFromBand(int band) {
            ArraySet<Integer> containingChannels = new ArraySet<>();
            WifiScanner.ChannelSpec[] bandChannels = getAvailableScanChannels(band);
            for (int i = 0; i < bandChannels.length; ++i) {
                if (mChannels.contains(bandChannels[i].frequency)) {
                    containingChannels.add(bandChannels[i].frequency);
                }
            }
            return containingChannels;
        }

        @Override
        public Set<Integer> getChannelSet() {
            if (!isEmpty() && mAllBands != mExactBands) {
                return mChannels;
            } else {
                return new ArraySet<>();
            }
        }

        @Override
        public void fillBucketSettings(WifiNative.BucketSettings bucketSettings, int maxChannels) {
            if ((mChannels.size() > maxChannels || mAllBands == mExactBands)
                    && mAllBands != 0) {
                bucketSettings.band = mAllBands;
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
            if (mExactBands == WifiScanner.WIFI_BAND_BOTH_WITH_DFS) {
                return null;
            } else {
                return new ArraySet<Integer>(mChannels);
            }
        }

        public Set<Integer> getAllChannels() {
            return new ArraySet<Integer>(mChannels);
        }
    }

    @Override

    public KnownBandsChannelCollection createChannelCollection() {
        return new KnownBandsChannelCollection();
    }
}
