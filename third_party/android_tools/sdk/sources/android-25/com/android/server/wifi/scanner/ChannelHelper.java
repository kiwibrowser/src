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
 * ChannelHelper offers an abstraction for channel manipulation utilities allowing operation to be
 * adjusted based on the amount of information known about the available channels.
 */
public abstract class ChannelHelper {

    // TODO: Currently this is simply an estimate and is used for both active and passive channels
    //       scans. Eventually it should be split between passive and active and perhaps retrieved
    //       from the driver.
    /**
     * The estimated period spent scanning each channel. This is used for estimating scan duration.
     */
    public static final int SCAN_PERIOD_PER_CHANNEL_MS = 200;

    protected static final WifiScanner.ChannelSpec[] NO_CHANNELS = new WifiScanner.ChannelSpec[0];

    /**
     * Create a new collection that can be used to store channels
     */
    public abstract ChannelCollection createChannelCollection();

    /**
     * Return true if the specified channel is expected for a scan with the given settings
     */
    public abstract boolean settingsContainChannel(WifiScanner.ScanSettings settings, int channel);

    /**
     * Get the channels that are available for scanning on the supplied band.
     * This method may return empty if the information is not available.
     */
    public abstract WifiScanner.ChannelSpec[] getAvailableScanChannels(int band);

    /**
     * Estimates the duration that the chip will spend scanning with the given settings
     */
    public abstract int estimateScanDuration(WifiScanner.ScanSettings settings);

    /**
     * Update the channel information that this object has. The source of the update is
     * implementation dependent and may result in no change. Warning the behavior of a
     * ChannelCollection created using {@link #createChannelCollection createChannelCollection} is
     * undefined after calling this method until the {@link ChannelColleciton#clear() clear} method
     * is called on it.
     */
    public void updateChannels() {
        // default implementation does nothing
    }

    /**
     * Object that supports accumulation of channels and bands
     */
    public abstract class ChannelCollection {
        /**
         * Add a channel to the collection
         */
        public abstract void addChannel(int channel);
        /**
         * Add all channels in the band to the collection
         */
        public abstract void addBand(int band);
        /**
         * @return true if the collection contains the supplied channel
         */
        public abstract boolean containsChannel(int channel);
        /**
         * @return true if the collection contains all the channels of the supplied band
         */
        public abstract boolean containsBand(int band);
        /**
         * @return true if the collection contains some of the channels of the supplied band
         */
        public abstract boolean partiallyContainsBand(int band);
        /**
         * @return true if the collection contains no channels
         */
        public abstract boolean isEmpty();
        /**
         * @return true if the collection contains all available channels
         */
        public abstract boolean isAllChannels();
        /**
         * Remove all channels from the collection
         */
        public abstract void clear();
        /**
         * Retrieves a list of channels from the band which are missing in the channel collection.
         */
        public abstract Set<Integer> getMissingChannelsFromBand(int band);
        /**
         * Retrieves a list of channels from the band which are contained in the channel collection.
         */
        public abstract Set<Integer> getContainingChannelsFromBand(int band);
        /**
         * Gets a list of channels specified in the current channel collection. This will return
         * an empty set if an entire Band if specified or if the list is empty.
         */
        public abstract Set<Integer> getChannelSet();

        /**
         * Add all channels in the ScanSetting to the collection
         */
        public void addChannels(WifiScanner.ScanSettings scanSettings) {
            if (scanSettings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
                for (int j = 0; j < scanSettings.channels.length; ++j) {
                    addChannel(scanSettings.channels[j].frequency);
                }
            } else {
                addBand(scanSettings.band);
            }
        }

        /**
         * Add all channels in the BucketSettings to the collection
         */
        public void addChannels(WifiNative.BucketSettings bucketSettings) {
            if (bucketSettings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
                for (int j = 0; j < bucketSettings.channels.length; ++j) {
                    addChannel(bucketSettings.channels[j].frequency);
                }
            } else {
                addBand(bucketSettings.band);
            }
        }

        /**
         * Checks if all channels in ScanSetting is in the collection
         */
        public boolean containsSettings(WifiScanner.ScanSettings scanSettings) {
            if (scanSettings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
                for (int j = 0; j < scanSettings.channels.length; ++j) {
                    if (!containsChannel(scanSettings.channels[j].frequency)) {
                        return false;
                    }
                }
                return true;
            } else {
                return containsBand(scanSettings.band);
            }
        }

        /**
         * Checks if at least some of the channels in ScanSetting is in the collection
         */
        public boolean partiallyContainsSettings(WifiScanner.ScanSettings scanSettings) {
            if (scanSettings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
                for (int j = 0; j < scanSettings.channels.length; ++j) {
                    if (containsChannel(scanSettings.channels[j].frequency)) {
                        return true;
                    }
                }
                return false;
            } else {
                return partiallyContainsBand(scanSettings.band);
            }
        }

        /**
         * Retrieves a list of missing channels in the collection from the provided settings.
         */
        public Set<Integer> getMissingChannelsFromSettings(WifiScanner.ScanSettings scanSettings) {
            if (scanSettings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
                ArraySet<Integer> missingChannels = new ArraySet<>();
                for (int j = 0; j < scanSettings.channels.length; ++j) {
                    if (!containsChannel(scanSettings.channels[j].frequency)) {
                        missingChannels.add(scanSettings.channels[j].frequency);
                    }
                }
                return missingChannels;
            } else {
                return getMissingChannelsFromBand(scanSettings.band);
            }
        }

        /**
         * Retrieves a list of containing channels in the collection from the provided settings.
         */
        public Set<Integer> getContainingChannelsFromSettings(
                WifiScanner.ScanSettings scanSettings) {
            if (scanSettings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
                ArraySet<Integer> containingChannels = new ArraySet<>();
                for (int j = 0; j < scanSettings.channels.length; ++j) {
                    if (containsChannel(scanSettings.channels[j].frequency)) {
                        containingChannels.add(scanSettings.channels[j].frequency);
                    }
                }
                return containingChannels;
            } else {
                return getContainingChannelsFromBand(scanSettings.band);
            }
        }

        /**
         * Store the channels in this collection in the supplied BucketSettings. If maxChannels is
         * exceeded or a band better describes the channels then a band is specified instead of a
         * channel list.
         */
        public abstract void fillBucketSettings(WifiNative.BucketSettings bucket, int maxChannels);

        /**
         * Gets the list of channels that should be supplied to supplicant for a scan. Will either
         * be a collection of all channels or null if all channels should be scanned.
         */
        public abstract Set<Integer> getSupplicantScanFreqs();
    }


    /*
     * Utility methods for converting band/channels to strings
     */

    /**
     * Create a string representation of the channels in the ScanSettings.
     * If it contains a list of channels then the channels are returned, otherwise a string name of
     * the band is returned.
     */
    public static String toString(WifiScanner.ScanSettings scanSettings) {
        if (scanSettings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
            return toString(scanSettings.channels);
        } else {
            return toString(scanSettings.band);
        }
    }

    /**
     * Create a string representation of the channels in the BucketSettings.
     * If it contains a list of channels then the channels are returned, otherwise a string name of
     * the band is returned.
     */
    public static String toString(WifiNative.BucketSettings bucketSettings) {
        if (bucketSettings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
            return toString(bucketSettings.channels, bucketSettings.num_channels);
        } else {
            return toString(bucketSettings.band);
        }
    }

    private static String toString(WifiScanner.ChannelSpec[] channels) {
        if (channels == null) {
            return "null";
        }

        StringBuilder sb = new StringBuilder();
        sb.append("[");
        for (int c = 0; c < channels.length; c++) {
            sb.append(channels[c].frequency);
            if (c != channels.length - 1) {
                sb.append(",");
            }
        }
        sb.append("]");
        return sb.toString();
    }

    private static String toString(WifiNative.ChannelSettings[] channels, int numChannels) {
        if (channels == null) {
            return "null";
        }

        StringBuilder sb = new StringBuilder();
        sb.append("[");
        for (int c = 0; c < numChannels; c++) {
            sb.append(channels[c].frequency);
            if (c != numChannels - 1) {
                sb.append(",");
            }
        }
        sb.append("]");
        return sb.toString();
    }

    private static String toString(int band) {
        switch (band) {
            case WifiScanner.WIFI_BAND_UNSPECIFIED:
                return "unspecified";
            case WifiScanner.WIFI_BAND_24_GHZ:
                return "24Ghz";
            case WifiScanner.WIFI_BAND_5_GHZ:
                return "5Ghz (no DFS)";
            case WifiScanner.WIFI_BAND_5_GHZ_DFS_ONLY:
                return "5Ghz (DFS only)";
            case WifiScanner.WIFI_BAND_5_GHZ_WITH_DFS:
                return "5Ghz (DFS incl)";
            case WifiScanner.WIFI_BAND_BOTH:
                return "24Ghz & 5Ghz (no DFS)";
            case WifiScanner.WIFI_BAND_BOTH_WITH_DFS:
                return "24Ghz & 5Ghz (DFS incl)";
        }

        return "invalid band";
    }
}
