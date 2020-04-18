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

import android.content.Context;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiScanner;
import android.os.Looper;

import com.android.server.wifi.Clock;
import com.android.server.wifi.WifiNative;

import java.util.Comparator;

/**
 * Defines the interface to the Wifi hardware required for the WifiScanner API
 */
public abstract class WifiScannerImpl {

    /**
     * A factory that create a {@link com.android.server.wifi.scanner.WifiScannerImpl}
     */
    public static interface WifiScannerImplFactory {
        WifiScannerImpl create(Context context, Looper looper, Clock clock);
    }

    /**
     * Factory that create the implementation that is most appropriate for the system.
     * This factory should only ever be used once.
     */
    public static final WifiScannerImplFactory DEFAULT_FACTORY = new WifiScannerImplFactory() {
            public WifiScannerImpl create(Context context, Looper looper, Clock clock) {
                WifiNative wifiNative = WifiNative.getWlanNativeInterface();
                if (wifiNative.getScanCapabilities(new WifiNative.ScanCapabilities())) {
                    return new HalWifiScannerImpl(context, wifiNative, looper, clock);
                } else {
                    return new SupplicantWifiScannerImpl(context, wifiNative, looper, clock);
                }
            }
        };

    /**
     * A comparator that implements the sort order that is expected for scan results
     */
    protected static final Comparator<ScanResult> SCAN_RESULT_SORT_COMPARATOR =
            new Comparator<ScanResult>() {
        public int compare(ScanResult r1, ScanResult r2) {
            return r2.level - r1.level;
        }
    };

    /**
     * Cleanup any ongoing operations. This may be called when the driver is unloaded.
     * There is no expectation that failure events are returned for ongoing operations.
     */
    public abstract void cleanup();

    /**
     * Get the supported scan capabilities.
     *
     * @param capabilities Object that will be filled with the supported capabilities if successful
     * @return true if the scan capabilities were retrieved successfully
     */
    public abstract boolean getScanCapabilities(WifiNative.ScanCapabilities capabilities);

    /**
     * Get a ChannelHelper that can be used to perform operations on scan channels
     */
    public abstract ChannelHelper getChannelHelper();

    /**
     * Start a one time scan. This method should only be called when there is no scan going on
     * (after a callback indicating that the previous scan succeeded/failed).
     * @return if the scan paramaters are valid
     * Note this may return true even if the parameters are not accepted by the chip because the
     * scan may be scheduled async.
     */
    public abstract boolean startSingleScan(WifiNative.ScanSettings settings,
            WifiNative.ScanEventHandler eventHandler);
    /**
     * Get the scan results of the most recent single scan. This should be called immediately when
     * the scan success callback is receieved.
     */
    public abstract WifiScanner.ScanData getLatestSingleScanResults();

    /**
     * Start a background scan. Calling this method while a background scan is already in process
     * will interrupt the previous scan settings and replace it with the new ones.
     * @return if the scan paramaters are valid
     * Note this may return true even if the parameters are not accepted by the chip because the
     * scan may be scheduled async.
     */
    public abstract boolean startBatchedScan(WifiNative.ScanSettings settings,
            WifiNative.ScanEventHandler eventHandler);
    /**
     * Stop the currently active background scan
     */
    public abstract void stopBatchedScan();

    /**
     * Pause the currently active background scan
     */
    public abstract void pauseBatchedScan();

    /**
     * Restart the currently paused background scan
     */
    public abstract void restartBatchedScan();

    /**
     * Get the latest cached scan results from the last scan event. This should be called
     * immediately when the scan success callback is receieved.
     */
    public abstract WifiScanner.ScanData[] getLatestBatchedScanResults(boolean flush);

    /**
     * Set PNO list to start PNO background scan.
     * @param settings PNO settings for this scan.
     * @param eventHandler Event handler for notifying the scan results.
     * @return true if success, false otherwise
     */
    public abstract boolean setHwPnoList(WifiNative.PnoSettings settings,
            WifiNative.PnoEventHandler eventHandler);

    /**
     * Reset PNO list to terminate PNO background scan.
     * @return true if success, false otherwise
     */
    public abstract boolean resetHwPnoList();

    /**
     * This returns whether HW PNO is supported or not.
     * @param isConnectedPno Whether this is connected PNO vs disconnected PNO.
     * @return true if HW PNO is supported, false otherwise.
     */
    public abstract boolean isHwPnoSupported(boolean isConnectedPno);

    /**
     * This returns whether a background scan should be running for HW PNO scan or not.
     * @return true if background scan needs to be started, false otherwise.
     */
    public abstract boolean shouldScheduleBackgroundScanForHwPno();

    /**
     * Set a new hotlist
     */
    public abstract boolean setHotlist(WifiScanner.HotlistSettings settings,
            WifiNative.HotlistEventHandler eventHandler);

    /**
     * Reset any active hotlist
     */
    public abstract void resetHotlist();

    /**
     * Start tracking significant wifi changes
     */
    public abstract boolean trackSignificantWifiChange(WifiScanner.WifiChangeSettings settings,
            WifiNative.SignificantWifiChangeEventHandler handler);

    /**
     * Stop tracking significant wifi changes
     */
    public abstract void untrackSignificantWifiChange();
}
