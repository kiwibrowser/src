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

import android.net.NetworkInfo;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiSsid;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pGroup;
import android.net.wifi.p2p.WifiP2pProvDiscEvent;
import android.net.wifi.p2p.nsd.WifiP2pServiceResponse;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Base64;
import android.util.LocalLog;
import android.util.Log;
import android.util.SparseArray;

import com.android.server.wifi.hotspot2.IconEvent;
import com.android.server.wifi.hotspot2.Utils;
import com.android.server.wifi.p2p.WifiP2pServiceImpl.P2pStatus;

import com.android.internal.util.Protocol;
import com.android.internal.util.StateMachine;

import java.io.IOException;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Listens for events from the wpa_supplicant server, and passes them on
 * to the {@link StateMachine} for handling. Runs in its own thread.
 *
 * @hide
 */
public class WifiMonitor {

    private static boolean DBG = false;
    private static final boolean VDBG = false;
    private static final String TAG = "WifiMonitor";

    /** Events we receive from the supplicant daemon */

    private static final int CONNECTED    = 1;
    private static final int DISCONNECTED = 2;
    private static final int STATE_CHANGE = 3;
    private static final int SCAN_RESULTS = 4;
    private static final int LINK_SPEED   = 5;
    private static final int TERMINATING  = 6;
    private static final int DRIVER_STATE = 7;
    private static final int EAP_FAILURE  = 8;
    private static final int ASSOC_REJECT = 9;
    private static final int SSID_TEMP_DISABLE = 10;
    private static final int SSID_REENABLE = 11;
    private static final int BSS_ADDED    = 12;
    private static final int BSS_REMOVED  = 13;
    private static final int UNKNOWN      = 14;
    private static final int SCAN_FAILED  = 15;

    /** All events coming from the supplicant start with this prefix */
    private static final String EVENT_PREFIX_STR = "CTRL-EVENT-";
    private static final int EVENT_PREFIX_LEN_STR = EVENT_PREFIX_STR.length();

    /** All events coming from the supplicant start with this prefix */
    private static final String REQUEST_PREFIX_STR = "CTRL-REQ-";
    private static final int REQUEST_PREFIX_LEN_STR = REQUEST_PREFIX_STR.length();


    /** All WPA events coming from the supplicant start with this prefix */
    private static final String WPA_EVENT_PREFIX_STR = "WPA:";
    private static final String PASSWORD_MAY_BE_INCORRECT_STR =
       "pre-shared key may be incorrect";

    /* WPS events */
    private static final String WPS_SUCCESS_STR = "WPS-SUCCESS";

    /* Format: WPS-FAIL msg=%d [config_error=%d] [reason=%d (%s)] */
    private static final String WPS_FAIL_STR    = "WPS-FAIL";
    private static final String WPS_FAIL_PATTERN =
            "WPS-FAIL msg=\\d+(?: config_error=(\\d+))?(?: reason=(\\d+))?";

    /* config error code values for config_error=%d */
    private static final int CONFIG_MULTIPLE_PBC_DETECTED = 12;
    private static final int CONFIG_AUTH_FAILURE = 18;

    /* reason code values for reason=%d */
    private static final int REASON_TKIP_ONLY_PROHIBITED = 1;
    private static final int REASON_WEP_PROHIBITED = 2;

    private static final String WPS_OVERLAP_STR = "WPS-OVERLAP-DETECTED";
    private static final String WPS_TIMEOUT_STR = "WPS-TIMEOUT";

    /* Hotspot 2.0 ANQP query events */
    private static final String GAS_QUERY_PREFIX_STR = "GAS-QUERY-";
    private static final String GAS_QUERY_START_STR = "GAS-QUERY-START";
    private static final String GAS_QUERY_DONE_STR = "GAS-QUERY-DONE";
    private static final String RX_HS20_ANQP_ICON_STR = "RX-HS20-ANQP-ICON";
    private static final int RX_HS20_ANQP_ICON_STR_LEN = RX_HS20_ANQP_ICON_STR.length();

    /* Hotspot 2.0 events */
    private static final String HS20_PREFIX_STR = "HS20-";
    public static final String HS20_SUB_REM_STR = "HS20-SUBSCRIPTION-REMEDIATION";
    public static final String HS20_DEAUTH_STR = "HS20-DEAUTH-IMMINENT-NOTICE";

    private static final String IDENTITY_STR = "IDENTITY";

    private static final String SIM_STR = "SIM";


    //used to debug and detect if we miss an event
    private static int eventLogCounter = 0;

    /**
     * Names of events from wpa_supplicant (minus the prefix). In the
     * format descriptions, * &quot;<code>x</code>&quot;
     * designates a dynamic value that needs to be parsed out from the event
     * string
     */
    /**
     * <pre>
     * CTRL-EVENT-CONNECTED - Connection to xx:xx:xx:xx:xx:xx completed
     * </pre>
     * <code>xx:xx:xx:xx:xx:xx</code> is the BSSID of the associated access point
     */
    private static final String CONNECTED_STR =    "CONNECTED";
    private static final String ConnectPrefix = "Connection to ";
    private static final String ConnectSuffix = " completed";

    /**
     * <pre>
     * CTRL-EVENT-DISCONNECTED - Disconnect event - remove keys
     * </pre>
     */
    private static final String DISCONNECTED_STR = "DISCONNECTED";
    /**
     * <pre>
     * CTRL-EVENT-STATE-CHANGE x
     * </pre>
     * <code>x</code> is the numerical value of the new state.
     */
    private static final String STATE_CHANGE_STR =  "STATE-CHANGE";
    /**
     * <pre>
     * CTRL-EVENT-SCAN-RESULTS ready
     * </pre>
     */
    private static final String SCAN_RESULTS_STR =  "SCAN-RESULTS";

    /**
     * <pre>
     * CTRL-EVENT-SCAN-FAILED ret=code[ retry=1]
     * </pre>
     */
    private static final String SCAN_FAILED_STR =  "SCAN-FAILED";

    /**
     * <pre>
     * CTRL-EVENT-LINK-SPEED x Mb/s
     * </pre>
     * {@code x} is the link speed in Mb/sec.
     */
    private static final String LINK_SPEED_STR = "LINK-SPEED";
    /**
     * <pre>
     * CTRL-EVENT-TERMINATING - signal x
     * </pre>
     * <code>x</code> is the signal that caused termination.
     */
    private static final String TERMINATING_STR =  "TERMINATING";
    /**
     * <pre>
     * CTRL-EVENT-DRIVER-STATE state
     * </pre>
     * <code>state</code> can be HANGED
     */
    private static final String DRIVER_STATE_STR = "DRIVER-STATE";
    /**
     * <pre>
     * CTRL-EVENT-EAP-FAILURE EAP authentication failed
     * </pre>
     */
    private static final String EAP_FAILURE_STR = "EAP-FAILURE";

    /**
     * This indicates an authentication failure on EAP FAILURE event
     */
    private static final String EAP_AUTH_FAILURE_STR = "EAP authentication failed";

    /* EAP authentication timeout events */
    private static final String AUTH_EVENT_PREFIX_STR = "Authentication with";
    private static final String AUTH_TIMEOUT_STR = "timed out.";

    /**
     * This indicates an assoc reject event
     */
    private static final String ASSOC_REJECT_STR = "ASSOC-REJECT";

    /**
     * This indicates auth or association failure bad enough so as network got disabled
     * - WPA_PSK auth failure suspecting shared key mismatch
     * - failed multiple Associations
     */
    private static final String TEMP_DISABLED_STR = "SSID-TEMP-DISABLED";

    /**
     * This indicates a previously disabled SSID was reenabled by supplicant
     */
    private static final String REENABLED_STR = "SSID-REENABLED";

    /**
     * This indicates supplicant found a given BSS
     */
    private static final String BSS_ADDED_STR = "BSS-ADDED";

    /**
     * This indicates supplicant removed a given BSS
     */
    private static final String BSS_REMOVED_STR = "BSS-REMOVED";

    /**
     * Regex pattern for extracting an Ethernet-style MAC address from a string.
     * Matches a strings like the following:<pre>
     * CTRL-EVENT-CONNECTED - Connection to 00:1e:58:ec:d5:6d completed (reauth) [id=1 id_str=]</pre>
     */
    private static Pattern mConnectedEventPattern =
        Pattern.compile("((?:[0-9a-f]{2}:){5}[0-9a-f]{2}) .* \\[id=([0-9]+) ");

    /**
     * Regex pattern for extracting an Ethernet-style MAC address from a string.
     * Matches a strings like the following:<pre>
     * CTRL-EVENT-DISCONNECTED - bssid=ac:22:0b:24:70:74 reason=3 locally_generated=1
     */
    private static Pattern mDisconnectedEventPattern =
            Pattern.compile("((?:[0-9a-f]{2}:){5}[0-9a-f]{2}) +" +
                    "reason=([0-9]+) +locally_generated=([0-1])");

    /**
     * Regex pattern for extracting an Ethernet-style MAC address from a string.
     * Matches a strings like the following:<pre>
     * CTRL-EVENT-ASSOC-REJECT - bssid=ac:22:0b:24:70:74 status_code=1
     */
    private static Pattern mAssocRejectEventPattern =
            Pattern.compile("((?:[0-9a-f]{2}:){5}[0-9a-f]{2}) +" +
                    "status_code=([0-9]+)");

    /**
     * Regex pattern for extracting an Ethernet-style MAC address from a string.
     * Matches a strings like the following:<pre>
     * IFNAME=wlan0 Trying to associate with 6c:f3:7f:ae:87:71
     */
    private static final String TARGET_BSSID_STR =  "Trying to associate with ";

    private static Pattern mTargetBSSIDPattern =
            Pattern.compile("Trying to associate with ((?:[0-9a-f]{2}:){5}[0-9a-f]{2}).*");

    /**
     * Regex pattern for extracting an Ethernet-style MAC address from a string.
     * Matches a strings like the following:<pre>
     * IFNAME=wlan0 Associated with 6c:f3:7f:ae:87:71
     */
    private static final String ASSOCIATED_WITH_STR =  "Associated with ";

    private static Pattern mAssociatedPattern =
            Pattern.compile("Associated with ((?:[0-9a-f]{2}:){5}[0-9a-f]{2}).*");

    /**
     * Regex pattern for extracting an external GSM sim authentication request from a string.
     * Matches a strings like the following:<pre>
     * CTRL-REQ-SIM-<network id>:GSM-AUTH:<RAND1>:<RAND2>[:<RAND3>] needed for SSID <SSID>
     * This pattern should find
     *    0 - id
     *    1 - Rand1
     *    2 - Rand2
     *    3 - Rand3
     *    4 - SSID
     */
    private static Pattern mRequestGsmAuthPattern =
            Pattern.compile("SIM-([0-9]*):GSM-AUTH((:[0-9a-f]+)+) needed for SSID (.+)");

    /**
     * Regex pattern for extracting an external 3G sim authentication request from a string.
     * Matches a strings like the following:<pre>
     * CTRL-REQ-SIM-<network id>:UMTS-AUTH:<RAND>:<AUTN> needed for SSID <SSID>
     * This pattern should find
     *    1 - id
     *    2 - Rand
     *    3 - Autn
     *    4 - SSID
     */
    private static Pattern mRequestUmtsAuthPattern =
            Pattern.compile("SIM-([0-9]*):UMTS-AUTH:([0-9a-f]+):([0-9a-f]+) needed for SSID (.+)");

    /**
     * Regex pattern for extracting SSIDs from request identity string.
     * Matches a strings like the following:<pre>
     * CTRL-REQ-IDENTITY-xx:Identity needed for SSID XXXX</pre>
     */
    private static Pattern mRequestIdentityPattern =
            Pattern.compile("IDENTITY-([0-9]+):Identity needed for SSID (.+)");

    /** P2P events */
    private static final String P2P_EVENT_PREFIX_STR = "P2P";

    /* P2P-DEVICE-FOUND fa:7b:7a:42:02:13 p2p_dev_addr=fa:7b:7a:42:02:13 pri_dev_type=1-0050F204-1
       name='p2p-TEST1' config_methods=0x188 dev_capab=0x27 group_capab=0x0 */
    private static final String P2P_DEVICE_FOUND_STR = "P2P-DEVICE-FOUND";

    /* P2P-DEVICE-LOST p2p_dev_addr=42:fc:89:e1:e2:27 */
    private static final String P2P_DEVICE_LOST_STR = "P2P-DEVICE-LOST";

    /* P2P-FIND-STOPPED */
    private static final String P2P_FIND_STOPPED_STR = "P2P-FIND-STOPPED";

    /* P2P-GO-NEG-REQUEST 42:fc:89:a8:96:09 dev_passwd_id=4 */
    private static final String P2P_GO_NEG_REQUEST_STR = "P2P-GO-NEG-REQUEST";

    private static final String P2P_GO_NEG_SUCCESS_STR = "P2P-GO-NEG-SUCCESS";

    /* P2P-GO-NEG-FAILURE status=x */
    private static final String P2P_GO_NEG_FAILURE_STR = "P2P-GO-NEG-FAILURE";

    private static final String P2P_GROUP_FORMATION_SUCCESS_STR =
            "P2P-GROUP-FORMATION-SUCCESS";

    private static final String P2P_GROUP_FORMATION_FAILURE_STR =
            "P2P-GROUP-FORMATION-FAILURE";

    /* P2P-GROUP-STARTED p2p-wlan0-0 [client|GO] ssid="DIRECT-W8" freq=2437
       [psk=2182b2e50e53f260d04f3c7b25ef33c965a3291b9b36b455a82d77fd82ca15bc|passphrase="fKG4jMe3"]
       go_dev_addr=fa:7b:7a:42:02:13 [PERSISTENT] */
    private static final String P2P_GROUP_STARTED_STR = "P2P-GROUP-STARTED";

    /* P2P-GROUP-REMOVED p2p-wlan0-0 [client|GO] reason=REQUESTED */
    private static final String P2P_GROUP_REMOVED_STR = "P2P-GROUP-REMOVED";

    /* P2P-INVITATION-RECEIVED sa=fa:7b:7a:42:02:13 go_dev_addr=f8:7b:7a:42:02:13
        bssid=fa:7b:7a:42:82:13 unknown-network */
    private static final String P2P_INVITATION_RECEIVED_STR = "P2P-INVITATION-RECEIVED";

    /* P2P-INVITATION-RESULT status=1 */
    private static final String P2P_INVITATION_RESULT_STR = "P2P-INVITATION-RESULT";

    /* P2P-PROV-DISC-PBC-REQ 42:fc:89:e1:e2:27 p2p_dev_addr=42:fc:89:e1:e2:27
       pri_dev_type=1-0050F204-1 name='p2p-TEST2' config_methods=0x188 dev_capab=0x27
       group_capab=0x0 */
    private static final String P2P_PROV_DISC_PBC_REQ_STR = "P2P-PROV-DISC-PBC-REQ";

    /* P2P-PROV-DISC-PBC-RESP 02:12:47:f2:5a:36 */
    private static final String P2P_PROV_DISC_PBC_RSP_STR = "P2P-PROV-DISC-PBC-RESP";

    /* P2P-PROV-DISC-ENTER-PIN 42:fc:89:e1:e2:27 p2p_dev_addr=42:fc:89:e1:e2:27
       pri_dev_type=1-0050F204-1 name='p2p-TEST2' config_methods=0x188 dev_capab=0x27
       group_capab=0x0 */
    private static final String P2P_PROV_DISC_ENTER_PIN_STR = "P2P-PROV-DISC-ENTER-PIN";
    /* P2P-PROV-DISC-SHOW-PIN 42:fc:89:e1:e2:27 44490607 p2p_dev_addr=42:fc:89:e1:e2:27
       pri_dev_type=1-0050F204-1 name='p2p-TEST2' config_methods=0x188 dev_capab=0x27
       group_capab=0x0 */
    private static final String P2P_PROV_DISC_SHOW_PIN_STR = "P2P-PROV-DISC-SHOW-PIN";
    /* P2P-PROV-DISC-FAILURE p2p_dev_addr=42:fc:89:e1:e2:27 */
    private static final String P2P_PROV_DISC_FAILURE_STR = "P2P-PROV-DISC-FAILURE";

    /*
     * Protocol format is as follows.<br>
     * See the Table.62 in the WiFi Direct specification for the detail.
     * ______________________________________________________________
     * |           Length(2byte)     | Type(1byte) | TransId(1byte)}|
     * ______________________________________________________________
     * | status(1byte)  |            vendor specific(variable)      |
     *
     * P2P-SERV-DISC-RESP 42:fc:89:e1:e2:27 1 0300000101
     * length=3, service type=0(ALL Service), transaction id=1,
     * status=1(service protocol type not available)<br>
     *
     * P2P-SERV-DISC-RESP 42:fc:89:e1:e2:27 1 0300020201
     * length=3, service type=2(UPnP), transaction id=2,
     * status=1(service protocol type not available)
     *
     * P2P-SERV-DISC-RESP 42:fc:89:e1:e2:27 1 990002030010757569643a3131323
     * 2646534652d383537342d353961622d393332322d3333333435363738393034343a3
     * a75726e3a736368656d61732d75706e702d6f72673a736572766963653a436f6e746
     * 56e744469726563746f72793a322c757569643a36383539646564652d383537342d3
     * 53961622d393333322d3132333435363738393031323a3a75706e703a726f6f74646
     * 576696365
     * length=153,type=2(UPnP),transaction id=3,status=0
     *
     * UPnP Protocol format is as follows.
     * ______________________________________________________
     * |  Version (1)  |          USN (Variable)            |
     *
     * version=0x10(UPnP1.0) data=usn:uuid:1122de4e-8574-59ab-9322-33345678
     * 9044::urn:schemas-upnp-org:service:ContentDirectory:2,usn:uuid:6859d
     * ede-8574-59ab-9332-123456789012::upnp:rootdevice
     *
     * P2P-SERV-DISC-RESP 58:17:0c:bc:dd:ca 21 1900010200045f6970
     * 70c00c000c01094d795072696e746572c027
     * length=25, type=1(Bonjour),transaction id=2,status=0
     *
     * Bonjour Protocol format is as follows.
     * __________________________________________________________
     * |DNS Name(Variable)|DNS Type(1)|Version(1)|RDATA(Variable)|
     *
     * DNS Name=_ipp._tcp.local.,DNS type=12(PTR), Version=1,
     * RDATA=MyPrinter._ipp._tcp.local.
     *
     */
    private static final String P2P_SERV_DISC_RESP_STR = "P2P-SERV-DISC-RESP";

    private static final String HOST_AP_EVENT_PREFIX_STR = "AP";
    /* AP-STA-CONNECTED 42:fc:89:a8:96:09 dev_addr=02:90:4c:a0:92:54 */
    private static final String AP_STA_CONNECTED_STR = "AP-STA-CONNECTED";
    /* AP-STA-DISCONNECTED 42:fc:89:a8:96:09 */
    private static final String AP_STA_DISCONNECTED_STR = "AP-STA-DISCONNECTED";
    private static final String ANQP_DONE_STR = "ANQP-QUERY-DONE";
    private static final String HS20_ICON_STR = "RX-HS20-ICON";

    /* Supplicant events reported to a state machine */
    private static final int BASE = Protocol.BASE_WIFI_MONITOR;

    /* Connection to supplicant established */
    public static final int SUP_CONNECTION_EVENT                 = BASE + 1;
    /* Connection to supplicant lost */
    public static final int SUP_DISCONNECTION_EVENT              = BASE + 2;
   /* Network connection completed */
    public static final int NETWORK_CONNECTION_EVENT             = BASE + 3;
    /* Network disconnection completed */
    public static final int NETWORK_DISCONNECTION_EVENT          = BASE + 4;
    /* Scan results are available */
    public static final int SCAN_RESULTS_EVENT                   = BASE + 5;
    /* Supplicate state changed */
    public static final int SUPPLICANT_STATE_CHANGE_EVENT        = BASE + 6;
    /* Password failure and EAP authentication failure */
    public static final int AUTHENTICATION_FAILURE_EVENT         = BASE + 7;
    /* WPS success detected */
    public static final int WPS_SUCCESS_EVENT                    = BASE + 8;
    /* WPS failure detected */
    public static final int WPS_FAIL_EVENT                       = BASE + 9;
     /* WPS overlap detected */
    public static final int WPS_OVERLAP_EVENT                    = BASE + 10;
     /* WPS timeout detected */
    public static final int WPS_TIMEOUT_EVENT                    = BASE + 11;
    /* Driver was hung */
    public static final int DRIVER_HUNG_EVENT                    = BASE + 12;
    /* SSID was disabled due to auth failure or excessive
     * connection failures */
    public static final int SSID_TEMP_DISABLED                   = BASE + 13;
    /* SSID reenabled by supplicant */
    public static final int SSID_REENABLED                       = BASE + 14;

    /* Request Identity */
    public static final int SUP_REQUEST_IDENTITY                 = BASE + 15;

    /* Request SIM Auth */
    public static final int SUP_REQUEST_SIM_AUTH                 = BASE + 16;

    public static final int SCAN_FAILED_EVENT                    = BASE + 17;

    /* P2P events */
    public static final int P2P_DEVICE_FOUND_EVENT               = BASE + 21;
    public static final int P2P_DEVICE_LOST_EVENT                = BASE + 22;
    public static final int P2P_GO_NEGOTIATION_REQUEST_EVENT     = BASE + 23;
    public static final int P2P_GO_NEGOTIATION_SUCCESS_EVENT     = BASE + 25;
    public static final int P2P_GO_NEGOTIATION_FAILURE_EVENT     = BASE + 26;
    public static final int P2P_GROUP_FORMATION_SUCCESS_EVENT    = BASE + 27;
    public static final int P2P_GROUP_FORMATION_FAILURE_EVENT    = BASE + 28;
    public static final int P2P_GROUP_STARTED_EVENT              = BASE + 29;
    public static final int P2P_GROUP_REMOVED_EVENT              = BASE + 30;
    public static final int P2P_INVITATION_RECEIVED_EVENT        = BASE + 31;
    public static final int P2P_INVITATION_RESULT_EVENT          = BASE + 32;
    public static final int P2P_PROV_DISC_PBC_REQ_EVENT          = BASE + 33;
    public static final int P2P_PROV_DISC_PBC_RSP_EVENT          = BASE + 34;
    public static final int P2P_PROV_DISC_ENTER_PIN_EVENT        = BASE + 35;
    public static final int P2P_PROV_DISC_SHOW_PIN_EVENT         = BASE + 36;
    public static final int P2P_FIND_STOPPED_EVENT               = BASE + 37;
    public static final int P2P_SERV_DISC_RESP_EVENT             = BASE + 38;
    public static final int P2P_PROV_DISC_FAILURE_EVENT          = BASE + 39;

    /* hostap events */
    public static final int AP_STA_DISCONNECTED_EVENT            = BASE + 41;
    public static final int AP_STA_CONNECTED_EVENT               = BASE + 42;

    /* Indicates assoc reject event */
    public static final int ASSOCIATION_REJECTION_EVENT          = BASE + 43;
    public static final int ANQP_DONE_EVENT                      = BASE + 44;

    /* hotspot 2.0 ANQP events */
    public static final int GAS_QUERY_START_EVENT                = BASE + 51;
    public static final int GAS_QUERY_DONE_EVENT                 = BASE + 52;
    public static final int RX_HS20_ANQP_ICON_EVENT              = BASE + 53;

    /* hotspot 2.0 events */
    public static final int HS20_REMEDIATION_EVENT               = BASE + 61;

    /**
     * This indicates a read error on the monitor socket conenction
     */
    private static final String WPA_RECV_ERROR_STR = "recv error";

    /**
     * Max errors before we close supplicant connection
     */
    private static final int MAX_RECV_ERRORS    = 10;

    // Singleton instance
    private static WifiMonitor sWifiMonitor = new WifiMonitor();
    public static WifiMonitor getInstance() {
        return sWifiMonitor;
    }

    private final WifiNative mWifiNative;
    private WifiMonitor() {
        mWifiNative = WifiNative.getWlanNativeInterface();
    }

    private int mRecvErrors = 0;

    void enableVerboseLogging(int verbose) {
        if (verbose > 0) {
            DBG = true;
        } else {
            DBG = false;
        }
    }

    private boolean mConnected = false;

    // TODO(b/27569474) remove support for multiple handlers for the same event
    private final Map<String, SparseArray<Set<Handler>>> mHandlerMap = new HashMap<>();
    public synchronized void registerHandler(String iface, int what, Handler handler) {
        SparseArray<Set<Handler>> ifaceHandlers = mHandlerMap.get(iface);
        if (ifaceHandlers == null) {
            ifaceHandlers = new SparseArray<>();
            mHandlerMap.put(iface, ifaceHandlers);
        }
        Set<Handler> ifaceWhatHandlers = ifaceHandlers.get(what);
        if (ifaceWhatHandlers == null) {
            ifaceWhatHandlers = new ArraySet<>();
            ifaceHandlers.put(what, ifaceWhatHandlers);
        }
        ifaceWhatHandlers.add(handler);
    }

    private final Map<String, Boolean> mMonitoringMap = new HashMap<>();
    private boolean isMonitoring(String iface) {
        Boolean val = mMonitoringMap.get(iface);
        if (val == null) {
            return false;
        }
        else {
            return val.booleanValue();
        }
    }

    private void setMonitoring(String iface, boolean enabled) {
        mMonitoringMap.put(iface, enabled);
    }
    private void setMonitoringNone() {
        for (String iface : mMonitoringMap.keySet()) {
            setMonitoring(iface, false);
        }
    }


    private boolean ensureConnectedLocked() {
        if (mConnected) {
            return true;
        }

        if (DBG) Log.d(TAG, "connecting to supplicant");
        int connectTries = 0;
        while (true) {
            if (mWifiNative.connectToSupplicant()) {
                mConnected = true;
                new MonitorThread(mWifiNative.getLocalLog()).start();
                return true;
            }
            if (connectTries++ < 5) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException ignore) {
                }
            } else {
                return false;
            }
        }
    }

    public synchronized void startMonitoring(String iface) {
        Log.d(TAG, "startMonitoring(" + iface + ") with mConnected = " + mConnected);

        if (ensureConnectedLocked()) {
            setMonitoring(iface, true);
            sendMessage(iface, SUP_CONNECTION_EVENT);
        }
        else {
            boolean originalMonitoring = isMonitoring(iface);
            setMonitoring(iface, true);
            sendMessage(iface, SUP_DISCONNECTION_EVENT);
            setMonitoring(iface, originalMonitoring);
            Log.e(TAG, "startMonitoring(" + iface + ") failed!");
        }
    }

    public synchronized void stopMonitoring(String iface) {
        if (DBG) Log.d(TAG, "stopMonitoring(" + iface + ")");
        setMonitoring(iface, true);
        sendMessage(iface, SUP_DISCONNECTION_EVENT);
        setMonitoring(iface, false);
    }

    public synchronized void stopSupplicant() {
        mWifiNative.stopSupplicant();
    }

    public synchronized void killSupplicant(boolean p2pSupported) {
        String suppState = System.getProperty("init.svc.wpa_supplicant");
        if (suppState == null) suppState = "unknown";
        String p2pSuppState = System.getProperty("init.svc.p2p_supplicant");
        if (p2pSuppState == null) p2pSuppState = "unknown";

        Log.e(TAG, "killSupplicant p2p" + p2pSupported
                + " init.svc.wpa_supplicant=" + suppState
                + " init.svc.p2p_supplicant=" + p2pSuppState);
        mWifiNative.killSupplicant(p2pSupported);
        mConnected = false;
        setMonitoringNone();
    }


    /**
     * Similar functions to Handler#sendMessage that send the message to the registered handler
     * for the given interface and message what.
     * All of these should be called with the WifiMonitor class lock
     */
    private void sendMessage(String iface, int what) {
        sendMessage(iface, Message.obtain(null, what));
    }

    private void sendMessage(String iface, int what, Object obj) {
        sendMessage(iface, Message.obtain(null, what, obj));
    }

    private void sendMessage(String iface, int what, int arg1) {
        sendMessage(iface, Message.obtain(null, what, arg1, 0));
    }

    private void sendMessage(String iface, int what, int arg1, int arg2) {
        sendMessage(iface, Message.obtain(null, what, arg1, arg2));
    }

    private void sendMessage(String iface, int what, int arg1, int arg2, Object obj) {
        sendMessage(iface, Message.obtain(null, what, arg1, arg2, obj));
    }

    private void sendMessage(String iface, Message message) {
        SparseArray<Set<Handler>> ifaceHandlers = mHandlerMap.get(iface);
        if (iface != null && ifaceHandlers != null) {
            if (isMonitoring(iface)) {
                boolean firstHandler = true;
                Set<Handler> ifaceWhatHandlers = ifaceHandlers.get(message.what);
                if (ifaceWhatHandlers != null) {
                    for (Handler handler : ifaceWhatHandlers) {
                        if (firstHandler) {
                            firstHandler = false;
                            sendMessage(handler, message);
                        }
                        else {
                            sendMessage(handler, Message.obtain(message));
                        }
                    }
                }
            } else {
                if (DBG) Log.d(TAG, "Dropping event because (" + iface + ") is stopped");
            }
        } else {
            if (DBG) Log.d(TAG, "Sending to all monitors because there's no matching iface");
            boolean firstHandler = true;
            for (Map.Entry<String, SparseArray<Set<Handler>>> entry : mHandlerMap.entrySet()) {
                if (isMonitoring(entry.getKey())) {
                    Set<Handler> ifaceWhatHandlers = entry.getValue().get(message.what);
                    for (Handler handler : ifaceWhatHandlers) {
                        if (firstHandler) {
                            firstHandler = false;
                            sendMessage(handler, message);
                        }
                        else {
                            sendMessage(handler, Message.obtain(message));
                        }
                    }
                }
            }
        }
    }

    private void sendMessage(Handler handler, Message message) {
        if (handler != null) {
            message.setTarget(handler);
            message.sendToTarget();
        }
    }

    private class MonitorThread extends Thread {
        private final LocalLog mLocalLog;

        public MonitorThread(LocalLog localLog) {
            super("WifiMonitor");
            mLocalLog = localLog;
        }

        public void run() {
            if (DBG) {
                Log.d(TAG, "MonitorThread start with mConnected=" + mConnected);
            }
            //noinspection InfiniteLoopStatement
            for (;;) {
                if (!mConnected) {
                    if (DBG) Log.d(TAG, "MonitorThread exit because mConnected is false");
                    break;
                }
                String eventStr = mWifiNative.waitForEvent();

                // Skip logging the common but mostly uninteresting events
                if (!eventStr.contains(BSS_ADDED_STR) && !eventStr.contains(BSS_REMOVED_STR)) {
                    if (DBG) Log.d(TAG, "Event [" + eventStr + "]");
                    mLocalLog.log("Event [" + eventStr + "]");
                }

                if (dispatchEvent(eventStr)) {
                    if (DBG) Log.d(TAG, "Disconnecting from the supplicant, no more events");
                    break;
                }
            }
        }
    }

    private synchronized boolean dispatchEvent(String eventStr) {
        String iface;
        // IFNAME=wlan0 ANQP-QUERY-DONE addr=18:cf:5e:26:a4:88 result=SUCCESS
        if (eventStr.startsWith("IFNAME=")) {
            int space = eventStr.indexOf(' ');
            if (space != -1) {
                iface = eventStr.substring(7, space);
                if (!mHandlerMap.containsKey(iface) && iface.startsWith("p2p-")) {
                    // p2p interfaces are created dynamically, but we have
                    // only one P2p state machine monitoring all of them; look
                    // for it explicitly, and send messages there ..
                    iface = "p2p0";
                }
                eventStr = eventStr.substring(space + 1);
            } else {
                // No point dispatching this event to any interface, the dispatched
                // event string will begin with "IFNAME=" which dispatchEvent can't really
                // do anything about.
                Log.e(TAG, "Dropping malformed event (unparsable iface): " + eventStr);
                return false;
            }
        } else {
            // events without prefix belong to p2p0 monitor
            iface = "p2p0";
        }

        if (VDBG) Log.d(TAG, "Dispatching event to interface: " + iface);

        if (dispatchEvent(eventStr, iface)) {
            mConnected = false;
            return true;
        }
        return false;
    }

    private Map<String, Long> mLastConnectBSSIDs = new HashMap<String, Long>() {
        public Long get(String iface) {
            Long value = super.get(iface);
            if (value != null) {
                return value;
            }
            return 0L;
        }
    };

    /* @return true if the event was supplicant disconnection */
    private boolean dispatchEvent(String eventStr, String iface) {
        if (DBG) {
            // Dont log CTRL-EVENT-BSS-ADDED which are too verbose and not handled
            if (eventStr != null && !eventStr.contains("CTRL-EVENT-BSS-ADDED")) {
                Log.d(TAG, iface + " cnt=" + Integer.toString(eventLogCounter)
                        + " dispatchEvent: " + eventStr);
            }
        }

        if (!eventStr.startsWith(EVENT_PREFIX_STR)) {
            if (eventStr.startsWith(WPS_SUCCESS_STR)) {
                sendMessage(iface, WPS_SUCCESS_EVENT);
            } else if (eventStr.startsWith(WPS_FAIL_STR)) {
                handleWpsFailEvent(eventStr, iface);
            } else if (eventStr.startsWith(WPS_OVERLAP_STR)) {
                sendMessage(iface, WPS_OVERLAP_EVENT);
            } else if (eventStr.startsWith(WPS_TIMEOUT_STR)) {
                sendMessage(iface, WPS_TIMEOUT_EVENT);
            } else if (eventStr.startsWith(P2P_EVENT_PREFIX_STR)) {
                handleP2pEvents(eventStr, iface);
            } else if (eventStr.startsWith(HOST_AP_EVENT_PREFIX_STR)) {
                handleHostApEvents(eventStr, iface);
            } else if (eventStr.startsWith(ANQP_DONE_STR)) {
                try {
                    handleAnqpResult(eventStr, iface);
                }
                catch (IllegalArgumentException iae) {
                    Log.e(TAG, "Bad ANQP event string: '" + eventStr + "': " + iae);
                }
            } else if (eventStr.startsWith(HS20_ICON_STR)) {
                try {
                    handleIconResult(eventStr, iface);
                }
                catch (IllegalArgumentException iae) {
                    Log.e(TAG, "Bad Icon event string: '" + eventStr + "': " + iae);
                }
            }
            else if (eventStr.startsWith(HS20_SUB_REM_STR)) {
                // Tack on the last connected BSSID so we have some idea what AP the WNM pertains to
                handleWnmFrame(String.format("%012x %s",
                                mLastConnectBSSIDs.get(iface), eventStr), iface);
            } else if (eventStr.startsWith(HS20_DEAUTH_STR)) {
                handleWnmFrame(String.format("%012x %s",
                                mLastConnectBSSIDs.get(iface), eventStr), iface);
            } else if (eventStr.startsWith(REQUEST_PREFIX_STR)) {
                handleRequests(eventStr, iface);
            } else if (eventStr.startsWith(TARGET_BSSID_STR)) {
                handleTargetBSSIDEvent(eventStr, iface);
            } else if (eventStr.startsWith(ASSOCIATED_WITH_STR)) {
                handleAssociatedBSSIDEvent(eventStr, iface);
            } else if (eventStr.startsWith(AUTH_EVENT_PREFIX_STR) &&
                    eventStr.endsWith(AUTH_TIMEOUT_STR)) {
                sendMessage(iface, AUTHENTICATION_FAILURE_EVENT);
            } else {
                if (DBG) Log.w(TAG, "couldn't identify event type - " + eventStr);
            }
            eventLogCounter++;
            return false;
        }

        String eventName = eventStr.substring(EVENT_PREFIX_LEN_STR);
        int nameEnd = eventName.indexOf(' ');
        if (nameEnd != -1)
            eventName = eventName.substring(0, nameEnd);
        if (eventName.length() == 0) {
            if (DBG) Log.i(TAG, "Received wpa_supplicant event with empty event name");
            eventLogCounter++;
            return false;
        }
        /*
        * Map event name into event enum
        */
        int event;
        if (eventName.equals(CONNECTED_STR)) {
            event = CONNECTED;
            long bssid = -1L;
            int prefix = eventStr.indexOf(ConnectPrefix);
            if (prefix >= 0) {
                int suffix = eventStr.indexOf(ConnectSuffix);
                if (suffix > prefix) {
                    try {
                        bssid = Utils.parseMac(
                                eventStr.substring(prefix + ConnectPrefix.length(), suffix));
                    } catch (IllegalArgumentException iae) {
                        bssid = -1L;
                    }
                }
            }
            mLastConnectBSSIDs.put(iface, bssid);
            if (bssid == -1L) {
                Log.w(TAG, "Failed to parse out BSSID from '" + eventStr + "'");
            }
        }
        else if (eventName.equals(DISCONNECTED_STR))
            event = DISCONNECTED;
        else if (eventName.equals(STATE_CHANGE_STR))
            event = STATE_CHANGE;
        else if (eventName.equals(SCAN_RESULTS_STR))
            event = SCAN_RESULTS;
        else if (eventName.equals(SCAN_FAILED_STR))
            event = SCAN_FAILED;
        else if (eventName.equals(LINK_SPEED_STR))
            event = LINK_SPEED;
        else if (eventName.equals(TERMINATING_STR))
            event = TERMINATING;
        else if (eventName.equals(DRIVER_STATE_STR))
            event = DRIVER_STATE;
        else if (eventName.equals(EAP_FAILURE_STR))
            event = EAP_FAILURE;
        else if (eventName.equals(ASSOC_REJECT_STR))
            event = ASSOC_REJECT;
        else if (eventName.equals(TEMP_DISABLED_STR)) {
            event = SSID_TEMP_DISABLE;
        } else if (eventName.equals(REENABLED_STR)) {
            event = SSID_REENABLE;
        } else if (eventName.equals(BSS_ADDED_STR)) {
            event = BSS_ADDED;
        } else if (eventName.equals(BSS_REMOVED_STR)) {
            event = BSS_REMOVED;
        } else
            event = UNKNOWN;

        String eventData = eventStr;
        if (event == DRIVER_STATE || event == LINK_SPEED)
            eventData = eventData.split(" ")[1];
        else if (event == STATE_CHANGE || event == EAP_FAILURE) {
            int ind = eventStr.indexOf(" ");
            if (ind != -1) {
                eventData = eventStr.substring(ind + 1);
            }
        } else {
            int ind = eventStr.indexOf(" - ");
            if (ind != -1) {
                eventData = eventStr.substring(ind + 3);
            }
        }

        if ((event == SSID_TEMP_DISABLE)||(event == SSID_REENABLE)) {
            String substr = null;
            int netId = -1;
            int ind = eventStr.indexOf(" ");
            if (ind != -1) {
                substr = eventStr.substring(ind + 1);
            }
            if (substr != null) {
                String status[] = substr.split(" ");
                for (String key : status) {
                    if (key.regionMatches(0, "id=", 0, 3)) {
                        int idx = 3;
                        netId = 0;
                        while (idx < key.length()) {
                            char c = key.charAt(idx);
                            if ((c >= 0x30) && (c <= 0x39)) {
                                netId *= 10;
                                netId += c - 0x30;
                                idx++;
                            } else {
                                break;
                            }
                        }
                    }
                }
            }
            sendMessage(iface, (event == SSID_TEMP_DISABLE)?
                    SSID_TEMP_DISABLED:SSID_REENABLED, netId, 0, substr);
        } else if (event == STATE_CHANGE) {
            handleSupplicantStateChange(eventData, iface);
        } else if (event == DRIVER_STATE) {
            handleDriverEvent(eventData, iface);
        } else if (event == TERMINATING) {
            /**
             * Close the supplicant connection if we see
             * too many recv errors
             */
            if (eventData.startsWith(WPA_RECV_ERROR_STR)) {
                if (++mRecvErrors > MAX_RECV_ERRORS) {
                    if (DBG) {
                        Log.d(TAG, "too many recv errors, closing connection");
                    }
                } else {
                    eventLogCounter++;
                    return false;
                }
            }

            // Notify and exit
            sendMessage(null, SUP_DISCONNECTION_EVENT, eventLogCounter);
            return true;
        } else if (event == EAP_FAILURE) {
            if (eventData.startsWith(EAP_AUTH_FAILURE_STR)) {
                sendMessage(iface, AUTHENTICATION_FAILURE_EVENT, eventLogCounter);
            }
        } else if (event == ASSOC_REJECT) {
            Matcher match = mAssocRejectEventPattern.matcher(eventData);
            String BSSID = "";
            int status = -1;
            if (!match.find()) {
                if (DBG) Log.d(TAG, "Assoc Reject: Could not parse assoc reject string");
            } else {
                int groupNumber = match.groupCount();
                int statusGroupNumber = -1;
                if (groupNumber == 2) {
                    BSSID = match.group(1);
                    statusGroupNumber = 2;
                } else {
                    // Under such case Supplicant does not report BSSID
                    BSSID = null;
                    statusGroupNumber = 1;
                }
                try {
                    status = Integer.parseInt(match.group(statusGroupNumber));
                } catch (NumberFormatException e) {
                    status = -1;
                }
            }
            sendMessage(iface, ASSOCIATION_REJECTION_EVENT, eventLogCounter, status, BSSID);
        } else if (event == BSS_ADDED && !VDBG) {
            // Ignore that event - it is not handled, and dont log it as it is too verbose
        } else if (event == BSS_REMOVED && !VDBG) {
            // Ignore that event - it is not handled, and dont log it as it is too verbose
        }  else {
            handleEvent(event, eventData, iface);
        }
        mRecvErrors = 0;
        eventLogCounter++;
        return false;
    }

    private void handleDriverEvent(String state, String iface) {
        if (state == null) {
            return;
        }
        if (state.equals("HANGED")) {
            sendMessage(iface, DRIVER_HUNG_EVENT);
        }
    }

    /**
     * Handle all supplicant events except STATE-CHANGE
     * @param event the event type
     * @param remainder the rest of the string following the
     * event name and &quot;&#8195;&#8212;&#8195;&quot;
     */
    private void handleEvent(int event, String remainder, String iface) {
        if (DBG) {
            Log.d(TAG, "handleEvent " + Integer.toString(event) + " " + remainder);
        }
        switch (event) {
            case DISCONNECTED:
                handleNetworkStateChange(NetworkInfo.DetailedState.DISCONNECTED, remainder, iface);
                break;

            case CONNECTED:
                handleNetworkStateChange(NetworkInfo.DetailedState.CONNECTED, remainder, iface);
                break;

            case SCAN_RESULTS:
                sendMessage(iface, SCAN_RESULTS_EVENT);
                break;

            case SCAN_FAILED:
                sendMessage(iface, SCAN_FAILED_EVENT);
                break;

            case UNKNOWN:
                if (DBG) {
                    Log.w(TAG, "handleEvent unknown: " + Integer.toString(event) + " " + remainder);
                }
                break;
            default:
                break;
        }
    }

    private void handleTargetBSSIDEvent(String eventStr, String iface) {
        String BSSID = null;
        Matcher match = mTargetBSSIDPattern.matcher(eventStr);
        if (match.find()) {
            BSSID = match.group(1);
        }
        sendMessage(iface, WifiStateMachine.CMD_TARGET_BSSID, eventLogCounter, 0, BSSID);
    }

    private void handleAssociatedBSSIDEvent(String eventStr, String iface) {
        String BSSID = null;
        Matcher match = mAssociatedPattern.matcher(eventStr);
        if (match.find()) {
            BSSID = match.group(1);
        }
        sendMessage(iface, WifiStateMachine.CMD_ASSOCIATED_BSSID, eventLogCounter, 0, BSSID);
    }


    private void handleWpsFailEvent(String dataString, String iface) {
        final Pattern p = Pattern.compile(WPS_FAIL_PATTERN);
        Matcher match = p.matcher(dataString);
        int reason = 0;
        if (match.find()) {
            String cfgErrStr = match.group(1);
            String reasonStr = match.group(2);

            if (reasonStr != null) {
                int reasonInt = Integer.parseInt(reasonStr);
                switch(reasonInt) {
                    case REASON_TKIP_ONLY_PROHIBITED:
                        sendMessage(iface, WPS_FAIL_EVENT, WifiManager.WPS_TKIP_ONLY_PROHIBITED);
                        return;
                    case REASON_WEP_PROHIBITED:
                        sendMessage(iface, WPS_FAIL_EVENT, WifiManager.WPS_WEP_PROHIBITED);
                        return;
                    default:
                        reason = reasonInt;
                        break;
                }
            }
            if (cfgErrStr != null) {
                int cfgErrInt = Integer.parseInt(cfgErrStr);
                switch(cfgErrInt) {
                    case CONFIG_AUTH_FAILURE:
                        sendMessage(iface, WPS_FAIL_EVENT, WifiManager.WPS_AUTH_FAILURE);
                        return;
                    case CONFIG_MULTIPLE_PBC_DETECTED:
                        sendMessage(iface, WPS_FAIL_EVENT, WifiManager.WPS_OVERLAP_ERROR);
                        return;
                    default:
                        if (reason == 0) reason = cfgErrInt;
                        break;
                }
            }
        }
        //For all other errors, return a generic internal error
        sendMessage(iface, WPS_FAIL_EVENT, WifiManager.ERROR, reason);
    }

    /* <event> status=<err> and the special case of <event> reason=FREQ_CONFLICT */
    private P2pStatus p2pError(String dataString) {
        P2pStatus err = P2pStatus.UNKNOWN;
        String[] tokens = dataString.split(" ");
        if (tokens.length < 2) return err;
        String[] nameValue = tokens[1].split("=");
        if (nameValue.length != 2) return err;

        /* Handle the special case of reason=FREQ+CONFLICT */
        if (nameValue[1].equals("FREQ_CONFLICT")) {
            return P2pStatus.NO_COMMON_CHANNEL;
        }
        try {
            err = P2pStatus.valueOf(Integer.parseInt(nameValue[1]));
        } catch (NumberFormatException e) {
            e.printStackTrace();
        }
        return err;
    }

    private WifiP2pDevice getWifiP2pDevice(String dataString) {
        try {
            return new WifiP2pDevice(dataString);
        } catch (IllegalArgumentException e) {
            return null;
        }
    }

    private WifiP2pGroup getWifiP2pGroup(String dataString) {
        try {
            return new WifiP2pGroup(dataString);
        } catch (IllegalArgumentException e) {
            return null;
        }
    }

    /**
     * Handle p2p events
     */
    private void handleP2pEvents(String dataString, String iface) {
        if (dataString.startsWith(P2P_DEVICE_FOUND_STR)) {
            WifiP2pDevice device = getWifiP2pDevice(dataString);
            if (device != null) sendMessage(iface, P2P_DEVICE_FOUND_EVENT, device);
        } else if (dataString.startsWith(P2P_DEVICE_LOST_STR)) {
            WifiP2pDevice device = getWifiP2pDevice(dataString);
            if (device != null) sendMessage(iface, P2P_DEVICE_LOST_EVENT, device);
        } else if (dataString.startsWith(P2P_FIND_STOPPED_STR)) {
            sendMessage(iface, P2P_FIND_STOPPED_EVENT);
        } else if (dataString.startsWith(P2P_GO_NEG_REQUEST_STR)) {
            sendMessage(iface, P2P_GO_NEGOTIATION_REQUEST_EVENT, new WifiP2pConfig(dataString));
        } else if (dataString.startsWith(P2P_GO_NEG_SUCCESS_STR)) {
            sendMessage(iface, P2P_GO_NEGOTIATION_SUCCESS_EVENT);
        } else if (dataString.startsWith(P2P_GO_NEG_FAILURE_STR)) {
            sendMessage(iface, P2P_GO_NEGOTIATION_FAILURE_EVENT, p2pError(dataString));
        } else if (dataString.startsWith(P2P_GROUP_FORMATION_SUCCESS_STR)) {
            sendMessage(iface, P2P_GROUP_FORMATION_SUCCESS_EVENT);
        } else if (dataString.startsWith(P2P_GROUP_FORMATION_FAILURE_STR)) {
            sendMessage(iface, P2P_GROUP_FORMATION_FAILURE_EVENT, p2pError(dataString));
        } else if (dataString.startsWith(P2P_GROUP_STARTED_STR)) {
            WifiP2pGroup group = getWifiP2pGroup(dataString);
            if (group != null) sendMessage(iface, P2P_GROUP_STARTED_EVENT, group);
        } else if (dataString.startsWith(P2P_GROUP_REMOVED_STR)) {
            WifiP2pGroup group = getWifiP2pGroup(dataString);
            if (group != null) sendMessage(iface, P2P_GROUP_REMOVED_EVENT, group);
        } else if (dataString.startsWith(P2P_INVITATION_RECEIVED_STR)) {
            sendMessage(iface, P2P_INVITATION_RECEIVED_EVENT, new WifiP2pGroup(dataString));
        } else if (dataString.startsWith(P2P_INVITATION_RESULT_STR)) {
            sendMessage(iface, P2P_INVITATION_RESULT_EVENT, p2pError(dataString));
        } else if (dataString.startsWith(P2P_PROV_DISC_PBC_REQ_STR)) {
            sendMessage(iface, P2P_PROV_DISC_PBC_REQ_EVENT, new WifiP2pProvDiscEvent(dataString));
        } else if (dataString.startsWith(P2P_PROV_DISC_PBC_RSP_STR)) {
            sendMessage(iface, P2P_PROV_DISC_PBC_RSP_EVENT, new WifiP2pProvDiscEvent(dataString));
        } else if (dataString.startsWith(P2P_PROV_DISC_ENTER_PIN_STR)) {
            sendMessage(iface, P2P_PROV_DISC_ENTER_PIN_EVENT, new WifiP2pProvDiscEvent(dataString));
        } else if (dataString.startsWith(P2P_PROV_DISC_SHOW_PIN_STR)) {
            sendMessage(iface, P2P_PROV_DISC_SHOW_PIN_EVENT, new WifiP2pProvDiscEvent(dataString));
        } else if (dataString.startsWith(P2P_PROV_DISC_FAILURE_STR)) {
            sendMessage(iface, P2P_PROV_DISC_FAILURE_EVENT);
        } else if (dataString.startsWith(P2P_SERV_DISC_RESP_STR)) {
            List<WifiP2pServiceResponse> list = WifiP2pServiceResponse.newInstance(dataString);
            if (list != null) {
                sendMessage(iface, P2P_SERV_DISC_RESP_EVENT, list);
            } else {
                Log.e(TAG, "Null service resp " + dataString);
            }
        }
    }

    /**
     * Handle hostap events
     */
    private void handleHostApEvents(String dataString, String iface) {
        String[] tokens = dataString.split(" ");
        /* AP-STA-CONNECTED 42:fc:89:a8:96:09 p2p_dev_addr=02:90:4c:a0:92:54 */
        if (tokens[0].equals(AP_STA_CONNECTED_STR)) {
            sendMessage(iface, AP_STA_CONNECTED_EVENT, new WifiP2pDevice(dataString));
            /* AP-STA-DISCONNECTED 42:fc:89:a8:96:09 p2p_dev_addr=02:90:4c:a0:92:54 */
        } else if (tokens[0].equals(AP_STA_DISCONNECTED_STR)) {
            sendMessage(iface, AP_STA_DISCONNECTED_EVENT, new WifiP2pDevice(dataString));
        }
    }

    private static final String ADDR_STRING = "addr=";
    private static final String RESULT_STRING = "result=";

    // ANQP-QUERY-DONE addr=18:cf:5e:26:a4:88 result=SUCCESS

    private void handleAnqpResult(String eventStr, String iface) {
        int addrPos = eventStr.indexOf(ADDR_STRING);
        int resPos = eventStr.indexOf(RESULT_STRING);
        if (addrPos < 0 || resPos < 0) {
            throw new IllegalArgumentException("Unexpected ANQP result notification");
        }
        int eoaddr = eventStr.indexOf(' ', addrPos + ADDR_STRING.length());
        if (eoaddr < 0) {
            eoaddr = eventStr.length();
        }
        int eoresult = eventStr.indexOf(' ', resPos + RESULT_STRING.length());
        if (eoresult < 0) {
            eoresult = eventStr.length();
        }

        try {
            long bssid = Utils.parseMac(eventStr.substring(addrPos + ADDR_STRING.length(), eoaddr));
            int result = eventStr.substring(
                    resPos + RESULT_STRING.length(), eoresult).equalsIgnoreCase("success") ? 1 : 0;

            sendMessage(iface, ANQP_DONE_EVENT, result, 0, bssid);
        }
        catch (IllegalArgumentException iae) {
            Log.e(TAG, "Bad MAC address in ANQP response: " + iae.getMessage());
        }
    }

    private void handleIconResult(String eventStr, String iface) {
        // RX-HS20-ICON c0:c5:20:27:d1:e8 <file> <size>
        String[] segments = eventStr.split(" ");
        if (segments.length != 4) {
            throw new IllegalArgumentException("Incorrect number of segments");
        }

        try {
            String bssid = segments[1];
            String fileName = segments[2];
            int size = Integer.parseInt(segments[3]);
            sendMessage(iface, RX_HS20_ANQP_ICON_EVENT,
                    new IconEvent(Utils.parseMac(bssid), fileName, size));
        }
        catch (NumberFormatException nfe) {
            throw new IllegalArgumentException("Bad numeral");
        }
    }

    private void handleWnmFrame(String eventStr, String iface) {
        try {
            WnmData wnmData = WnmData.buildWnmData(eventStr);
            sendMessage(iface, HS20_REMEDIATION_EVENT, wnmData);
        } catch (IOException | NumberFormatException e) {
            Log.w(TAG, "Bad WNM event: '" + eventStr + "'");
        }
    }

    /**
     * Handle Supplicant Requests
     */
    private void handleRequests(String dataString, String iface) {
        String SSID = null;
        int reason = -2;
        String requestName = dataString.substring(REQUEST_PREFIX_LEN_STR);
        if (TextUtils.isEmpty(requestName)) {
            return;
        }
        if (requestName.startsWith(IDENTITY_STR)) {
            Matcher match = mRequestIdentityPattern.matcher(requestName);
            if (match.find()) {
                SSID = match.group(2);
                try {
                    reason = Integer.parseInt(match.group(1));
                } catch (NumberFormatException e) {
                    reason = -1;
                }
            } else {
                Log.e(TAG, "didn't find SSID " + requestName);
            }
            sendMessage(iface, SUP_REQUEST_IDENTITY, eventLogCounter, reason, SSID);
        } else if (requestName.startsWith(SIM_STR)) {
            Matcher matchGsm = mRequestGsmAuthPattern.matcher(requestName);
            Matcher matchUmts = mRequestUmtsAuthPattern.matcher(requestName);
            WifiStateMachine.SimAuthRequestData data =
                    new WifiStateMachine.SimAuthRequestData();
            if (matchGsm.find()) {
                data.networkId = Integer.parseInt(matchGsm.group(1));
                data.protocol = WifiEnterpriseConfig.Eap.SIM;
                data.ssid = matchGsm.group(4);
                data.data = matchGsm.group(2).split(":");
                sendMessage(iface, SUP_REQUEST_SIM_AUTH, data);
            } else if (matchUmts.find()) {
                data.networkId = Integer.parseInt(matchUmts.group(1));
                data.protocol = WifiEnterpriseConfig.Eap.AKA;
                data.ssid = matchUmts.group(4);
                data.data = new String[2];
                data.data[0] = matchUmts.group(2);
                data.data[1] = matchUmts.group(3);
                sendMessage(iface, SUP_REQUEST_SIM_AUTH, data);
            } else {
                Log.e(TAG, "couldn't parse SIM auth request - " + requestName);
            }

        } else {
            if (DBG) Log.w(TAG, "couldn't identify request type - " + dataString);
        }
    }

    /**
     * Handle the supplicant STATE-CHANGE event
     * @param dataString New supplicant state string in the format:
     * id=network-id state=new-state
     */
    private void handleSupplicantStateChange(String dataString, String iface) {
        WifiSsid wifiSsid = null;
        int index = dataString.lastIndexOf("SSID=");
        if (index != -1) {
            wifiSsid = WifiSsid.createFromAsciiEncoded(
                    dataString.substring(index + 5));
        }
        String[] dataTokens = dataString.split(" ");

        String BSSID = null;
        int networkId = -1;
        int newState  = -1;
        for (String token : dataTokens) {
            String[] nameValue = token.split("=");
            if (nameValue.length != 2) {
                continue;
            }

            if (nameValue[0].equals("BSSID")) {
                BSSID = nameValue[1];
                continue;
            }

            int value;
            try {
                value = Integer.parseInt(nameValue[1]);
            } catch (NumberFormatException e) {
                continue;
            }

            if (nameValue[0].equals("id")) {
                networkId = value;
            } else if (nameValue[0].equals("state")) {
                newState = value;
            }
        }

        if (newState == -1) return;

        SupplicantState newSupplicantState = SupplicantState.INVALID;
        for (SupplicantState state : SupplicantState.values()) {
            if (state.ordinal() == newState) {
                newSupplicantState = state;
                break;
            }
        }
        if (newSupplicantState == SupplicantState.INVALID) {
            Log.w(TAG, "Invalid supplicant state: " + newState);
        }
        sendMessage(iface, SUPPLICANT_STATE_CHANGE_EVENT, eventLogCounter, 0,
                new StateChangeResult(networkId, wifiSsid, BSSID, newSupplicantState));
    }

    private void handleNetworkStateChange(NetworkInfo.DetailedState newState, String data,
            String iface) {
        String BSSID = null;
        int networkId = -1;
        int reason = 0;
        int ind = -1;
        int local = 0;
        Matcher match;
        if (newState == NetworkInfo.DetailedState.CONNECTED) {
            match = mConnectedEventPattern.matcher(data);
            if (!match.find()) {
               if (DBG) Log.d(TAG, "handleNetworkStateChange: Couldnt find BSSID in event string");
            } else {
                BSSID = match.group(1);
                try {
                    networkId = Integer.parseInt(match.group(2));
                } catch (NumberFormatException e) {
                    networkId = -1;
                }
            }
            sendMessage(iface, NETWORK_CONNECTION_EVENT, networkId, reason, BSSID);
        } else if (newState == NetworkInfo.DetailedState.DISCONNECTED) {
            match = mDisconnectedEventPattern.matcher(data);
            if (!match.find()) {
               if (DBG) Log.d(TAG, "handleNetworkStateChange: Could not parse disconnect string");
            } else {
                BSSID = match.group(1);
                try {
                    reason = Integer.parseInt(match.group(2));
                } catch (NumberFormatException e) {
                    reason = -1;
                }
                try {
                    local = Integer.parseInt(match.group(3));
                } catch (NumberFormatException e) {
                    local = -1;
                }
            }
            if (DBG) Log.d(TAG, "WifiMonitor notify network disconnect: "
                    + BSSID
                    + " reason=" + Integer.toString(reason));
            sendMessage(iface, NETWORK_DISCONNECTION_EVENT, local, reason, BSSID);
        }
    }
}
