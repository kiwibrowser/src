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

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiScanner;
import android.net.wifi.WifiScanner.ScanData;
import android.net.wifi.WifiScanner.ScanSettings;
import android.util.ArraySet;
import android.util.Pair;
import android.util.Rational;
import android.util.Slog;

import com.android.server.wifi.WifiNative;
import com.android.server.wifi.scanner.ChannelHelper.ChannelCollection;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Set;

/**
 * <p>This class takes a series of scan requests and formulates the best hardware level scanning
 * schedule it can to try and satisfy requests. The hardware level accepts a series of buckets,
 * where each bucket represents a set of channels and an interval to scan at. This
 * scheduler operates as follows:</p>
 *
 * <p>Each new request is placed in the best predefined bucket. Once all requests have been added
 * the last buckets (lower priority) are placed in the next best bucket until the number of buckets
 * is less than the number supported by the hardware.
 *
 * <p>Finally, the scheduler creates a WifiNative.ScanSettings from the list of buckets which may be
 * passed through the Wifi HAL.</p>
 *
 * <p>This class is not thread safe.</p>
 */
public class BackgroundScanScheduler {

    private static final String TAG = "BackgroundScanScheduler";
    private static final boolean DBG = false;

    public static final int DEFAULT_MAX_BUCKETS = 8;
    // Max channels that can be specified per bucket
    public static final int DEFAULT_MAX_CHANNELS_PER_BUCKET = 16;
    // anecdotally, some chipsets will fail without explanation with a higher batch size, and
    // there is apparently no way to retrieve the maximum batch size
    public static final int DEFAULT_MAX_SCANS_TO_BATCH = 10;
    public static final int DEFAULT_MAX_AP_PER_SCAN = 32;

    /**
     * Value that all scan periods must be an integer multiple of
     */
    private static final int PERIOD_MIN_GCD_MS = 10000;
    /**
     * Default period to use if no buckets are being scheduled
     */
    private static final int DEFAULT_PERIOD_MS = 30000;
    /**
     * Scan report threshold percentage to assign to the schedule by default
     * @see com.android.server.wifi.WifiNative.ScanSettings#report_threshold_percent
     */
    private static final int DEFAULT_REPORT_THRESHOLD_PERCENTAGE = 100;

    /**
     * List of predefined periods (in ms) that buckets can be scheduled at. Ordered by preference
     * if there are not enough buckets for all periods. All periods MUST be an integer multiple of
     * the next smallest bucket with the smallest bucket having a period of PERIOD_MIN_GCD_MS.
     * This requirement allows scans to be scheduled more efficiently because scan requests with
     * intersecting channels will result in those channels being scanned exactly once at the smaller
     * period and no unnecessary scan being scheduled. If this was not the case and two requests
     * had channel 5 with periods of 15 seconds and 25 seconds then channel 5 would be scanned
     * 296  (3600/15 + 3600/25 - 3500/75) times an hour instead of 240 times an hour (3600/15) if
     * the 25s scan is rescheduled at 30s. This is less important with higher periods as it has
     * significantly less impact. Ranking could be done by favoring shorter or longer; however,
     * this would result in straying further from the requested period and possibly power
     * implications if the scan is scheduled at a significantly lower period.
     *
     * For example if the hardware only supports 2 buckets and scans are requested with periods of
     * 40s, 20s and 10s then the two buckets scheduled will have periods 40s and 20s and the 10s
     * scan will be placed in the 20s bucket.
     *
     * If there are special scan requests such as exponential back off, we always dedicate a bucket
     * for each type. Regular scan requests will be packed into the remaining buckets.
     */
    private static final int[] PREDEFINED_BUCKET_PERIODS = {
        3 * PERIOD_MIN_GCD_MS,   // 30s
        12 * PERIOD_MIN_GCD_MS,  // 120s
        48 * PERIOD_MIN_GCD_MS,  // 480s
        1 * PERIOD_MIN_GCD_MS,   // 10s
        6 * PERIOD_MIN_GCD_MS,   // 60s
        192 * PERIOD_MIN_GCD_MS, // 1920s
        24 * PERIOD_MIN_GCD_MS,  // 240s
        96 * PERIOD_MIN_GCD_MS,  // 960s
        384 * PERIOD_MIN_GCD_MS, // 3840s
        -1,                      // place holder for exponential back off scan
    };

    private static final int EXPONENTIAL_BACK_OFF_BUCKET_IDX =
            (PREDEFINED_BUCKET_PERIODS.length - 1);
    private static final int NUM_OF_REGULAR_BUCKETS =
            (PREDEFINED_BUCKET_PERIODS.length - 1);

    /**
     * This class is an intermediate representation for scheduling. This maintins the channel
     * collection to be scanned by the bucket as settings are added to it.
     */
    private class Bucket {
        public int period;
        public int bucketId;
        private final List<ScanSettings> mScanSettingsList = new ArrayList<>();
        private final ChannelCollection mChannelCollection;

        Bucket(int period) {
            this.period = period;
            this.bucketId = 0;
            mScanSettingsList.clear();
            mChannelCollection = mChannelHelper.createChannelCollection();
        }

        /**
         * Copy constructor which populates the settings list from the original bucket object.
         */
        Bucket(Bucket originalBucket) {
            this(originalBucket.period);
            for (ScanSettings settings : originalBucket.getSettingsList()) {
                mScanSettingsList.add(settings);
            }
        }

        /**
         * convert ChannelSpec to native representation
         */
        private WifiNative.ChannelSettings createChannelSettings(int frequency) {
            WifiNative.ChannelSettings channelSettings = new WifiNative.ChannelSettings();
            channelSettings.frequency = frequency;
            return channelSettings;
        }

        public boolean addSettings(ScanSettings scanSettings) {
            mChannelCollection.addChannels(scanSettings);
            return mScanSettingsList.add(scanSettings);
        }

        public boolean removeSettings(ScanSettings scanSettings) {
            if (mScanSettingsList.remove(scanSettings)) {
                // It's difficult to handle settings removal from buckets in terms of
                // maintaining the correct channel collection, so recreate the channel
                // collection from the remaining elements.
                updateChannelCollection();
                return true;
            }
            return false;
        }

        public List<ScanSettings> getSettingsList() {
            return mScanSettingsList;
        }

        public void updateChannelCollection() {
            mChannelCollection.clear();
            for (ScanSettings settings : mScanSettingsList) {
                mChannelCollection.addChannels(settings);
            }
        }

        public ChannelCollection getChannelCollection() {
            return mChannelCollection;
        }

        /**
         * convert the setting for this bucket to HAL representation
         */
        public WifiNative.BucketSettings createBucketSettings(int bucketId, int maxChannels) {
            this.bucketId = bucketId;
            int reportEvents = WifiScanner.REPORT_EVENT_NO_BATCH;
            int maxPeriodInMs = 0;
            int stepCount = 0;
            int bucketIndex = 0;

            for (int i = 0; i < mScanSettingsList.size(); ++i) {
                WifiScanner.ScanSettings setting = mScanSettingsList.get(i);
                int requestedReportEvents = setting.reportEvents;
                if ((requestedReportEvents & WifiScanner.REPORT_EVENT_NO_BATCH) == 0) {
                    reportEvents &= ~WifiScanner.REPORT_EVENT_NO_BATCH;
                }
                if ((requestedReportEvents & WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN) != 0) {
                    reportEvents |= WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN;
                }
                if ((requestedReportEvents & WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT) != 0) {
                    reportEvents |= WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT;
                }
                // For the bucket allocated to exponential back off scan, the values of
                // the exponential back off scan related parameters from the very first
                // setting in the settings list will be used to configure this bucket.
                //
                if (i == 0 && setting.maxPeriodInMs != 0
                        && setting.maxPeriodInMs != setting.periodInMs) {
                    // Align the starting period with one of the pre-defined regular
                    // scan periods. This will optimize the scan schedule when it has
                    // both exponential back off scan and regular scan(s).
                    bucketIndex = findBestRegularBucketIndex(setting.periodInMs,
                                                     NUM_OF_REGULAR_BUCKETS);
                    period = PREDEFINED_BUCKET_PERIODS[bucketIndex];
                    maxPeriodInMs = (setting.maxPeriodInMs < period)
                                    ? period
                                    : setting.maxPeriodInMs;
                    stepCount = setting.stepCount;
                }
            }

            WifiNative.BucketSettings bucketSettings = new WifiNative.BucketSettings();
            bucketSettings.bucket = bucketId;
            bucketSettings.report_events = reportEvents;
            bucketSettings.period_ms = period;
            bucketSettings.max_period_ms = maxPeriodInMs;
            bucketSettings.step_count = stepCount;
            mChannelCollection.fillBucketSettings(bucketSettings, maxChannels);
            return bucketSettings;
        }
    }

    /**
     * Maintains a list of buckets and the number that are active (non-null)
     */
    private class BucketList {
        // Comparator to sort the buckets in order of increasing time periods
        private final Comparator<Bucket> mTimePeriodSortComparator =
                new Comparator<Bucket>() {
                    public int compare(Bucket b1, Bucket b2) {
                        return b1.period - b2.period;
                    }
                };
        private final Bucket[] mBuckets;
        private int mActiveBucketCount = 0;

        BucketList() {
            mBuckets = new Bucket[PREDEFINED_BUCKET_PERIODS.length];
        }

        public void clearAll() {
            Arrays.fill(mBuckets, null);
            mActiveBucketCount = 0;
        }

        public void clear(int index) {
            if (mBuckets[index] != null) {
                --mActiveBucketCount;
                mBuckets[index] = null;
            }
        }

        public Bucket getOrCreate(int index) {
            Bucket bucket = mBuckets[index];
            if (bucket == null) {
                ++mActiveBucketCount;
                bucket = mBuckets[index] = new Bucket(PREDEFINED_BUCKET_PERIODS[index]);
            }
            return bucket;
        }

        public boolean isActive(int index) {
            return mBuckets[index] != null;
        }

        public Bucket get(int index) {
            return mBuckets[index];
        }

        public int size() {
            return mBuckets.length;
        }

        public int getActiveCount() {
            return mActiveBucketCount;
        }

        public int getActiveRegularBucketCount() {
            if (isActive(EXPONENTIAL_BACK_OFF_BUCKET_IDX)) {
                return mActiveBucketCount - 1;
            } else {
                return mActiveBucketCount;
            }
        }

        /**
         * Returns the active regular buckets sorted by their increasing time periods.
         */
        public List<Bucket> getSortedActiveRegularBucketList() {
            ArrayList<Bucket> activeBuckets = new ArrayList<>();
            for (int i = 0; i < mBuckets.length; i++) {
                if (mBuckets[i] != null && i != EXPONENTIAL_BACK_OFF_BUCKET_IDX) {
                    activeBuckets.add(mBuckets[i]);
                }
            }
            Collections.sort(activeBuckets, mTimePeriodSortComparator);
            return activeBuckets;
        }
    }

    private int mMaxBuckets = DEFAULT_MAX_BUCKETS;
    private int mMaxChannelsPerBucket = DEFAULT_MAX_CHANNELS_PER_BUCKET;
    private int mMaxBatch = DEFAULT_MAX_SCANS_TO_BATCH;
    private int mMaxApPerScan = DEFAULT_MAX_AP_PER_SCAN;

    public int getMaxBuckets() {
        return mMaxBuckets;
    }

    public void setMaxBuckets(int maxBuckets) {
        mMaxBuckets = maxBuckets;
    }

    public int getMaxChannelsPerBucket() {
        return mMaxChannelsPerBucket;
    }

    // TODO: find a way to get max channels
    public void setMaxChannelsPerBucket(int maxChannels) {
        mMaxChannelsPerBucket = maxChannels;
    }

    public int getMaxBatch() {
        return mMaxBatch;
    }

    // TODO: find a way to get max batch size
    public void setMaxBatch(int maxBatch) {
        mMaxBatch = maxBatch;
    }

    public int getMaxApPerScan() {
        return mMaxApPerScan;
    }

    public void setMaxApPerScan(int maxApPerScan) {
        mMaxApPerScan = maxApPerScan;
    }

    private final BucketList mBuckets = new BucketList();
    private final ChannelHelper mChannelHelper;
    private WifiNative.ScanSettings mSchedule;
    // This keeps track of the settings to the max time period bucket to which it was scheduled.
    private final Map<ScanSettings, Bucket> mSettingsToScheduledBucket = new HashMap<>();

    public BackgroundScanScheduler(ChannelHelper channelHelper) {
        mChannelHelper = channelHelper;
        createSchedule(new ArrayList<Bucket>(), getMaxChannelsPerBucket());
    }

    /**
     * Updates the schedule from the given set of requests.
     */
    public void updateSchedule(@NonNull Collection<ScanSettings> requests) {
        // create initial schedule
        mBuckets.clearAll();
        for (ScanSettings request : requests) {
            addScanToBuckets(request);
        }

        compactBuckets(getMaxBuckets());

        List<Bucket> bucketList = optimizeBuckets();

        List<Bucket> fixedBucketList =
                fixBuckets(bucketList, getMaxBuckets(), getMaxChannelsPerBucket());

        createSchedule(fixedBucketList, getMaxChannelsPerBucket());
    }

    /**
     * Retrieves the current scanning schedule.
     */
    public @NonNull WifiNative.ScanSettings getSchedule() {
        return mSchedule;
    }

    /**
     * Returns true if the given scan result should be reported to a listener with the given
     * settings.
     */
    public boolean shouldReportFullScanResultForSettings(@NonNull ScanResult result,
            int bucketsScanned, @NonNull ScanSettings settings) {
        return ScanScheduleUtil.shouldReportFullScanResultForSettings(mChannelHelper,
                result, bucketsScanned, settings, getScheduledBucket(settings));
    }

    /**
     * Returns a filtered version of the scan results from the chip that represents only the data
     * requested in the settings. Will return null if the result should not be reported.
     */
    public @Nullable ScanData[] filterResultsForSettings(@NonNull ScanData[] scanDatas,
            @NonNull ScanSettings settings) {
        return ScanScheduleUtil.filterResultsForSettings(mChannelHelper, scanDatas, settings,
                getScheduledBucket(settings));
    }

    /**
     * Retrieves the max time period bucket idx at which this setting was scheduled
     */
    public int getScheduledBucket(ScanSettings settings) {
        Bucket maxScheduledBucket = mSettingsToScheduledBucket.get(settings);
        if (maxScheduledBucket != null) {
            return maxScheduledBucket.bucketId;
        } else {
            Slog.wtf(TAG, "No bucket found for settings");
            return -1;
        }
    }

    /**
     * creates a schedule for the current buckets
     */
    private void createSchedule(List<Bucket> bucketList, int maxChannelsPerBucket) {
        WifiNative.ScanSettings schedule = new WifiNative.ScanSettings();
        schedule.num_buckets = bucketList.size();
        schedule.buckets = new WifiNative.BucketSettings[bucketList.size()];

        schedule.max_ap_per_scan = 0;
        schedule.report_threshold_num_scans = getMaxBatch();
        HashSet<Integer> hiddenNetworkIdSet = new HashSet<>();

        // set all buckets in schedule
        int bucketId = 0;
        for (Bucket bucket : bucketList) {
            schedule.buckets[bucketId] =
                    bucket.createBucketSettings(bucketId, maxChannelsPerBucket);
            for (ScanSettings settings : bucket.getSettingsList()) {
                // set APs per scan
                if (settings.numBssidsPerScan > schedule.max_ap_per_scan) {
                    schedule.max_ap_per_scan = settings.numBssidsPerScan;
                }
                // set batching
                if (settings.maxScansToCache != 0
                        && settings.maxScansToCache < schedule.report_threshold_num_scans) {
                    schedule.report_threshold_num_scans = settings.maxScansToCache;
                }
                // note hidden networks
                if (settings.hiddenNetworkIds != null) {
                    for (int j = 0; j < settings.hiddenNetworkIds.length; j++) {
                        hiddenNetworkIdSet.add(settings.hiddenNetworkIds[j]);
                    }
                }
            }
            bucketId++;
        }

        schedule.report_threshold_percent = DEFAULT_REPORT_THRESHOLD_PERCENTAGE;

        if (schedule.max_ap_per_scan == 0 || schedule.max_ap_per_scan > getMaxApPerScan()) {
            schedule.max_ap_per_scan = getMaxApPerScan();
        }
        if (hiddenNetworkIdSet.size() > 0) {
            schedule.hiddenNetworkIds = new int[hiddenNetworkIdSet.size()];
            int numHiddenNetworks = 0;
            for (Integer hiddenNetworkId : hiddenNetworkIdSet) {
                schedule.hiddenNetworkIds[numHiddenNetworks++] = hiddenNetworkId;
            }
        }

        // update base period as gcd of periods
        if (schedule.num_buckets > 0) {
            int gcd = schedule.buckets[0].period_ms;
            for (int b = 1; b < schedule.num_buckets; b++) {
                gcd = Rational.gcd(schedule.buckets[b].period_ms, gcd);
            }

            if (gcd < PERIOD_MIN_GCD_MS) {
                Slog.wtf(TAG, "found gcd less than min gcd");
                gcd = PERIOD_MIN_GCD_MS;
            }

            schedule.base_period_ms = gcd;
        } else {
            schedule.base_period_ms = DEFAULT_PERIOD_MS;
        }

        mSchedule = schedule;
    }

    /**
     * Add a scan to the most appropriate bucket, creating the bucket if necessary.
     */
    private void addScanToBuckets(ScanSettings settings) {
        int bucketIndex;

        if (settings.maxPeriodInMs != 0 && settings.maxPeriodInMs != settings.periodInMs) {
            // exponential back off scan has a dedicated bucket
            bucketIndex = EXPONENTIAL_BACK_OFF_BUCKET_IDX;
        } else {
            bucketIndex = findBestRegularBucketIndex(settings.periodInMs, NUM_OF_REGULAR_BUCKETS);
        }

        mBuckets.getOrCreate(bucketIndex).addSettings(settings);
    }

    /**
     * find closest bucket period to the requested period in all predefined buckets
     */
    private static int findBestRegularBucketIndex(int requestedPeriod, int maxNumBuckets) {
        maxNumBuckets = Math.min(maxNumBuckets, NUM_OF_REGULAR_BUCKETS);
        int index = -1;
        int minDiff = Integer.MAX_VALUE;
        for (int i = 0; i < maxNumBuckets; ++i) {
            int diff = Math.abs(PREDEFINED_BUCKET_PERIODS[i] - requestedPeriod);
            if (diff < minDiff) {
                minDiff = diff;
                index = i;
            }
        }
        if (index == -1) {
            Slog.wtf(TAG, "Could not find best bucket for period " + requestedPeriod + " in "
                     + maxNumBuckets + " buckets");
        }
        return index;
    }

    /**
     * Reduce the number of required buckets by reassigning lower priority buckets to the next
     * closest period bucket.
     */
    private void compactBuckets(int maxBuckets) {
        int maxRegularBuckets = maxBuckets;

        // reserve one bucket for exponential back off scan if there is
        // such request(s)
        if (mBuckets.isActive(EXPONENTIAL_BACK_OFF_BUCKET_IDX)) {
            maxRegularBuckets--;
        }
        for (int i = NUM_OF_REGULAR_BUCKETS - 1;
                i >= 0 && mBuckets.getActiveRegularBucketCount() > maxRegularBuckets; --i) {
            if (mBuckets.isActive(i)) {
                for (ScanSettings scanRequest : mBuckets.get(i).getSettingsList()) {
                    int newBucketIndex = findBestRegularBucketIndex(scanRequest.periodInMs, i);
                    mBuckets.getOrCreate(newBucketIndex).addSettings(scanRequest);
                }
                mBuckets.clear(i);
            }
        }
    }

    /**
     * Clone the provided scan settings fields to a new ScanSettings object.
     */
    private ScanSettings cloneScanSettings(ScanSettings originalSettings) {
        ScanSettings settings = new ScanSettings();
        settings.band = originalSettings.band;
        settings.channels = originalSettings.channels;
        settings.hiddenNetworkIds = originalSettings.hiddenNetworkIds;
        settings.periodInMs = originalSettings.periodInMs;
        settings.reportEvents = originalSettings.reportEvents;
        settings.numBssidsPerScan = originalSettings.numBssidsPerScan;
        settings.maxScansToCache = originalSettings.maxScansToCache;
        settings.maxPeriodInMs = originalSettings.maxPeriodInMs;
        settings.stepCount = originalSettings.stepCount;
        settings.isPnoScan = originalSettings.isPnoScan;
        return settings;
    }

    /**
     * Creates a split scan setting that needs to be added back to the current bucket.
     */
    private ScanSettings createCurrentBucketSplitSettings(ScanSettings originalSettings,
            Set<Integer> currentBucketChannels) {
        ScanSettings currentBucketSettings = cloneScanSettings(originalSettings);
        // Let's create a new settings for the current bucket with the same flags, but the missing
        // channels from the other bucket
        currentBucketSettings.band = WifiScanner.WIFI_BAND_UNSPECIFIED;
        currentBucketSettings.channels = new WifiScanner.ChannelSpec[currentBucketChannels.size()];
        int chanIdx = 0;
        for (Integer channel : currentBucketChannels) {
            currentBucketSettings.channels[chanIdx++] = new WifiScanner.ChannelSpec(channel);
        }
        return currentBucketSettings;
    }

    /**
     * Creates a split scan setting that needs to be added to the target lower time period bucket.
     * The reportEvents field is modified to remove REPORT_EVENT_AFTER_EACH_SCAN because we
     * need this flag only in the higher time period bucket.
     */
    private ScanSettings createTargetBucketSplitSettings(ScanSettings originalSettings,
            Set<Integer> targetBucketChannels) {
        ScanSettings targetBucketSettings = cloneScanSettings(originalSettings);
        // The new settings for the other bucket will have the channels that already in the that
        // bucket. We'll need to do some migration of the |reportEvents| flags.
        targetBucketSettings.band = WifiScanner.WIFI_BAND_UNSPECIFIED;
        targetBucketSettings.channels = new WifiScanner.ChannelSpec[targetBucketChannels.size()];
        int chanIdx = 0;
        for (Integer channel : targetBucketChannels) {
            targetBucketSettings.channels[chanIdx++] = new WifiScanner.ChannelSpec(channel);
        }
        targetBucketSettings.reportEvents =
                originalSettings.reportEvents
                        & (WifiScanner.REPORT_EVENT_NO_BATCH
                                | WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT);
        return targetBucketSettings;
    }

    /**
     * Split the scan settings into 2 so that they can be put into 2 separate buckets.
     * @return The first scan setting needs to be added back to the current bucket
     *         The second scan setting needs to be added to the other bucket
     */
    private Pair<ScanSettings, ScanSettings> createSplitSettings(ScanSettings originalSettings,
            ChannelCollection targetBucketChannelCol) {
        Set<Integer> currentBucketChannels =
                targetBucketChannelCol.getMissingChannelsFromSettings(originalSettings);
        Set<Integer> targetBucketChannels =
                targetBucketChannelCol.getContainingChannelsFromSettings(originalSettings);
        // Two Copy of the original settings
        ScanSettings currentBucketSettings =
                createCurrentBucketSplitSettings(originalSettings, currentBucketChannels);
        ScanSettings targetBucketSettings =
                createTargetBucketSplitSettings(originalSettings, targetBucketChannels);
        return Pair.create(currentBucketSettings, targetBucketSettings);
    }

    /**
     * Try to merge the settings to lower buckets.
     * Check if the channels in this settings is already covered by a lower time period
     * bucket. If it's partially covered, the settings is split else the entire settings
     * is moved to the lower time period bucket.
     * This method updates the |mSettingsToScheduledBucket| mapping.
     * @return Pair<wasMerged, remainingSplitSettings>
     *         wasMerged -  boolean indicating whether the original setting was merged to lower time
     *                      period buckets.
     *         remainingSplitSettings - Partial Scan Settings that need to be added back to the
     *                                  current bucket.
     */
    private Pair<Boolean, ScanSettings> mergeSettingsToLowerBuckets(ScanSettings originalSettings,
            Bucket currentBucket, ListIterator<Bucket> iterTargetBuckets) {
        ScanSettings remainingSplitSettings = null;
        boolean wasMerged = false;
        Bucket maxScheduledBucket = currentBucket;

        while (iterTargetBuckets.hasPrevious()) {
            Bucket targetBucket = iterTargetBuckets.previous();
            ChannelCollection targetBucketChannelCol = targetBucket.getChannelCollection();
            if (targetBucketChannelCol.containsSettings(originalSettings)) {
                targetBucket.addSettings(originalSettings);
                // Update the max scheduled bucket for this setting
                maxScheduledBucket = targetBucket;
                wasMerged = true;
            } else if (targetBucketChannelCol.partiallyContainsSettings(originalSettings)) {
                Pair<ScanSettings, ScanSettings> splitSettings;
                if (remainingSplitSettings == null) {
                    splitSettings = createSplitSettings(originalSettings, targetBucketChannelCol);
                } else {
                    splitSettings =
                            createSplitSettings(remainingSplitSettings, targetBucketChannelCol);
                }
                targetBucket.addSettings(splitSettings.second);
                // Update the |remainingSplitSettings| to keep track of the remaining scan settings.
                // The original settings could be split across multiple buckets.
                remainingSplitSettings = splitSettings.first;
                wasMerged = true;
            }
        }
        // Update the settings to scheduled bucket mapping. This is needed for event
        // reporting lookup
        mSettingsToScheduledBucket.put(originalSettings, maxScheduledBucket);

        return Pair.create(wasMerged, remainingSplitSettings);
    }

    /**
     * Optimize all the active buckets by removing duplicate channels in the buckets.
     * This method tries to go through the settings in all the buckets and checks if the same
     * channels for the setting is already being scanned by another bucked with lower time period.
     * If yes, move the setting to the lower time period bucket. If all the settings from a higher
     * period has been moved out, that bucket can be removed.
     *
     * We're trying to avoid cases where we have the same channels being scanned in different
     * buckets. This is to workaround the fact that the HAL implementations have a max number of
     * cumulative channel across buckets (b/28022609).
     */
    private List<Bucket> optimizeBuckets() {
        mSettingsToScheduledBucket.clear();
        List<Bucket> sortedBuckets = mBuckets.getSortedActiveRegularBucketList();
        ListIterator<Bucket> iterBuckets = sortedBuckets.listIterator();
        // This is needed to keep track of split settings that need to be added back to the same
        // bucket at the end of iterating thru all the settings. This has to be a separate temp list
        // to prevent concurrent modification exceptions during iterations.
        List<ScanSettings> currentBucketSplitSettingsList = new ArrayList<>();

        // We need to go thru each setting starting from the lowest time period bucket and check
        // if they're already contained in a lower time period bucket. If yes, delete the setting
        // from the current bucket and move it to the other bucket. If the settings are only
        // partially contained, split the settings into two and move the partial bucket back
        // to the same bucket. Finally, if all the settings have been moved out, remove the current
        // bucket altogether.
        while (iterBuckets.hasNext()) {
            Bucket currentBucket = iterBuckets.next();
            Iterator<ScanSettings> iterSettings = currentBucket.getSettingsList().iterator();

            currentBucketSplitSettingsList.clear();

            while (iterSettings.hasNext()) {
                ScanSettings currentSettings = iterSettings.next();
                ListIterator<Bucket> iterTargetBuckets =
                        sortedBuckets.listIterator(iterBuckets.previousIndex());

                Pair<Boolean, ScanSettings> mergeResult =
                        mergeSettingsToLowerBuckets(
                                currentSettings, currentBucket, iterTargetBuckets);

                boolean wasMerged = mergeResult.first.booleanValue();
                if (wasMerged) {
                    // Remove the original settings from the current bucket.
                    iterSettings.remove();
                    ScanSettings remainingSplitSettings = mergeResult.second;
                    if (remainingSplitSettings != null) {
                        // Add back the remaining split settings to the current bucket.
                        currentBucketSplitSettingsList.add(remainingSplitSettings);
                    }
                }
            }

            for (ScanSettings splitSettings: currentBucketSplitSettingsList) {
                currentBucket.addSettings(splitSettings);
            }
            if (currentBucket.getSettingsList().isEmpty()) {
                iterBuckets.remove();
            } else {
                // Update the channel collection to account for the removed settings
                currentBucket.updateChannelCollection();
            }
        }

        // Update the settings to scheduled bucket map for all exponential scans.
        if (mBuckets.isActive(EXPONENTIAL_BACK_OFF_BUCKET_IDX)) {
            Bucket exponentialBucket = mBuckets.get(EXPONENTIAL_BACK_OFF_BUCKET_IDX);
            for (ScanSettings settings : exponentialBucket.getSettingsList()) {
                mSettingsToScheduledBucket.put(settings, exponentialBucket);
            }
            sortedBuckets.add(exponentialBucket);
        }

        return sortedBuckets;
    }

    /**
     * Partition the channel set into 2 or more based on the max channels that can be specified for
     * each bucket.
     */
    private List<Set<Integer>> partitionChannelSet(Set<Integer> originalChannelSet,
            int maxChannelsPerBucket) {
        ArrayList<Set<Integer>> channelSetList = new ArrayList();
        ArraySet<Integer> channelSet = new ArraySet<>();
        Iterator<Integer> iterChannels = originalChannelSet.iterator();

        while (iterChannels.hasNext()) {
            channelSet.add(iterChannels.next());
            if (channelSet.size() == maxChannelsPerBucket) {
                channelSetList.add(channelSet);
                channelSet = new ArraySet<>();
            }
        }
        // Add the last partial set if any
        if (!channelSet.isEmpty()) {
            channelSetList.add(channelSet);
        }
        return channelSetList;
    }

    /**
     * Creates a list of split buckets with the channel collection corrected to fit the
     * max channel list size that can be specified. The original channel collection will be split
     * into multiple buckets with the same scan settings.
     * Note: This does not update the mSettingsToScheduledBucket map because this bucket is
     * essentially a copy of the original bucket, so it should not affect the event reporting.
     * This bucket results will come back the same time the original bucket results come back.
     */
    private List<Bucket> createSplitBuckets(Bucket originalBucket, List<Set<Integer>> channelSets) {
        List<Bucket> splitBucketList = new ArrayList<>();
        int channelSetIdx = 0;

        for (Set<Integer> channelSet : channelSets) {
            Bucket splitBucket;
            if (channelSetIdx == 0) {
                // Need to keep the original bucket to keep track of the settings to scheduled
                // bucket mapping.
                splitBucket = originalBucket;
            } else {
                splitBucket = new Bucket(originalBucket);
            }
            ChannelCollection splitBucketChannelCollection = splitBucket.getChannelCollection();
            splitBucketChannelCollection.clear();
            for (Integer channel : channelSet) {
                splitBucketChannelCollection.addChannel(channel);
            }
            channelSetIdx++;
            splitBucketList.add(splitBucket);
        }
        return splitBucketList;
    }

    /**
     * Check if any of the buckets don't fit into the bucket specification and fix it. This
     * creates duplicate buckets to fit all the channels. So, the channels to be scanned
     * will be split across 2 (or more) buckets.
     * TODO: If we reach the max number of buckets, then this fix will be skipped!
     */
    private List<Bucket> fixBuckets(List<Bucket> originalBucketList, int maxBuckets,
            int maxChannelsPerBucket) {
        List<Bucket> fixedBucketList = new ArrayList<>();
        int totalNumBuckets = originalBucketList.size();

        for (Bucket originalBucket : originalBucketList) {
            ChannelCollection channelCollection = originalBucket.getChannelCollection();
            Set<Integer> channelSet = channelCollection.getChannelSet();
            if (channelSet.size() > maxChannelsPerBucket) {
                List<Set<Integer>> channelSetList =
                        partitionChannelSet(channelSet, maxChannelsPerBucket);
                int newTotalNumBuckets = totalNumBuckets + channelSetList.size() - 1;
                if (newTotalNumBuckets <= maxBuckets) {
                    List<Bucket> splitBuckets = createSplitBuckets(originalBucket, channelSetList);
                    for (Bucket bucket : splitBuckets) {
                        fixedBucketList.add(bucket);
                    }
                    totalNumBuckets = newTotalNumBuckets;
                } else {
                    fixedBucketList.add(originalBucket);
                }
            } else {
                fixedBucketList.add(originalBucket);
            }
        }
        return fixedBucketList;
    }
}
