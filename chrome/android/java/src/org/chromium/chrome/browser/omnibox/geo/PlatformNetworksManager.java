// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox.geo;

import android.Manifest;
import android.annotation.TargetApi;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Build.VERSION_CODES;
import android.os.Process;
import android.os.SystemClock;
import android.telephony.CellIdentityCdma;
import android.telephony.CellIdentityGsm;
import android.telephony.CellIdentityLte;
import android.telephony.CellIdentityWcdma;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoWcdma;
import android.telephony.TelephonyManager;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.browser.omnibox.geo.VisibleNetworks.VisibleCell;
import org.chromium.chrome.browser.omnibox.geo.VisibleNetworks.VisibleWifi;

import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

/**
 * Util methods for platform networking APIs.
 */
class PlatformNetworksManager {
    @VisibleForTesting
    static TimeProvider sTimeProvider = new TimeProvider();

    /**
     * Equivalent to WifiSsid.NONE which is hidden for some reason. This is returned by
     * {@link WifiManager} if it cannot get the ssid for the connected wifi access point.
     */
    static final String UNKNOWN_SSID = "<unknown ssid>";

    static VisibleWifi getConnectedWifi(Context context, WifiManager wifiManager) {
        if (hasLocationAndWifiPermission(context)) {
            WifiInfo wifiInfo = wifiManager.getConnectionInfo();
            return connectedWifiInfoToVisibleWifi(wifiInfo);
        }
        if (hasLocationPermission(context)) {
            // Only location permission, so fallback to pre-marshmallow.
            return getConnectedWifiPreMarshmallow(context);
        }
        return VisibleWifi.NO_WIFI_INFO;
    }

    static VisibleWifi getConnectedWifiPreMarshmallow(Context context) {
        Intent intent = context.getApplicationContext().registerReceiver(
                null, new IntentFilter(WifiManager.NETWORK_STATE_CHANGED_ACTION));
        if (intent != null) {
            WifiInfo wifiInfo = intent.getParcelableExtra(WifiManager.EXTRA_WIFI_INFO);
            return connectedWifiInfoToVisibleWifi(wifiInfo);
        }
        return VisibleWifi.NO_WIFI_INFO;
    }

    private static VisibleWifi connectedWifiInfoToVisibleWifi(@Nullable WifiInfo wifiInfo) {
        if (wifiInfo == null) {
            return VisibleWifi.NO_WIFI_INFO;
        }
        String ssid = wifiInfo.getSSID();
        if (ssid == null || UNKNOWN_SSID.equals(ssid)) {
            // No SSID.
            ssid = null;
        } else {
            // Remove double quotation if ssid has double quotation.
            if (ssid.startsWith("\"") && ssid.endsWith("\"") && ssid.length() > 2) {
                ssid = ssid.substring(1, ssid.length() - 1);
            }
        }
        String bssid = wifiInfo.getBSSID();
        // It's connected, so use current time.
        return VisibleWifi.create(ssid, bssid, null, sTimeProvider.getCurrentTime());
    }

    static Set<VisibleWifi> getAllVisibleWifis(Context context, WifiManager wifiManager) {
        if (!hasLocationAndWifiPermission(context)) {
            return Collections.emptySet();
        }
        Set<VisibleWifi> visibleWifis = new HashSet<>();
        // Do not trigger a scan, but use current visible networks from latest scan.
        List<ScanResult> scanResults = wifiManager.getScanResults();
        if (scanResults == null) {
            return visibleWifis;
        }
        long elapsedTime = sTimeProvider.getElapsedRealtime();
        long currentTime = sTimeProvider.getCurrentTime();
        for (int i = 0; i < scanResults.size(); i++) {
            ScanResult scanResult = scanResults.get(i);
            String bssid = scanResult.BSSID;
            if (bssid == null) continue;
            Long scanResultTimestamp = scanResultTimestamp(scanResult);
            Long wifiTimestamp = null;
            if (scanResultTimestamp != null) {
                long ageMs = elapsedTime - TimeUnit.MICROSECONDS.toMillis(scanResultTimestamp);
                wifiTimestamp = currentTime - ageMs;
            }
            visibleWifis.add(
                    VisibleWifi.create(scanResult.SSID, bssid, scanResult.level, wifiTimestamp));
        }
        return visibleWifis;
    }

    @TargetApi(VERSION_CODES.JELLY_BEAN_MR1)
    @Nullable
    private static Long scanResultTimestamp(ScanResult scanResult) {
        if (Build.VERSION.SDK_INT < VERSION_CODES.JELLY_BEAN_MR1) {
            return null;
        }
        return scanResult.timestamp;
    }

    static Set<VisibleCell> getAllVisibleCells(Context context, TelephonyManager telephonyManager) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1) {
            // CellInfo is only available JB MR1 upwards.
            return Collections.emptySet();
        }
        if (!hasLocationPermission(context)) {
            return Collections.emptySet();
        }
        return getAllVisibleCellsPostJellyBeanMr1(telephonyManager);
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    private static Set<VisibleCell> getAllVisibleCellsPostJellyBeanMr1(
            TelephonyManager telephonyManager) {
        Set<VisibleCell> visibleCells = new HashSet<>();
        // Retrieve visible cell networks
        List<CellInfo> cellInfos = telephonyManager.getAllCellInfo();
        if (cellInfos == null) {
            return visibleCells;
        }

        long elapsedTime = sTimeProvider.getElapsedRealtime();
        long currentTime = sTimeProvider.getCurrentTime();
        for (int i = 0; i < cellInfos.size(); i++) {
            CellInfo cellInfo = cellInfos.get(i);
            VisibleCell visibleCell =
                    getVisibleCellPostJellyBeanMr1(cellInfo, elapsedTime, currentTime);
            if (visibleCell.radioType() != VisibleCell.UNKNOWN_RADIO_TYPE) {
                visibleCells.add(visibleCell);
            }
        }
        return visibleCells;
    }

    static VisibleCell getConnectedCell(Context context, TelephonyManager telephonyManager) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1) {
            // CellInfo is only available JB MR1 upwards.
            return VisibleCell.UNKNOWN_VISIBLE_CELL;
        }
        return getConnectedCellPostJellyBeanMr1(context, telephonyManager);
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    private static VisibleCell getConnectedCellPostJellyBeanMr1(
            Context context, TelephonyManager telephonyManager) {
        if (!hasLocationPermission(context)) {
            return VisibleCell.UNKNOWN_MISSING_LOCATION_PERMISSION_VISIBLE_CELL;
        }
        CellInfo cellInfo = getActiveCellInfo(telephonyManager);
        return getVisibleCellPostJellyBeanMr1(
                cellInfo, sTimeProvider.getElapsedRealtime(), sTimeProvider.getCurrentTime());
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    private static VisibleCell getVisibleCellPostJellyBeanMr1(
            @Nullable CellInfo cellInfo, long elapsedTime, long currentTime) {
        if (cellInfo == null) {
            return VisibleCell.UNKNOWN_VISIBLE_CELL;
        }
        long cellInfoAge = elapsedTime - TimeUnit.NANOSECONDS.toMillis(cellInfo.getTimeStamp());
        long cellTimestamp = currentTime - cellInfoAge;
        if (cellInfo instanceof CellInfoCdma) {
            CellIdentityCdma cellIdentityCdma = ((CellInfoCdma) cellInfo).getCellIdentity();
            return VisibleCell.builder(VisibleCell.CDMA_RADIO_TYPE)
                    .setCellId(cellIdentityCdma.getBasestationId())
                    .setLocationAreaCode(cellIdentityCdma.getNetworkId())
                    .setMobileNetworkCode(cellIdentityCdma.getSystemId())
                    .setTimestamp(cellTimestamp)
                    .build();
        }
        if (cellInfo instanceof CellInfoGsm) {
            CellIdentityGsm cellIdentityGsm = ((CellInfoGsm) cellInfo).getCellIdentity();
            return VisibleCell.builder(VisibleCell.GSM_RADIO_TYPE)
                    .setCellId(cellIdentityGsm.getCid())
                    .setLocationAreaCode(cellIdentityGsm.getLac())
                    .setMobileCountryCode(cellIdentityGsm.getMcc())
                    .setMobileNetworkCode(cellIdentityGsm.getMnc())
                    .setTimestamp(cellTimestamp)
                    .build();
        }
        if (cellInfo instanceof CellInfoLte) {
            CellIdentityLte cellIdLte = ((CellInfoLte) cellInfo).getCellIdentity();
            return VisibleCell.builder(VisibleCell.LTE_RADIO_TYPE)
                    .setCellId(cellIdLte.getCi())
                    .setMobileCountryCode(cellIdLte.getMcc())
                    .setMobileNetworkCode(cellIdLte.getMnc())
                    .setPhysicalCellId(cellIdLte.getPci())
                    .setTrackingAreaCode(cellIdLte.getTac())
                    .setTimestamp(cellTimestamp)
                    .build();
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2
                && cellInfo instanceof CellInfoWcdma) {
            // CellInfoWcdma is only usable JB MR2 upwards.
            CellIdentityWcdma cellIdentityWcdma = ((CellInfoWcdma) cellInfo).getCellIdentity();
            return VisibleCell.builder(VisibleCell.WCDMA_RADIO_TYPE)
                    .setCellId(cellIdentityWcdma.getCid())
                    .setLocationAreaCode(cellIdentityWcdma.getLac())
                    .setMobileCountryCode(cellIdentityWcdma.getMcc())
                    .setMobileNetworkCode(cellIdentityWcdma.getMnc())
                    .setPrimaryScramblingCode(cellIdentityWcdma.getPsc())
                    .setTimestamp(cellTimestamp)
                    .build();
        }
        return VisibleCell.UNKNOWN_VISIBLE_CELL;
    }

    /**
     * Returns a CellInfo object representing the currently registered base stations, containing
     * its identity fields and signal strength. Null if no base station is active.
     */
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    @Nullable
    private static CellInfo getActiveCellInfo(TelephonyManager telephonyManager) {
        int numRegisteredCellInfo = 0;
        List<CellInfo> cellInfos = telephonyManager.getAllCellInfo();

        if (cellInfos == null) {
            return null;
        }
        CellInfo result = null;

        for (int i = 0; i < cellInfos.size(); i++) {
            CellInfo cellInfo = cellInfos.get(i);
            if (cellInfo.isRegistered()) {
                numRegisteredCellInfo++;
                if (numRegisteredCellInfo > 1) {
                    return null;
                }
                result = cellInfo;
            }
        }
        // Only found one registered cellinfo, so we know which base station was used to measure
        // network quality
        return result;
    }

    /**
     * Computes the visible networks.
     *
     * @param context The application context
     * @param includeAllVisibleNotConnectedNetworks Whether to include all visible networks that are
     *     not connected. This should only be true when performing a background non-synchronous
     *     call, since including not connected networks can degrade latency.
     */
    static VisibleNetworks computeVisibleNetworks(
            Context context, boolean includeAllVisibleNotConnectedNetworks) {
        WifiManager wifiManager = (WifiManager) context.getApplicationContext().getSystemService(
                Context.WIFI_SERVICE);
        TelephonyManager telephonyManager =
                (TelephonyManager) context.getApplicationContext().getSystemService(
                        Context.TELEPHONY_SERVICE);

        VisibleWifi connectedWifi;
        VisibleCell connectedCell;
        Set<VisibleWifi> allVisibleWifis = null;
        Set<VisibleCell> allVisibleCells = null;

        connectedWifi = getConnectedWifi(context, wifiManager);
        if (connectedWifi != null && connectedWifi.bssid() == null) {
            // If the connected wifi is unknown, do not use it.
            connectedWifi = null;
        }
        connectedCell = getConnectedCell(context, telephonyManager);
        if (connectedCell != null
                && (connectedCell.radioType() == VisibleCell.UNKNOWN_RADIO_TYPE
                           || connectedCell.radioType()
                                   == VisibleCell.UNKNOWN_MISSING_LOCATION_PERMISSION_RADIO_TYPE)) {
            // If the radio type is unknown, do not use it.
            connectedCell = null;
        }
        if (includeAllVisibleNotConnectedNetworks) {
            allVisibleWifis = getAllVisibleWifis(context, wifiManager);
            allVisibleCells = getAllVisibleCells(context, telephonyManager);
        }
        return VisibleNetworks.create(
                connectedWifi, connectedCell, allVisibleWifis, allVisibleCells);
    }

    private static boolean hasPermission(Context context, String permission) {
        return ApiCompatibilityUtils.checkPermission(
                       context, permission, Process.myPid(), Process.myUid())
                == PackageManager.PERMISSION_GRANTED;
    }

    private static boolean hasLocationPermission(Context context) {
        return hasPermission(context, Manifest.permission.ACCESS_COARSE_LOCATION)
                || hasPermission(context, Manifest.permission.ACCESS_FINE_LOCATION);
    }

    private static boolean hasLocationAndWifiPermission(Context context) {
        return hasLocationPermission(context)
                && hasPermission(context, Manifest.permission.ACCESS_WIFI_STATE);
    }

    /**
     * Wrapper around static time providers that allows us to mock the implementation in
     * tests.
     */
    static class TimeProvider {
        /**
         * Get current time in milliseconds.
         */
        long getCurrentTime() {
            return System.currentTimeMillis();
        }

        /**
         * Get elapsed real time in milliseconds.
         */
        long getElapsedRealtime() {
            return SystemClock.elapsedRealtime();
        }
    }
}
