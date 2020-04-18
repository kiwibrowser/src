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

package com.android.server.wifi;

import android.content.Context;

import android.net.wifi.ScanResult;

import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.NetworkSelectionStatus;
import android.net.wifi.WifiSsid;
import android.os.Environment;
import android.os.Process;
import android.text.TextUtils;

import android.util.LocalLog;
import android.util.Log;

import com.android.server.net.DelayedDiskWrite;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.text.DateFormat;
import java.util.BitSet;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Provides an API to read and write the network history from WifiConfigurations to file
 * This is largely separate and extra to the supplicant config file.
 */
public class WifiNetworkHistory {
    public static final String TAG = "WifiNetworkHistory";
    private static final boolean DBG = true;
    private static final boolean VDBG = true;
    static final String NETWORK_HISTORY_CONFIG_FILE = Environment.getDataDirectory()
            + "/misc/wifi/networkHistory.txt";
    /* Network History Keys */
    private static final String SSID_KEY = "SSID";
    static final String CONFIG_KEY = "CONFIG";
    private static final String CONFIG_BSSID_KEY = "CONFIG_BSSID";
    private static final String CHOICE_KEY = "CHOICE";
    private static final String CHOICE_TIME_KEY = "CHOICE_TIME";
    private static final String LINK_KEY = "LINK";
    private static final String BSSID_KEY = "BSSID";
    private static final String BSSID_KEY_END = "/BSSID";
    private static final String RSSI_KEY = "RSSI";
    private static final String FREQ_KEY = "FREQ";
    private static final String DATE_KEY = "DATE";
    private static final String MILLI_KEY = "MILLI";
    private static final String NETWORK_ID_KEY = "ID";
    private static final String PRIORITY_KEY = "PRIORITY";
    private static final String DEFAULT_GW_KEY = "DEFAULT_GW";
    private static final String AUTH_KEY = "AUTH";
    private static final String BSSID_STATUS_KEY = "BSSID_STATUS";
    private static final String SELF_ADDED_KEY = "SELF_ADDED";
    private static final String FAILURE_KEY = "FAILURE";
    private static final String DID_SELF_ADD_KEY = "DID_SELF_ADD";
    private static final String PEER_CONFIGURATION_KEY = "PEER_CONFIGURATION";
    static final String CREATOR_UID_KEY = "CREATOR_UID_KEY";
    private static final String CONNECT_UID_KEY = "CONNECT_UID_KEY";
    private static final String UPDATE_UID_KEY = "UPDATE_UID";
    private static final String FQDN_KEY = "FQDN";
    private static final String SCORER_OVERRIDE_KEY = "SCORER_OVERRIDE";
    private static final String SCORER_OVERRIDE_AND_SWITCH_KEY = "SCORER_OVERRIDE_AND_SWITCH";
    private static final String VALIDATED_INTERNET_ACCESS_KEY = "VALIDATED_INTERNET_ACCESS";
    private static final String NO_INTERNET_ACCESS_REPORTS_KEY = "NO_INTERNET_ACCESS_REPORTS";
    private static final String NO_INTERNET_ACCESS_EXPECTED_KEY = "NO_INTERNET_ACCESS_EXPECTED";
    private static final String EPHEMERAL_KEY = "EPHEMERAL";
    private static final String USE_EXTERNAL_SCORES_KEY = "USE_EXTERNAL_SCORES";
    private static final String METERED_HINT_KEY = "METERED_HINT";
    private static final String NUM_ASSOCIATION_KEY = "NUM_ASSOCIATION";
    private static final String DELETED_EPHEMERAL_KEY = "DELETED_EPHEMERAL";
    private static final String CREATOR_NAME_KEY = "CREATOR_NAME";
    private static final String UPDATE_NAME_KEY = "UPDATE_NAME";
    private static final String USER_APPROVED_KEY = "USER_APPROVED";
    private static final String CREATION_TIME_KEY = "CREATION_TIME";
    private static final String UPDATE_TIME_KEY = "UPDATE_TIME";
    static final String SHARED_KEY = "SHARED";
    private static final String NETWORK_SELECTION_STATUS_KEY = "NETWORK_SELECTION_STATUS";
    private static final String NETWORK_SELECTION_DISABLE_REASON_KEY =
            "NETWORK_SELECTION_DISABLE_REASON";
    private static final String HAS_EVER_CONNECTED_KEY = "HAS_EVER_CONNECTED";

    private static final String SEPARATOR = ":  ";
    private static final String NL = "\n";

    protected final DelayedDiskWrite mWriter;
    Context mContext;
    private final LocalLog mLocalLog;
    /*
     * Lost config list, whenever we read a config from networkHistory.txt that was not in
     * wpa_supplicant.conf
     */
    HashSet<String> mLostConfigsDbg = new HashSet<String>();

    public WifiNetworkHistory(Context c, LocalLog localLog, DelayedDiskWrite writer) {
        mContext = c;
        mWriter = writer;
        mLocalLog = localLog;
    }

    /**
     * Write network history to file, for configured networks
     *
     * @param networks List of ConfiguredNetworks to write to NetworkHistory
     */
    public void writeKnownNetworkHistory(final List<WifiConfiguration> networks,
            final ConcurrentHashMap<Integer, ScanDetailCache> scanDetailCaches,
            final Set<String> deletedEphemeralSSIDs) {

        /* Make a copy */
        //final List<WifiConfiguration> networks = new ArrayList<WifiConfiguration>();

        //for (WifiConfiguration config : mConfiguredNetworks.valuesForAllUsers()) {
        //    networks.add(new WifiConfiguration(config));
        //}

        mWriter.write(NETWORK_HISTORY_CONFIG_FILE, new DelayedDiskWrite.Writer() {
            public void onWriteCalled(DataOutputStream out) throws IOException {
                for (WifiConfiguration config : networks) {
                    //loge("onWriteCalled write SSID: " + config.SSID);
                   /* if (config.getLinkProperties() != null)
                        loge(" lp " + config.getLinkProperties().toString());
                    else
                        loge("attempt config w/o lp");
                    */
                    NetworkSelectionStatus status = config.getNetworkSelectionStatus();
                    if (VDBG) {
                        int numlink = 0;
                        if (config.linkedConfigurations != null) {
                            numlink = config.linkedConfigurations.size();
                        }
                        String disableTime;
                        if (config.getNetworkSelectionStatus().isNetworkEnabled()) {
                            disableTime = "";
                        } else {
                            disableTime = "Disable time: " + DateFormat.getInstance().format(
                                    config.getNetworkSelectionStatus().getDisableTime());
                        }
                        logd("saving network history: " + config.configKey()  + " gw: "
                                + config.defaultGwMacAddress + " Network Selection-status: "
                                + status.getNetworkStatusString()
                                + disableTime + " ephemeral=" + config.ephemeral
                                + " choice:" + status.getConnectChoice()
                                + " link:" + numlink
                                + " status:" + config.status
                                + " nid:" + config.networkId
                                + " hasEverConnected: " + status.getHasEverConnected());
                    }

                    if (!isValid(config)) {
                        continue;
                    }

                    if (config.SSID == null) {
                        if (VDBG) {
                            logv("writeKnownNetworkHistory trying to write config with null SSID");
                        }
                        continue;
                    }
                    if (VDBG) {
                        logv("writeKnownNetworkHistory write config " + config.configKey());
                    }
                    out.writeUTF(CONFIG_KEY + SEPARATOR + config.configKey() + NL);

                    if (config.SSID != null) {
                        out.writeUTF(SSID_KEY + SEPARATOR + config.SSID + NL);
                    }
                    if (config.BSSID != null) {
                        out.writeUTF(CONFIG_BSSID_KEY + SEPARATOR + config.BSSID + NL);
                    } else {
                        out.writeUTF(CONFIG_BSSID_KEY + SEPARATOR + "null" + NL);
                    }
                    if (config.FQDN != null) {
                        out.writeUTF(FQDN_KEY + SEPARATOR + config.FQDN + NL);
                    }

                    out.writeUTF(PRIORITY_KEY + SEPARATOR + Integer.toString(config.priority) + NL);
                    out.writeUTF(NETWORK_ID_KEY + SEPARATOR
                            + Integer.toString(config.networkId) + NL);
                    out.writeUTF(SELF_ADDED_KEY + SEPARATOR
                            + Boolean.toString(config.selfAdded) + NL);
                    out.writeUTF(DID_SELF_ADD_KEY + SEPARATOR
                            + Boolean.toString(config.didSelfAdd) + NL);
                    out.writeUTF(NO_INTERNET_ACCESS_REPORTS_KEY + SEPARATOR
                            + Integer.toString(config.numNoInternetAccessReports) + NL);
                    out.writeUTF(VALIDATED_INTERNET_ACCESS_KEY + SEPARATOR
                            + Boolean.toString(config.validatedInternetAccess) + NL);
                    out.writeUTF(NO_INTERNET_ACCESS_EXPECTED_KEY + SEPARATOR +
                            Boolean.toString(config.noInternetAccessExpected) + NL);
                    out.writeUTF(EPHEMERAL_KEY + SEPARATOR
                            + Boolean.toString(config.ephemeral) + NL);
                    out.writeUTF(METERED_HINT_KEY + SEPARATOR
                            + Boolean.toString(config.meteredHint) + NL);
                    out.writeUTF(USE_EXTERNAL_SCORES_KEY + SEPARATOR
                            + Boolean.toString(config.useExternalScores) + NL);
                    if (config.creationTime != null) {
                        out.writeUTF(CREATION_TIME_KEY + SEPARATOR + config.creationTime + NL);
                    }
                    if (config.updateTime != null) {
                        out.writeUTF(UPDATE_TIME_KEY + SEPARATOR + config.updateTime + NL);
                    }
                    if (config.peerWifiConfiguration != null) {
                        out.writeUTF(PEER_CONFIGURATION_KEY + SEPARATOR
                                + config.peerWifiConfiguration + NL);
                    }
                    out.writeUTF(SCORER_OVERRIDE_KEY + SEPARATOR
                            + Integer.toString(config.numScorerOverride) + NL);
                    out.writeUTF(SCORER_OVERRIDE_AND_SWITCH_KEY + SEPARATOR
                            + Integer.toString(config.numScorerOverrideAndSwitchedNetwork) + NL);
                    out.writeUTF(NUM_ASSOCIATION_KEY + SEPARATOR
                            + Integer.toString(config.numAssociation) + NL);
                    out.writeUTF(CREATOR_UID_KEY + SEPARATOR
                            + Integer.toString(config.creatorUid) + NL);
                    out.writeUTF(CONNECT_UID_KEY + SEPARATOR
                            + Integer.toString(config.lastConnectUid) + NL);
                    out.writeUTF(UPDATE_UID_KEY + SEPARATOR
                            + Integer.toString(config.lastUpdateUid) + NL);
                    out.writeUTF(CREATOR_NAME_KEY + SEPARATOR
                            + config.creatorName + NL);
                    out.writeUTF(UPDATE_NAME_KEY + SEPARATOR
                            + config.lastUpdateName + NL);
                    out.writeUTF(USER_APPROVED_KEY + SEPARATOR
                            + Integer.toString(config.userApproved) + NL);
                    out.writeUTF(SHARED_KEY + SEPARATOR + Boolean.toString(config.shared) + NL);
                    String allowedKeyManagementString =
                            makeString(config.allowedKeyManagement,
                                    WifiConfiguration.KeyMgmt.strings);
                    out.writeUTF(AUTH_KEY + SEPARATOR
                            + allowedKeyManagementString + NL);
                    out.writeUTF(NETWORK_SELECTION_STATUS_KEY + SEPARATOR
                            + status.getNetworkSelectionStatus() + NL);
                    out.writeUTF(NETWORK_SELECTION_DISABLE_REASON_KEY + SEPARATOR
                            + status.getNetworkSelectionDisableReason() + NL);

                    if (status.getConnectChoice() != null) {
                        out.writeUTF(CHOICE_KEY + SEPARATOR + status.getConnectChoice() + NL);
                        out.writeUTF(CHOICE_TIME_KEY + SEPARATOR
                                + status.getConnectChoiceTimestamp() + NL);
                    }

                    if (config.linkedConfigurations != null) {
                        log("writeKnownNetworkHistory write linked "
                                + config.linkedConfigurations.size());

                        for (String key : config.linkedConfigurations.keySet()) {
                            out.writeUTF(LINK_KEY + SEPARATOR + key + NL);
                        }
                    }

                    String macAddress = config.defaultGwMacAddress;
                    if (macAddress != null) {
                        out.writeUTF(DEFAULT_GW_KEY + SEPARATOR + macAddress + NL);
                    }

                    if (getScanDetailCache(config, scanDetailCaches) != null) {
                        for (ScanDetail scanDetail : getScanDetailCache(config,
                                    scanDetailCaches).values()) {
                            ScanResult result = scanDetail.getScanResult();
                            out.writeUTF(BSSID_KEY + SEPARATOR
                                    + result.BSSID + NL);
                            out.writeUTF(FREQ_KEY + SEPARATOR
                                    + Integer.toString(result.frequency) + NL);

                            out.writeUTF(RSSI_KEY + SEPARATOR
                                    + Integer.toString(result.level) + NL);

                            out.writeUTF(BSSID_KEY_END + NL);
                        }
                    }
                    if (config.lastFailure != null) {
                        out.writeUTF(FAILURE_KEY + SEPARATOR + config.lastFailure + NL);
                    }
                    out.writeUTF(HAS_EVER_CONNECTED_KEY + SEPARATOR
                            + Boolean.toString(status.getHasEverConnected()) + NL);
                    out.writeUTF(NL);
                    // Add extra blank lines for clarity
                    out.writeUTF(NL);
                    out.writeUTF(NL);
                }
                if (deletedEphemeralSSIDs != null && deletedEphemeralSSIDs.size() > 0) {
                    for (String ssid : deletedEphemeralSSIDs) {
                        out.writeUTF(DELETED_EPHEMERAL_KEY);
                        out.writeUTF(ssid);
                        out.writeUTF(NL);
                    }
                }
            }
        });
    }

    /**
     * Adds information stored in networkHistory.txt to the given configs. The configs are provided
     * as a mapping from configKey to WifiConfiguration, because the WifiConfigurations themselves
     * do not contain sufficient information to compute their configKeys until after the information
     * that is stored in networkHistory.txt has been added to them.
     *
     * @param configs mapping from configKey to a WifiConfiguration that contains the information
     *         information read from wpa_supplicant.conf
     */
    public void readNetworkHistory(Map<String, WifiConfiguration> configs,
            ConcurrentHashMap<Integer, ScanDetailCache> scanDetailCaches,
            Set<String> deletedEphemeralSSIDs) {
        localLog("readNetworkHistory() path:" + NETWORK_HISTORY_CONFIG_FILE);

        try (DataInputStream in =
                     new DataInputStream(new BufferedInputStream(
                             new FileInputStream(NETWORK_HISTORY_CONFIG_FILE)))) {

            String bssid = null;
            String ssid = null;

            int freq = 0;
            int status = 0;
            long seen = 0;
            int rssi = WifiConfiguration.INVALID_RSSI;
            String caps = null;

            WifiConfiguration config = null;
            while (true) {
                String line = in.readUTF();
                if (line == null) {
                    break;
                }
                int colon = line.indexOf(':');
                if (colon < 0) {
                    continue;
                }

                String key = line.substring(0, colon).trim();
                String value = line.substring(colon + 1).trim();

                if (key.equals(CONFIG_KEY)) {
                    config = configs.get(value);

                    // skip reading that configuration data
                    // since we don't have a corresponding network ID
                    if (config == null) {
                        localLog("readNetworkHistory didnt find netid for hash="
                                + Integer.toString(value.hashCode())
                                + " key: " + value);
                        mLostConfigsDbg.add(value);
                        continue;
                    } else {
                        // After an upgrade count old connections as owned by system
                        if (config.creatorName == null || config.lastUpdateName == null) {
                            config.creatorName =
                                mContext.getPackageManager().getNameForUid(Process.SYSTEM_UID);
                            config.lastUpdateName = config.creatorName;

                            if (DBG) {
                                Log.w(TAG, "Upgrading network " + config.networkId
                                        + " to " + config.creatorName);
                            }
                        }
                    }
                } else if (config != null) {
                    NetworkSelectionStatus networkStatus = config.getNetworkSelectionStatus();
                    switch (key) {
                        case SSID_KEY:
                            if (config.isPasspoint()) {
                                break;
                            }
                            ssid = value;
                            if (config.SSID != null && !config.SSID.equals(ssid)) {
                                loge("Error parsing network history file, mismatched SSIDs");
                                config = null; //error
                                ssid = null;
                            } else {
                                config.SSID = ssid;
                            }
                            break;
                        case CONFIG_BSSID_KEY:
                            config.BSSID = value.equals("null") ? null : value;
                            break;
                        case FQDN_KEY:
                            // Check for literal 'null' to be backwards compatible.
                            config.FQDN = value.equals("null") ? null : value;
                            break;
                        case DEFAULT_GW_KEY:
                            config.defaultGwMacAddress = value;
                            break;
                        case SELF_ADDED_KEY:
                            config.selfAdded = Boolean.parseBoolean(value);
                            break;
                        case DID_SELF_ADD_KEY:
                            config.didSelfAdd = Boolean.parseBoolean(value);
                            break;
                        case NO_INTERNET_ACCESS_REPORTS_KEY:
                            config.numNoInternetAccessReports = Integer.parseInt(value);
                            break;
                        case VALIDATED_INTERNET_ACCESS_KEY:
                            config.validatedInternetAccess = Boolean.parseBoolean(value);
                            break;
                        case NO_INTERNET_ACCESS_EXPECTED_KEY:
                            config.noInternetAccessExpected = Boolean.parseBoolean(value);
                            break;
                        case CREATION_TIME_KEY:
                            config.creationTime = value;
                            break;
                        case UPDATE_TIME_KEY:
                            config.updateTime = value;
                            break;
                        case EPHEMERAL_KEY:
                            config.ephemeral = Boolean.parseBoolean(value);
                            break;
                        case METERED_HINT_KEY:
                            config.meteredHint = Boolean.parseBoolean(value);
                            break;
                        case USE_EXTERNAL_SCORES_KEY:
                            config.useExternalScores = Boolean.parseBoolean(value);
                            break;
                        case CREATOR_UID_KEY:
                            config.creatorUid = Integer.parseInt(value);
                            break;
                        case SCORER_OVERRIDE_KEY:
                            config.numScorerOverride = Integer.parseInt(value);
                            break;
                        case SCORER_OVERRIDE_AND_SWITCH_KEY:
                            config.numScorerOverrideAndSwitchedNetwork = Integer.parseInt(value);
                            break;
                        case NUM_ASSOCIATION_KEY:
                            config.numAssociation = Integer.parseInt(value);
                            break;
                        case CONNECT_UID_KEY:
                            config.lastConnectUid = Integer.parseInt(value);
                            break;
                        case UPDATE_UID_KEY:
                            config.lastUpdateUid = Integer.parseInt(value);
                            break;
                        case FAILURE_KEY:
                            config.lastFailure = value;
                            break;
                        case PEER_CONFIGURATION_KEY:
                            config.peerWifiConfiguration = value;
                            break;
                        case NETWORK_SELECTION_STATUS_KEY:
                            int networkStatusValue = Integer.parseInt(value);
                            // Reset temporarily disabled network status
                            if (networkStatusValue ==
                                    NetworkSelectionStatus.NETWORK_SELECTION_TEMPORARY_DISABLED) {
                                networkStatusValue =
                                        NetworkSelectionStatus.NETWORK_SELECTION_ENABLED;
                            }
                            networkStatus.setNetworkSelectionStatus(networkStatusValue);
                            break;
                        case NETWORK_SELECTION_DISABLE_REASON_KEY:
                            networkStatus.setNetworkSelectionDisableReason(Integer.parseInt(value));
                            break;
                        case CHOICE_KEY:
                            networkStatus.setConnectChoice(value);
                            break;
                        case CHOICE_TIME_KEY:
                            networkStatus.setConnectChoiceTimestamp(Long.parseLong(value));
                            break;
                        case LINK_KEY:
                            if (config.linkedConfigurations == null) {
                                config.linkedConfigurations = new HashMap<>();
                            } else {
                                config.linkedConfigurations.put(value, -1);
                            }
                            break;
                        case BSSID_KEY:
                            status = 0;
                            ssid = null;
                            bssid = null;
                            freq = 0;
                            seen = 0;
                            rssi = WifiConfiguration.INVALID_RSSI;
                            caps = "";
                            break;
                        case RSSI_KEY:
                            rssi = Integer.parseInt(value);
                            break;
                        case FREQ_KEY:
                            freq = Integer.parseInt(value);
                            break;
                        case DATE_KEY:
                            /*
                             * when reading the configuration from file we don't update the date
                             * so as to avoid reading back stale or non-sensical data that would
                             * depend on network time.
                             * The date of a WifiConfiguration should only come from actual scan
                             * result.
                             *
                            String s = key.replace(FREQ_KEY, "");
                            seen = Integer.getInteger(s);
                            */
                            break;
                        case BSSID_KEY_END:
                            if ((bssid != null) && (ssid != null)) {
                                if (getScanDetailCache(config, scanDetailCaches) != null) {
                                    WifiSsid wssid = WifiSsid.createFromAsciiEncoded(ssid);
                                    ScanDetail scanDetail = new ScanDetail(wssid, bssid,
                                            caps, rssi, freq, (long) 0, seen);
                                    getScanDetailCache(config, scanDetailCaches).put(scanDetail);
                                }
                            }
                            break;
                        case DELETED_EPHEMERAL_KEY:
                            if (!TextUtils.isEmpty(value)) {
                                deletedEphemeralSSIDs.add(value);
                            }
                            break;
                        case CREATOR_NAME_KEY:
                            config.creatorName = value;
                            break;
                        case UPDATE_NAME_KEY:
                            config.lastUpdateName = value;
                            break;
                        case USER_APPROVED_KEY:
                            config.userApproved = Integer.parseInt(value);
                            break;
                        case SHARED_KEY:
                            config.shared = Boolean.parseBoolean(value);
                            break;
                        case HAS_EVER_CONNECTED_KEY:
                            networkStatus.setHasEverConnected(Boolean.parseBoolean(value));
                            break;
                    }
                }
            }
        } catch (EOFException e) {
            // do nothing
        } catch (FileNotFoundException e) {
            Log.i(TAG, "readNetworkHistory: no config file, " + e);
        } catch (NumberFormatException e) {
            Log.e(TAG, "readNetworkHistory: failed to parse, " + e, e);
        } catch (IOException e) {
            Log.e(TAG, "readNetworkHistory: failed to read, " + e, e);
        }
    }

    /**
     * Ported this out of WifiServiceImpl, I have no idea what it's doing
     * <TODO> figure out what/why this is doing
     * <TODO> Port it into WifiConfiguration, then remove all the silly business from ServiceImpl
     */
    public boolean isValid(WifiConfiguration config) {
        if (config.allowedKeyManagement == null) {
            return false;
        }
        if (config.allowedKeyManagement.cardinality() > 1) {
            if (config.allowedKeyManagement.cardinality() != 2) {
                return false;
            }
            if (!config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.WPA_EAP)) {
                return false;
            }
            if ((!config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.IEEE8021X))
                    && (!config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.WPA_PSK))) {
                return false;
            }
        }
        return true;
    }

    private static String makeString(BitSet set, String[] strings) {
        StringBuffer buf = new StringBuffer();
        int nextSetBit = -1;

        /* Make sure all set bits are in [0, strings.length) to avoid
         * going out of bounds on strings.  (Shouldn't happen, but...) */
        set = set.get(0, strings.length);

        while ((nextSetBit = set.nextSetBit(nextSetBit + 1)) != -1) {
            buf.append(strings[nextSetBit].replace('_', '-')).append(' ');
        }

        // remove trailing space
        if (set.cardinality() > 0) {
            buf.setLength(buf.length() - 1);
        }

        return buf.toString();
    }

    protected void logv(String s) {
        Log.v(TAG, s);
    }
    protected void logd(String s) {
        Log.d(TAG, s);
    }
    protected void log(String s) {
        Log.d(TAG, s);
    }
    protected void loge(String s) {
        loge(s, false);
    }
    protected void loge(String s, boolean stack) {
        if (stack) {
            Log.e(TAG, s + " stack:" + Thread.currentThread().getStackTrace()[2].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[3].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[4].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[5].getMethodName());
        } else {
            Log.e(TAG, s);
        }
    }

    private void localLog(String s) {
        if (mLocalLog != null) {
            mLocalLog.log(s);
        }
    }

    private ScanDetailCache getScanDetailCache(WifiConfiguration config,
            ConcurrentHashMap<Integer, ScanDetailCache> scanDetailCaches) {
        if (config == null || scanDetailCaches == null) return null;
        ScanDetailCache cache = scanDetailCaches.get(config.networkId);
        if (cache == null && config.networkId != WifiConfiguration.INVALID_NETWORK_ID) {
            cache = new ScanDetailCache(config);
            scanDetailCaches.put(config.networkId, cache);
        }
        return cache;
    }
}
