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

import android.annotation.Nullable;
import android.content.Context;
import android.net.NetworkKey;
import android.net.NetworkScoreManager;
import android.net.WifiKey;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.text.TextUtils;
import android.util.LocalLog;
import android.util.Log;
import android.util.Pair;

import com.android.internal.R;
import com.android.internal.annotations.VisibleForTesting;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * This class looks at all the connectivity scan results then
 * select an network for the phone to connect/roam to.
 */
public class WifiQualifiedNetworkSelector {
    private WifiConfigManager mWifiConfigManager;
    private WifiInfo mWifiInfo;
    private NetworkScoreManager mScoreManager;
    private WifiNetworkScoreCache mNetworkScoreCache;
    private Clock mClock;
    private static final String TAG = "WifiQualifiedNetworkSelector:";
    // Always enable debugging logs for now since QNS is still a new feature.
    private static final boolean FORCE_DEBUG = true;
    private boolean mDbg = FORCE_DEBUG;
    private WifiConfiguration mCurrentConnectedNetwork = null;
    private String mCurrentBssid = null;
    //buffer most recent scan results
    private List<ScanDetail> mScanDetails = null;
    //buffer of filtered scan results (Scan results considered by network selection) & associated
    //WifiConfiguration (if any)
    private volatile List<Pair<ScanDetail, WifiConfiguration>> mFilteredScanDetails = null;

    //Minimum time gap between last successful Qualified Network Selection and new selection attempt
    //usable only when current state is connected state   default 10 s
    private static final int MINIMUM_QUALIFIED_NETWORK_SELECTION_INTERVAL = 10 * 1000;

    //if current network is on 2.4GHz band and has a RSSI over this, need not new network selection
    public static final int QUALIFIED_RSSI_24G_BAND = -73;
    //if current network is on 5GHz band and has a RSSI over this, need not new network selection
    public static final int QUALIFIED_RSSI_5G_BAND = -70;
    //any RSSI larger than this will benefit the traffic very limited
    public static final int RSSI_SATURATION_2G_BAND = -60;
    public static final int RSSI_SATURATION_5G_BAND = -57;
    //Any value below this will be considered not usable
    public static final int MINIMUM_2G_ACCEPT_RSSI = -85;
    public static final int MINIMUM_5G_ACCEPT_RSSI = -82;

    public static final int RSSI_SCORE_SLOPE = 4;
    public static final int RSSI_SCORE_OFFSET = 85;

    public static final int BAND_AWARD_5GHz = 40;
    public static final int SAME_NETWORK_AWARD = 16;

    public static final int SAME_BSSID_AWARD = 24;
    public static final int LAST_SELECTION_AWARD = 480;
    public static final int PASSPOINT_SECURITY_AWARD = 40;
    public static final int SECURITY_AWARD = 80;
    public static final int BSSID_BLACKLIST_THRESHOLD = 3;
    public static final int BSSID_BLACKLIST_EXPIRE_TIME = 5 * 60 * 1000;
    private final int mNoIntnetPenalty;
    //TODO: check whether we still need this one when we update the scan manager
    public static final int SCAN_RESULT_MAXIMUNM_AGE = 40000;
    private static final int INVALID_TIME_STAMP = -1;
    private long mLastQualifiedNetworkSelectionTimeStamp = INVALID_TIME_STAMP;

    private final LocalLog mLocalLog = new LocalLog(512);
    private int mRssiScoreSlope = RSSI_SCORE_SLOPE;
    private int mRssiScoreOffset = RSSI_SCORE_OFFSET;
    private int mSameBssidAward = SAME_BSSID_AWARD;
    private int mLastSelectionAward = LAST_SELECTION_AWARD;
    private int mPasspointSecurityAward = PASSPOINT_SECURITY_AWARD;
    private int mSecurityAward = SECURITY_AWARD;
    private int mUserPreferedBand = WifiManager.WIFI_FREQUENCY_BAND_AUTO;
    private Map<String, BssidBlacklistStatus> mBssidBlacklist =
            new HashMap<String, BssidBlacklistStatus>();

    /**
     * class save the blacklist status of a given BSSID
     */
    private static class BssidBlacklistStatus {
        //how many times it is requested to be blacklisted (association rejection trigger this)
        int mCounter;
        boolean mIsBlacklisted;
        long mBlacklistedTimeStamp = INVALID_TIME_STAMP;
    }

    private void localLog(String log) {
        if (mDbg) {
            mLocalLog.log(log);
        }
    }

    private void localLoge(String log) {
        mLocalLog.log(log);
    }

    @VisibleForTesting
    void setWifiNetworkScoreCache(WifiNetworkScoreCache cache) {
        mNetworkScoreCache = cache;
    }

    /**
     * @return current target connected network
     */
    public WifiConfiguration getConnetionTargetNetwork() {
        return mCurrentConnectedNetwork;
    }

    /**
     * @return the list of ScanDetails scored as potential candidates by the last run of
     * selectQualifiedNetwork, this will be empty if QNS determined no selection was needed on last
     * run. This includes scan details of sufficient signal strength, and had an associated
     * WifiConfiguration.
     */
    public List<Pair<ScanDetail, WifiConfiguration>> getFilteredScanDetails() {
        return mFilteredScanDetails;
    }

    /**
     * set the user selected preferred band
     *
     * @param band preferred band user selected
     */
    public void setUserPreferredBand(int band) {
        mUserPreferedBand = band;
    }

    WifiQualifiedNetworkSelector(WifiConfigManager configureStore, Context context,
            WifiInfo wifiInfo, Clock clock) {
        mWifiConfigManager = configureStore;
        mWifiInfo = wifiInfo;
        mClock = clock;
        mScoreManager =
                (NetworkScoreManager) context.getSystemService(Context.NETWORK_SCORE_SERVICE);
        if (mScoreManager != null) {
            mNetworkScoreCache = new WifiNetworkScoreCache(context);
            mScoreManager.registerNetworkScoreCache(NetworkKey.TYPE_WIFI, mNetworkScoreCache);
        } else {
            localLoge("No network score service: Couldn't register as a WiFi score Manager, type="
                    + NetworkKey.TYPE_WIFI + " service= " + Context.NETWORK_SCORE_SERVICE);
            mNetworkScoreCache = null;
        }

        mRssiScoreSlope = context.getResources().getInteger(
                R.integer.config_wifi_framework_RSSI_SCORE_SLOPE);
        mRssiScoreOffset = context.getResources().getInteger(
                R.integer.config_wifi_framework_RSSI_SCORE_OFFSET);
        mSameBssidAward = context.getResources().getInteger(
                R.integer.config_wifi_framework_SAME_BSSID_AWARD);
        mLastSelectionAward = context.getResources().getInteger(
                R.integer.config_wifi_framework_LAST_SELECTION_AWARD);
        mPasspointSecurityAward = context.getResources().getInteger(
                R.integer.config_wifi_framework_PASSPOINT_SECURITY_AWARD);
        mSecurityAward = context.getResources().getInteger(
                R.integer.config_wifi_framework_SECURITY_AWARD);
        mNoIntnetPenalty = (mWifiConfigManager.mThresholdSaturatedRssi24.get() + mRssiScoreOffset)
                * mRssiScoreSlope + mWifiConfigManager.mBandAward5Ghz.get()
                + mWifiConfigManager.mCurrentNetworkBoost.get() + mSameBssidAward + mSecurityAward;
    }

    void enableVerboseLogging(int verbose) {
        mDbg = verbose > 0 || FORCE_DEBUG;
    }

    private String getNetworkString(WifiConfiguration network) {
        if (network == null) {
            return null;
        }

        return (network.SSID + ":" + network.networkId);

    }

    /**
     * check whether current network is good enough we need not consider any potential switch
     *
     * @param currentNetwork -- current connected network
     * @return true -- qualified and do not consider potential network switch
     *         false -- not good enough and should try potential network switch
     */
    private boolean isNetworkQualified(WifiConfiguration currentNetwork) {

        if (currentNetwork == null) {
            localLog("Disconnected");
            return false;
        } else {
            localLog("Current network is: " + currentNetwork.SSID + " ,ID is: "
                    + currentNetwork.networkId);
        }

        //if current connected network is an ephemeral network,we will consider
        // there is no current network
        if (currentNetwork.ephemeral) {
            localLog("Current is ephemeral. Start reselect");
            return false;
        }

        //if current network is open network, not qualified
        if (mWifiConfigManager.isOpenNetwork(currentNetwork)) {
            localLog("Current network is open network");
            return false;
        }

        // Current network band must match with user preference selection
        if (mWifiInfo.is24GHz() && (mUserPreferedBand != WifiManager.WIFI_FREQUENCY_BAND_2GHZ)) {
            localLog("Current band dose not match user preference. Start Qualified Network"
                    + " Selection Current band = " + (mWifiInfo.is24GHz() ? "2.4GHz band"
                    : "5GHz band") + "UserPreference band = " + mUserPreferedBand);
            return false;
        }

        int currentRssi = mWifiInfo.getRssi();
        if ((mWifiInfo.is24GHz()
                        && currentRssi < mWifiConfigManager.mThresholdQualifiedRssi24.get())
                || (mWifiInfo.is5GHz()
                        && currentRssi < mWifiConfigManager.mThresholdQualifiedRssi5.get())) {
            localLog("Current band = " + (mWifiInfo.is24GHz() ? "2.4GHz band" : "5GHz band")
                    + "current RSSI is: " + currentRssi);
            return false;
        }

        return true;
    }

    /**
     * check whether QualifiedNetworkSelection is needed or not
     *
     * @param isLinkDebouncing true -- Link layer is under debouncing
     *                         false -- Link layer is not under debouncing
     * @param isConnected true -- device is connected to an AP currently
     *                    false -- device is not connected to an AP currently
     * @param isDisconnected true -- WifiStateMachine is at disconnected state
     *                       false -- WifiStateMachine is not at disconnected state
     * @param isSupplicantTransientState true -- supplicant is in a transient state now
     *                                   false -- supplicant is not in a transient state now
     * @return true -- need a Qualified Network Selection procedure
     *         false -- do not need a QualifiedNetworkSelection procedure
     */
    private boolean needQualifiedNetworkSelection(boolean isLinkDebouncing, boolean isConnected,
            boolean isDisconnected, boolean isSupplicantTransientState) {
        if (mScanDetails.size() == 0) {
            localLog("empty scan result");
            return false;
        }

        // Do not trigger Qualified Network Selection during L2 link debouncing procedure
        if (isLinkDebouncing) {
            localLog("Need not Qualified Network Selection during L2 debouncing");
            return false;
        }

        if (isConnected) {
            //already connected. Just try to find better candidate
            //if switch network is not allowed in connected mode, do not trigger Qualified Network
            //Selection
            if (!mWifiConfigManager.getEnableAutoJoinWhenAssociated()) {
                localLog("Switch network under connection is not allowed");
                return false;
            }

            //Do not select again if last selection is within
            //MINIMUM_QUALIFIED_NETWORK_SELECTION_INTERVAL
            if (mLastQualifiedNetworkSelectionTimeStamp != INVALID_TIME_STAMP) {
                long gap = mClock.elapsedRealtime() - mLastQualifiedNetworkSelectionTimeStamp;
                if (gap < MINIMUM_QUALIFIED_NETWORK_SELECTION_INTERVAL) {
                    localLog("Too short to last successful Qualified Network Selection Gap is:"
                            + gap + " ms!");
                    return false;
                }
            }

            WifiConfiguration currentNetwork =
                    mWifiConfigManager.getWifiConfiguration(mWifiInfo.getNetworkId());
            if (currentNetwork == null) {
                // WifiStateMachine in connected state but WifiInfo is not. It means there is a race
                // condition happened. Do not make QNS until WifiStateMachine goes into
                // disconnected state
                return false;
            }

            if (!isNetworkQualified(mCurrentConnectedNetwork)) {
                //need not trigger Qualified Network Selection if current network is qualified
                localLog("Current network is not qualified");
                return true;
            } else {
                return false;
            }
        } else if (isDisconnected) {
            mCurrentConnectedNetwork = null;
            mCurrentBssid = null;
            //Do not start Qualified Network Selection if current state is a transient state
            if (isSupplicantTransientState) {
                return false;
            }
        } else {
            //Do not allow new network selection in other state
            localLog("WifiStateMachine is not on connected or disconnected state");
            return false;
        }

        return true;
    }

    int calculateBssidScore(ScanResult scanResult, WifiConfiguration network,
            WifiConfiguration currentNetwork, boolean sameBssid, boolean sameSelect,
            StringBuffer sbuf) {

        int score = 0;
        //calculate the RSSI score
        int rssi = scanResult.level <= mWifiConfigManager.mThresholdSaturatedRssi24.get()
                ? scanResult.level : mWifiConfigManager.mThresholdSaturatedRssi24.get();
        score += (rssi + mRssiScoreOffset) * mRssiScoreSlope;
        sbuf.append(" RSSI score: " +  score);
        if (scanResult.is5GHz()) {
            //5GHz band
            score += mWifiConfigManager.mBandAward5Ghz.get();
            sbuf.append(" 5GHz bonus: " + mWifiConfigManager.mBandAward5Ghz.get());
        }

        //last user selection award
        if (sameSelect) {
            long timeDifference = mClock.elapsedRealtime()
                    - mWifiConfigManager.getLastSelectedTimeStamp();

            if (timeDifference > 0) {
                int bonus = mLastSelectionAward - (int) (timeDifference / 1000 / 60);
                score += bonus > 0 ? bonus : 0;
                sbuf.append(" User selected it last time " + (timeDifference / 1000 / 60)
                        + " minutes ago, bonus:" + bonus);
            }
        }

        //same network award
        if (network == currentNetwork || network.isLinked(currentNetwork)) {
            score += mWifiConfigManager.mCurrentNetworkBoost.get();
            sbuf.append(" Same network with current associated. Bonus: "
                    + mWifiConfigManager.mCurrentNetworkBoost.get());
        }

        //same BSSID award
        if (sameBssid) {
            score += mSameBssidAward;
            sbuf.append(" Same BSSID with current association. Bonus: " + mSameBssidAward);
        }

        //security award
        if (network.isPasspoint()) {
            score += mPasspointSecurityAward;
            sbuf.append(" Passpoint Bonus:" + mPasspointSecurityAward);
        } else if (!mWifiConfigManager.isOpenNetwork(network)) {
            score += mSecurityAward;
            sbuf.append(" Secure network Bonus:" + mSecurityAward);
        }

        //Penalty for no internet network. Make sure if there is any network with Internet,
        //however, if there is no any other network with internet, this network can be chosen
        if (network.numNoInternetAccessReports > 0 && !network.validatedInternetAccess) {
            score -= mNoIntnetPenalty;
            sbuf.append(" No internet Penalty:-" + mNoIntnetPenalty);
        }


        sbuf.append(" Score for scanResult: " + scanResult +  " and Network ID: "
                + network.networkId + " final score:" + score + "\n\n");

        return score;
    }

    /**
     * This API try to update all the saved networks' network selection status
     */
    private void updateSavedNetworkSelectionStatus() {
        List<WifiConfiguration> savedNetworks = mWifiConfigManager.getSavedNetworks();
        if (savedNetworks.size() == 0) {
            localLog("no saved network");
            return;
        }

        StringBuffer sbuf = new StringBuffer("Saved Network List\n");
        for (WifiConfiguration network : savedNetworks) {
            WifiConfiguration config = mWifiConfigManager.getWifiConfiguration(network.networkId);
            WifiConfiguration.NetworkSelectionStatus status =
                    config.getNetworkSelectionStatus();

            //If the configuration is temporarily disabled, try to re-enable it
            if (status.isNetworkTemporaryDisabled()) {
                mWifiConfigManager.tryEnableQualifiedNetwork(network.networkId);
            }

            //clean the cached candidate, score and seen
            status.setCandidate(null);
            status.setCandidateScore(Integer.MIN_VALUE);
            status.setSeenInLastQualifiedNetworkSelection(false);

            //print the debug messages
            sbuf.append("    " + getNetworkString(network) + " " + " User Preferred BSSID:"
                    + network.BSSID + " FQDN:" + network.FQDN + " "
                    + status.getNetworkStatusString() + " Disable account: ");
            for (int index = status.NETWORK_SELECTION_ENABLE;
                    index < status.NETWORK_SELECTION_DISABLED_MAX; index++) {
                sbuf.append(status.getDisableReasonCounter(index) + " ");
            }
            sbuf.append("Connect Choice:" + status.getConnectChoice() + " set time:"
                    + status.getConnectChoiceTimestamp());
            sbuf.append("\n");
        }
        localLog(sbuf.toString());
    }

    /**
     * This API is called when user explicitly select a network. Currently, it is used in following
     * cases:
     * (1) User explicitly choose to connect to a saved network
     * (2) User save a network after add a new network
     * (3) User save a network after modify a saved network
     * Following actions will be triggered:
     * 1. if this network is disabled, we need re-enable it again
     * 2. we considered user prefer this network over all the networks visible in latest network
     *    selection procedure
     *
     * @param netId new network ID for either the network the user choose or add
     * @param persist whether user has the authority to overwrite current connect choice
     * @return true -- There is change made to connection choice of any saved network
     *         false -- There is no change made to connection choice of any saved network
     */
    public boolean userSelectNetwork(int netId, boolean persist) {
        WifiConfiguration selected = mWifiConfigManager.getWifiConfiguration(netId);
        localLog("userSelectNetwork:" + netId + " persist:" + persist);
        if (selected == null || selected.SSID == null) {
            localLoge("userSelectNetwork: Bad configuration with nid=" + netId);
            return false;
        }


        if (!selected.getNetworkSelectionStatus().isNetworkEnabled()) {
            mWifiConfigManager.updateNetworkSelectionStatus(netId,
                    WifiConfiguration.NetworkSelectionStatus.NETWORK_SELECTION_ENABLE);
        }

        if (!persist) {
            localLog("User has no privilege to overwrite the current priority");
            return false;
        }

        boolean change = false;
        String key = selected.configKey();
        // This is only used for setting the connect choice timestamp for debugging purposes.
        long currentTime = mClock.currentTimeMillis();
        List<WifiConfiguration> savedNetworks = mWifiConfigManager.getSavedNetworks();

        for (WifiConfiguration network : savedNetworks) {
            WifiConfiguration config = mWifiConfigManager.getWifiConfiguration(network.networkId);
            WifiConfiguration.NetworkSelectionStatus status = config.getNetworkSelectionStatus();
            if (config.networkId == selected.networkId) {
                if (status.getConnectChoice() != null) {
                    localLog("Remove user selection preference of " + status.getConnectChoice()
                            + " Set Time: " + status.getConnectChoiceTimestamp() + " from "
                            + config.SSID + " : " + config.networkId);
                    status.setConnectChoice(null);
                    status.setConnectChoiceTimestamp(WifiConfiguration.NetworkSelectionStatus
                            .INVALID_NETWORK_SELECTION_DISABLE_TIMESTAMP);
                    change = true;
                }
                continue;
            }

            if (status.getSeenInLastQualifiedNetworkSelection()
                    && (status.getConnectChoice() == null
                    || !status.getConnectChoice().equals(key))) {
                localLog("Add key:" + key + " Set Time: " + currentTime + " to "
                        + getNetworkString(config));
                status.setConnectChoice(key);
                status.setConnectChoiceTimestamp(currentTime);
                change = true;
            }
        }
        //Write this change to file
        if (change) {
            mWifiConfigManager.writeKnownNetworkHistory();
            return true;
        }

        return false;
    }

    /**
     * enable/disable a BSSID for Quality Network Selection
     * When an association rejection event is obtained, Quality Network Selector will disable this
     * BSSID but supplicant still can try to connect to this bssid. If supplicant connect to it
     * successfully later, this bssid can be re-enabled.
     *
     * @param bssid the bssid to be enabled / disabled
     * @param enable -- true enable a bssid if it has been disabled
     *               -- false disable a bssid
     */
    public boolean enableBssidForQualityNetworkSelection(String bssid, boolean enable) {
        if (enable) {
            return (mBssidBlacklist.remove(bssid) != null);
        } else {
            if (bssid != null) {
                BssidBlacklistStatus status = mBssidBlacklist.get(bssid);
                if (status == null) {
                    //first time
                    BssidBlacklistStatus newStatus = new BssidBlacklistStatus();
                    newStatus.mCounter++;
                    mBssidBlacklist.put(bssid, newStatus);
                } else if (!status.mIsBlacklisted) {
                    status.mCounter++;
                    if (status.mCounter >= BSSID_BLACKLIST_THRESHOLD) {
                        status.mIsBlacklisted = true;
                        status.mBlacklistedTimeStamp = mClock.elapsedRealtime();
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /**
     * update the buffered BSSID blacklist
     *
     * Go through the whole buffered BSSIDs blacklist and check when the BSSIDs is blocked. If they
     * were blacked before BSSID_BLACKLIST_EXPIRE_TIME, re-enable it again.
     */
    private void updateBssidBlacklist() {
        Iterator<BssidBlacklistStatus> iter = mBssidBlacklist.values().iterator();
        while (iter.hasNext()) {
            BssidBlacklistStatus status = iter.next();
            if (status != null && status.mIsBlacklisted) {
                if (mClock.elapsedRealtime() - status.mBlacklistedTimeStamp
                            >= BSSID_BLACKLIST_EXPIRE_TIME) {
                    iter.remove();
                }
            }
        }
    }

    /**
     * Check whether a bssid is disabled
     * @param bssid -- the bssid to check
     * @return true -- bssid is disabled
     *         false -- bssid is not disabled
     */
    public boolean isBssidDisabled(String bssid) {
        BssidBlacklistStatus status = mBssidBlacklist.get(bssid);
        return status == null ? false : status.mIsBlacklisted;
    }

    /**
     * ToDo: This should be called in Connectivity Manager when it gets new scan result
     * check whether a network slection is needed. If need, check all the new scan results and
     * select a new qualified network/BSSID to connect to
     *
     * @param forceSelectNetwork true -- start a qualified network selection anyway,no matter
     *                           current network is already qualified or not.
     *                           false -- if current network is already qualified, do not do new
     *                           selection
     * @param isUntrustedConnectionsAllowed true -- user allow to connect to untrusted network
     *                                      false -- user do not allow to connect to untrusted
     *                                      network
     * @param scanDetails latest scan result obtained (should be connectivity scan only)
     * @param isLinkDebouncing true -- Link layer is under debouncing
     *                         false -- Link layer is not under debouncing
     * @param isConnected true -- device is connected to an AP currently
     *                    false -- device is not connected to an AP currently
     * @param isDisconnected true -- WifiStateMachine is at disconnected state
     *                       false -- WifiStateMachine is not at disconnected state
     * @param isSupplicantTransient true -- supplicant is in a transient state
     *                              false -- supplicant is not in a transient state
     * @return the qualified network candidate found. If no available candidate, return null
     */
    public WifiConfiguration selectQualifiedNetwork(boolean forceSelectNetwork ,
            boolean isUntrustedConnectionsAllowed, List<ScanDetail>  scanDetails,
            boolean isLinkDebouncing, boolean isConnected, boolean isDisconnected,
            boolean isSupplicantTransient) {
        localLog("==========start qualified Network Selection==========");
        mScanDetails = scanDetails;
        List<Pair<ScanDetail, WifiConfiguration>>  filteredScanDetails = new ArrayList<>();
        if (mCurrentConnectedNetwork == null) {
            mCurrentConnectedNetwork =
                    mWifiConfigManager.getWifiConfiguration(mWifiInfo.getNetworkId());
        }

        // Always get the current BSSID from WifiInfo in case that firmware initiated roaming
        // happened.
        mCurrentBssid = mWifiInfo.getBSSID();

        if (!forceSelectNetwork && !needQualifiedNetworkSelection(isLinkDebouncing, isConnected,
                isDisconnected, isSupplicantTransient)) {
            localLog("Quit qualified Network Selection since it is not forced and current network"
                    + " is qualified already");
            mFilteredScanDetails = filteredScanDetails;
            return null;
        }

        int currentHighestScore = Integer.MIN_VALUE;
        ScanResult scanResultCandidate = null;
        WifiConfiguration networkCandidate = null;
        final ExternalScoreEvaluator externalScoreEvaluator =
                new ExternalScoreEvaluator(mLocalLog, mDbg);
        String lastUserSelectedNetWorkKey = mWifiConfigManager.getLastSelectedConfiguration();
        WifiConfiguration lastUserSelectedNetwork =
                mWifiConfigManager.getWifiConfiguration(lastUserSelectedNetWorkKey);
        if (lastUserSelectedNetwork != null) {
            localLog("Last selection is " + lastUserSelectedNetwork.SSID + " Time to now: "
                    + ((mClock.elapsedRealtime() - mWifiConfigManager.getLastSelectedTimeStamp())
                            / 1000 / 60 + " minutes"));
        }

        updateSavedNetworkSelectionStatus();
        updateBssidBlacklist();

        StringBuffer lowSignalScan = new StringBuffer();
        StringBuffer notSavedScan = new StringBuffer();
        StringBuffer noValidSsid = new StringBuffer();
        StringBuffer scoreHistory =  new StringBuffer();
        ArrayList<NetworkKey> unscoredNetworks = new ArrayList<NetworkKey>();

        //iterate all scan results and find the best candidate with the highest score
        for (ScanDetail scanDetail : mScanDetails) {
            ScanResult scanResult = scanDetail.getScanResult();
            //skip bad scan result
            if (scanResult.SSID == null || TextUtils.isEmpty(scanResult.SSID)) {
                if (mDbg) {
                    //We should not see this in ePNO
                    noValidSsid.append(scanResult.BSSID + " / ");
                }
                continue;
            }

            final String scanId = toScanId(scanResult);
            //check whether this BSSID is blocked or not
            if (mWifiConfigManager.isBssidBlacklisted(scanResult.BSSID)
                    || isBssidDisabled(scanResult.BSSID)) {
                //We should not see this in ePNO
                Log.e(TAG, scanId + " is in blacklist.");
                continue;
            }

            //skip scan result with too weak signals
            if ((scanResult.is24GHz() && scanResult.level
                    < mWifiConfigManager.mThresholdMinimumRssi24.get())
                    || (scanResult.is5GHz() && scanResult.level
                    < mWifiConfigManager.mThresholdMinimumRssi5.get())) {
                if (mDbg) {
                    lowSignalScan.append(scanId + "(" + (scanResult.is24GHz() ? "2.4GHz" : "5GHz")
                            + ")" + scanResult.level + " / ");
                }
                continue;
            }

            //check if there is already a score for this network
            if (mNetworkScoreCache != null && !mNetworkScoreCache.isScoredNetwork(scanResult)) {
                //no score for this network yet.
                WifiKey wifiKey;

                try {
                    wifiKey = new WifiKey("\"" + scanResult.SSID + "\"", scanResult.BSSID);
                    NetworkKey ntwkKey = new NetworkKey(wifiKey);
                    //add to the unscoredNetworks list so we can request score later
                    unscoredNetworks.add(ntwkKey);
                } catch (IllegalArgumentException e) {
                    Log.w(TAG, "Invalid SSID=" + scanResult.SSID + " BSSID=" + scanResult.BSSID
                            + " for network score. Skip.");
                }
            }

            //check whether this scan result belong to a saved network
            boolean potentiallyEphemeral = false;
            // Stores WifiConfiguration of potential connection candidates for scan result filtering
            WifiConfiguration potentialEphemeralCandidate = null;
            List<WifiConfiguration> associatedWifiConfigurations =
                    mWifiConfigManager.updateSavedNetworkWithNewScanDetail(scanDetail,
                            isSupplicantTransient || isConnected || isLinkDebouncing);
            if (associatedWifiConfigurations == null) {
                potentiallyEphemeral =  true;
                if (mDbg) {
                    notSavedScan.append(scanId + " / ");
                }
            } else if (associatedWifiConfigurations.size() == 1) {
                //if there are more than 1 associated network, it must be a passpoint network
                WifiConfiguration network = associatedWifiConfigurations.get(0);
                if (network.ephemeral) {
                    potentialEphemeralCandidate = network;
                    potentiallyEphemeral =  true;
                }
            }

            // Evaluate the potentially ephemeral network as a possible candidate if untrusted
            // connections are allowed and we have an external score for the scan result.
            if (potentiallyEphemeral) {
                if (isUntrustedConnectionsAllowed) {
                    Integer netScore = getNetworkScore(scanResult, false);
                    if (netScore != null
                        && !mWifiConfigManager.wasEphemeralNetworkDeleted(scanResult.SSID)) {
                        externalScoreEvaluator.evalUntrustedCandidate(netScore, scanResult);
                        // scanDetail is for available ephemeral network
                        filteredScanDetails.add(Pair.create(scanDetail,
                                potentialEphemeralCandidate));
                    }
                }
                continue;
            }

            // calculate the score of each scanresult whose associated network is not ephemeral. Due
            // to one scan result can associated with more than 1 network, we need calculate all
            // the scores and use the highest one as the scanresults score.
            int highestScore = Integer.MIN_VALUE;
            int score;
            WifiConfiguration configurationCandidateForThisScan = null;
            WifiConfiguration potentialCandidate = null;
            for (WifiConfiguration network : associatedWifiConfigurations) {
                WifiConfiguration.NetworkSelectionStatus status =
                        network.getNetworkSelectionStatus();
                status.setSeenInLastQualifiedNetworkSelection(true);
                if (potentialCandidate == null) {
                    potentialCandidate = network;
                }
                if (!status.isNetworkEnabled()) {
                    continue;
                } else if (network.BSSID != null && !network.BSSID.equals("any")
                        && !network.BSSID.equals(scanResult.BSSID)) {
                    //in such scenario, user (APP) has specified the only BSSID to connect for this
                    // configuration. So only the matched scan result can be candidate
                    localLog("Network: " + getNetworkString(network) + " has specified" + "BSSID:"
                            + network.BSSID + ". Skip " + scanResult.BSSID);
                    continue;
                }

                // If the network is marked to use external scores then attempt to fetch the score.
                // These networks will not be considered alongside the other saved networks.
                if (network.useExternalScores) {
                    Integer netScore = getNetworkScore(scanResult, false);
                    externalScoreEvaluator.evalSavedCandidate(netScore, network, scanResult);
                    continue;
                }

                score = calculateBssidScore(scanResult, network, mCurrentConnectedNetwork,
                        (mCurrentBssid == null ? false : mCurrentBssid.equals(scanResult.BSSID)),
                        (lastUserSelectedNetwork == null ? false : lastUserSelectedNetwork.networkId
                         == network.networkId), scoreHistory);
                if (score > highestScore) {
                    highestScore = score;
                    configurationCandidateForThisScan = network;
                    potentialCandidate = network;
                }
                //update the cached candidate
                if (score > status.getCandidateScore() || (score == status.getCandidateScore()
                      && status.getCandidate() != null
                      && scanResult.level > status.getCandidate().level)) {
                    status.setCandidate(scanResult);
                    status.setCandidateScore(score);
                }
            }
            // Create potential filteredScanDetail entry
            filteredScanDetails.add(Pair.create(scanDetail, potentialCandidate));

            if (highestScore > currentHighestScore || (highestScore == currentHighestScore
                    && scanResultCandidate != null
                    && scanResult.level > scanResultCandidate.level)) {
                currentHighestScore = highestScore;
                scanResultCandidate = scanResult;
                networkCandidate = configurationCandidateForThisScan;
                networkCandidate.getNetworkSelectionStatus().setCandidate(scanResultCandidate);
            }
        }

        mFilteredScanDetails = filteredScanDetails;

        //kick the score manager if there is any unscored network
        if (mScoreManager != null && unscoredNetworks.size() != 0) {
            NetworkKey[] unscoredNetworkKeys =
                    unscoredNetworks.toArray(new NetworkKey[unscoredNetworks.size()]);
            mScoreManager.requestScores(unscoredNetworkKeys);
        }

        if (mDbg) {
            localLog(lowSignalScan + " skipped due to low signal\n");
            localLog(notSavedScan + " skipped due to not saved\n ");
            localLog(noValidSsid + " skipped due to not valid SSID\n");
            localLog(scoreHistory.toString());
        }

        //we need traverse the whole user preference to choose the one user like most now
        if (scanResultCandidate != null) {
            WifiConfiguration tempConfig = networkCandidate;

            while (tempConfig.getNetworkSelectionStatus().getConnectChoice() != null) {
                String key = tempConfig.getNetworkSelectionStatus().getConnectChoice();
                tempConfig = mWifiConfigManager.getWifiConfiguration(key);

                if (tempConfig != null) {
                    WifiConfiguration.NetworkSelectionStatus tempStatus =
                            tempConfig.getNetworkSelectionStatus();
                    if (tempStatus.getCandidate() != null && tempStatus.isNetworkEnabled()) {
                        scanResultCandidate = tempStatus.getCandidate();
                        networkCandidate = tempConfig;
                    }
                } else {
                    //we should not come here in theory
                    localLoge("Connect choice: " + key + " has no corresponding saved config");
                    break;
                }
            }
            localLog("After user choice adjust, the final candidate is:"
                    + getNetworkString(networkCandidate) + " : " + scanResultCandidate.BSSID);
        }

        // At this point none of the saved networks were good candidates so we fall back to
        // externally scored networks if any are available.
        if (scanResultCandidate == null) {
            localLog("Checking the externalScoreEvaluator for candidates...");
            networkCandidate = getExternalScoreCandidate(externalScoreEvaluator);
            if (networkCandidate != null) {
                scanResultCandidate = networkCandidate.getNetworkSelectionStatus().getCandidate();
            }
        }

        if (scanResultCandidate == null) {
            localLog("Can not find any suitable candidates");
            return null;
        }

        String currentAssociationId = mCurrentConnectedNetwork == null ? "Disconnected" :
                getNetworkString(mCurrentConnectedNetwork);
        String targetAssociationId = getNetworkString(networkCandidate);
        //In passpoint, saved configuration has garbage SSID. We need update it with the SSID of
        //the scan result.
        if (networkCandidate.isPasspoint()) {
            // This will update the passpoint configuration in WifiConfigManager
            networkCandidate.SSID = "\"" + scanResultCandidate.SSID + "\"";
        }

        //For debug purpose only
        if (scanResultCandidate.BSSID.equals(mCurrentBssid)) {
            localLog(currentAssociationId + " is already the best choice!");
        } else if (mCurrentConnectedNetwork != null
                && (mCurrentConnectedNetwork.networkId == networkCandidate.networkId
                || mCurrentConnectedNetwork.isLinked(networkCandidate))) {
            localLog("Roaming from " + currentAssociationId + " to " + targetAssociationId);
        } else {
            localLog("reconnect from " + currentAssociationId + " to " + targetAssociationId);
        }

        mCurrentBssid = scanResultCandidate.BSSID;
        mCurrentConnectedNetwork = networkCandidate;
        mLastQualifiedNetworkSelectionTimeStamp = mClock.elapsedRealtime();
        return networkCandidate;
    }

    /**
     * Returns the best candidate network according to the given ExternalScoreEvaluator.
     */
    @Nullable
    WifiConfiguration getExternalScoreCandidate(ExternalScoreEvaluator scoreEvaluator) {
        WifiConfiguration networkCandidate = null;
        switch (scoreEvaluator.getBestCandidateType()) {
            case ExternalScoreEvaluator.BestCandidateType.UNTRUSTED_NETWORK:
                ScanResult untrustedScanResultCandidate =
                        scoreEvaluator.getScanResultCandidate();
                WifiConfiguration unTrustedNetworkCandidate =
                        mWifiConfigManager.wifiConfigurationFromScanResult(
                                untrustedScanResultCandidate);

                // Mark this config as ephemeral so it isn't persisted.
                unTrustedNetworkCandidate.ephemeral = true;
                if (mNetworkScoreCache != null) {
                    unTrustedNetworkCandidate.meteredHint =
                            mNetworkScoreCache.getMeteredHint(untrustedScanResultCandidate);
                }
                mWifiConfigManager.saveNetwork(unTrustedNetworkCandidate,
                        WifiConfiguration.UNKNOWN_UID);

                localLog(String.format("new ephemeral candidate %s network ID:%d, "
                                + "meteredHint=%b",
                        toScanId(untrustedScanResultCandidate), unTrustedNetworkCandidate.networkId,
                        unTrustedNetworkCandidate.meteredHint));

                unTrustedNetworkCandidate.getNetworkSelectionStatus()
                        .setCandidate(untrustedScanResultCandidate);
                networkCandidate = unTrustedNetworkCandidate;
                break;

            case ExternalScoreEvaluator.BestCandidateType.SAVED_NETWORK:
                ScanResult scanResultCandidate = scoreEvaluator.getScanResultCandidate();
                networkCandidate = scoreEvaluator.getSavedConfig();
                networkCandidate.getNetworkSelectionStatus().setCandidate(scanResultCandidate);
                localLog(String.format("new scored candidate %s network ID:%d",
                        toScanId(scanResultCandidate), networkCandidate.networkId));
                break;

            case ExternalScoreEvaluator.BestCandidateType.NONE:
                localLog("ExternalScoreEvaluator did not see any good candidates.");
                break;

            default:
                localLoge("Unhandled ExternalScoreEvaluator case. No candidate selected.");
                break;
        }
        return networkCandidate;
    }

    /**
     * Returns the available external network score or NULL if no score is available.
     *
     * @param scanResult The scan result of the network to score.
     * @param isActiveNetwork Whether or not the network is currently connected.
     * @return A valid external score if one is available or NULL.
     */
    @Nullable
    Integer getNetworkScore(ScanResult scanResult, boolean isActiveNetwork) {
        if (mNetworkScoreCache != null && mNetworkScoreCache.isScoredNetwork(scanResult)) {
            int networkScore = mNetworkScoreCache.getNetworkScore(scanResult, isActiveNetwork);
            localLog(toScanId(scanResult) + " has score: " + networkScore);
            return networkScore;
        }
        return null;
    }

    /**
     * Formats the given ScanResult as a scan ID for logging.
     */
    private static String toScanId(@Nullable ScanResult scanResult) {
        return scanResult == null ? "NULL"
                                  : String.format("%s:%s", scanResult.SSID, scanResult.BSSID);
    }

    //Dump the logs
    void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("Dump of WifiQualifiedNetworkSelector");
        pw.println("WifiQualifiedNetworkSelector - Log Begin ----");
        mLocalLog.dump(fd, pw, args);
        pw.println("WifiQualifiedNetworkSelector - Log End ----");
    }

    /**
     * Used to track and evaluate networks that are assigned external scores.
     */
    static class ExternalScoreEvaluator {
        @Retention(RetentionPolicy.SOURCE)
        @interface BestCandidateType {
            int NONE = 0;
            int SAVED_NETWORK = 1;
            int UNTRUSTED_NETWORK = 2;
        }
        // Always set to the best known candidate.
        private @BestCandidateType int mBestCandidateType = BestCandidateType.NONE;
        private int mHighScore = WifiNetworkScoreCache.INVALID_NETWORK_SCORE;
        private WifiConfiguration mSavedConfig;
        private ScanResult mScanResultCandidate;
        private final LocalLog mLocalLog;
        private final boolean mDbg;

        ExternalScoreEvaluator(LocalLog localLog, boolean dbg) {
            mLocalLog = localLog;
            mDbg = dbg;
        }

        // Determines whether or not the given scan result is the best one its seen so far.
        void evalUntrustedCandidate(@Nullable Integer score, ScanResult scanResult) {
            if (score != null && score > mHighScore) {
                mHighScore = score;
                mScanResultCandidate = scanResult;
                mBestCandidateType = BestCandidateType.UNTRUSTED_NETWORK;
                localLog(toScanId(scanResult) + " become the new untrusted candidate");
            }
        }

        // Determines whether or not the given saved network is the best one its seen so far.
        void evalSavedCandidate(@Nullable Integer score, WifiConfiguration config,
                ScanResult scanResult) {
            // Always take the highest score. If there's a tie and an untrusted network is currently
            // the best then pick the saved network.
            if (score != null
                    && (score > mHighScore
                        || (mBestCandidateType == BestCandidateType.UNTRUSTED_NETWORK
                            && score == mHighScore))) {
                mHighScore = score;
                mSavedConfig = config;
                mScanResultCandidate = scanResult;
                mBestCandidateType = BestCandidateType.SAVED_NETWORK;
                localLog(toScanId(scanResult) + " become the new externally scored saved network "
                        + "candidate");
            }
        }

        int getBestCandidateType() {
            return mBestCandidateType;
        }

        int getHighScore() {
            return mHighScore;
        }

        public ScanResult getScanResultCandidate() {
            return mScanResultCandidate;
        }

        WifiConfiguration getSavedConfig() {
            return mSavedConfig;
        }

        private void localLog(String log) {
            if (mDbg) {
                mLocalLog.log(log);
            }
        }
    }
}
