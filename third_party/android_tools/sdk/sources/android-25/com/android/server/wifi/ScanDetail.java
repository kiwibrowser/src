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

package com.android.server.wifi;

import android.net.wifi.AnqpInformationElement;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiSsid;

import com.android.server.wifi.anqp.ANQPElement;
import com.android.server.wifi.anqp.Constants;
import com.android.server.wifi.anqp.HSFriendlyNameElement;
import com.android.server.wifi.anqp.RawByteElement;
import com.android.server.wifi.anqp.VenueNameElement;
import com.android.server.wifi.hotspot2.NetworkDetail;
import com.android.server.wifi.hotspot2.PasspointMatch;
import com.android.server.wifi.hotspot2.Utils;
import com.android.server.wifi.hotspot2.pps.HomeSP;

import java.util.List;
import java.util.Map;

/**
 * Wifi scan result details.
 */
public class ScanDetail {
    private final ScanResult mScanResult;
    private volatile NetworkDetail mNetworkDetail;
    private final Map<HomeSP, PasspointMatch> mMatches;
    private long mSeen = 0;

    public ScanDetail(NetworkDetail networkDetail, WifiSsid wifiSsid, String bssid,
            String caps, int level, int frequency, long tsf,
            ScanResult.InformationElement[] informationElements, List<String> anqpLines) {
        mNetworkDetail = networkDetail;
        mScanResult = new ScanResult(wifiSsid, bssid, networkDetail.getHESSID(),
                networkDetail.getAnqpDomainID(), networkDetail.getOsuProviders(),
                caps, level, frequency, tsf);
        mSeen = System.currentTimeMillis();
        //mScanResult.seen = mSeen;
        mScanResult.channelWidth = networkDetail.getChannelWidth();
        mScanResult.centerFreq0 = networkDetail.getCenterfreq0();
        mScanResult.centerFreq1 = networkDetail.getCenterfreq1();
        mScanResult.informationElements = informationElements;
        mScanResult.anqpLines = anqpLines;
        if (networkDetail.is80211McResponderSupport()) {
            mScanResult.setFlag(ScanResult.FLAG_80211mc_RESPONDER);
        }
        if (networkDetail.isInterworking()) {
            mScanResult.setFlag(ScanResult.FLAG_PASSPOINT_NETWORK);
        }
        mMatches = null;
    }

    public ScanDetail(WifiSsid wifiSsid, String bssid, String caps, int level, int frequency,
                      long tsf, long seen) {
        mNetworkDetail = null;
        mScanResult = new ScanResult(wifiSsid, bssid, 0L, -1, null, caps, level, frequency, tsf);
        mSeen = seen;
        //mScanResult.seen = mSeen;
        mScanResult.channelWidth = 0;
        mScanResult.centerFreq0 = 0;
        mScanResult.centerFreq1 = 0;
        mScanResult.flags = 0;
        mMatches = null;
    }

    public ScanDetail(ScanResult scanResult, NetworkDetail networkDetail,
                       Map<HomeSP, PasspointMatch> matches) {
        mScanResult = scanResult;
        mNetworkDetail = networkDetail;
        mMatches = matches;
        mSeen = mScanResult.seen;
    }

    /**
     * Update the data stored in the scan result with the provided information.
     *
     * @param networkDetail NetworkDetail
     * @param level int
     * @param wssid WifiSsid
     * @param ssid String
     * @param flags String
     * @param freq int
     * @param tsf long
     */
    public void updateResults(NetworkDetail networkDetail, int level, WifiSsid wssid, String ssid,
                              String flags, int freq, long tsf) {
        mScanResult.level = level;
        mScanResult.wifiSsid = wssid;
        // Keep existing API
        mScanResult.SSID = ssid;
        mScanResult.capabilities = flags;
        mScanResult.frequency = freq;
        mScanResult.timestamp = tsf;
        mSeen = System.currentTimeMillis();
        //mScanResult.seen = mSeen;
        mScanResult.channelWidth = networkDetail.getChannelWidth();
        mScanResult.centerFreq0 = networkDetail.getCenterfreq0();
        mScanResult.centerFreq1 = networkDetail.getCenterfreq1();
        if (networkDetail.is80211McResponderSupport()) {
            mScanResult.setFlag(ScanResult.FLAG_80211mc_RESPONDER);
        }
        if (networkDetail.isInterworking()) {
            mScanResult.setFlag(ScanResult.FLAG_PASSPOINT_NETWORK);
        }
    }

    /**
     * Store ANQ element information
     *
     * @param anqpElements Map<Constants.ANQPElementType, ANQPElement>
     */
    public void propagateANQPInfo(Map<Constants.ANQPElementType, ANQPElement> anqpElements) {
        if (anqpElements.isEmpty()) {
            return;
        }
        mNetworkDetail = mNetworkDetail.complete(anqpElements);
        HSFriendlyNameElement fne = (HSFriendlyNameElement) anqpElements.get(
                Constants.ANQPElementType.HSFriendlyName);
        // !!! Match with language
        if (fne != null && !fne.getNames().isEmpty()) {
            mScanResult.venueName = fne.getNames().get(0).getText();
        } else {
            VenueNameElement vne =
                    (((VenueNameElement) anqpElements.get(
                            Constants.ANQPElementType.ANQPVenueName)));
            if (vne != null && !vne.getNames().isEmpty()) {
                mScanResult.venueName = vne.getNames().get(0).getText();
            }
        }
        RawByteElement osuProviders = (RawByteElement) anqpElements
                .get(Constants.ANQPElementType.HSOSUProviders);
        if (osuProviders != null) {
            mScanResult.anqpElements = new AnqpInformationElement[1];
            mScanResult.anqpElements[0] =
                    new AnqpInformationElement(AnqpInformationElement.HOTSPOT20_VENDOR_ID,
                            AnqpInformationElement.HS_OSU_PROVIDERS, osuProviders.getPayload());
        }
    }

    public ScanResult getScanResult() {
        return mScanResult;
    }

    public NetworkDetail getNetworkDetail() {
        return mNetworkDetail;
    }

    public String getSSID() {
        return mNetworkDetail == null ? mScanResult.SSID : mNetworkDetail.getSSID();
    }

    public String getBSSIDString() {
        return  mNetworkDetail == null ? mScanResult.BSSID : mNetworkDetail.getBSSIDString();
    }

    /**
     *  Return the network detail key string.
     */
    public String toKeyString() {
        NetworkDetail networkDetail = mNetworkDetail;
        if (networkDetail != null) {
            return networkDetail.toKeyString();
        } else {
            return String.format("'%s':%012x",
                                 mScanResult.BSSID,
                                 Utils.parseMac(mScanResult.BSSID));
        }
    }

    /**
     * Return the time this network was last seen.
     */
    public long getSeen() {
        return mSeen;
    }

    /**
     * Update the time this network was last seen to the current system time.
     */
    public long setSeen() {
        mSeen = System.currentTimeMillis();
        mScanResult.seen = mSeen;
        return mSeen;
    }

    @Override
    public String toString() {
        try {
            return String.format("'%s'/%012x",
                                 mScanResult.SSID,
                                 Utils.parseMac(mScanResult.BSSID));
        } catch (IllegalArgumentException iae) {
            return String.format("'%s'/----", mScanResult.BSSID);
        }
    }
}
