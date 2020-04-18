/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.apf.ApfCapabilities;
import android.net.wifi.RttManager;
import android.net.wifi.RttManager.ResponderConfig;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiLinkLayerStats;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.net.wifi.WifiSsid;
import android.net.wifi.WifiWakeReasonAndCounts;
import android.net.wifi.WpsInfo;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pGroup;
import android.net.wifi.p2p.nsd.WifiP2pServiceInfo;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.text.TextUtils;
import android.util.LocalLog;
import android.util.Log;

import com.android.internal.annotations.Immutable;
import com.android.internal.util.HexDump;
import com.android.server.connectivity.KeepalivePacketData;
import com.android.server.wifi.hotspot2.NetworkDetail;
import com.android.server.wifi.hotspot2.SupplicantBridge;
import com.android.server.wifi.hotspot2.Utils;
import com.android.server.wifi.util.FrameParser;
import com.android.server.wifi.util.InformationElementUtil;

import libcore.util.HexEncoding;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.BitSet;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.TimeZone;


/**
 * Native calls for bring up/shut down of the supplicant daemon and for
 * sending requests to the supplicant daemon
 *
 * waitForEvent() is called on the monitor thread for events. All other methods
 * must be serialized from the framework.
 *
 * {@hide}
 */
public class WifiNative {
    private static boolean DBG = false;

    // Must match wifi_hal.h
    public static final int WIFI_SUCCESS = 0;

    /**
     * Hold this lock before calling supplicant or HAL methods
     * it is required to mutually exclude access to the driver
     */
    public static final Object sLock = new Object();

    private static final LocalLog sLocalLog = new LocalLog(8192);

    public @NonNull LocalLog getLocalLog() {
        return sLocalLog;
    }

    /* Register native functions */
    static {
        /* Native functions are defined in libwifi-service.so */
        System.loadLibrary("wifi-service");
        registerNatives();
    }

    private static native int registerNatives();

    /*
     * Singleton WifiNative instances
     */
    private static WifiNative wlanNativeInterface =
            new WifiNative(SystemProperties.get("wifi.interface", "wlan0"), true);
    public static WifiNative getWlanNativeInterface() {
        return wlanNativeInterface;
    }

    private static WifiNative p2pNativeInterface =
            // commands for p2p0 interface don't need prefix
            new WifiNative(SystemProperties.get("wifi.direct.interface", "p2p0"), false);
    public static WifiNative getP2pNativeInterface() {
        return p2pNativeInterface;
    }


    private final String mTAG;
    private final String mInterfaceName;
    private final String mInterfacePrefix;

    private Context mContext = null;
    public void initContext(Context context) {
        if (mContext == null && context != null) {
            mContext = context;
        }
    }

    private WifiNative(String interfaceName,
                       boolean requiresPrefix) {
        mInterfaceName = interfaceName;
        mTAG = "WifiNative-" + interfaceName;

        if (requiresPrefix) {
            mInterfacePrefix = "IFNAME=" + interfaceName + " ";
        } else {
            mInterfacePrefix = "";
        }
    }

    public String getInterfaceName() {
        return mInterfaceName;
    }

    // Note this affects logging on for all interfaces
    void enableVerboseLogging(int verbose) {
        if (verbose > 0) {
            DBG = true;
        } else {
            DBG = false;
        }
    }

    private void localLog(String s) {
        if (sLocalLog != null) sLocalLog.log(mInterfaceName + ": " + s);
    }



    /*
     * Driver and Supplicant management
     */
    private native static boolean loadDriverNative();
    public boolean loadDriver() {
        synchronized (sLock) {
            return loadDriverNative();
        }
    }

    private native static boolean isDriverLoadedNative();
    public boolean isDriverLoaded() {
        synchronized (sLock) {
            return isDriverLoadedNative();
        }
    }

    private native static boolean unloadDriverNative();
    public boolean unloadDriver() {
        synchronized (sLock) {
            return unloadDriverNative();
        }
    }

    private native static boolean startSupplicantNative(boolean p2pSupported);
    public boolean startSupplicant(boolean p2pSupported) {
        synchronized (sLock) {
            return startSupplicantNative(p2pSupported);
        }
    }

    /* Sends a kill signal to supplicant. To be used when we have lost connection
       or when the supplicant is hung */
    private native static boolean killSupplicantNative(boolean p2pSupported);
    public boolean killSupplicant(boolean p2pSupported) {
        synchronized (sLock) {
            return killSupplicantNative(p2pSupported);
        }
    }

    private native static boolean connectToSupplicantNative();
    public boolean connectToSupplicant() {
        synchronized (sLock) {
            localLog(mInterfacePrefix + "connectToSupplicant");
            return connectToSupplicantNative();
        }
    }

    private native static void closeSupplicantConnectionNative();
    public void closeSupplicantConnection() {
        synchronized (sLock) {
            localLog(mInterfacePrefix + "closeSupplicantConnection");
            closeSupplicantConnectionNative();
        }
    }

    /**
     * Wait for the supplicant to send an event, returning the event string.
     * @return the event string sent by the supplicant.
     */
    private native static String waitForEventNative();
    public String waitForEvent() {
        // No synchronization necessary .. it is implemented in WifiMonitor
        return waitForEventNative();
    }


    /*
     * Supplicant Command Primitives
     */
    private native boolean doBooleanCommandNative(String command);

    private native int doIntCommandNative(String command);

    private native String doStringCommandNative(String command);

    private boolean doBooleanCommand(String command) {
        if (DBG) Log.d(mTAG, "doBoolean: " + command);
        synchronized (sLock) {
            String toLog = mInterfacePrefix + command;
            boolean result = doBooleanCommandNative(mInterfacePrefix + command);
            localLog(toLog + " -> " + result);
            if (DBG) Log.d(mTAG, command + ": returned " + result);
            return result;
        }
    }

    private boolean doBooleanCommandWithoutLogging(String command) {
        if (DBG) Log.d(mTAG, "doBooleanCommandWithoutLogging: " + command);
        synchronized (sLock) {
            boolean result = doBooleanCommandNative(mInterfacePrefix + command);
            if (DBG) Log.d(mTAG, command + ": returned " + result);
            return result;
        }
    }

    private int doIntCommand(String command) {
        if (DBG) Log.d(mTAG, "doInt: " + command);
        synchronized (sLock) {
            String toLog = mInterfacePrefix + command;
            int result = doIntCommandNative(mInterfacePrefix + command);
            localLog(toLog + " -> " + result);
            if (DBG) Log.d(mTAG, "   returned " + result);
            return result;
        }
    }

    private String doStringCommand(String command) {
        if (DBG) {
            //GET_NETWORK commands flood the logs
            if (!command.startsWith("GET_NETWORK")) {
                Log.d(mTAG, "doString: [" + command + "]");
            }
        }
        synchronized (sLock) {
            String toLog = mInterfacePrefix + command;
            String result = doStringCommandNative(mInterfacePrefix + command);
            if (result == null) {
                if (DBG) Log.d(mTAG, "doStringCommandNative no result");
            } else {
                if (!command.startsWith("STATUS-")) {
                    localLog(toLog + " -> " + result);
                }
                if (DBG) Log.d(mTAG, "   returned " + result.replace("\n", " "));
            }
            return result;
        }
    }

    private String doStringCommandWithoutLogging(String command) {
        if (DBG) {
            //GET_NETWORK commands flood the logs
            if (!command.startsWith("GET_NETWORK")) {
                Log.d(mTAG, "doString: [" + command + "]");
            }
        }
        synchronized (sLock) {
            return doStringCommandNative(mInterfacePrefix + command);
        }
    }

    public String doCustomSupplicantCommand(String command) {
        return doStringCommand(command);
    }

    /*
     * Wrappers for supplicant commands
     */
    public boolean ping() {
        String pong = doStringCommand("PING");
        return (pong != null && pong.equals("PONG"));
    }

    public void setSupplicantLogLevel(String level) {
        doStringCommand("LOG_LEVEL " + level);
    }

    public String getFreqCapability() {
        return doStringCommand("GET_CAPABILITY freq");
    }

    /**
     * Create a comma separate string from integer set.
     * @param values List of integers.
     * @return comma separated string.
     */
    private static String createCSVStringFromIntegerSet(Set<Integer> values) {
        StringBuilder list = new StringBuilder();
        boolean first = true;
        for (Integer value : values) {
            if (!first) {
                list.append(",");
            }
            list.append(value);
            first = false;
        }
        return list.toString();
    }

    /**
     * Start a scan using wpa_supplicant for the given frequencies.
     * @param freqs list of frequencies to scan for, if null scan all supported channels.
     * @param hiddenNetworkIds List of hidden networks to be scanned for.
     */
    public boolean scan(Set<Integer> freqs, Set<Integer> hiddenNetworkIds) {
        String freqList = null;
        String hiddenNetworkIdList = null;
        if (freqs != null && freqs.size() != 0) {
            freqList = createCSVStringFromIntegerSet(freqs);
        }
        if (hiddenNetworkIds != null && hiddenNetworkIds.size() != 0) {
            hiddenNetworkIdList = createCSVStringFromIntegerSet(hiddenNetworkIds);
        }
        return scanWithParams(freqList, hiddenNetworkIdList);
    }

    private boolean scanWithParams(String freqList, String hiddenNetworkIdList) {
        StringBuilder scanCommand = new StringBuilder();
        scanCommand.append("SCAN TYPE=ONLY");
        if (freqList != null) {
            scanCommand.append(" freq=" + freqList);
        }
        if (hiddenNetworkIdList != null) {
            scanCommand.append(" scan_id=" + hiddenNetworkIdList);
        }
        return doBooleanCommand(scanCommand.toString());
    }

    /* Does a graceful shutdown of supplicant. Is a common stop function for both p2p and sta.
     *
     * Note that underneath we use a harsh-sounding "terminate" supplicant command
     * for a graceful stop and a mild-sounding "stop" interface
     * to kill the process
     */
    public boolean stopSupplicant() {
        return doBooleanCommand("TERMINATE");
    }

    public String listNetworks() {
        return doStringCommand("LIST_NETWORKS");
    }

    public String listNetworks(int last_id) {
        return doStringCommand("LIST_NETWORKS LAST_ID=" + last_id);
    }

    public int addNetwork() {
        return doIntCommand("ADD_NETWORK");
    }

    public boolean setNetworkExtra(int netId, String name, Map<String, String> values) {
        final String encoded;
        try {
            encoded = URLEncoder.encode(new JSONObject(values).toString(), "UTF-8");
        } catch (NullPointerException e) {
            Log.e(TAG, "Unable to serialize networkExtra: " + e.toString());
            return false;
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, "Unable to serialize networkExtra: " + e.toString());
            return false;
        }
        return setNetworkVariable(netId, name, "\"" + encoded + "\"");
    }

    public boolean setNetworkVariable(int netId, String name, String value) {
        if (TextUtils.isEmpty(name) || TextUtils.isEmpty(value)) return false;
        if (name.equals(WifiConfiguration.pskVarName)
                || name.equals(WifiEnterpriseConfig.PASSWORD_KEY)) {
            return doBooleanCommandWithoutLogging("SET_NETWORK " + netId + " " + name + " " + value);
        } else {
            return doBooleanCommand("SET_NETWORK " + netId + " " + name + " " + value);
        }
    }

    public Map<String, String> getNetworkExtra(int netId, String name) {
        final String wrapped = getNetworkVariable(netId, name);
        if (wrapped == null || !wrapped.startsWith("\"") || !wrapped.endsWith("\"")) {
            return null;
        }
        try {
            final String encoded = wrapped.substring(1, wrapped.length() - 1);
            // This method reads a JSON dictionary that was written by setNetworkExtra(). However,
            // on devices that upgraded from Marshmallow, it may encounter a legacy value instead -
            // an FQDN stored as a plain string. If such a value is encountered, the JSONObject
            // constructor will thrown a JSONException and the method will return null.
            final JSONObject json = new JSONObject(URLDecoder.decode(encoded, "UTF-8"));
            final Map<String, String> values = new HashMap<String, String>();
            final Iterator<?> it = json.keys();
            while (it.hasNext()) {
                final String key = (String) it.next();
                final Object value = json.get(key);
                if (value instanceof String) {
                    values.put(key, (String) value);
                }
            }
            return values;
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, "Unable to deserialize networkExtra: " + e.toString());
            return null;
        } catch (JSONException e) {
            // This is not necessarily an error. This exception will also occur if we encounter a
            // legacy FQDN stored as a plain string. We want to return null in this case as no JSON
            // dictionary of extras was found.
            return null;
        }
    }

    public String getNetworkVariable(int netId, String name) {
        if (TextUtils.isEmpty(name)) return null;

        // GET_NETWORK will likely flood the logs ...
        return doStringCommandWithoutLogging("GET_NETWORK " + netId + " " + name);
    }

    public boolean removeNetwork(int netId) {
        return doBooleanCommand("REMOVE_NETWORK " + netId);
    }


    private void logDbg(String debug) {
        long now = SystemClock.elapsedRealtimeNanos();
        String ts = String.format("[%,d us] ", now/1000);
        Log.e("WifiNative: ", ts+debug+ " stack:"
                + Thread.currentThread().getStackTrace()[2].getMethodName() +" - "
                + Thread.currentThread().getStackTrace()[3].getMethodName() +" - "
                + Thread.currentThread().getStackTrace()[4].getMethodName() +" - "
                + Thread.currentThread().getStackTrace()[5].getMethodName()+" - "
                + Thread.currentThread().getStackTrace()[6].getMethodName());

    }

    /**
     * Enables a network in wpa_supplicant.
     * @param netId - Network ID of the network to be enabled.
     * @return true if command succeeded, false otherwise.
     */
    public boolean enableNetwork(int netId) {
        if (DBG) logDbg("enableNetwork nid=" + Integer.toString(netId));
        return doBooleanCommand("ENABLE_NETWORK " + netId);
    }

    /**
     * Enable a network in wpa_supplicant, do not connect.
     * @param netId - Network ID of the network to be enabled.
     * @return true if command succeeded, false otherwise.
     */
    public boolean enableNetworkWithoutConnect(int netId) {
        if (DBG) logDbg("enableNetworkWithoutConnect nid=" + Integer.toString(netId));
        return doBooleanCommand("ENABLE_NETWORK " + netId + " " + "no-connect");
    }

    /**
     * Disables a network in wpa_supplicant.
     * @param netId - Network ID of the network to be disabled.
     * @return true if command succeeded, false otherwise.
     */
    public boolean disableNetwork(int netId) {
        if (DBG) logDbg("disableNetwork nid=" + Integer.toString(netId));
        return doBooleanCommand("DISABLE_NETWORK " + netId);
    }

    /**
     * Select a network in wpa_supplicant (Disables all others).
     * @param netId - Network ID of the network to be selected.
     * @return true if command succeeded, false otherwise.
     */
    public boolean selectNetwork(int netId) {
        if (DBG) logDbg("selectNetwork nid=" + Integer.toString(netId));
        return doBooleanCommand("SELECT_NETWORK " + netId);
    }

    public boolean reconnect() {
        if (DBG) logDbg("RECONNECT ");
        return doBooleanCommand("RECONNECT");
    }

    public boolean reassociate() {
        if (DBG) logDbg("REASSOCIATE ");
        return doBooleanCommand("REASSOCIATE");
    }

    public boolean disconnect() {
        if (DBG) logDbg("DISCONNECT ");
        return doBooleanCommand("DISCONNECT");
    }

    public String status() {
        return status(false);
    }

    public String status(boolean noEvents) {
        if (noEvents) {
            return doStringCommand("STATUS-NO_EVENTS");
        } else {
            return doStringCommand("STATUS");
        }
    }

    public String getMacAddress() {
        //Macaddr = XX.XX.XX.XX.XX.XX
        String ret = doStringCommand("DRIVER MACADDR");
        if (!TextUtils.isEmpty(ret)) {
            String[] tokens = ret.split(" = ");
            if (tokens.length == 2) return tokens[1];
        }
        return null;
    }



    /**
     * Format of results:
     * =================
     * id=1
     * bssid=68:7f:76:d7:1a:6e
     * freq=2412
     * level=-44
     * tsf=1344626243700342
     * flags=[WPA2-PSK-CCMP][WPS][ESS]
     * ssid=zfdy
     * ====
     * id=2
     * bssid=68:5f:74:d7:1a:6f
     * freq=5180
     * level=-73
     * tsf=1344626243700373
     * flags=[WPA2-PSK-CCMP][WPS][ESS]
     * ssid=zuby
     * ====
     *
     * RANGE=ALL gets all scan results
     * RANGE=ID- gets results from ID
     * MASK=<N> BSS command information mask.
     *
     * The mask used in this method, 0x29d87, gets the following fields:
     *
     *     WPA_BSS_MASK_ID         (Bit 0)
     *     WPA_BSS_MASK_BSSID      (Bit 1)
     *     WPA_BSS_MASK_FREQ       (Bit 2)
     *     WPA_BSS_MASK_LEVEL      (Bit 7)
     *     WPA_BSS_MASK_TSF        (Bit 8)
     *     WPA_BSS_MASK_IE         (Bit 10)
     *     WPA_BSS_MASK_FLAGS      (Bit 11)
     *     WPA_BSS_MASK_SSID       (Bit 12)
     *     WPA_BSS_MASK_INTERNETW  (Bit 15) (adds ANQP info)
     *     WPA_BSS_MASK_DELIM      (Bit 17)
     *
     * See wpa_supplicant/src/common/wpa_ctrl.h for details.
     */
    private String getRawScanResults(String range) {
        return doStringCommandWithoutLogging("BSS RANGE=" + range + " MASK=0x29d87");
    }

    private static final String BSS_IE_STR = "ie=";
    private static final String BSS_ID_STR = "id=";
    private static final String BSS_BSSID_STR = "bssid=";
    private static final String BSS_FREQ_STR = "freq=";
    private static final String BSS_LEVEL_STR = "level=";
    private static final String BSS_TSF_STR = "tsf=";
    private static final String BSS_FLAGS_STR = "flags=";
    private static final String BSS_SSID_STR = "ssid=";
    private static final String BSS_DELIMITER_STR = "====";
    private static final String BSS_END_STR = "####";

    public ArrayList<ScanDetail> getScanResults() {
        int next_sid = 0;
        ArrayList<ScanDetail> results = new ArrayList<>();
        while(next_sid >= 0) {
            String rawResult = getRawScanResults(next_sid+"-");
            next_sid = -1;

            if (TextUtils.isEmpty(rawResult))
                break;

            String[] lines = rawResult.split("\n");


            // note that all these splits and substrings keep references to the original
            // huge string buffer while the amount we really want is generally pretty small
            // so make copies instead (one example b/11087956 wasted 400k of heap here).
            final int bssidStrLen = BSS_BSSID_STR.length();
            final int flagLen = BSS_FLAGS_STR.length();

            String bssid = "";
            int level = 0;
            int freq = 0;
            long tsf = 0;
            String flags = "";
            WifiSsid wifiSsid = null;
            String infoElementsStr = null;
            List<String> anqpLines = null;

            for (String line : lines) {
                if (line.startsWith(BSS_ID_STR)) { // Will find the last id line
                    try {
                        next_sid = Integer.parseInt(line.substring(BSS_ID_STR.length())) + 1;
                    } catch (NumberFormatException e) {
                        // Nothing to do
                    }
                } else if (line.startsWith(BSS_BSSID_STR)) {
                    bssid = new String(line.getBytes(), bssidStrLen, line.length() - bssidStrLen);
                } else if (line.startsWith(BSS_FREQ_STR)) {
                    try {
                        freq = Integer.parseInt(line.substring(BSS_FREQ_STR.length()));
                    } catch (NumberFormatException e) {
                        freq = 0;
                    }
                } else if (line.startsWith(BSS_LEVEL_STR)) {
                    try {
                        level = Integer.parseInt(line.substring(BSS_LEVEL_STR.length()));
                        /* some implementations avoid negative values by adding 256
                         * so we need to adjust for that here.
                         */
                        if (level > 0) level -= 256;
                    } catch (NumberFormatException e) {
                        level = 0;
                    }
                } else if (line.startsWith(BSS_TSF_STR)) {
                    try {
                        tsf = Long.parseLong(line.substring(BSS_TSF_STR.length()));
                    } catch (NumberFormatException e) {
                        tsf = 0;
                    }
                } else if (line.startsWith(BSS_FLAGS_STR)) {
                    flags = new String(line.getBytes(), flagLen, line.length() - flagLen);
                } else if (line.startsWith(BSS_SSID_STR)) {
                    wifiSsid = WifiSsid.createFromAsciiEncoded(
                            line.substring(BSS_SSID_STR.length()));
                } else if (line.startsWith(BSS_IE_STR)) {
                    infoElementsStr = line;
                } else if (SupplicantBridge.isAnqpAttribute(line)) {
                    if (anqpLines == null) {
                        anqpLines = new ArrayList<>();
                    }
                    anqpLines.add(line);
                } else if (line.startsWith(BSS_DELIMITER_STR) || line.startsWith(BSS_END_STR)) {
                    if (bssid != null) {
                        try {
                            if (infoElementsStr == null) {
                                throw new IllegalArgumentException("Null information element data");
                            }
                            int seperator = infoElementsStr.indexOf('=');
                            if (seperator < 0) {
                                throw new IllegalArgumentException("No element separator");
                            }

                            ScanResult.InformationElement[] infoElements =
                                        InformationElementUtil.parseInformationElements(
                                        Utils.hexToBytes(infoElementsStr.substring(seperator + 1)));

                            NetworkDetail networkDetail = new NetworkDetail(bssid,
                                    infoElements, anqpLines, freq);
                            String xssid = (wifiSsid != null) ? wifiSsid.toString() : WifiSsid.NONE;
                            if (!xssid.equals(networkDetail.getTrimmedSSID())) {
                                Log.d(TAG, String.format(
                                        "Inconsistent SSID on BSSID '%s': '%s' vs '%s': %s",
                                        bssid, xssid, networkDetail.getSSID(), infoElementsStr));
                            }

                            if (networkDetail.hasInterworking()) {
                                if (DBG) Log.d(TAG, "HSNwk: '" + networkDetail);
                            }
                            ScanDetail scan = new ScanDetail(networkDetail, wifiSsid, bssid, flags,
                                    level, freq, tsf, infoElements, anqpLines);
                            results.add(scan);
                        } catch (IllegalArgumentException iae) {
                            Log.d(TAG, "Failed to parse information elements: " + iae);
                        }
                    }
                    bssid = null;
                    level = 0;
                    freq = 0;
                    tsf = 0;
                    flags = "";
                    wifiSsid = null;
                    infoElementsStr = null;
                    anqpLines = null;
                }
            }
        }
        return results;
    }

    /**
     * Format of result:
     * id=1016
     * bssid=00:03:7f:40:84:10
     * freq=2462
     * beacon_int=200
     * capabilities=0x0431
     * qual=0
     * noise=0
     * level=-46
     * tsf=0000002669008476
     * age=5
     * ie=00105143412d485332302d52322d54455354010882848b960c12182403010b0706555...
     * flags=[WPA2-EAP-CCMP][ESS][P2P][HS20]
     * ssid=QCA-HS20-R2-TEST
     * p2p_device_name=
     * p2p_config_methods=0x0SET_NE
     * anqp_venue_name=02083d656e6757692d466920416c6c69616e63650a3239383920436f...
     * anqp_network_auth_type=010000
     * anqp_roaming_consortium=03506f9a05001bc504bd
     * anqp_ip_addr_type_availability=0c
     * anqp_nai_realm=0200300000246d61696c2e6578616d706c652e636f6d3b636973636f2...
     * anqp_3gpp=000600040132f465
     * anqp_domain_name=0b65786d61706c652e636f6d
     * hs20_operator_friendly_name=11656e6757692d466920416c6c69616e63650e636869...
     * hs20_wan_metrics=01c40900008001000000000a00
     * hs20_connection_capability=0100000006140001061600000650000106bb010106bb0...
     * hs20_osu_providers_list=0b5143412d4f53552d425353010901310015656e6757692d...
     */
    public String scanResult(String bssid) {
        return doStringCommand("BSS " + bssid);
    }

    public boolean startDriver() {
        return doBooleanCommand("DRIVER START");
    }

    public boolean stopDriver() {
        return doBooleanCommand("DRIVER STOP");
    }


    /**
     * Start filtering out Multicast V4 packets
     * @return {@code true} if the operation succeeded, {@code false} otherwise
     *
     * Multicast filtering rules work as follows:
     *
     * The driver can filter multicast (v4 and/or v6) and broadcast packets when in
     * a power optimized mode (typically when screen goes off).
     *
     * In order to prevent the driver from filtering the multicast/broadcast packets, we have to
     * add a DRIVER RXFILTER-ADD rule followed by DRIVER RXFILTER-START to make the rule effective
     *
     * DRIVER RXFILTER-ADD Num
     *   where Num = 0 - Unicast, 1 - Broadcast, 2 - Mutil4 or 3 - Multi6
     *
     * and DRIVER RXFILTER-START
     * In order to stop the usage of these rules, we do
     *
     * DRIVER RXFILTER-STOP
     * DRIVER RXFILTER-REMOVE Num
     *   where Num is as described for RXFILTER-ADD
     *
     * The  SETSUSPENDOPT driver command overrides the filtering rules
     */
    public boolean startFilteringMulticastV4Packets() {
        return doBooleanCommand("DRIVER RXFILTER-STOP")
            && doBooleanCommand("DRIVER RXFILTER-REMOVE 2")
            && doBooleanCommand("DRIVER RXFILTER-START");
    }

    /**
     * Stop filtering out Multicast V4 packets.
     * @return {@code true} if the operation succeeded, {@code false} otherwise
     */
    public boolean stopFilteringMulticastV4Packets() {
        return doBooleanCommand("DRIVER RXFILTER-STOP")
            && doBooleanCommand("DRIVER RXFILTER-ADD 2")
            && doBooleanCommand("DRIVER RXFILTER-START");
    }

    /**
     * Start filtering out Multicast V6 packets
     * @return {@code true} if the operation succeeded, {@code false} otherwise
     */
    public boolean startFilteringMulticastV6Packets() {
        return doBooleanCommand("DRIVER RXFILTER-STOP")
            && doBooleanCommand("DRIVER RXFILTER-REMOVE 3")
            && doBooleanCommand("DRIVER RXFILTER-START");
    }

    /**
     * Stop filtering out Multicast V6 packets.
     * @return {@code true} if the operation succeeded, {@code false} otherwise
     */
    public boolean stopFilteringMulticastV6Packets() {
        return doBooleanCommand("DRIVER RXFILTER-STOP")
            && doBooleanCommand("DRIVER RXFILTER-ADD 3")
            && doBooleanCommand("DRIVER RXFILTER-START");
    }

    /**
     * Set the operational frequency band
     * @param band One of
     *     {@link WifiManager#WIFI_FREQUENCY_BAND_AUTO},
     *     {@link WifiManager#WIFI_FREQUENCY_BAND_5GHZ},
     *     {@link WifiManager#WIFI_FREQUENCY_BAND_2GHZ},
     * @return {@code true} if the operation succeeded, {@code false} otherwise
     */
    public boolean setBand(int band) {
        String bandstr;

        if (band == WifiManager.WIFI_FREQUENCY_BAND_5GHZ)
            bandstr = "5G";
        else if (band == WifiManager.WIFI_FREQUENCY_BAND_2GHZ)
            bandstr = "2G";
        else
            bandstr = "AUTO";
        return doBooleanCommand("SET SETBAND " + bandstr);
    }

    public static final int BLUETOOTH_COEXISTENCE_MODE_ENABLED     = 0;
    public static final int BLUETOOTH_COEXISTENCE_MODE_DISABLED    = 1;
    public static final int BLUETOOTH_COEXISTENCE_MODE_SENSE       = 2;
    /**
      * Sets the bluetooth coexistence mode.
      *
      * @param mode One of {@link #BLUETOOTH_COEXISTENCE_MODE_DISABLED},
      *            {@link #BLUETOOTH_COEXISTENCE_MODE_ENABLED}, or
      *            {@link #BLUETOOTH_COEXISTENCE_MODE_SENSE}.
      * @return Whether the mode was successfully set.
      */
    public boolean setBluetoothCoexistenceMode(int mode) {
        return doBooleanCommand("DRIVER BTCOEXMODE " + mode);
    }

    /**
     * Enable or disable Bluetooth coexistence scan mode. When this mode is on,
     * some of the low-level scan parameters used by the driver are changed to
     * reduce interference with A2DP streaming.
     *
     * @param isSet whether to enable or disable this mode
     * @return {@code true} if the command succeeded, {@code false} otherwise.
     */
    public boolean setBluetoothCoexistenceScanMode(boolean setCoexScanMode) {
        if (setCoexScanMode) {
            return doBooleanCommand("DRIVER BTCOEXSCAN-START");
        } else {
            return doBooleanCommand("DRIVER BTCOEXSCAN-STOP");
        }
    }

    public void enableSaveConfig() {
        doBooleanCommand("SET update_config 1");
    }

    public boolean saveConfig() {
        return doBooleanCommand("SAVE_CONFIG");
    }

    public boolean addToBlacklist(String bssid) {
        if (TextUtils.isEmpty(bssid)) return false;
        return doBooleanCommand("BLACKLIST " + bssid);
    }

    public boolean clearBlacklist() {
        return doBooleanCommand("BLACKLIST clear");
    }

    public boolean setSuspendOptimizations(boolean enabled) {
        if (enabled) {
            return doBooleanCommand("DRIVER SETSUSPENDMODE 1");
        } else {
            return doBooleanCommand("DRIVER SETSUSPENDMODE 0");
        }
    }

    public boolean setCountryCode(String countryCode) {
        if (countryCode != null)
            return doBooleanCommand("DRIVER COUNTRY " + countryCode.toUpperCase(Locale.ROOT));
        else
            return doBooleanCommand("DRIVER COUNTRY");
    }

    /**
     * Start/Stop PNO scan.
     * @param enable boolean indicating whether PNO is being enabled or disabled.
     */
    public boolean setPnoScan(boolean enable) {
        String cmd = enable ? "SET pno 1" : "SET pno 0";
        return doBooleanCommand(cmd);
    }

    public void enableAutoConnect(boolean enable) {
        if (enable) {
            doBooleanCommand("STA_AUTOCONNECT 1");
        } else {
            doBooleanCommand("STA_AUTOCONNECT 0");
        }
    }

    public void setScanInterval(int scanInterval) {
        doBooleanCommand("SCAN_INTERVAL " + scanInterval);
    }

    public void setHs20(boolean hs20) {
        if (hs20) {
            doBooleanCommand("SET HS20 1");
        } else {
            doBooleanCommand("SET HS20 0");
        }
    }

    public void startTdls(String macAddr, boolean enable) {
        if (enable) {
            synchronized (sLock) {
                doBooleanCommand("TDLS_DISCOVER " + macAddr);
                doBooleanCommand("TDLS_SETUP " + macAddr);
            }
        } else {
            doBooleanCommand("TDLS_TEARDOWN " + macAddr);
        }
    }

    /** Example output:
     * RSSI=-65
     * LINKSPEED=48
     * NOISE=9999
     * FREQUENCY=0
     */
    public String signalPoll() {
        return doStringCommandWithoutLogging("SIGNAL_POLL");
    }

    /** Example outout:
     * TXGOOD=396
     * TXBAD=1
     */
    public String pktcntPoll() {
        return doStringCommand("PKTCNT_POLL");
    }

    public void bssFlush() {
        doBooleanCommand("BSS_FLUSH 0");
    }

    public boolean startWpsPbc(String bssid) {
        if (TextUtils.isEmpty(bssid)) {
            return doBooleanCommand("WPS_PBC");
        } else {
            return doBooleanCommand("WPS_PBC " + bssid);
        }
    }

    public boolean startWpsPbc(String iface, String bssid) {
        synchronized (sLock) {
            if (TextUtils.isEmpty(bssid)) {
                return doBooleanCommandNative("IFNAME=" + iface + " WPS_PBC");
            } else {
                return doBooleanCommandNative("IFNAME=" + iface + " WPS_PBC " + bssid);
            }
        }
    }

    public boolean startWpsPinKeypad(String pin) {
        if (TextUtils.isEmpty(pin)) return false;
        return doBooleanCommand("WPS_PIN any " + pin);
    }

    public boolean startWpsPinKeypad(String iface, String pin) {
        if (TextUtils.isEmpty(pin)) return false;
        synchronized (sLock) {
            return doBooleanCommandNative("IFNAME=" + iface + " WPS_PIN any " + pin);
        }
    }


    public String startWpsPinDisplay(String bssid) {
        if (TextUtils.isEmpty(bssid)) {
            return doStringCommand("WPS_PIN any");
        } else {
            return doStringCommand("WPS_PIN " + bssid);
        }
    }

    public String startWpsPinDisplay(String iface, String bssid) {
        synchronized (sLock) {
            if (TextUtils.isEmpty(bssid)) {
                return doStringCommandNative("IFNAME=" + iface + " WPS_PIN any");
            } else {
                return doStringCommandNative("IFNAME=" + iface + " WPS_PIN " + bssid);
            }
        }
    }

    public boolean setExternalSim(boolean external) {
        String value = external ? "1" : "0";
        Log.d(TAG, "Setting external_sim to " + value);
        return doBooleanCommand("SET external_sim " + value);
    }

    public boolean simAuthResponse(int id, String type, String response) {
        // with type = GSM-AUTH, UMTS-AUTH or UMTS-AUTS
        return doBooleanCommand("CTRL-RSP-SIM-" + id + ":" + type + response);
    }

    public boolean simAuthFailedResponse(int id) {
        // should be used with type GSM-AUTH
        return doBooleanCommand("CTRL-RSP-SIM-" + id + ":GSM-FAIL");
    }

    public boolean umtsAuthFailedResponse(int id) {
        // should be used with type UMTS-AUTH
        return doBooleanCommand("CTRL-RSP-SIM-" + id + ":UMTS-FAIL");
    }

    public boolean simIdentityResponse(int id, String response) {
        return doBooleanCommand("CTRL-RSP-IDENTITY-" + id + ":" + response);
    }

    /* Configures an access point connection */
    public boolean startWpsRegistrar(String bssid, String pin) {
        if (TextUtils.isEmpty(bssid) || TextUtils.isEmpty(pin)) return false;
        return doBooleanCommand("WPS_REG " + bssid + " " + pin);
    }

    public boolean cancelWps() {
        return doBooleanCommand("WPS_CANCEL");
    }

    public boolean setPersistentReconnect(boolean enabled) {
        int value = (enabled == true) ? 1 : 0;
        return doBooleanCommand("SET persistent_reconnect " + value);
    }

    public boolean setDeviceName(String name) {
        return doBooleanCommand("SET device_name " + name);
    }

    public boolean setDeviceType(String type) {
        return doBooleanCommand("SET device_type " + type);
    }

    public boolean setConfigMethods(String cfg) {
        return doBooleanCommand("SET config_methods " + cfg);
    }

    public boolean setManufacturer(String value) {
        return doBooleanCommand("SET manufacturer " + value);
    }

    public boolean setModelName(String value) {
        return doBooleanCommand("SET model_name " + value);
    }

    public boolean setModelNumber(String value) {
        return doBooleanCommand("SET model_number " + value);
    }

    public boolean setSerialNumber(String value) {
        return doBooleanCommand("SET serial_number " + value);
    }

    public boolean setP2pSsidPostfix(String postfix) {
        return doBooleanCommand("SET p2p_ssid_postfix " + postfix);
    }

    public boolean setP2pGroupIdle(String iface, int time) {
        synchronized (sLock) {
            return doBooleanCommandNative("IFNAME=" + iface + " SET p2p_group_idle " + time);
        }
    }

    public void setPowerSave(boolean enabled) {
        if (enabled) {
            doBooleanCommand("SET ps 1");
        } else {
            doBooleanCommand("SET ps 0");
        }
    }

    public boolean setP2pPowerSave(String iface, boolean enabled) {
        synchronized (sLock) {
            if (enabled) {
                return doBooleanCommandNative("IFNAME=" + iface + " P2P_SET ps 1");
            } else {
                return doBooleanCommandNative("IFNAME=" + iface + " P2P_SET ps 0");
            }
        }
    }

    public boolean setWfdEnable(boolean enable) {
        return doBooleanCommand("SET wifi_display " + (enable ? "1" : "0"));
    }

    public boolean setWfdDeviceInfo(String hex) {
        return doBooleanCommand("WFD_SUBELEM_SET 0 " + hex);
    }

    /**
     * "sta" prioritizes STA connection over P2P and "p2p" prioritizes
     * P2P connection over STA
     */
    public boolean setConcurrencyPriority(String s) {
        return doBooleanCommand("P2P_SET conc_pref " + s);
    }

    public boolean p2pFind() {
        return doBooleanCommand("P2P_FIND");
    }

    public boolean p2pFind(int timeout) {
        if (timeout <= 0) {
            return p2pFind();
        }
        return doBooleanCommand("P2P_FIND " + timeout);
    }

    public boolean p2pStopFind() {
       return doBooleanCommand("P2P_STOP_FIND");
    }

    public boolean p2pListen() {
        return doBooleanCommand("P2P_LISTEN");
    }

    public boolean p2pListen(int timeout) {
        if (timeout <= 0) {
            return p2pListen();
        }
        return doBooleanCommand("P2P_LISTEN " + timeout);
    }

    public boolean p2pExtListen(boolean enable, int period, int interval) {
        if (enable && interval < period) {
            return false;
        }
        return doBooleanCommand("P2P_EXT_LISTEN"
                    + (enable ? (" " + period + " " + interval) : ""));
    }

    public boolean p2pSetChannel(int lc, int oc) {
        if (DBG) Log.d(mTAG, "p2pSetChannel: lc="+lc+", oc="+oc);

        synchronized (sLock) {
            if (lc >=1 && lc <= 11) {
                if (!doBooleanCommand("P2P_SET listen_channel " + lc)) {
                    return false;
                }
            } else if (lc != 0) {
                return false;
            }

            if (oc >= 1 && oc <= 165 ) {
                int freq = (oc <= 14 ? 2407 : 5000) + oc * 5;
                return doBooleanCommand("P2P_SET disallow_freq 1000-"
                        + (freq - 5) + "," + (freq + 5) + "-6000");
            } else if (oc == 0) {
                /* oc==0 disables "P2P_SET disallow_freq" (enables all freqs) */
                return doBooleanCommand("P2P_SET disallow_freq \"\"");
            }
        }
        return false;
    }

    public boolean p2pFlush() {
        return doBooleanCommand("P2P_FLUSH");
    }

    private static final int DEFAULT_GROUP_OWNER_INTENT     = 6;
    /* p2p_connect <peer device address> <pbc|pin|PIN#> [label|display|keypad]
        [persistent] [join|auth] [go_intent=<0..15>] [freq=<in MHz>] */
    public String p2pConnect(WifiP2pConfig config, boolean joinExistingGroup) {
        if (config == null) return null;
        List<String> args = new ArrayList<String>();
        WpsInfo wps = config.wps;
        args.add(config.deviceAddress);

        switch (wps.setup) {
            case WpsInfo.PBC:
                args.add("pbc");
                break;
            case WpsInfo.DISPLAY:
                if (TextUtils.isEmpty(wps.pin)) {
                    args.add("pin");
                } else {
                    args.add(wps.pin);
                }
                args.add("display");
                break;
            case WpsInfo.KEYPAD:
                args.add(wps.pin);
                args.add("keypad");
                break;
            case WpsInfo.LABEL:
                args.add(wps.pin);
                args.add("label");
            default:
                break;
        }

        if (config.netId == WifiP2pGroup.PERSISTENT_NET_ID) {
            args.add("persistent");
        }

        if (joinExistingGroup) {
            args.add("join");
        } else {
            //TODO: This can be adapted based on device plugged in state and
            //device battery state
            int groupOwnerIntent = config.groupOwnerIntent;
            if (groupOwnerIntent < 0 || groupOwnerIntent > 15) {
                groupOwnerIntent = DEFAULT_GROUP_OWNER_INTENT;
            }
            args.add("go_intent=" + groupOwnerIntent);
        }

        String command = "P2P_CONNECT ";
        for (String s : args) command += s + " ";

        return doStringCommand(command);
    }

    public boolean p2pCancelConnect() {
        return doBooleanCommand("P2P_CANCEL");
    }

    public boolean p2pProvisionDiscovery(WifiP2pConfig config) {
        if (config == null) return false;

        switch (config.wps.setup) {
            case WpsInfo.PBC:
                return doBooleanCommand("P2P_PROV_DISC " + config.deviceAddress + " pbc");
            case WpsInfo.DISPLAY:
                //We are doing display, so provision discovery is keypad
                return doBooleanCommand("P2P_PROV_DISC " + config.deviceAddress + " keypad");
            case WpsInfo.KEYPAD:
                //We are doing keypad, so provision discovery is display
                return doBooleanCommand("P2P_PROV_DISC " + config.deviceAddress + " display");
            default:
                break;
        }
        return false;
    }

    public boolean p2pGroupAdd(boolean persistent) {
        if (persistent) {
            return doBooleanCommand("P2P_GROUP_ADD persistent");
        }
        return doBooleanCommand("P2P_GROUP_ADD");
    }

    public boolean p2pGroupAdd(int netId) {
        return doBooleanCommand("P2P_GROUP_ADD persistent=" + netId);
    }

    public boolean p2pGroupRemove(String iface) {
        if (TextUtils.isEmpty(iface)) return false;
        synchronized (sLock) {
            return doBooleanCommandNative("IFNAME=" + iface + " P2P_GROUP_REMOVE " + iface);
        }
    }

    public boolean p2pReject(String deviceAddress) {
        return doBooleanCommand("P2P_REJECT " + deviceAddress);
    }

    /* Invite a peer to a group */
    public boolean p2pInvite(WifiP2pGroup group, String deviceAddress) {
        if (TextUtils.isEmpty(deviceAddress)) return false;

        if (group == null) {
            return doBooleanCommand("P2P_INVITE peer=" + deviceAddress);
        } else {
            return doBooleanCommand("P2P_INVITE group=" + group.getInterface()
                    + " peer=" + deviceAddress + " go_dev_addr=" + group.getOwner().deviceAddress);
        }
    }

    /* Reinvoke a persistent connection */
    public boolean p2pReinvoke(int netId, String deviceAddress) {
        if (TextUtils.isEmpty(deviceAddress) || netId < 0) return false;

        return doBooleanCommand("P2P_INVITE persistent=" + netId + " peer=" + deviceAddress);
    }

    public String p2pGetSsid(String deviceAddress) {
        return p2pGetParam(deviceAddress, "oper_ssid");
    }

    public String p2pGetDeviceAddress() {
        Log.d(TAG, "p2pGetDeviceAddress");

        String status = null;

        /* Explicitly calling the API without IFNAME= prefix to take care of the devices that
        don't have p2p0 interface. Supplicant seems to be returning the correct address anyway. */

        synchronized (sLock) {
            status = doStringCommandNative("STATUS");
        }

        String result = "";
        if (status != null) {
            String[] tokens = status.split("\n");
            for (String token : tokens) {
                if (token.startsWith("p2p_device_address=")) {
                    String[] nameValue = token.split("=");
                    if (nameValue.length != 2)
                        break;
                    result = nameValue[1];
                }
            }
        }

        Log.d(TAG, "p2pGetDeviceAddress returning " + result);
        return result;
    }

    public int getGroupCapability(String deviceAddress) {
        int gc = 0;
        if (TextUtils.isEmpty(deviceAddress)) return gc;
        String peerInfo = p2pPeer(deviceAddress);
        if (TextUtils.isEmpty(peerInfo)) return gc;

        String[] tokens = peerInfo.split("\n");
        for (String token : tokens) {
            if (token.startsWith("group_capab=")) {
                String[] nameValue = token.split("=");
                if (nameValue.length != 2) break;
                try {
                    return Integer.decode(nameValue[1]);
                } catch(NumberFormatException e) {
                    return gc;
                }
            }
        }
        return gc;
    }

    public String p2pPeer(String deviceAddress) {
        return doStringCommand("P2P_PEER " + deviceAddress);
    }

    private String p2pGetParam(String deviceAddress, String key) {
        if (deviceAddress == null) return null;

        String peerInfo = p2pPeer(deviceAddress);
        if (peerInfo == null) return null;
        String[] tokens= peerInfo.split("\n");

        key += "=";
        for (String token : tokens) {
            if (token.startsWith(key)) {
                String[] nameValue = token.split("=");
                if (nameValue.length != 2) break;
                return nameValue[1];
            }
        }
        return null;
    }

    public boolean p2pServiceAdd(WifiP2pServiceInfo servInfo) {
        /*
         * P2P_SERVICE_ADD bonjour <query hexdump> <RDATA hexdump>
         * P2P_SERVICE_ADD upnp <version hex> <service>
         *
         * e.g)
         * [Bonjour]
         * # IP Printing over TCP (PTR) (RDATA=MyPrinter._ipp._tcp.local.)
         * P2P_SERVICE_ADD bonjour 045f697070c00c000c01 094d795072696e746572c027
         * # IP Printing over TCP (TXT) (RDATA=txtvers=1,pdl=application/postscript)
         * P2P_SERVICE_ADD bonjour 096d797072696e746572045f697070c00c001001
         *  09747874766572733d311a70646c3d6170706c69636174696f6e2f706f7374736372797074
         *
         * [UPnP]
         * P2P_SERVICE_ADD upnp 10 uuid:6859dede-8574-59ab-9332-123456789012
         * P2P_SERVICE_ADD upnp 10 uuid:6859dede-8574-59ab-9332-123456789012::upnp:rootdevice
         * P2P_SERVICE_ADD upnp 10 uuid:6859dede-8574-59ab-9332-123456789012::urn:schemas-upnp
         * -org:device:InternetGatewayDevice:1
         * P2P_SERVICE_ADD upnp 10 uuid:6859dede-8574-59ab-9322-123456789012::urn:schemas-upnp
         * -org:service:ContentDirectory:2
         */
        synchronized (sLock) {
            for (String s : servInfo.getSupplicantQueryList()) {
                String command = "P2P_SERVICE_ADD";
                command += (" " + s);
                if (!doBooleanCommand(command)) {
                    return false;
                }
            }
        }
        return true;
    }

    public boolean p2pServiceDel(WifiP2pServiceInfo servInfo) {
        /*
         * P2P_SERVICE_DEL bonjour <query hexdump>
         * P2P_SERVICE_DEL upnp <version hex> <service>
         */
        synchronized (sLock) {
            for (String s : servInfo.getSupplicantQueryList()) {
                String command = "P2P_SERVICE_DEL ";

                String[] data = s.split(" ");
                if (data.length < 2) {
                    return false;
                }
                if ("upnp".equals(data[0])) {
                    command += s;
                } else if ("bonjour".equals(data[0])) {
                    command += data[0];
                    command += (" " + data[1]);
                } else {
                    return false;
                }
                if (!doBooleanCommand(command)) {
                    return false;
                }
            }
        }
        return true;
    }

    public boolean p2pServiceFlush() {
        return doBooleanCommand("P2P_SERVICE_FLUSH");
    }

    public String p2pServDiscReq(String addr, String query) {
        String command = "P2P_SERV_DISC_REQ";
        command += (" " + addr);
        command += (" " + query);

        return doStringCommand(command);
    }

    public boolean p2pServDiscCancelReq(String id) {
        return doBooleanCommand("P2P_SERV_DISC_CANCEL_REQ " + id);
    }

    /* Set the current mode of miracast operation.
     *  0 = disabled
     *  1 = operating as source
     *  2 = operating as sink
     */
    public void setMiracastMode(int mode) {
        // Note: optional feature on the driver. It is ok for this to fail.
        doBooleanCommand("DRIVER MIRACAST " + mode);
    }

    public boolean fetchAnqp(String bssid, String subtypes) {
        return doBooleanCommand("ANQP_GET " + bssid + " " + subtypes);
    }

    /*
     * NFC-related calls
     */
    public String getNfcWpsConfigurationToken(int netId) {
        return doStringCommand("WPS_NFC_CONFIG_TOKEN WPS " + netId);
    }

    public String getNfcHandoverRequest() {
        return doStringCommand("NFC_GET_HANDOVER_REQ NDEF P2P-CR");
    }

    public String getNfcHandoverSelect() {
        return doStringCommand("NFC_GET_HANDOVER_SEL NDEF P2P-CR");
    }

    public boolean initiatorReportNfcHandover(String selectMessage) {
        return doBooleanCommand("NFC_REPORT_HANDOVER INIT P2P 00 " + selectMessage);
    }

    public boolean responderReportNfcHandover(String requestMessage) {
        return doBooleanCommand("NFC_REPORT_HANDOVER RESP P2P " + requestMessage + " 00");
    }


    /* kernel logging support */
    private static native byte[] readKernelLogNative();

    synchronized public String readKernelLog() {
        byte[] bytes = readKernelLogNative();
        if (bytes != null) {
            CharsetDecoder decoder = StandardCharsets.UTF_8.newDecoder();
            try {
                CharBuffer decoded = decoder.decode(ByteBuffer.wrap(bytes));
                return decoded.toString();
            } catch (CharacterCodingException cce) {
                return new String(bytes, StandardCharsets.ISO_8859_1);
            }
        } else {
            return "*** failed to read kernel log ***";
        }
    }

    /* WIFI HAL support */

    // HAL command ids
    private static int sCmdId = 1;
    private static int getNewCmdIdLocked() {
        return sCmdId++;
    }

    private static final String TAG = "WifiNative-HAL";
    private static long sWifiHalHandle = 0;             /* used by JNI to save wifi_handle */
    private static long[] sWifiIfaceHandles = null;     /* used by JNI to save interface handles */
    public static int sWlan0Index = -1;
    private static MonitorThread sThread;
    private static final int STOP_HAL_TIMEOUT_MS = 1000;

    private static native boolean startHalNative();
    private static native void stopHalNative();
    private static native void waitForHalEventNative();

    private static class MonitorThread extends Thread {
        public void run() {
            Log.i(TAG, "Waiting for HAL events mWifiHalHandle=" + Long.toString(sWifiHalHandle));
            waitForHalEventNative();
        }
    }

    public boolean startHal() {
        String debugLog = "startHal stack: ";
        java.lang.StackTraceElement[] elements = Thread.currentThread().getStackTrace();
        for (int i = 2; i < elements.length && i <= 7; i++ ) {
            debugLog = debugLog + " - " + elements[i].getMethodName();
        }

        sLocalLog.log(debugLog);

        synchronized (sLock) {
            if (startHalNative()) {
                int wlan0Index = queryInterfaceIndex(mInterfaceName);
                if (wlan0Index == -1) {
                    if (DBG) sLocalLog.log("Could not find interface with name: " + mInterfaceName);
                    return false;
                }
                sWlan0Index = wlan0Index;
                sThread = new MonitorThread();
                sThread.start();
                return true;
            } else {
                if (DBG) sLocalLog.log("Could not start hal");
                Log.e(TAG, "Could not start hal");
                return false;
            }
        }
    }

    public void stopHal() {
        synchronized (sLock) {
            if (isHalStarted()) {
                stopHalNative();
                try {
                    sThread.join(STOP_HAL_TIMEOUT_MS);
                    Log.d(TAG, "HAL event thread stopped successfully");
                } catch (InterruptedException e) {
                    Log.e(TAG, "Could not stop HAL cleanly");
                }
                sThread = null;
                sWifiHalHandle = 0;
                sWifiIfaceHandles = null;
                sWlan0Index = -1;
            }
        }
    }

    public boolean isHalStarted() {
        return (sWifiHalHandle != 0);
    }
    private static native int getInterfacesNative();

    public int queryInterfaceIndex(String interfaceName) {
        synchronized (sLock) {
            if (isHalStarted()) {
                int num = getInterfacesNative();
                for (int i = 0; i < num; i++) {
                    String name = getInterfaceNameNative(i);
                    if (name.equals(interfaceName)) {
                        return i;
                    }
                }
            }
        }
        return -1;
    }

    private static native String getInterfaceNameNative(int index);
    public String getInterfaceName(int index) {
        synchronized (sLock) {
            return getInterfaceNameNative(index);
        }
    }

    // TODO: Change variable names to camel style.
    public static class ScanCapabilities {
        public int  max_scan_cache_size;
        public int  max_scan_buckets;
        public int  max_ap_cache_per_scan;
        public int  max_rssi_sample_size;
        public int  max_scan_reporting_threshold;
        public int  max_hotlist_bssids;
        public int  max_significant_wifi_change_aps;
        public int  max_bssid_history_entries;
        public int  max_number_epno_networks;
        public int  max_number_epno_networks_by_ssid;
        public int  max_number_of_white_listed_ssid;
    }

    public boolean getScanCapabilities(ScanCapabilities capabilities) {
        synchronized (sLock) {
            return isHalStarted() && getScanCapabilitiesNative(sWlan0Index, capabilities);
        }
    }

    private static native boolean getScanCapabilitiesNative(
            int iface, ScanCapabilities capabilities);

    private static native boolean startScanNative(int iface, int id, ScanSettings settings);
    private static native boolean stopScanNative(int iface, int id);
    private static native WifiScanner.ScanData[] getScanResultsNative(int iface, boolean flush);
    private static native WifiLinkLayerStats getWifiLinkLayerStatsNative(int iface);
    private static native void setWifiLinkLayerStatsNative(int iface, int enable);

    public static class ChannelSettings {
        public int frequency;
        public int dwell_time_ms;
        public boolean passive;
    }

    public static class BucketSettings {
        public int bucket;
        public int band;
        public int period_ms;
        public int max_period_ms;
        public int step_count;
        public int report_events;
        public int num_channels;
        public ChannelSettings[] channels;
    }

    public static class ScanSettings {
        public int base_period_ms;
        public int max_ap_per_scan;
        public int report_threshold_percent;
        public int report_threshold_num_scans;
        public int num_buckets;
        /* Not part of gscan HAL API. Used only for wpa_supplicant scanning */
        public int[] hiddenNetworkIds;
        public BucketSettings[] buckets;
    }

    /**
     * Network parameters to start PNO scan.
     */
    public static class PnoNetwork {
        public String ssid;
        public int networkId;
        public int priority;
        public byte flags;
        public byte auth_bit_field;

        @Override
        public boolean equals(Object otherObj) {
            if (this == otherObj) {
                return true;
            } else if (otherObj == null || getClass() != otherObj.getClass()) {
                return false;
            }
            PnoNetwork other = (PnoNetwork) otherObj;
            return ((Objects.equals(ssid, other.ssid)) && (networkId == other.networkId)
                    && (priority == other.priority) && (flags == other.flags)
                    && (auth_bit_field == other.auth_bit_field));
        }
    }

    /**
     * Parameters to start PNO scan. This holds the list of networks which are going to used for
     * PNO scan.
     */
    public static class PnoSettings {
        public int min5GHzRssi;
        public int min24GHzRssi;
        public int initialScoreMax;
        public int currentConnectionBonus;
        public int sameNetworkBonus;
        public int secureBonus;
        public int band5GHzBonus;
        public boolean isConnected;
        public PnoNetwork[] networkList;
    }

    /**
     * Wi-Fi channel information.
     */
    public static class WifiChannelInfo {
        int mPrimaryFrequency;
        int mCenterFrequency0;
        int mCenterFrequency1;
        int mChannelWidth;
        // TODO: add preamble once available in HAL.
    }

    public static interface ScanEventHandler {
        /**
         * Called for each AP as it is found with the entire contents of the beacon/probe response.
         * Only called when WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT is specified.
         */
        void onFullScanResult(ScanResult fullScanResult, int bucketsScanned);
        /**
         * Callback on an event during a gscan scan.
         * See WifiNative.WIFI_SCAN_* for possible values.
         */
        void onScanStatus(int event);
        /**
         * Called with the current cached scan results when gscan is paused.
         */
        void onScanPaused(WifiScanner.ScanData[] data);
        /**
         * Called with the current cached scan results when gscan is resumed.
         */
        void onScanRestarted();
    }

    /**
     * Handler to notify the occurrence of various events during PNO scan.
     */
    public interface PnoEventHandler {
        /**
         * Callback to notify when one of the shortlisted networks is found during PNO scan.
         * @param results List of Scan results received.
         */
        void onPnoNetworkFound(ScanResult[] results);

        /**
         * Callback to notify when the PNO scan schedule fails.
         */
        void onPnoScanFailed();
    }

    /* scan status, keep these values in sync with gscan.h */
    public static final int WIFI_SCAN_RESULTS_AVAILABLE = 0;
    public static final int WIFI_SCAN_THRESHOLD_NUM_SCANS = 1;
    public static final int WIFI_SCAN_THRESHOLD_PERCENT = 2;
    public static final int WIFI_SCAN_FAILED = 3;

    // Callback from native
    private static void onScanStatus(int id, int event) {
        ScanEventHandler handler = sScanEventHandler;
        if (handler != null) {
            handler.onScanStatus(event);
        }
    }

    public static  WifiSsid createWifiSsid(byte[] rawSsid) {
        String ssidHexString = String.valueOf(HexEncoding.encode(rawSsid));

        if (ssidHexString == null) {
            return null;
        }

        WifiSsid wifiSsid = WifiSsid.createFromHex(ssidHexString);

        return wifiSsid;
    }

    public static String ssidConvert(byte[] rawSsid) {
        String ssid;

        CharsetDecoder decoder = StandardCharsets.UTF_8.newDecoder();
        try {
            CharBuffer decoded = decoder.decode(ByteBuffer.wrap(rawSsid));
            ssid = decoded.toString();
        } catch (CharacterCodingException cce) {
            ssid = null;
        }

        if (ssid == null) {
            ssid = new String(rawSsid, StandardCharsets.ISO_8859_1);
        }

        return ssid;
    }

    // Called from native
    public static boolean setSsid(byte[] rawSsid, ScanResult result) {
        if (rawSsid == null || rawSsid.length == 0 || result == null) {
            return false;
        }

        result.SSID = ssidConvert(rawSsid);
        result.wifiSsid = createWifiSsid(rawSsid);
        return true;
    }

    private static void populateScanResult(ScanResult result, int beaconCap, String dbg) {
        if (dbg == null) dbg = "";

        InformationElementUtil.HtOperation htOperation = new InformationElementUtil.HtOperation();
        InformationElementUtil.VhtOperation vhtOperation =
                new InformationElementUtil.VhtOperation();
        InformationElementUtil.ExtendedCapabilities extendedCaps =
                new InformationElementUtil.ExtendedCapabilities();

        ScanResult.InformationElement elements[] =
                InformationElementUtil.parseInformationElements(result.bytes);
        for (ScanResult.InformationElement ie : elements) {
            if(ie.id == ScanResult.InformationElement.EID_HT_OPERATION) {
                htOperation.from(ie);
            } else if(ie.id == ScanResult.InformationElement.EID_VHT_OPERATION) {
                vhtOperation.from(ie);
            } else if (ie.id == ScanResult.InformationElement.EID_EXTENDED_CAPS) {
                extendedCaps.from(ie);
            }
        }

        if (extendedCaps.is80211McRTTResponder) {
            result.setFlag(ScanResult.FLAG_80211mc_RESPONDER);
        } else {
            result.clearFlag(ScanResult.FLAG_80211mc_RESPONDER);
        }

        //handle RTT related information
        if (vhtOperation.isValid()) {
            result.channelWidth = vhtOperation.getChannelWidth();
            result.centerFreq0 = vhtOperation.getCenterFreq0();
            result.centerFreq1 = vhtOperation.getCenterFreq1();
        } else {
            result.channelWidth = htOperation.getChannelWidth();
            result.centerFreq0 = htOperation.getCenterFreq0(result.frequency);
            result.centerFreq1  = 0;
        }

        // build capabilities string
        BitSet beaconCapBits = new BitSet(16);
        for (int i = 0; i < 16; i++) {
            if ((beaconCap & (1 << i)) != 0) {
                beaconCapBits.set(i);
            }
        }
        result.capabilities = InformationElementUtil.Capabilities.buildCapabilities(elements,
                                               beaconCapBits);

        if(DBG) {
            Log.d(TAG, dbg + "SSID: " + result.SSID + " ChannelWidth is: " + result.channelWidth
                    + " PrimaryFreq: " + result.frequency + " mCenterfreq0: " + result.centerFreq0
                    + " mCenterfreq1: " + result.centerFreq1 + (extendedCaps.is80211McRTTResponder
                    ? "Support RTT reponder: " : "Do not support RTT responder")
                    + " Capabilities: " + result.capabilities);
        }

        result.informationElements = elements;
    }

    // Callback from native
    private static void onFullScanResult(int id, ScanResult result,
            int bucketsScanned, int beaconCap) {
        if (DBG) Log.i(TAG, "Got a full scan results event, ssid = " + result.SSID);

        ScanEventHandler handler = sScanEventHandler;
        if (handler != null) {
            populateScanResult(result, beaconCap, " onFullScanResult ");
            handler.onFullScanResult(result, bucketsScanned);
        }
    }

    private static int sScanCmdId = 0;
    private static ScanEventHandler sScanEventHandler;
    private static ScanSettings sScanSettings;

    public boolean startScan(ScanSettings settings, ScanEventHandler eventHandler) {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sScanCmdId != 0) {
                    stopScan();
                } else if (sScanSettings != null || sScanEventHandler != null) {
                /* current scan is paused; no need to stop it */
                }

                sScanCmdId = getNewCmdIdLocked();

                sScanSettings = settings;
                sScanEventHandler = eventHandler;

                if (startScanNative(sWlan0Index, sScanCmdId, settings) == false) {
                    sScanEventHandler = null;
                    sScanSettings = null;
                    sScanCmdId = 0;
                    return false;
                }

                return true;
            } else {
                return false;
            }
        }
    }

    public void stopScan() {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sScanCmdId != 0) {
                    stopScanNative(sWlan0Index, sScanCmdId);
                }
                sScanSettings = null;
                sScanEventHandler = null;
                sScanCmdId = 0;
            }
        }
    }

    public void pauseScan() {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sScanCmdId != 0 && sScanSettings != null && sScanEventHandler != null) {
                    Log.d(TAG, "Pausing scan");
                    WifiScanner.ScanData scanData[] = getScanResultsNative(sWlan0Index, true);
                    stopScanNative(sWlan0Index, sScanCmdId);
                    sScanCmdId = 0;
                    sScanEventHandler.onScanPaused(scanData);
                }
            }
        }
    }

    public void restartScan() {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sScanCmdId == 0 && sScanSettings != null && sScanEventHandler != null) {
                    Log.d(TAG, "Restarting scan");
                    ScanEventHandler handler = sScanEventHandler;
                    ScanSettings settings = sScanSettings;
                    if (startScan(sScanSettings, sScanEventHandler)) {
                        sScanEventHandler.onScanRestarted();
                    } else {
                    /* we are still paused; don't change state */
                        sScanEventHandler = handler;
                        sScanSettings = settings;
                    }
                }
            }
        }
    }

    public WifiScanner.ScanData[] getScanResults(boolean flush) {
        synchronized (sLock) {
            WifiScanner.ScanData[] sd = null;
            if (isHalStarted()) {
                sd = getScanResultsNative(sWlan0Index, flush);
            }

            if (sd != null) {
                return sd;
            } else {
                return new WifiScanner.ScanData[0];
            }
        }
    }

    public static interface HotlistEventHandler {
        void onHotlistApFound (ScanResult[] result);
        void onHotlistApLost  (ScanResult[] result);
    }

    private static int sHotlistCmdId = 0;
    private static HotlistEventHandler sHotlistEventHandler;

    private native static boolean setHotlistNative(int iface, int id,
            WifiScanner.HotlistSettings settings);
    private native static boolean resetHotlistNative(int iface, int id);

    public boolean setHotlist(WifiScanner.HotlistSettings settings,
            HotlistEventHandler eventHandler) {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sHotlistCmdId != 0) {
                    return false;
                } else {
                    sHotlistCmdId = getNewCmdIdLocked();
                }

                sHotlistEventHandler = eventHandler;
                if (setHotlistNative(sWlan0Index, sHotlistCmdId, settings) == false) {
                    sHotlistEventHandler = null;
                    return false;
                }

                return true;
            } else {
                return false;
            }
        }
    }

    public void resetHotlist() {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sHotlistCmdId != 0) {
                    resetHotlistNative(sWlan0Index, sHotlistCmdId);
                    sHotlistCmdId = 0;
                    sHotlistEventHandler = null;
                }
            }
        }
    }

    // Callback from native
    private static void onHotlistApFound(int id, ScanResult[] results) {
        HotlistEventHandler handler = sHotlistEventHandler;
        if (handler != null) {
            handler.onHotlistApFound(results);
        } else {
            /* this can happen because of race conditions */
            Log.d(TAG, "Ignoring hotlist AP found event");
        }
    }

    // Callback from native
    private static void onHotlistApLost(int id, ScanResult[] results) {
        HotlistEventHandler handler = sHotlistEventHandler;
        if (handler != null) {
            handler.onHotlistApLost(results);
        } else {
            /* this can happen because of race conditions */
            Log.d(TAG, "Ignoring hotlist AP lost event");
        }
    }

    public static interface SignificantWifiChangeEventHandler {
        void onChangesFound(ScanResult[] result);
    }

    private static SignificantWifiChangeEventHandler sSignificantWifiChangeHandler;
    private static int sSignificantWifiChangeCmdId;

    private static native boolean trackSignificantWifiChangeNative(
            int iface, int id, WifiScanner.WifiChangeSettings settings);
    private static native boolean untrackSignificantWifiChangeNative(int iface, int id);

    public boolean trackSignificantWifiChange(
            WifiScanner.WifiChangeSettings settings, SignificantWifiChangeEventHandler handler) {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sSignificantWifiChangeCmdId != 0) {
                    return false;
                } else {
                    sSignificantWifiChangeCmdId = getNewCmdIdLocked();
                }

                sSignificantWifiChangeHandler = handler;
                if (trackSignificantWifiChangeNative(sWlan0Index, sSignificantWifiChangeCmdId,
                        settings) == false) {
                    sSignificantWifiChangeHandler = null;
                    return false;
                }

                return true;
            } else {
                return false;
            }

        }
    }

    public void untrackSignificantWifiChange() {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sSignificantWifiChangeCmdId != 0) {
                    untrackSignificantWifiChangeNative(sWlan0Index, sSignificantWifiChangeCmdId);
                    sSignificantWifiChangeCmdId = 0;
                    sSignificantWifiChangeHandler = null;
                }
            }
        }
    }

    // Callback from native
    private static void onSignificantWifiChange(int id, ScanResult[] results) {
        SignificantWifiChangeEventHandler handler = sSignificantWifiChangeHandler;
        if (handler != null) {
            handler.onChangesFound(results);
        } else {
            /* this can happen because of race conditions */
            Log.d(TAG, "Ignoring significant wifi change");
        }
    }

    public WifiLinkLayerStats getWifiLinkLayerStats(String iface) {
        // TODO: use correct iface name to Index translation
        if (iface == null) return null;
        synchronized (sLock) {
            if (isHalStarted()) {
                return getWifiLinkLayerStatsNative(sWlan0Index);
            } else {
                return null;
            }
        }
    }

    public void setWifiLinkLayerStats(String iface, int enable) {
        if (iface == null) return;
        synchronized (sLock) {
            if (isHalStarted()) {
                setWifiLinkLayerStatsNative(sWlan0Index, enable);
            }
        }
    }

    public static native int getSupportedFeatureSetNative(int iface);
    public int getSupportedFeatureSet() {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getSupportedFeatureSetNative(sWlan0Index);
            } else {
                Log.d(TAG, "Failing getSupportedFeatureset because HAL isn't started");
                return 0;
            }
        }
    }

    /* Rtt related commands/events */
    public static interface RttEventHandler {
        void onRttResults(RttManager.RttResult[] result);
    }

    private static RttEventHandler sRttEventHandler;
    private static int sRttCmdId;

    // Callback from native
    private static void onRttResults(int id, RttManager.RttResult[] results) {
        RttEventHandler handler = sRttEventHandler;
        if (handler != null && id == sRttCmdId) {
            Log.d(TAG, "Received " + results.length + " rtt results");
            handler.onRttResults(results);
            sRttCmdId = 0;
        } else {
            Log.d(TAG, "RTT Received event for unknown cmd = " + id +
                    ", current id = " + sRttCmdId);
        }
    }

    private static native boolean requestRangeNative(
            int iface, int id, RttManager.RttParams[] params);
    private static native boolean cancelRangeRequestNative(
            int iface, int id, RttManager.RttParams[] params);

    public boolean requestRtt(
            RttManager.RttParams[] params, RttEventHandler handler) {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sRttCmdId != 0) {
                    Log.w(TAG, "Last one is still under measurement!");
                    return false;
                } else {
                    sRttCmdId = getNewCmdIdLocked();
                }
                sRttEventHandler = handler;
                return requestRangeNative(sWlan0Index, sRttCmdId, params);
            } else {
                return false;
            }
        }
    }

    public boolean cancelRtt(RttManager.RttParams[] params) {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sRttCmdId == 0) {
                    return false;
                }

                sRttCmdId = 0;

                if (cancelRangeRequestNative(sWlan0Index, sRttCmdId, params)) {
                    sRttEventHandler = null;
                    return true;
                } else {
                    Log.e(TAG, "RTT cancel Request failed");
                    return false;
                }
            } else {
                return false;
            }
        }
    }

    private static int sRttResponderCmdId = 0;

    private static native ResponderConfig enableRttResponderNative(int iface, int commandId,
            int timeoutSeconds, WifiChannelInfo channelHint);
    /**
     * Enable RTT responder role on the device. Returns {@link ResponderConfig} if the responder
     * role is successfully enabled, {@code null} otherwise.
     */
    @Nullable
    public ResponderConfig enableRttResponder(int timeoutSeconds) {
        synchronized (sLock) {
            if (!isHalStarted()) return null;
            if (sRttResponderCmdId != 0) {
                if (DBG) Log.e(mTAG, "responder mode already enabled - this shouldn't happen");
                return null;
            }
            int id = getNewCmdIdLocked();
            ResponderConfig config = enableRttResponderNative(
                    sWlan0Index, id, timeoutSeconds, null);
            if (config != null) sRttResponderCmdId = id;
            if (DBG) Log.d(TAG, "enabling rtt " + (config != null));
            return config;
        }
    }

    private static native boolean disableRttResponderNative(int iface, int commandId);
    /**
     * Disable RTT responder role. Returns {@code true} if responder role is successfully disabled,
     * {@code false} otherwise.
     */
    public boolean disableRttResponder() {
        synchronized (sLock) {
            if (!isHalStarted()) return false;
            if (sRttResponderCmdId == 0) {
                Log.e(mTAG, "responder role not enabled yet");
                return true;
            }
            sRttResponderCmdId = 0;
            return disableRttResponderNative(sWlan0Index, sRttResponderCmdId);
        }
    }

    private static native boolean setScanningMacOuiNative(int iface, byte[] oui);

    public boolean setScanningMacOui(byte[] oui) {
        synchronized (sLock) {
            if (isHalStarted()) {
                return setScanningMacOuiNative(sWlan0Index, oui);
            } else {
                return false;
            }
        }
    }

    private static native int[] getChannelsForBandNative(
            int iface, int band);

    public int [] getChannelsForBand(int band) {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getChannelsForBandNative(sWlan0Index, band);
            } else {
                return null;
            }
        }
    }

    private static native boolean isGetChannelsForBandSupportedNative();
    public boolean isGetChannelsForBandSupported(){
        synchronized (sLock) {
            if (isHalStarted()) {
                return isGetChannelsForBandSupportedNative();
            } else {
                return false;
            }
        }
    }

    private static native boolean setDfsFlagNative(int iface, boolean dfsOn);
    public boolean setDfsFlag(boolean dfsOn) {
        synchronized (sLock) {
            if (isHalStarted()) {
                return setDfsFlagNative(sWlan0Index, dfsOn);
            } else {
                return false;
            }
        }
    }

    private static native boolean setInterfaceUpNative(boolean up);
    public boolean setInterfaceUp(boolean up) {
        synchronized (sLock) {
            if (isHalStarted()) {
                return setInterfaceUpNative(up);
            } else {
                return false;
            }
        }
    }

    private static native RttManager.RttCapabilities getRttCapabilitiesNative(int iface);
    public RttManager.RttCapabilities getRttCapabilities() {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getRttCapabilitiesNative(sWlan0Index);
            } else {
                return null;
            }
        }
    }

    private static native ApfCapabilities getApfCapabilitiesNative(int iface);
    public ApfCapabilities getApfCapabilities() {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getApfCapabilitiesNative(sWlan0Index);
            } else {
                return null;
            }
        }
    }

    private static native boolean installPacketFilterNative(int iface, byte[] filter);
    public boolean installPacketFilter(byte[] filter) {
        synchronized (sLock) {
            if (isHalStarted()) {
                return installPacketFilterNative(sWlan0Index, filter);
            } else {
                return false;
            }
        }
    }

    private static native boolean setCountryCodeHalNative(int iface, String CountryCode);
    public boolean setCountryCodeHal(String CountryCode) {
        synchronized (sLock) {
            if (isHalStarted()) {
                return setCountryCodeHalNative(sWlan0Index, CountryCode);
            } else {
                return false;
            }
        }
    }

    /* Rtt related commands/events */
    public abstract class TdlsEventHandler {
        abstract public void onTdlsStatus(String macAddr, int status, int reason);
    }

    private static TdlsEventHandler sTdlsEventHandler;

    private static native boolean enableDisableTdlsNative(int iface, boolean enable,
            String macAddr);
    public boolean enableDisableTdls(boolean enable, String macAdd, TdlsEventHandler tdlsCallBack) {
        synchronized (sLock) {
            sTdlsEventHandler = tdlsCallBack;
            return enableDisableTdlsNative(sWlan0Index, enable, macAdd);
        }
    }

    // Once TDLS per mac and event feature is implemented, this class definition should be
    // moved to the right place, like WifiManager etc
    public static class TdlsStatus {
        int channel;
        int global_operating_class;
        int state;
        int reason;
    }
    private static native TdlsStatus getTdlsStatusNative(int iface, String macAddr);
    public TdlsStatus getTdlsStatus(String macAdd) {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getTdlsStatusNative(sWlan0Index, macAdd);
            } else {
                return null;
            }
        }
    }

    //ToFix: Once TDLS per mac and event feature is implemented, this class definition should be
    // moved to the right place, like WifiStateMachine etc
    public static class TdlsCapabilities {
        /* Maximum TDLS session number can be supported by the Firmware and hardware */
        int maxConcurrentTdlsSessionNumber;
        boolean isGlobalTdlsSupported;
        boolean isPerMacTdlsSupported;
        boolean isOffChannelTdlsSupported;
    }



    private static native TdlsCapabilities getTdlsCapabilitiesNative(int iface);
    public TdlsCapabilities getTdlsCapabilities () {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getTdlsCapabilitiesNative(sWlan0Index);
            } else {
                return null;
            }
        }
    }

    private static boolean onTdlsStatus(String macAddr, int status, int reason) {
        TdlsEventHandler handler = sTdlsEventHandler;
        if (handler == null) {
            return false;
        } else {
            handler.onTdlsStatus(macAddr, status, reason);
            return true;
        }
    }

    //---------------------------------------------------------------------------------

    /* Wifi Logger commands/events */

    public static interface WifiLoggerEventHandler {
        void onRingBufferData(RingBufferStatus status, byte[] buffer);
        void onWifiAlert(int errorCode, byte[] buffer);
    }

    private static WifiLoggerEventHandler sWifiLoggerEventHandler = null;

    // Callback from native
    private static void onRingBufferData(RingBufferStatus status, byte[] buffer) {
        WifiLoggerEventHandler handler = sWifiLoggerEventHandler;
        if (handler != null)
            handler.onRingBufferData(status, buffer);
    }

    // Callback from native
    private static void onWifiAlert(byte[] buffer, int errorCode) {
        WifiLoggerEventHandler handler = sWifiLoggerEventHandler;
        if (handler != null)
            handler.onWifiAlert(errorCode, buffer);
    }

    private static int sLogCmdId = -1;
    private static native boolean setLoggingEventHandlerNative(int iface, int id);
    public boolean setLoggingEventHandler(WifiLoggerEventHandler handler) {
        synchronized (sLock) {
            if (isHalStarted()) {
                int oldId =  sLogCmdId;
                sLogCmdId = getNewCmdIdLocked();
                if (!setLoggingEventHandlerNative(sWlan0Index, sLogCmdId)) {
                    sLogCmdId = oldId;
                    return false;
                }
                sWifiLoggerEventHandler = handler;
                return true;
            } else {
                return false;
            }
        }
    }

    private static native boolean startLoggingRingBufferNative(int iface, int verboseLevel,
            int flags, int minIntervalSec ,int minDataSize, String ringName);
    public boolean startLoggingRingBuffer(int verboseLevel, int flags, int maxInterval,
            int minDataSize, String ringName){
        synchronized (sLock) {
            if (isHalStarted()) {
                return startLoggingRingBufferNative(sWlan0Index, verboseLevel, flags, maxInterval,
                        minDataSize, ringName);
            } else {
                return false;
            }
        }
    }

    private static native int getSupportedLoggerFeatureSetNative(int iface);
    public int getSupportedLoggerFeatureSet() {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getSupportedLoggerFeatureSetNative(sWlan0Index);
            } else {
                return 0;
            }
        }
    }

    private static native boolean resetLogHandlerNative(int iface, int id);
    public boolean resetLogHandler() {
        synchronized (sLock) {
            if (isHalStarted()) {
                if (sLogCmdId == -1) {
                    Log.e(TAG,"Can not reset handler Before set any handler");
                    return false;
                }
                sWifiLoggerEventHandler = null;
                if (resetLogHandlerNative(sWlan0Index, sLogCmdId)) {
                    sLogCmdId = -1;
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
    }

    private static native String getDriverVersionNative(int iface);
    public String getDriverVersion() {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getDriverVersionNative(sWlan0Index);
            } else {
                return "";
            }
        }
    }


    private static native String getFirmwareVersionNative(int iface);
    public String getFirmwareVersion() {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getFirmwareVersionNative(sWlan0Index);
            } else {
                return "";
            }
        }
    }

    public static class RingBufferStatus{
        String name;
        int flag;
        int ringBufferId;
        int ringBufferByteSize;
        int verboseLevel;
        int writtenBytes;
        int readBytes;
        int writtenRecords;

        @Override
        public String toString() {
            return "name: " + name + " flag: " + flag + " ringBufferId: " + ringBufferId +
                    " ringBufferByteSize: " +ringBufferByteSize + " verboseLevel: " +verboseLevel +
                    " writtenBytes: " + writtenBytes + " readBytes: " + readBytes +
                    " writtenRecords: " + writtenRecords;
        }
    }

    private static native RingBufferStatus[] getRingBufferStatusNative(int iface);
    public RingBufferStatus[] getRingBufferStatus() {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getRingBufferStatusNative(sWlan0Index);
            } else {
                return null;
            }
        }
    }

    private static native boolean getRingBufferDataNative(int iface, String ringName);
    public boolean getRingBufferData(String ringName) {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getRingBufferDataNative(sWlan0Index, ringName);
            } else {
                return false;
            }
        }
    }

    private static byte[] mFwMemoryDump;
    // Callback from native
    private static void onWifiFwMemoryAvailable(byte[] buffer) {
        mFwMemoryDump = buffer;
        if (DBG) {
            Log.d(TAG, "onWifiFwMemoryAvailable is called and buffer length is: " +
                    (buffer == null ? 0 :  buffer.length));
        }
    }

    private static native boolean getFwMemoryDumpNative(int iface);
    public byte[] getFwMemoryDump() {
        synchronized (sLock) {
            if (isHalStarted()) {
                if(getFwMemoryDumpNative(sWlan0Index)) {
                    byte[] fwMemoryDump = mFwMemoryDump;
                    mFwMemoryDump = null;
                    return fwMemoryDump;
                } else {
                    return null;
                }
            }
            return null;
        }
    }

    private static native byte[] getDriverStateDumpNative(int iface);
    /** Fetch the driver state, for driver debugging. */
    public byte[] getDriverStateDump() {
        synchronized (sLock) {
            if (isHalStarted()) {
                return getDriverStateDumpNative(sWlan0Index);
            } else {
                return null;
            }
        }
    }

    //---------------------------------------------------------------------------------
    /* Packet fate API */

    @Immutable
    abstract static class FateReport {
        final static int USEC_PER_MSEC = 1000;
        // The driver timestamp is a 32-bit counter, in microseconds. This field holds the
        // maximal value of a driver timestamp in milliseconds.
        final static int MAX_DRIVER_TIMESTAMP_MSEC = (int) (0xffffffffL / 1000);
        final static SimpleDateFormat dateFormatter = new SimpleDateFormat("HH:mm:ss.SSS");

        final byte mFate;
        final long mDriverTimestampUSec;
        final byte mFrameType;
        final byte[] mFrameBytes;
        final long mEstimatedWallclockMSec;

        FateReport(byte fate, long driverTimestampUSec, byte frameType, byte[] frameBytes) {
            mFate = fate;
            mDriverTimestampUSec = driverTimestampUSec;
            mEstimatedWallclockMSec =
                    convertDriverTimestampUSecToWallclockMSec(mDriverTimestampUSec);
            mFrameType = frameType;
            mFrameBytes = frameBytes;
        }

        public String toTableRowString() {
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            FrameParser parser = new FrameParser(mFrameType, mFrameBytes);
            dateFormatter.setTimeZone(TimeZone.getDefault());
            pw.format("%-15s  %12s  %-9s  %-32s  %-12s  %-23s  %s\n",
                    mDriverTimestampUSec,
                    dateFormatter.format(new Date(mEstimatedWallclockMSec)),
                    directionToString(), fateToString(), parser.mMostSpecificProtocolString,
                    parser.mTypeString, parser.mResultString);
            return sw.toString();
        }

        public String toVerboseStringWithPiiAllowed() {
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            FrameParser parser = new FrameParser(mFrameType, mFrameBytes);
            pw.format("Frame direction: %s\n", directionToString());
            pw.format("Frame timestamp: %d\n", mDriverTimestampUSec);
            pw.format("Frame fate: %s\n", fateToString());
            pw.format("Frame type: %s\n", frameTypeToString(mFrameType));
            pw.format("Frame protocol: %s\n", parser.mMostSpecificProtocolString);
            pw.format("Frame protocol type: %s\n", parser.mTypeString);
            pw.format("Frame length: %d\n", mFrameBytes.length);
            pw.append("Frame bytes");
            pw.append(HexDump.dumpHexString(mFrameBytes));  // potentially contains PII
            pw.append("\n");
            return sw.toString();
        }

        /* Returns a header to match the output of toTableRowString(). */
        public static String getTableHeader() {
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            pw.format("\n%-15s  %-12s  %-9s  %-32s  %-12s  %-23s  %s\n",
                    "Time usec", "Walltime", "Direction", "Fate", "Protocol", "Type", "Result");
            pw.format("%-15s  %-12s  %-9s  %-32s  %-12s  %-23s  %s\n",
                    "---------", "--------", "---------", "----", "--------", "----", "------");
            return sw.toString();
        }

        protected abstract String directionToString();

        protected abstract String fateToString();

        private static String frameTypeToString(byte frameType) {
            switch (frameType) {
                case WifiLoggerHal.FRAME_TYPE_UNKNOWN:
                    return "unknown";
                case WifiLoggerHal.FRAME_TYPE_ETHERNET_II:
                    return "data";
                case WifiLoggerHal.FRAME_TYPE_80211_MGMT:
                    return "802.11 management";
                default:
                    return Byte.toString(frameType);
            }
        }

        /**
         * Converts a driver timestamp to a wallclock time, based on the current
         * BOOTTIME to wallclock mapping. The driver timestamp is a 32-bit counter of
         * microseconds, with the same base as BOOTTIME.
         */
        private static long convertDriverTimestampUSecToWallclockMSec(long driverTimestampUSec) {
            final long wallclockMillisNow = System.currentTimeMillis();
            final long boottimeMillisNow = SystemClock.elapsedRealtime();
            final long driverTimestampMillis = driverTimestampUSec / USEC_PER_MSEC;

            long boottimeTimestampMillis = boottimeMillisNow % MAX_DRIVER_TIMESTAMP_MSEC;
            if (boottimeTimestampMillis < driverTimestampMillis) {
                // The 32-bit microsecond count has wrapped between the time that the driver
                // recorded the packet, and the call to this function. Adjust the BOOTTIME
                // timestamp, to compensate.
                //
                // Note that overflow is not a concern here, since the result is less than
                // 2 * MAX_DRIVER_TIMESTAMP_MSEC. (Given the modulus operation above,
                // boottimeTimestampMillis must be less than MAX_DRIVER_TIMESTAMP_MSEC.) And, since
                // MAX_DRIVER_TIMESTAMP_MSEC is an int, 2 * MAX_DRIVER_TIMESTAMP_MSEC must fit
                // within a long.
                boottimeTimestampMillis += MAX_DRIVER_TIMESTAMP_MSEC;
            }

            final long millisSincePacketTimestamp = boottimeTimestampMillis - driverTimestampMillis;
            return wallclockMillisNow - millisSincePacketTimestamp;
        }
    }

    /**
     * Represents the fate information for one outbound packet.
     */
    @Immutable
    public static final class TxFateReport extends FateReport {
        TxFateReport(byte fate, long driverTimestampUSec, byte frameType, byte[] frameBytes) {
            super(fate, driverTimestampUSec, frameType, frameBytes);
        }

        @Override
        protected String directionToString() {
            return "TX";
        }

        @Override
        protected String fateToString() {
            switch (mFate) {
                case WifiLoggerHal.TX_PKT_FATE_ACKED:
                    return "acked";
                case WifiLoggerHal.TX_PKT_FATE_SENT:
                    return "sent";
                case WifiLoggerHal.TX_PKT_FATE_FW_QUEUED:
                    return "firmware queued";
                case WifiLoggerHal.TX_PKT_FATE_FW_DROP_INVALID:
                    return "firmware dropped (invalid frame)";
                case WifiLoggerHal.TX_PKT_FATE_FW_DROP_NOBUFS:
                    return "firmware dropped (no bufs)";
                case WifiLoggerHal.TX_PKT_FATE_FW_DROP_OTHER:
                    return "firmware dropped (other)";
                case WifiLoggerHal.TX_PKT_FATE_DRV_QUEUED:
                    return "driver queued";
                case WifiLoggerHal.TX_PKT_FATE_DRV_DROP_INVALID:
                    return "driver dropped (invalid frame)";
                case WifiLoggerHal.TX_PKT_FATE_DRV_DROP_NOBUFS:
                    return "driver dropped (no bufs)";
                case WifiLoggerHal.TX_PKT_FATE_DRV_DROP_OTHER:
                    return "driver dropped (other)";
                default:
                    return Byte.toString(mFate);
            }
        }
    }

    /**
     * Represents the fate information for one inbound packet.
     */
    @Immutable
    public static final class RxFateReport extends FateReport {
        RxFateReport(byte fate, long driverTimestampUSec, byte frameType, byte[] frameBytes) {
            super(fate, driverTimestampUSec, frameType, frameBytes);
        }

        @Override
        protected String directionToString() {
            return "RX";
        }

        @Override
        protected String fateToString() {
            switch (mFate) {
                case WifiLoggerHal.RX_PKT_FATE_SUCCESS:
                    return "success";
                case WifiLoggerHal.RX_PKT_FATE_FW_QUEUED:
                    return "firmware queued";
                case WifiLoggerHal.RX_PKT_FATE_FW_DROP_FILTER:
                    return "firmware dropped (filter)";
                case WifiLoggerHal.RX_PKT_FATE_FW_DROP_INVALID:
                    return "firmware dropped (invalid frame)";
                case WifiLoggerHal.RX_PKT_FATE_FW_DROP_NOBUFS:
                    return "firmware dropped (no bufs)";
                case WifiLoggerHal.RX_PKT_FATE_FW_DROP_OTHER:
                    return "firmware dropped (other)";
                case WifiLoggerHal.RX_PKT_FATE_DRV_QUEUED:
                    return "driver queued";
                case WifiLoggerHal.RX_PKT_FATE_DRV_DROP_FILTER:
                    return "driver dropped (filter)";
                case WifiLoggerHal.RX_PKT_FATE_DRV_DROP_INVALID:
                    return "driver dropped (invalid frame)";
                case WifiLoggerHal.RX_PKT_FATE_DRV_DROP_NOBUFS:
                    return "driver dropped (no bufs)";
                case WifiLoggerHal.RX_PKT_FATE_DRV_DROP_OTHER:
                    return "driver dropped (other)";
                default:
                    return Byte.toString(mFate);
            }
        }
    }

    private static native int startPktFateMonitoringNative(int iface);
    /**
     * Ask the HAL to enable packet fate monitoring. Fails unless HAL is started.
     */
    public boolean startPktFateMonitoring() {
        synchronized (sLock) {
            if (isHalStarted()) {
                return startPktFateMonitoringNative(sWlan0Index) == WIFI_SUCCESS;
            } else {
                return false;
            }
        }
    }

    private static native int getTxPktFatesNative(int iface, TxFateReport[] reportBufs);
    /**
     * Fetch the most recent TX packet fates from the HAL. Fails unless HAL is started.
     */
    public boolean getTxPktFates(TxFateReport[] reportBufs) {
        synchronized (sLock) {
            if (isHalStarted()) {
                int res = getTxPktFatesNative(sWlan0Index, reportBufs);
                if (res != WIFI_SUCCESS) {
                    Log.e(TAG, "getTxPktFatesNative returned " + res);
                    return false;
                } else {
                    return true;
                }
            } else {
                return false;
            }
        }
    }

    private static native int getRxPktFatesNative(int iface, RxFateReport[] reportBufs);
    /**
     * Fetch the most recent RX packet fates from the HAL. Fails unless HAL is started.
     */
    public boolean getRxPktFates(RxFateReport[] reportBufs) {
        synchronized (sLock) {
            if (isHalStarted()) {
                int res = getRxPktFatesNative(sWlan0Index, reportBufs);
                if (res != WIFI_SUCCESS) {
                    Log.e(TAG, "getRxPktFatesNative returned " + res);
                    return false;
                } else {
                    return true;
                }
            } else {
                return false;
            }
        }
    }

    //---------------------------------------------------------------------------------
    /* Configure ePNO/PNO */
    private static PnoEventHandler sPnoEventHandler;
    private static int sPnoCmdId = 0;

    private static native boolean setPnoListNative(int iface, int id, PnoSettings settings);

    /**
     * Set the PNO settings & the network list in HAL to start PNO.
     * @param settings PNO settings and network list.
     * @param eventHandler Handler to receive notifications back during PNO scan.
     * @return true if success, false otherwise
     */
    public boolean setPnoList(PnoSettings settings, PnoEventHandler eventHandler) {
        Log.e(TAG, "setPnoList cmd " + sPnoCmdId);

        synchronized (sLock) {
            if (isHalStarted()) {
                sPnoCmdId = getNewCmdIdLocked();
                sPnoEventHandler = eventHandler;
                if (setPnoListNative(sWlan0Index, sPnoCmdId, settings)) {
                    return true;
                }
            }
            sPnoEventHandler = null;
            return false;
        }
    }

    /**
     * Set the PNO network list in HAL to start PNO.
     * @param list PNO network list.
     * @param eventHandler Handler to receive notifications back during PNO scan.
     * @return true if success, false otherwise
     */
    public boolean setPnoList(PnoNetwork[] list, PnoEventHandler eventHandler) {
        PnoSettings settings = new PnoSettings();
        settings.networkList = list;
        return setPnoList(settings, eventHandler);
    }

    private static native boolean resetPnoListNative(int iface, int id);

    /**
     * Reset the PNO settings in HAL to stop PNO.
     * @return true if success, false otherwise
     */
    public boolean resetPnoList() {
        Log.e(TAG, "resetPnoList cmd " + sPnoCmdId);

        synchronized (sLock) {
            if (isHalStarted()) {
                sPnoCmdId = getNewCmdIdLocked();
                sPnoEventHandler = null;
                if (resetPnoListNative(sWlan0Index, sPnoCmdId)) {
                    return true;
                }
            }
            return false;
        }
    }

    // Callback from native
    private static void onPnoNetworkFound(int id, ScanResult[] results, int[] beaconCaps) {
        if (results == null) {
            Log.e(TAG, "onPnoNetworkFound null results");
            return;

        }
        Log.d(TAG, "WifiNative.onPnoNetworkFound result " + results.length);

        PnoEventHandler handler = sPnoEventHandler;
        if (sPnoCmdId != 0 && handler != null) {
            for (int i=0; i<results.length; i++) {
                Log.e(TAG, "onPnoNetworkFound SSID " + results[i].SSID
                        + " " + results[i].level + " " + results[i].frequency);

                populateScanResult(results[i], beaconCaps[i], "onPnoNetworkFound ");
                results[i].wifiSsid = WifiSsid.createFromAsciiEncoded(results[i].SSID);
            }

            handler.onPnoNetworkFound(results);
        } else {
            /* this can happen because of race conditions */
            Log.d(TAG, "Ignoring Pno Network found event");
        }
    }

    private native static boolean setBssidBlacklistNative(int iface, int id,
                                              String list[]);

    public boolean setBssidBlacklist(String list[]) {
        int size = 0;
        if (list != null) {
            size = list.length;
        }
        Log.e(TAG, "setBssidBlacklist cmd " + sPnoCmdId + " size " + size);

        synchronized (sLock) {
            if (isHalStarted()) {
                sPnoCmdId = getNewCmdIdLocked();
                return setBssidBlacklistNative(sWlan0Index, sPnoCmdId, list);
            } else {
                return false;
            }
        }
    }

    private native static int startSendingOffloadedPacketNative(int iface, int idx,
                                    byte[] srcMac, byte[] dstMac, byte[] pktData, int period);

    public int
    startSendingOffloadedPacket(int slot, KeepalivePacketData keepAlivePacket, int period) {
        Log.d(TAG, "startSendingOffloadedPacket slot=" + slot + " period=" + period);

        String[] macAddrStr = getMacAddress().split(":");
        byte[] srcMac = new byte[6];
        for(int i = 0; i < 6; i++) {
            Integer hexVal = Integer.parseInt(macAddrStr[i], 16);
            srcMac[i] = hexVal.byteValue();
        }
        synchronized (sLock) {
            if (isHalStarted()) {
                return startSendingOffloadedPacketNative(sWlan0Index, slot, srcMac,
                        keepAlivePacket.dstMac, keepAlivePacket.data, period);
            } else {
                return -1;
            }
        }
    }

    private native static int stopSendingOffloadedPacketNative(int iface, int idx);

    public int
    stopSendingOffloadedPacket(int slot) {
        Log.d(TAG, "stopSendingOffloadedPacket " + slot);
        synchronized (sLock) {
            if (isHalStarted()) {
                return stopSendingOffloadedPacketNative(sWlan0Index, slot);
            } else {
                return -1;
            }
        }
    }

    public static interface WifiRssiEventHandler {
        void onRssiThresholdBreached(byte curRssi);
    }

    private static WifiRssiEventHandler sWifiRssiEventHandler;

    // Callback from native
    private static void onRssiThresholdBreached(int id, byte curRssi) {
        WifiRssiEventHandler handler = sWifiRssiEventHandler;
        if (handler != null) {
            handler.onRssiThresholdBreached(curRssi);
        }
    }

    private native static int startRssiMonitoringNative(int iface, int id,
                                        byte maxRssi, byte minRssi);

    private static int sRssiMonitorCmdId = 0;

    public int startRssiMonitoring(byte maxRssi, byte minRssi,
                                                WifiRssiEventHandler rssiEventHandler) {
        Log.d(TAG, "startRssiMonitoring: maxRssi=" + maxRssi + " minRssi=" + minRssi);
        synchronized (sLock) {
            sWifiRssiEventHandler = rssiEventHandler;
            if (isHalStarted()) {
                if (sRssiMonitorCmdId != 0) {
                    stopRssiMonitoring();
                }

                sRssiMonitorCmdId = getNewCmdIdLocked();
                Log.d(TAG, "sRssiMonitorCmdId = " + sRssiMonitorCmdId);
                int ret = startRssiMonitoringNative(sWlan0Index, sRssiMonitorCmdId,
                        maxRssi, minRssi);
                if (ret != 0) { // if not success
                    sRssiMonitorCmdId = 0;
                }
                return ret;
            } else {
                return -1;
            }
        }
    }

    private native static int stopRssiMonitoringNative(int iface, int idx);

    public int stopRssiMonitoring() {
        Log.d(TAG, "stopRssiMonitoring, cmdId " + sRssiMonitorCmdId);
        synchronized (sLock) {
            if (isHalStarted()) {
                int ret = 0;
                if (sRssiMonitorCmdId != 0) {
                    ret = stopRssiMonitoringNative(sWlan0Index, sRssiMonitorCmdId);
                }
                sRssiMonitorCmdId = 0;
                return ret;
            } else {
                return -1;
            }
        }
    }

    private static native WifiWakeReasonAndCounts getWlanWakeReasonCountNative(int iface);

    /**
     * Fetch the host wakeup reasons stats from wlan driver.
     * @return the |WifiWakeReasonAndCounts| object retrieved from the wlan driver.
     */
    public WifiWakeReasonAndCounts getWlanWakeReasonCount() {
        Log.d(TAG, "getWlanWakeReasonCount " + sWlan0Index);
        synchronized (sLock) {
            if (isHalStarted()) {
                return getWlanWakeReasonCountNative(sWlan0Index);
            } else {
                return null;
            }
        }
    }

    private static native int configureNeighborDiscoveryOffload(int iface, boolean enabled);

    public boolean configureNeighborDiscoveryOffload(boolean enabled) {
        final String logMsg =  "configureNeighborDiscoveryOffload(" + enabled + ")";
        Log.d(mTAG, logMsg);
        synchronized (sLock) {
            if (isHalStarted()) {
                final int ret = configureNeighborDiscoveryOffload(sWlan0Index, enabled);
                if (ret != 0) {
                    Log.d(mTAG, logMsg + " returned: " + ret);
                }
                return (ret == 0);
            }
        }
        return false;
    }
}
