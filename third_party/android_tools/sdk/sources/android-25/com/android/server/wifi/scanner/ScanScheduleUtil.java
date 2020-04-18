/*
 * Copyright (C) 2015 The Android Open Source Project
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

import android.annotation.Nullable;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiScanner.ScanData;
import android.net.wifi.WifiScanner.ScanSettings;

import com.android.server.wifi.WifiNative;

import java.util.ArrayList;
import java.util.List;

/**
 * A class with utilities for dealing with scan schedules.
 */
public class ScanScheduleUtil {

    /**
     * Compares two ChannelSettings for equality.
     */
    public static boolean channelEquals(@Nullable WifiNative.ChannelSettings channel1,
                                         @Nullable WifiNative.ChannelSettings channel2) {
        if (channel1 == null || channel2 == null) return false;
        if (channel1 == channel2) return true;

        if (channel1.frequency != channel2.frequency) return false;
        if (channel1.dwell_time_ms != channel2.dwell_time_ms) return false;
        return channel1.passive == channel2.passive;
    }

    /**
     * Compares two BucketSettings for equality.
     */
    public static boolean bucketEquals(@Nullable WifiNative.BucketSettings bucket1,
                                        @Nullable WifiNative.BucketSettings bucket2) {
        if (bucket1 == null || bucket2 == null) return false;
        if (bucket1 == bucket2) return true;

        if (bucket1.bucket != bucket2.bucket) return false;
        if (bucket1.band != bucket2.band) return false;
        if (bucket1.period_ms != bucket2.period_ms) return false;
        if (bucket1.report_events != bucket2.report_events) return false;
        if (bucket1.num_channels != bucket2.num_channels) return false;
        for (int c = 0; c < bucket1.num_channels; c++) {
            if (!channelEquals(bucket1.channels[c], bucket2.channels[c])) {
                return false;
            }
        }

        return true;
    }

    /**
     * Compares two ScanSettings for equality.
     */
    public static boolean scheduleEquals(@Nullable WifiNative.ScanSettings schedule1,
                                         @Nullable WifiNative.ScanSettings schedule2) {
        if (schedule1 == null || schedule2 == null) return false;
        if (schedule1 == schedule2) return true;

        if (schedule1.base_period_ms != schedule2.base_period_ms) return false;
        if (schedule1.max_ap_per_scan != schedule2.max_ap_per_scan) return false;
        if (schedule1.report_threshold_percent != schedule2.report_threshold_percent) return false;
        if (schedule1.report_threshold_num_scans != schedule2.report_threshold_num_scans) {
            return false;
        }
        if (schedule1.num_buckets != schedule2.num_buckets) return false;
        for (int b = 0; b < schedule1.num_buckets; b++) {
            if (!bucketEquals(schedule1.buckets[b], schedule2.buckets[b])) {
                return false;
            }
        }

        return true;
    }

    /**
     * Check if the specified bucket was scanned. If not all information is available then this
     * method will return true.
     *
     * @param scheduledBucket Index of the bucket to check for, zero indexed, or -1 if any scan
     *                        should be treated as scanning this bucket.
     * @param bucketsScannedBitSet The bitset of all buckets scanned, 0 if unavailable
     */
    private static boolean isBucketMaybeScanned(int scheduledBucket, int bucketsScannedBitSet) {
        if (bucketsScannedBitSet == 0 || scheduledBucket < 0) {
            return true;
        } else {
            return (bucketsScannedBitSet & (1 << scheduledBucket)) != 0;
        }
    }

    /**
     * Check if the specified bucket was scanned. If not all information is available then this
     * method will return false.
     *
     * @param scheduledBucket Index of the bucket to check for, zero indexed, or -1 if any scan
     *                        should be treated as scanning this bucket.
     * @param bucketsScannedBitSet The bitset of all buckets scanned, 0 if unavailable
     */
    private static boolean isBucketDefinitlyScanned(int scheduledBucket, int bucketsScannedBitSet) {
        if (scheduledBucket < 0) {
            return true;
        } else if (bucketsScannedBitSet == 0) {
            return false;
        } else {
            return (bucketsScannedBitSet & (1 << scheduledBucket)) != 0;
        }
    }

    /**
     * Returns true if the given scan result should be reported to a listener with the given
     * settings.
     */
    public static boolean shouldReportFullScanResultForSettings(ChannelHelper channelHelper,
            ScanResult result, int bucketsScanned, ScanSettings settings, int scheduledBucket) {
        if (isBucketMaybeScanned(scheduledBucket, bucketsScanned)) {
            return channelHelper.settingsContainChannel(settings, result.frequency);
        } else {
            return false;
        }
    }

    /**
     * Returns a filtered version of the scan results from the chip that represents only the data
     * requested in the settings. Will return null if the result should not be reported.
     *
     * If a ScanData indicates that the bucket the settings were placed in was scanned then it
     * will always be included (filtered to only include requested channels). If it indicates that
     * the bucket was definitely not scanned then the scan data will not be reported.
     * If it is not possible to determine if the settings bucket was scanned or not then a
     * ScanData will be included if the scan was empty or there was at least one scan result that
     * matches a requested channel (again the results will be filtered to only include requested
     * channels.
     */
    public static ScanData[] filterResultsForSettings(ChannelHelper channelHelper,
            ScanData[] scanDatas, ScanSettings settings, int scheduledBucket) {
        List<ScanData> filteredScanDatas = new ArrayList<>(scanDatas.length);
        List<ScanResult> filteredResults = new ArrayList<>();
        for (ScanData scanData : scanDatas) {
            // only report ScanData if the settings bucket could have been scanned
            if (isBucketMaybeScanned(scheduledBucket, scanData.getBucketsScanned())) {
                filteredResults.clear();
                for (ScanResult scanResult : scanData.getResults()) {
                    if (channelHelper.settingsContainChannel(settings, scanResult.frequency)) {
                        filteredResults.add(scanResult);
                    }
                    if (settings.numBssidsPerScan > 0
                            && filteredResults.size() >= settings.numBssidsPerScan) {
                        break;
                    }
                }
                // will include scan results if the scan was empty, there was at least one
                // one result that matched the scan request or we are sure that all the requested
                // channels were scanned.
                if (filteredResults.size() == scanData.getResults().length) {
                    filteredScanDatas.add(scanData);
                } else if (filteredResults.size() > 0 || isBucketDefinitlyScanned(scheduledBucket,
                                scanData.getBucketsScanned())) {
                    filteredScanDatas.add(new ScanData(scanData.getId(),
                                    scanData.getFlags(),
                                    filteredResults.toArray(
                                            new ScanResult[filteredResults.size()])));
                }
            }
        }
        if (filteredScanDatas.size() == 0) {
            return null;
        } else {
            return filteredScanDatas.toArray(new ScanData[filteredScanDatas.size()]);
        }
    }
}
