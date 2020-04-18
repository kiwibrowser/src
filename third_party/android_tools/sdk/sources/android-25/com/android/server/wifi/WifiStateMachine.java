/*
 * Copyright (C) 2010 The Android Open Source Project
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

import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_DISABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLED;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_ENABLING;
import static android.net.wifi.WifiManager.WIFI_AP_STATE_FAILED;
import static android.net.wifi.WifiManager.WIFI_STATE_DISABLED;
import static android.net.wifi.WifiManager.WIFI_STATE_DISABLING;
import static android.net.wifi.WifiManager.WIFI_STATE_ENABLED;
import static android.net.wifi.WifiManager.WIFI_STATE_ENABLING;
import static android.net.wifi.WifiManager.WIFI_STATE_UNKNOWN;

import android.Manifest;
import android.app.ActivityManager;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageManager;
import android.database.ContentObserver;
import android.net.ConnectivityManager;
import android.net.DhcpResults;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkAgent;
import android.net.NetworkCapabilities;
import android.net.NetworkFactory;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.NetworkMisc;
import android.net.NetworkRequest;
import android.net.NetworkUtils;
import android.net.RouteInfo;
import android.net.StaticIpConfiguration;
import android.net.dhcp.DhcpClient;
import android.net.ip.IpManager;
import android.net.wifi.PasspointManagementObjectDefinition;
import android.net.wifi.RssiPacketCountInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.ScanSettings;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiChannel;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConnectionStatistics;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiLinkLayerStats;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.net.wifi.WifiSsid;
import android.net.wifi.WpsInfo;
import android.net.wifi.WpsResult;
import android.net.wifi.WpsResult.Status;
import android.net.wifi.p2p.IWifiP2pManager;
import android.os.BatteryStats;
import android.os.Binder;
import android.os.Bundle;
import android.os.IBinder;
import android.os.INetworkManagementService;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.PowerManager;
import android.os.Process;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.UserHandle;
import android.os.UserManager;
import android.os.WorkSource;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;

import com.android.internal.R;
import com.android.internal.annotations.GuardedBy;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.app.IBatteryStats;
import com.android.internal.util.AsyncChannel;
import com.android.internal.util.MessageUtils;
import com.android.internal.util.Protocol;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.server.connectivity.KeepalivePacketData;
import com.android.server.wifi.hotspot2.IconEvent;
import com.android.server.wifi.hotspot2.NetworkDetail;
import com.android.server.wifi.hotspot2.Utils;
import com.android.server.wifi.p2p.WifiP2pServiceImpl;
import com.android.server.wifi.util.TelephonyUtil;

import java.io.BufferedReader;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.Random;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * TODO:
 * Deprecate WIFI_STATE_UNKNOWN
 */

/**
 * Track the state of Wifi connectivity. All event handling is done here,
 * and all changes in connectivity state are initiated here.
 *
 * Wi-Fi now supports three modes of operation: Client, SoftAp and p2p
 * In the current implementation, we support concurrent wifi p2p and wifi operation.
 * The WifiStateMachine handles SoftAp and Client operations while WifiP2pService
 * handles p2p operation.
 *
 * @hide
 */
public class WifiStateMachine extends StateMachine implements WifiNative.WifiRssiEventHandler {

    private static final String NETWORKTYPE = "WIFI";
    private static final String NETWORKTYPE_UNTRUSTED = "WIFI_UT";
    @VisibleForTesting public static final short NUM_LOG_RECS_NORMAL = 100;
    @VisibleForTesting public static final short NUM_LOG_RECS_VERBOSE_LOW_MEMORY = 200;
    @VisibleForTesting public static final short NUM_LOG_RECS_VERBOSE = 3000;
    private static boolean DBG = false;
    private static boolean USE_PAUSE_SCANS = false;
    private static final String TAG = "WifiStateMachine";

    private static final int ONE_HOUR_MILLI = 1000 * 60 * 60;

    private static final String GOOGLE_OUI = "DA-A1-19";

    private int mVerboseLoggingLevel = 0;
    /* debug flag, indicating if handling of ASSOCIATION_REJECT ended up blacklisting
     * the corresponding BSSID.
     */
    private boolean didBlackListBSSID = false;

    /**
     * Log with error attribute
     *
     * @param s is string log
     */
    protected void loge(String s) {
        Log.e(getName(), s);
    }
    protected void logd(String s) {
        Log.d(getName(), s);
    }
    protected void log(String s) {;
        Log.d(getName(), s);
    }
    private WifiLastResortWatchdog mWifiLastResortWatchdog;
    private WifiMetrics mWifiMetrics;
    private WifiInjector mWifiInjector;
    private WifiMonitor mWifiMonitor;
    private WifiNative mWifiNative;
    private WifiConfigManager mWifiConfigManager;
    private WifiConnectivityManager mWifiConnectivityManager;
    private WifiQualifiedNetworkSelector mWifiQualifiedNetworkSelector;
    private INetworkManagementService mNwService;
    private ConnectivityManager mCm;
    private BaseWifiLogger mWifiLogger;
    private WifiApConfigStore mWifiApConfigStore;
    private final boolean mP2pSupported;
    private final AtomicBoolean mP2pConnected = new AtomicBoolean(false);
    private boolean mTemporarilyDisconnectWifi = false;
    private final String mPrimaryDeviceType;
    private final Clock mClock;
    private final PropertyService mPropertyService;
    private final BuildProperties mBuildProperties;
    private final WifiCountryCode mCountryCode;

    /* Scan results handling */
    private List<ScanDetail> mScanResults = new ArrayList<>();
    private final Object mScanResultsLock = new Object();

    // For debug, number of known scan results that were found as part of last scan result event,
    // as well the number of scans results returned by the supplicant with that message
    private int mNumScanResultsKnown;
    private int mNumScanResultsReturned;

    private boolean mScreenOn = false;

    /* Chipset supports background scan */
    private final boolean mBackgroundScanSupported;

    private final String mInterfaceName;
    /* Tethering interface could be separate from wlan interface */
    private String mTetherInterfaceName;

    private int mLastSignalLevel = -1;
    private String mLastBssid;
    private int mLastNetworkId; // The network Id we successfully joined
    private boolean linkDebouncing = false;

    @Override
    public void onRssiThresholdBreached(byte curRssi) {
        if (DBG) {
            Log.e(TAG, "onRssiThresholdBreach event. Cur Rssi = " + curRssi);
        }
        sendMessage(CMD_RSSI_THRESHOLD_BREACH, curRssi);
    }

    public void processRssiThreshold(byte curRssi, int reason) {
        if (curRssi == Byte.MAX_VALUE || curRssi == Byte.MIN_VALUE) {
            Log.wtf(TAG, "processRssiThreshold: Invalid rssi " + curRssi);
            return;
        }
        for (int i = 0; i < mRssiRanges.length; i++) {
            if (curRssi < mRssiRanges[i]) {
                // Assume sorted values(ascending order) for rssi,
                // bounded by high(127) and low(-128) at extremeties
                byte maxRssi = mRssiRanges[i];
                byte minRssi = mRssiRanges[i-1];
                // This value of hw has to be believed as this value is averaged and has breached
                // the rssi thresholds and raised event to host. This would be eggregious if this
                // value is invalid
                mWifiInfo.setRssi((int) curRssi);
                updateCapabilities(getCurrentWifiConfiguration());
                int ret = startRssiMonitoringOffload(maxRssi, minRssi);
                Log.d(TAG, "Re-program RSSI thresholds for " + smToString(reason) +
                        ": [" + minRssi + ", " + maxRssi + "], curRssi=" + curRssi + " ret=" + ret);
                break;
            }
        }
    }

    // Testing various network disconnect cases by sending lots of spurious
    // disconnect to supplicant
    private boolean testNetworkDisconnect = false;

    private boolean mEnableRssiPolling = false;
    private int mRssiPollToken = 0;
    /* 3 operational states for STA operation: CONNECT_MODE, SCAN_ONLY_MODE, SCAN_ONLY_WIFI_OFF_MODE
    * In CONNECT_MODE, the STA can scan and connect to an access point
    * In SCAN_ONLY_MODE, the STA can only scan for access points
    * In SCAN_ONLY_WIFI_OFF_MODE, the STA can only scan for access points with wifi toggle being off
    */
    private int mOperationalMode = CONNECT_MODE;
    private boolean mIsScanOngoing = false;
    private boolean mIsFullScanOngoing = false;
    private boolean mSendScanResultsBroadcast = false;

    private final Queue<Message> mBufferedScanMsg = new LinkedList<Message>();
    private WorkSource mScanWorkSource = null;
    private static final int UNKNOWN_SCAN_SOURCE = -1;
    private static final int ADD_OR_UPDATE_SOURCE = -3;
    private static final int SET_ALLOW_UNTRUSTED_SOURCE = -4;
    private static final int ENABLE_WIFI = -5;
    public static final int DFS_RESTRICTED_SCAN_REQUEST = -6;

    private static final int SCAN_REQUEST_BUFFER_MAX_SIZE = 10;
    private static final String CUSTOMIZED_SCAN_SETTING = "customized_scan_settings";
    private static final String CUSTOMIZED_SCAN_WORKSOURCE = "customized_scan_worksource";
    private static final String SCAN_REQUEST_TIME = "scan_request_time";

    /* Tracks if state machine has received any screen state change broadcast yet.
     * We can miss one of these at boot.
     */
    private AtomicBoolean mScreenBroadcastReceived = new AtomicBoolean(false);

    private boolean mBluetoothConnectionActive = false;

    private PowerManager.WakeLock mSuspendWakeLock;

    /**
     * Interval in milliseconds between polling for RSSI
     * and linkspeed information
     */
    private static final int POLL_RSSI_INTERVAL_MSECS = 3000;

    /**
     * Interval in milliseconds between receiving a disconnect event
     * while connected to a good AP, and handling the disconnect proper
     */
    private static final int LINK_FLAPPING_DEBOUNCE_MSEC = 4000;

    /**
     * Delay between supplicant restarts upon failure to establish connection
     */
    private static final int SUPPLICANT_RESTART_INTERVAL_MSECS = 5000;

    /**
     * Number of times we attempt to restart supplicant
     */
    private static final int SUPPLICANT_RESTART_TRIES = 5;

    private int mSupplicantRestartCount = 0;
    /* Tracks sequence number on stop failure message */
    private int mSupplicantStopFailureToken = 0;

    /**
     * Tether state change notification time out
     */
    private static final int TETHER_NOTIFICATION_TIME_OUT_MSECS = 5000;

    /* Tracks sequence number on a tether notification time out */
    private int mTetherToken = 0;

    /**
     * Driver start time out.
     */
    private static final int DRIVER_START_TIME_OUT_MSECS = 10000;

    /* Tracks sequence number on a driver time out */
    private int mDriverStartToken = 0;

    /**
     * Don't select new network when previous network selection is
     * pending connection for this much time
     */
    private static final int CONNECT_TIMEOUT_MSEC = 3000;

    /**
     * The link properties of the wifi interface.
     * Do not modify this directly; use updateLinkProperties instead.
     */
    private LinkProperties mLinkProperties;

    /* Tracks sequence number on a periodic scan message */
    private int mPeriodicScanToken = 0;

    // Wakelock held during wifi start/stop and driver load/unload
    private PowerManager.WakeLock mWakeLock;

    private Context mContext;

    private final Object mDhcpResultsLock = new Object();
    private DhcpResults mDhcpResults;

    // NOTE: Do not return to clients - use #getWiFiInfoForUid(int)
    private final WifiInfo mWifiInfo;
    private NetworkInfo mNetworkInfo;
    private final NetworkCapabilities mDfltNetworkCapabilities;
    private SupplicantStateTracker mSupplicantStateTracker;

    private int mWifiLinkLayerStatsSupported = 4; // Temporary disable

    // Whether the state machine goes thru the Disconnecting->Disconnected->ObtainingIpAddress
    private boolean mAutoRoaming = false;

    // Roaming failure count
    private int mRoamFailCount = 0;

    // This is the BSSID we are trying to associate to, it can be set to "any"
    // if we havent selected a BSSID for joining.
    // if we havent selected a BSSID for joining.
    // The BSSID we are associated to is found in mWifiInfo
    private String mTargetRoamBSSID = "any";
    //This one is used to track whta is the current target network ID. This is used for error
    // handling during connection setup since many error message from supplicant does not report
    // SSID Once connected, it will be set to invalid
    private int mTargetNetworkId = WifiConfiguration.INVALID_NETWORK_ID;

    private long mLastDriverRoamAttempt = 0;

    private WifiConfiguration targetWificonfiguration = null;

    // Used as debug to indicate which configuration last was saved
    private WifiConfiguration lastSavedConfigurationAttempt = null;

    // Used as debug to indicate which configuration last was removed
    private WifiConfiguration lastForgetConfigurationAttempt = null;

    //Random used by softAP channel Selection
    private static Random mRandom = new Random(Calendar.getInstance().getTimeInMillis());

    boolean isRoaming() {
        return mAutoRoaming;
    }

    public void autoRoamSetBSSID(int netId, String bssid) {
        autoRoamSetBSSID(mWifiConfigManager.getWifiConfiguration(netId), bssid);
    }

    public boolean autoRoamSetBSSID(WifiConfiguration config, String bssid) {
        boolean ret = true;
        if (mTargetRoamBSSID == null) mTargetRoamBSSID = "any";
        if (bssid == null) bssid = "any";
        if (config == null) return false; // Nothing to do

        if (mTargetRoamBSSID != null
                && bssid.equals(mTargetRoamBSSID) && bssid.equals(config.BSSID)) {
            return false; // We didnt change anything
        }
        if (!mTargetRoamBSSID.equals("any") && bssid.equals("any")) {
            // Changing to ANY
            if (!mWifiConfigManager.ROAM_ON_ANY) {
                ret = false; // Nothing to do
            }
        }
        if (config.BSSID != null) {
            bssid = config.BSSID;
            if (DBG) {
                Log.d(TAG, "force BSSID to " + bssid + "due to config");
            }
        }

        if (DBG) {
            logd("autoRoamSetBSSID " + bssid + " key=" + config.configKey());
        }
        mTargetRoamBSSID = bssid;
        mWifiConfigManager.saveWifiConfigBSSID(config, bssid);
        return ret;
    }

    /**
     * set Config's default BSSID (for association purpose)
     * @param config config need set BSSID
     * @param bssid  default BSSID to assocaite with when connect to this network
     * @return false -- does not change the current default BSSID of the configure
     *         true -- change the  current default BSSID of the configur
     */
    private boolean setTargetBssid(WifiConfiguration config, String bssid) {
        if (config == null) {
            return false;
        }

        if (config.BSSID != null) {
            bssid = config.BSSID;
            if (DBG) {
                Log.d(TAG, "force BSSID to " + bssid + "due to config");
            }
        }

        if (bssid == null) {
            bssid = "any";
        }

        String networkSelectionBSSID = config.getNetworkSelectionStatus()
                .getNetworkSelectionBSSID();
        if (networkSelectionBSSID != null && networkSelectionBSSID.equals(bssid)) {
            if (DBG) {
                Log.d(TAG, "Current preferred BSSID is the same as the target one");
            }
            return false;
        }

        if (DBG) {
            Log.d(TAG, "target set to " + config.SSID + ":" + bssid);
        }
        mTargetRoamBSSID = bssid;
        mWifiConfigManager.saveWifiConfigBSSID(config, bssid);
        return true;
    }
    /**
     * Save the UID correctly depending on if this is a new or existing network.
     * @return true if operation is authorized, false otherwise
     */
    boolean recordUidIfAuthorized(WifiConfiguration config, int uid, boolean onlyAnnotate) {
        if (!mWifiConfigManager.isNetworkConfigured(config)) {
            config.creatorUid = uid;
            config.creatorName = mContext.getPackageManager().getNameForUid(uid);
        } else if (!mWifiConfigManager.canModifyNetwork(uid, config, onlyAnnotate)) {
            return false;
        }

        config.lastUpdateUid = uid;
        config.lastUpdateName = mContext.getPackageManager().getNameForUid(uid);

        return true;

    }

    /**
     * Checks to see if user has specified if the apps configuration is connectable.
     * If the user hasn't specified we query the user and return true.
     *
     * @param message The message to be deferred
     * @param netId Network id of the configuration to check against
     * @param allowOverride If true we won't defer to the user if the uid of the message holds the
     *                      CONFIG_OVERRIDE_PERMISSION
     * @return True if we are waiting for user feedback or netId is invalid. False otherwise.
     */
    boolean deferForUserInput(Message message, int netId, boolean allowOverride){
        final WifiConfiguration config = mWifiConfigManager.getWifiConfiguration(netId);

        // We can only evaluate saved configurations.
        if (config == null) {
            logd("deferForUserInput: configuration for netId=" + netId + " not stored");
            return true;
        }

        switch (config.userApproved) {
            case WifiConfiguration.USER_APPROVED:
            case WifiConfiguration.USER_BANNED:
                return false;
            case WifiConfiguration.USER_PENDING:
            default: // USER_UNSPECIFIED
               /* the intention was to ask user here; but a dialog box is   *
                * too invasive; so we are going to allow connection for now */
                config.userApproved = WifiConfiguration.USER_APPROVED;
                return false;
        }
    }

    private final IpManager mIpManager;

    private AlarmManager mAlarmManager;
    private PendingIntent mScanIntent;

    /* Tracks current frequency mode */
    private AtomicInteger mFrequencyBand = new AtomicInteger(WifiManager.WIFI_FREQUENCY_BAND_AUTO);

    // Channel for sending replies.
    private AsyncChannel mReplyChannel = new AsyncChannel();

    private WifiP2pServiceImpl mWifiP2pServiceImpl;

    // Used to initiate a connection with WifiP2pService
    private AsyncChannel mWifiP2pChannel;

    private WifiScanner mWifiScanner;

    @GuardedBy("mWifiReqCountLock")
    private int mConnectionReqCount = 0;
    private WifiNetworkFactory mNetworkFactory;
    @GuardedBy("mWifiReqCountLock")
    private int mUntrustedReqCount = 0;
    private UntrustedWifiNetworkFactory mUntrustedNetworkFactory;
    private WifiNetworkAgent mNetworkAgent;
    private final Object mWifiReqCountLock = new Object();

    private String[] mWhiteListedSsids = null;

    private byte[] mRssiRanges;

    // Keep track of various statistics, for retrieval by System Apps, i.e. under @SystemApi
    // We should really persist that into the networkHistory.txt file, and read it back when
    // WifiStateMachine starts up
    private WifiConnectionStatistics mWifiConnectionStatistics = new WifiConnectionStatistics();

    // Used to filter out requests we couldn't possibly satisfy.
    private final NetworkCapabilities mNetworkCapabilitiesFilter = new NetworkCapabilities();

    // Provide packet filter capabilities to ConnectivityService.
    private final NetworkMisc mNetworkMisc = new NetworkMisc();

    /* The base for wifi message types */
    static final int BASE = Protocol.BASE_WIFI;
    /* Start the supplicant */
    static final int CMD_START_SUPPLICANT                               = BASE + 11;
    /* Stop the supplicant */
    static final int CMD_STOP_SUPPLICANT                                = BASE + 12;
    /* Start the driver */
    static final int CMD_START_DRIVER                                   = BASE + 13;
    /* Stop the driver */
    static final int CMD_STOP_DRIVER                                    = BASE + 14;
    /* Indicates Static IP succeeded */
    static final int CMD_STATIC_IP_SUCCESS                              = BASE + 15;
    /* Indicates Static IP failed */
    static final int CMD_STATIC_IP_FAILURE                              = BASE + 16;
    /* Indicates supplicant stop failed */
    static final int CMD_STOP_SUPPLICANT_FAILED                         = BASE + 17;
    /* A delayed message sent to start driver when it fail to come up */
    static final int CMD_DRIVER_START_TIMED_OUT                         = BASE + 19;

    /* Start the soft access point */
    static final int CMD_START_AP                                       = BASE + 21;
    /* Indicates soft ap start failed */
    static final int CMD_START_AP_FAILURE                               = BASE + 22;
    /* Stop the soft access point */
    static final int CMD_STOP_AP                                        = BASE + 23;
    /* Soft access point teardown is completed. */
    static final int CMD_AP_STOPPED                                     = BASE + 24;

    static final int CMD_BLUETOOTH_ADAPTER_STATE_CHANGE                 = BASE + 31;

    /* Supplicant commands */
    /* Is supplicant alive ? */
    static final int CMD_PING_SUPPLICANT                                = BASE + 51;
    /* Add/update a network configuration */
    static final int CMD_ADD_OR_UPDATE_NETWORK                          = BASE + 52;
    /* Delete a network */
    static final int CMD_REMOVE_NETWORK                                 = BASE + 53;
    /* Enable a network. The device will attempt a connection to the given network. */
    static final int CMD_ENABLE_NETWORK                                 = BASE + 54;
    /* Enable all networks */
    static final int CMD_ENABLE_ALL_NETWORKS                            = BASE + 55;
    /* Blacklist network. De-prioritizes the given BSSID for connection. */
    static final int CMD_BLACKLIST_NETWORK                              = BASE + 56;
    /* Clear the blacklist network list */
    static final int CMD_CLEAR_BLACKLIST                                = BASE + 57;
    /* Save configuration */
    static final int CMD_SAVE_CONFIG                                    = BASE + 58;
    /* Get configured networks */
    static final int CMD_GET_CONFIGURED_NETWORKS                        = BASE + 59;
    /* Get available frequencies */
    static final int CMD_GET_CAPABILITY_FREQ                            = BASE + 60;
    /* Get adaptors */
    static final int CMD_GET_SUPPORTED_FEATURES                         = BASE + 61;
    /* Get configured networks with real preSharedKey */
    static final int CMD_GET_PRIVILEGED_CONFIGURED_NETWORKS             = BASE + 62;
    /* Get Link Layer Stats thru HAL */
    static final int CMD_GET_LINK_LAYER_STATS                           = BASE + 63;
    /* Supplicant commands after driver start*/
    /* Initiate a scan */
    static final int CMD_START_SCAN                                     = BASE + 71;
    /* Set operational mode. CONNECT, SCAN ONLY, SCAN_ONLY with Wi-Fi off mode */
    static final int CMD_SET_OPERATIONAL_MODE                           = BASE + 72;
    /* Disconnect from a network */
    static final int CMD_DISCONNECT                                     = BASE + 73;
    /* Reconnect to a network */
    static final int CMD_RECONNECT                                      = BASE + 74;
    /* Reassociate to a network */
    static final int CMD_REASSOCIATE                                    = BASE + 75;
    /* Get Connection Statistis */
    static final int CMD_GET_CONNECTION_STATISTICS                      = BASE + 76;

    /* Controls suspend mode optimizations
     *
     * When high perf mode is enabled, suspend mode optimizations are disabled
     *
     * When high perf mode is disabled, suspend mode optimizations are enabled
     *
     * Suspend mode optimizations include:
     * - packet filtering
     * - turn off roaming
     * - DTIM wake up settings
     */
    static final int CMD_SET_HIGH_PERF_MODE                             = BASE + 77;
    /* Enables RSSI poll */
    static final int CMD_ENABLE_RSSI_POLL                               = BASE + 82;
    /* RSSI poll */
    static final int CMD_RSSI_POLL                                      = BASE + 83;
    /* Enable suspend mode optimizations in the driver */
    static final int CMD_SET_SUSPEND_OPT_ENABLED                        = BASE + 86;
    /* Delayed NETWORK_DISCONNECT */
    static final int CMD_DELAYED_NETWORK_DISCONNECT                     = BASE + 87;
    /* When there are no saved networks, we do a periodic scan to notify user of
     * an open network */
    static final int CMD_NO_NETWORKS_PERIODIC_SCAN                      = BASE + 88;
    /* Test network Disconnection NETWORK_DISCONNECT */
    static final int CMD_TEST_NETWORK_DISCONNECT                        = BASE + 89;

    private int testNetworkDisconnectCounter = 0;

    /* Set the frequency band */
    static final int CMD_SET_FREQUENCY_BAND                             = BASE + 90;
    /* Enable TDLS on a specific MAC address */
    static final int CMD_ENABLE_TDLS                                    = BASE + 92;
    /* DHCP/IP configuration watchdog */
    static final int CMD_OBTAINING_IP_ADDRESS_WATCHDOG_TIMER            = BASE + 93;

    /**
     * Watchdog for protecting against b/16823537
     * Leave time for 4-way handshake to succeed
     */
    static final int ROAM_GUARD_TIMER_MSEC = 15000;

    int roamWatchdogCount = 0;
    /* Roam state watchdog */
    static final int CMD_ROAM_WATCHDOG_TIMER                            = BASE + 94;
    /* Screen change intent handling */
    static final int CMD_SCREEN_STATE_CHANGED                           = BASE + 95;

    /* Disconnecting state watchdog */
    static final int CMD_DISCONNECTING_WATCHDOG_TIMER                   = BASE + 96;

    /* Remove a packages associated configrations */
    static final int CMD_REMOVE_APP_CONFIGURATIONS                      = BASE + 97;

    /* Disable an ephemeral network */
    static final int CMD_DISABLE_EPHEMERAL_NETWORK                      = BASE + 98;

    /* Get matching network */
    static final int CMD_GET_MATCHING_CONFIG                            = BASE + 99;

    /* alert from firmware */
    static final int CMD_FIRMWARE_ALERT                                 = BASE + 100;

    /* SIM is removed; reset any cached data for it */
    static final int CMD_RESET_SIM_NETWORKS                             = BASE + 101;

    /* OSU APIs */
    static final int CMD_ADD_PASSPOINT_MO                               = BASE + 102;
    static final int CMD_MODIFY_PASSPOINT_MO                            = BASE + 103;
    static final int CMD_QUERY_OSU_ICON                                 = BASE + 104;

    /* try to match a provider with current network */
    static final int CMD_MATCH_PROVIDER_NETWORK                         = BASE + 105;

    /**
     * Make this timer 40 seconds, which is about the normal DHCP timeout.
     * In no valid case, the WiFiStateMachine should remain stuck in ObtainingIpAddress
     * for more than 30 seconds.
     */
    static final int OBTAINING_IP_ADDRESS_GUARD_TIMER_MSEC = 40000;

    int obtainingIpWatchdogCount = 0;

    /* Commands from/to the SupplicantStateTracker */
    /* Reset the supplicant state tracker */
    static final int CMD_RESET_SUPPLICANT_STATE                         = BASE + 111;

    int disconnectingWatchdogCount = 0;
    static final int DISCONNECTING_GUARD_TIMER_MSEC = 5000;

    /* P2p commands */
    /* We are ok with no response here since we wont do much with it anyway */
    public static final int CMD_ENABLE_P2P                              = BASE + 131;
    /* In order to shut down supplicant cleanly, we wait till p2p has
     * been disabled */
    public static final int CMD_DISABLE_P2P_REQ                         = BASE + 132;
    public static final int CMD_DISABLE_P2P_RSP                         = BASE + 133;

    public static final int CMD_BOOT_COMPLETED                          = BASE + 134;

    /* We now have a valid IP configuration. */
    static final int CMD_IP_CONFIGURATION_SUCCESSFUL                    = BASE + 138;
    /* We no longer have a valid IP configuration. */
    static final int CMD_IP_CONFIGURATION_LOST                          = BASE + 139;
    /* Link configuration (IP address, DNS, ...) changes notified via netlink */
    static final int CMD_UPDATE_LINKPROPERTIES                          = BASE + 140;

    /* Supplicant is trying to associate to a given BSSID */
    static final int CMD_TARGET_BSSID                                   = BASE + 141;

    /* Reload all networks and reconnect */
    static final int CMD_RELOAD_TLS_AND_RECONNECT                       = BASE + 142;

    static final int CMD_AUTO_CONNECT                                   = BASE + 143;

    private static final int NETWORK_STATUS_UNWANTED_DISCONNECT         = 0;
    private static final int NETWORK_STATUS_UNWANTED_VALIDATION_FAILED  = 1;
    private static final int NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN   = 2;

    static final int CMD_UNWANTED_NETWORK                               = BASE + 144;

    static final int CMD_AUTO_ROAM                                      = BASE + 145;

    static final int CMD_AUTO_SAVE_NETWORK                              = BASE + 146;

    static final int CMD_ASSOCIATED_BSSID                               = BASE + 147;

    static final int CMD_NETWORK_STATUS                                 = BASE + 148;

    /* A layer 3 neighbor on the Wi-Fi link became unreachable. */
    static final int CMD_IP_REACHABILITY_LOST                           = BASE + 149;

    /* Remove a packages associated configrations */
    static final int CMD_REMOVE_USER_CONFIGURATIONS                     = BASE + 152;

    static final int CMD_ACCEPT_UNVALIDATED                             = BASE + 153;

    /* used to log if PNO was started */
    static final int CMD_UPDATE_ASSOCIATED_SCAN_PERMISSION              = BASE + 158;

    /* used to offload sending IP packet */
    static final int CMD_START_IP_PACKET_OFFLOAD                        = BASE + 160;

    /* used to stop offload sending IP packet */
    static final int CMD_STOP_IP_PACKET_OFFLOAD                         = BASE + 161;

    /* used to start rssi monitoring in hw */
    static final int CMD_START_RSSI_MONITORING_OFFLOAD                  = BASE + 162;

    /* used to stop rssi moniroting in hw */
    static final int CMD_STOP_RSSI_MONITORING_OFFLOAD                   = BASE + 163;

    /* used to indicated RSSI threshold breach in hw */
    static final int CMD_RSSI_THRESHOLD_BREACH                          = BASE + 164;

    /* used to indicate that the foreground user was switched */
    static final int CMD_USER_SWITCH                                    = BASE + 165;

    /* Enable/Disable WifiConnectivityManager */
    static final int CMD_ENABLE_WIFI_CONNECTIVITY_MANAGER               = BASE + 166;

    /* Enable/Disable AutoJoin when associated */
    static final int CMD_ENABLE_AUTOJOIN_WHEN_ASSOCIATED                = BASE + 167;

    /**
     * Used to handle messages bounced between WifiStateMachine and IpManager.
     */
    static final int CMD_IPV4_PROVISIONING_SUCCESS                      = BASE + 200;
    static final int CMD_IPV4_PROVISIONING_FAILURE                      = BASE + 201;

    /* Push a new APF program to the HAL */
    static final int CMD_INSTALL_PACKET_FILTER                          = BASE + 202;

    /* Enable/disable fallback packet filtering */
    static final int CMD_SET_FALLBACK_PACKET_FILTERING                  = BASE + 203;

    /* Enable/disable Neighbor Discovery offload functionality. */
    static final int CMD_CONFIG_ND_OFFLOAD                              = BASE + 204;

    // For message logging.
    private static final Class[] sMessageClasses = {
            AsyncChannel.class, WifiStateMachine.class, DhcpClient.class };
    private static final SparseArray<String> sSmToString =
            MessageUtils.findMessageNames(sMessageClasses);


    /* Wifi state machine modes of operation */
    /* CONNECT_MODE - connect to any 'known' AP when it becomes available */
    public static final int CONNECT_MODE = 1;
    /* SCAN_ONLY_MODE - don't connect to any APs; scan, but only while apps hold lock */
    public static final int SCAN_ONLY_MODE = 2;
    /* SCAN_ONLY_WITH_WIFI_OFF - scan, but don't connect to any APs */
    public static final int SCAN_ONLY_WITH_WIFI_OFF_MODE = 3;

    private static final int SUCCESS = 1;
    private static final int FAILURE = -1;

    /* Tracks if suspend optimizations need to be disabled by DHCP,
     * screen or due to high perf mode.
     * When any of them needs to disable it, we keep the suspend optimizations
     * disabled
     */
    private int mSuspendOptNeedsDisabled = 0;

    private static final int SUSPEND_DUE_TO_DHCP = 1;
    private static final int SUSPEND_DUE_TO_HIGH_PERF = 1 << 1;
    private static final int SUSPEND_DUE_TO_SCREEN = 1 << 2;

    /* Tracks if user has enabled suspend optimizations through settings */
    private AtomicBoolean mUserWantsSuspendOpt = new AtomicBoolean(true);

    /**
     * Default framework scan interval in milliseconds. This is used in the scenario in which
     * wifi chipset does not support background scanning to set up a
     * periodic wake up scan so that the device can connect to a new access
     * point on the move. {@link Settings.Global#WIFI_FRAMEWORK_SCAN_INTERVAL_MS} can
     * override this.
     */
    private final int mDefaultFrameworkScanIntervalMs;


    /**
     * Scan period for the NO_NETWORKS_PERIIDOC_SCAN_FEATURE
     */
    private final int mNoNetworksPeriodicScan;

    /**
     * Supplicant scan interval in milliseconds.
     * Comes from {@link Settings.Global#WIFI_SUPPLICANT_SCAN_INTERVAL_MS} or
     * from the default config if the setting is not set
     */
    private long mSupplicantScanIntervalMs;

    /**
     * Minimum time interval between enabling all networks.
     * A device can end up repeatedly connecting to a bad network on screen on/off toggle
     * due to enabling every time. We add a threshold to avoid this.
     */
    private static final int MIN_INTERVAL_ENABLE_ALL_NETWORKS_MS = 10 * 60 * 1000; /* 10 minutes */
    private long mLastEnableAllNetworksTime;

    int mRunningBeaconCount = 0;

    /* Default parent state */
    private State mDefaultState = new DefaultState();
    /* Temporary initial state */
    private State mInitialState = new InitialState();
    /* Driver loaded, waiting for supplicant to start */
    private State mSupplicantStartingState = new SupplicantStartingState();
    /* Driver loaded and supplicant ready */
    private State mSupplicantStartedState = new SupplicantStartedState();
    /* Waiting for supplicant to stop and monitor to exit */
    private State mSupplicantStoppingState = new SupplicantStoppingState();
    /* Driver start issued, waiting for completed event */
    private State mDriverStartingState = new DriverStartingState();
    /* Driver started */
    private State mDriverStartedState = new DriverStartedState();
    /* Wait until p2p is disabled
     * This is a special state which is entered right after we exit out of DriverStartedState
     * before transitioning to another state.
     */
    private State mWaitForP2pDisableState = new WaitForP2pDisableState();
    /* Driver stopping */
    private State mDriverStoppingState = new DriverStoppingState();
    /* Driver stopped */
    private State mDriverStoppedState = new DriverStoppedState();
    /* Scan for networks, no connection will be established */
    private State mScanModeState = new ScanModeState();
    /* Connecting to an access point */
    private State mConnectModeState = new ConnectModeState();
    /* Connected at 802.11 (L2) level */
    private State mL2ConnectedState = new L2ConnectedState();
    /* fetching IP after connection to access point (assoc+auth complete) */
    private State mObtainingIpState = new ObtainingIpState();
    /* Connected with IP addr */
    private State mConnectedState = new ConnectedState();
    /* Roaming */
    private State mRoamingState = new RoamingState();
    /* disconnect issued, waiting for network disconnect confirmation */
    private State mDisconnectingState = new DisconnectingState();
    /* Network is not connected, supplicant assoc+auth is not complete */
    private State mDisconnectedState = new DisconnectedState();
    /* Waiting for WPS to be completed*/
    private State mWpsRunningState = new WpsRunningState();
    /* Soft ap state */
    private State mSoftApState = new SoftApState();

    public static class SimAuthRequestData {
        int networkId;
        int protocol;
        String ssid;
        // EAP-SIM: data[] contains the 3 rand, one for each of the 3 challenges
        // EAP-AKA/AKA': data[] contains rand & authn couple for the single challenge
        String[] data;
    }

    /**
     * One of  {@link WifiManager#WIFI_STATE_DISABLED},
     * {@link WifiManager#WIFI_STATE_DISABLING},
     * {@link WifiManager#WIFI_STATE_ENABLED},
     * {@link WifiManager#WIFI_STATE_ENABLING},
     * {@link WifiManager#WIFI_STATE_UNKNOWN}
     */
    private final AtomicInteger mWifiState = new AtomicInteger(WIFI_STATE_DISABLED);

    /**
     * One of  {@link WifiManager#WIFI_AP_STATE_DISABLED},
     * {@link WifiManager#WIFI_AP_STATE_DISABLING},
     * {@link WifiManager#WIFI_AP_STATE_ENABLED},
     * {@link WifiManager#WIFI_AP_STATE_ENABLING},
     * {@link WifiManager#WIFI_AP_STATE_FAILED}
     */
    private final AtomicInteger mWifiApState = new AtomicInteger(WIFI_AP_STATE_DISABLED);

    private static final int SCAN_REQUEST = 0;

    /**
     * Work source to use to blame usage on the WiFi service
     */
    public static final WorkSource WIFI_WORK_SOURCE = new WorkSource(Process.WIFI_UID);

    /**
     * Keep track of whether WIFI is running.
     */
    private boolean mIsRunning = false;

    /**
     * Keep track of whether we last told the battery stats we had started.
     */
    private boolean mReportedRunning = false;

    /**
     * Most recently set source of starting WIFI.
     */
    private final WorkSource mRunningWifiUids = new WorkSource();

    /**
     * The last reported UIDs that were responsible for starting WIFI.
     */
    private final WorkSource mLastRunningWifiUids = new WorkSource();

    private final IBatteryStats mBatteryStats;

    private final String mTcpBufferSizes;

    // Used for debug and stats gathering
    private static int sScanAlarmIntentCount = 0;

    private static final int sFrameworkMinScanIntervalSaneValue = 10000;

    private long mGScanStartTimeMilli;
    private long mGScanPeriodMilli;

    private FrameworkFacade mFacade;

    private final BackupManagerProxy mBackupManagerProxy;

    private int mSystemUiUid = -1;

    public WifiStateMachine(Context context, FrameworkFacade facade, Looper looper,
                            UserManager userManager, WifiInjector wifiInjector,
                            BackupManagerProxy backupManagerProxy,
                            WifiCountryCode countryCode) {
        super("WifiStateMachine", looper);
        mWifiInjector = wifiInjector;
        mWifiMetrics = mWifiInjector.getWifiMetrics();
        mWifiLastResortWatchdog = wifiInjector.getWifiLastResortWatchdog();
        mClock = wifiInjector.getClock();
        mPropertyService = wifiInjector.getPropertyService();
        mBuildProperties = wifiInjector.getBuildProperties();
        mContext = context;
        mFacade = facade;
        mWifiNative = WifiNative.getWlanNativeInterface();
        mBackupManagerProxy = backupManagerProxy;

        // TODO refactor WifiNative use of context out into it's own class
        mWifiNative.initContext(mContext);
        mInterfaceName = mWifiNative.getInterfaceName();
        mNetworkInfo = new NetworkInfo(ConnectivityManager.TYPE_WIFI, 0, NETWORKTYPE, "");
        mBatteryStats = IBatteryStats.Stub.asInterface(mFacade.getService(
                BatteryStats.SERVICE_NAME));

        IBinder b = mFacade.getService(Context.NETWORKMANAGEMENT_SERVICE);
        mNwService = INetworkManagementService.Stub.asInterface(b);

        mP2pSupported = mContext.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_WIFI_DIRECT);

        mWifiConfigManager = mFacade.makeWifiConfigManager(context, mWifiNative, facade,
                mWifiInjector.getClock(), userManager, mWifiInjector.getKeyStore());

        mWifiMonitor = WifiMonitor.getInstance();

        boolean enableFirmwareLogs = mContext.getResources().getBoolean(
                R.bool.config_wifi_enable_wifi_firmware_debugging);

        if (enableFirmwareLogs) {
            mWifiLogger = facade.makeRealLogger(mContext, this, mWifiNative, mBuildProperties);
        } else {
            mWifiLogger = facade.makeBaseLogger();
        }

        mWifiInfo = new WifiInfo();
        mWifiQualifiedNetworkSelector = new WifiQualifiedNetworkSelector(mWifiConfigManager,
                mContext, mWifiInfo, mWifiInjector.getClock());
        mSupplicantStateTracker = mFacade.makeSupplicantStateTracker(
                context, mWifiConfigManager, getHandler());

        mLinkProperties = new LinkProperties();

        IBinder s1 = mFacade.getService(Context.WIFI_P2P_SERVICE);
        mWifiP2pServiceImpl = (WifiP2pServiceImpl) IWifiP2pManager.Stub.asInterface(s1);

        mNetworkInfo.setIsAvailable(false);
        mLastBssid = null;
        mLastNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
        mLastSignalLevel = -1;

        mIpManager = mFacade.makeIpManager(mContext, mInterfaceName, new IpManagerCallback());
        mIpManager.setMulticastFilter(true);

        mAlarmManager = (AlarmManager) mContext.getSystemService(Context.ALARM_SERVICE);

        // Make sure the interval is not configured less than 10 seconds
        int period = mContext.getResources().getInteger(
                R.integer.config_wifi_framework_scan_interval);
        if (period < sFrameworkMinScanIntervalSaneValue) {
            period = sFrameworkMinScanIntervalSaneValue;
        }
        mDefaultFrameworkScanIntervalMs = period;

        mNoNetworksPeriodicScan = mContext.getResources().getInteger(
                R.integer.config_wifi_no_network_periodic_scan_interval);

        mBackgroundScanSupported = mContext.getResources().getBoolean(
                R.bool.config_wifi_background_scan_support);

        mPrimaryDeviceType = mContext.getResources().getString(
                R.string.config_wifi_p2p_device_type);

        mCountryCode = countryCode;

        mUserWantsSuspendOpt.set(mFacade.getIntegerSetting(mContext,
                Settings.Global.WIFI_SUSPEND_OPTIMIZATIONS_ENABLED, 1) == 1);

        mNetworkCapabilitiesFilter.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        mNetworkCapabilitiesFilter.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
        mNetworkCapabilitiesFilter.addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED);
        mNetworkCapabilitiesFilter.addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED);
        mNetworkCapabilitiesFilter.setLinkUpstreamBandwidthKbps(1024 * 1024);
        mNetworkCapabilitiesFilter.setLinkDownstreamBandwidthKbps(1024 * 1024);
        // TODO - needs to be a bit more dynamic
        mDfltNetworkCapabilities = new NetworkCapabilities(mNetworkCapabilitiesFilter);

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        mContext.registerReceiver(
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        String action = intent.getAction();

                        if (action.equals(Intent.ACTION_SCREEN_ON)) {
                            sendMessage(CMD_SCREEN_STATE_CHANGED, 1);
                        } else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
                            sendMessage(CMD_SCREEN_STATE_CHANGED, 0);
                        }
                    }
                }, filter);

        mContext.getContentResolver().registerContentObserver(Settings.Global.getUriFor(
                        Settings.Global.WIFI_SUSPEND_OPTIMIZATIONS_ENABLED), false,
                new ContentObserver(getHandler()) {
                    @Override
                    public void onChange(boolean selfChange) {
                        mUserWantsSuspendOpt.set(mFacade.getIntegerSetting(mContext,
                                Settings.Global.WIFI_SUSPEND_OPTIMIZATIONS_ENABLED, 1) == 1);
                    }
                });

        mContext.registerReceiver(
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        sendMessage(CMD_BOOT_COMPLETED);
                    }
                },
                new IntentFilter(Intent.ACTION_LOCKED_BOOT_COMPLETED));

        PowerManager powerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        mWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, getName());

        mSuspendWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "WifiSuspend");
        mSuspendWakeLock.setReferenceCounted(false);

        mTcpBufferSizes = mContext.getResources().getString(
                com.android.internal.R.string.config_wifi_tcp_buffers);

        // CHECKSTYLE:OFF IndentationCheck
        addState(mDefaultState);
            addState(mInitialState, mDefaultState);
            addState(mSupplicantStartingState, mDefaultState);
            addState(mSupplicantStartedState, mDefaultState);
                addState(mDriverStartingState, mSupplicantStartedState);
                addState(mDriverStartedState, mSupplicantStartedState);
                    addState(mScanModeState, mDriverStartedState);
                    addState(mConnectModeState, mDriverStartedState);
                        addState(mL2ConnectedState, mConnectModeState);
                            addState(mObtainingIpState, mL2ConnectedState);
                            addState(mConnectedState, mL2ConnectedState);
                            addState(mRoamingState, mL2ConnectedState);
                        addState(mDisconnectingState, mConnectModeState);
                        addState(mDisconnectedState, mConnectModeState);
                        addState(mWpsRunningState, mConnectModeState);
                addState(mWaitForP2pDisableState, mSupplicantStartedState);
                addState(mDriverStoppingState, mSupplicantStartedState);
                addState(mDriverStoppedState, mSupplicantStartedState);
            addState(mSupplicantStoppingState, mDefaultState);
            addState(mSoftApState, mDefaultState);
        // CHECKSTYLE:ON IndentationCheck

        setInitialState(mInitialState);

        setLogRecSize(NUM_LOG_RECS_NORMAL);
        setLogOnlyTransitions(false);

        //start the state machine
        start();

        mWifiMonitor.registerHandler(mInterfaceName, CMD_TARGET_BSSID, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, CMD_ASSOCIATED_BSSID, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.ANQP_DONE_EVENT, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.ASSOCIATION_REJECTION_EVENT,
                getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.AUTHENTICATION_FAILURE_EVENT,
                getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.DRIVER_HUNG_EVENT, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.GAS_QUERY_DONE_EVENT, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.GAS_QUERY_START_EVENT,
                getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.HS20_REMEDIATION_EVENT,
                getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.NETWORK_CONNECTION_EVENT,
                getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.NETWORK_DISCONNECTION_EVENT,
                getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.RX_HS20_ANQP_ICON_EVENT,
                getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.SCAN_FAILED_EVENT, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.SCAN_RESULTS_EVENT, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.SSID_REENABLED, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.SSID_TEMP_DISABLED, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.SUP_CONNECTION_EVENT, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.SUP_DISCONNECTION_EVENT,
                getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT,
                getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.SUP_REQUEST_IDENTITY, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.SUP_REQUEST_SIM_AUTH, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.WPS_FAIL_EVENT, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.WPS_OVERLAP_EVENT, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.WPS_SUCCESS_EVENT, getHandler());
        mWifiMonitor.registerHandler(mInterfaceName, WifiMonitor.WPS_TIMEOUT_EVENT, getHandler());

        final Intent intent = new Intent(WifiManager.WIFI_SCAN_AVAILABLE);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_SCAN_AVAILABLE, WIFI_STATE_DISABLED);
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);

        try {
            mSystemUiUid = mContext.getPackageManager().getPackageUidAsUser("com.android.systemui",
                    PackageManager.MATCH_SYSTEM_ONLY, UserHandle.USER_SYSTEM);
        } catch (PackageManager.NameNotFoundException e) {
            loge("Unable to resolve SystemUI's UID.");
        }

        mVerboseLoggingLevel = mFacade.getIntegerSetting(
                mContext, Settings.Global.WIFI_VERBOSE_LOGGING_ENABLED, 0);
        updateLoggingLevel();
    }

    class IpManagerCallback extends IpManager.Callback {
        @Override
        public void onPreDhcpAction() {
            sendMessage(DhcpClient.CMD_PRE_DHCP_ACTION);
        }

        @Override
        public void onPostDhcpAction() {
            sendMessage(DhcpClient.CMD_POST_DHCP_ACTION);
        }

        @Override
        public void onNewDhcpResults(DhcpResults dhcpResults) {
            if (dhcpResults != null) {
                sendMessage(CMD_IPV4_PROVISIONING_SUCCESS, dhcpResults);
            } else {
                sendMessage(CMD_IPV4_PROVISIONING_FAILURE);
                mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(getTargetSsid(),
                        mTargetRoamBSSID,
                        WifiLastResortWatchdog.FAILURE_CODE_DHCP);
            }
        }

        @Override
        public void onProvisioningSuccess(LinkProperties newLp) {
            sendMessage(CMD_UPDATE_LINKPROPERTIES, newLp);
            sendMessage(CMD_IP_CONFIGURATION_SUCCESSFUL);
        }

        @Override
        public void onProvisioningFailure(LinkProperties newLp) {
            sendMessage(CMD_IP_CONFIGURATION_LOST);
        }

        @Override
        public void onLinkPropertiesChange(LinkProperties newLp) {
            sendMessage(CMD_UPDATE_LINKPROPERTIES, newLp);
        }

        @Override
        public void onReachabilityLost(String logMsg) {
            sendMessage(CMD_IP_REACHABILITY_LOST, logMsg);
        }

        @Override
        public void installPacketFilter(byte[] filter) {
            sendMessage(CMD_INSTALL_PACKET_FILTER, filter);
        }

        @Override
        public void setFallbackMulticastFilter(boolean enabled) {
            sendMessage(CMD_SET_FALLBACK_PACKET_FILTERING, enabled);
        }

        @Override
        public void setNeighborDiscoveryOffload(boolean enabled) {
            sendMessage(CMD_CONFIG_ND_OFFLOAD, (enabled ? 1 : 0));
        }
    }

    private void stopIpManager() {
        /* Restore power save and suspend optimizations */
        handlePostDhcpSetup();
        mIpManager.stop();
    }

    PendingIntent getPrivateBroadcast(String action, int requestCode) {
        Intent intent = new Intent(action, null);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.setPackage("android");
        return mFacade.getBroadcast(mContext, requestCode, intent, 0);
    }

    int getVerboseLoggingLevel() {
        return mVerboseLoggingLevel;
    }

    void enableVerboseLogging(int verbose) {
        if (mVerboseLoggingLevel == verbose) {
            // We are already at the desired verbosity, avoid resetting StateMachine log records by
            // returning here until underlying bug is fixed (b/28027593)
            return;
        }
        mVerboseLoggingLevel = verbose;
        mFacade.setIntegerSetting(
                mContext, Settings.Global.WIFI_VERBOSE_LOGGING_ENABLED, verbose);
        updateLoggingLevel();
    }

    /**
     * Set wpa_supplicant log level using |mVerboseLoggingLevel| flag.
     */
    void setSupplicantLogLevel() {
        if (mVerboseLoggingLevel > 0) {
            mWifiNative.setSupplicantLogLevel("DEBUG");
        } else {
            mWifiNative.setSupplicantLogLevel("INFO");
        }
    }

    void updateLoggingLevel() {
        if (mVerboseLoggingLevel > 0) {
            DBG = true;
            setLogRecSize(ActivityManager.isLowRamDeviceStatic()
                    ? NUM_LOG_RECS_VERBOSE_LOW_MEMORY : NUM_LOG_RECS_VERBOSE);
        } else {
            DBG = false;
            setLogRecSize(NUM_LOG_RECS_NORMAL);
        }
        configureVerboseHalLogging(mVerboseLoggingLevel > 0);
        setSupplicantLogLevel();
        mCountryCode.enableVerboseLogging(mVerboseLoggingLevel);
        mWifiLogger.startLogging(DBG);
        mWifiMonitor.enableVerboseLogging(mVerboseLoggingLevel);
        mWifiNative.enableVerboseLogging(mVerboseLoggingLevel);
        mWifiConfigManager.enableVerboseLogging(mVerboseLoggingLevel);
        mSupplicantStateTracker.enableVerboseLogging(mVerboseLoggingLevel);
        mWifiQualifiedNetworkSelector.enableVerboseLogging(mVerboseLoggingLevel);
        if (mWifiConnectivityManager != null) {
            mWifiConnectivityManager.enableVerboseLogging(mVerboseLoggingLevel);
        }
    }

    private static final String SYSTEM_PROPERTY_LOG_CONTROL_WIFIHAL = "log.tag.WifiHAL";
    private static final String LOGD_LEVEL_DEBUG = "D";
    private static final String LOGD_LEVEL_VERBOSE = "V";
    private void configureVerboseHalLogging(boolean enableVerbose) {
        if (mBuildProperties.isUserBuild()) {  // Verbose HAL logging not supported on user builds.
            return;
        }
        mPropertyService.set(SYSTEM_PROPERTY_LOG_CONTROL_WIFIHAL,
                enableVerbose ? LOGD_LEVEL_VERBOSE : LOGD_LEVEL_DEBUG);
    }

    long mLastScanPermissionUpdate = 0;
    boolean mConnectedModeGScanOffloadStarted = false;
    // Don't do a G-scan enable/re-enable cycle more than once within 20seconds
    // The function updateAssociatedScanPermission() can be called quite frequently, hence
    // we want to throttle the GScan Stop->Start transition
    static final long SCAN_PERMISSION_UPDATE_THROTTLE_MILLI = 20000;
    void updateAssociatedScanPermission() {
    }

    private int mAggressiveHandover = 0;

    int getAggressiveHandover() {
        return mAggressiveHandover;
    }

    void enableAggressiveHandover(int enabled) {
        mAggressiveHandover = enabled;
    }

    public void clearANQPCache() {
        mWifiConfigManager.trimANQPCache(true);
    }

    public void setAllowScansWithTraffic(int enabled) {
        mWifiConfigManager.mAlwaysEnableScansWhileAssociated.set(enabled);
    }

    public int getAllowScansWithTraffic() {
        return mWifiConfigManager.mAlwaysEnableScansWhileAssociated.get();
    }

    /*
     * Dynamically turn on/off if switching networks while connected is allowd.
     */
    public boolean setEnableAutoJoinWhenAssociated(boolean enabled) {
        sendMessage(CMD_ENABLE_AUTOJOIN_WHEN_ASSOCIATED, enabled ? 1 : 0);
        return true;
    }

    public boolean getEnableAutoJoinWhenAssociated() {
        return mWifiConfigManager.getEnableAutoJoinWhenAssociated();
    }

    private boolean setRandomMacOui() {
        String oui = mContext.getResources().getString(R.string.config_wifi_random_mac_oui);
        if (TextUtils.isEmpty(oui)) {
            oui = GOOGLE_OUI;
        }
        String[] ouiParts = oui.split("-");
        byte[] ouiBytes = new byte[3];
        ouiBytes[0] = (byte) (Integer.parseInt(ouiParts[0], 16) & 0xFF);
        ouiBytes[1] = (byte) (Integer.parseInt(ouiParts[1], 16) & 0xFF);
        ouiBytes[2] = (byte) (Integer.parseInt(ouiParts[2], 16) & 0xFF);

        logd("Setting OUI to " + oui);
        return mWifiNative.setScanningMacOui(ouiBytes);
    }

    /**
     * ******************************************************
     * Methods exposed for public use
     * ******************************************************
     */

    public Messenger getMessenger() {
        return new Messenger(getHandler());
    }

    /**
     * TODO: doc
     */
    public boolean syncPingSupplicant(AsyncChannel channel) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_PING_SUPPLICANT);
        boolean result = (resultMsg.arg1 != FAILURE);
        resultMsg.recycle();
        return result;
    }

    /**
     * Initiate a wifi scan. If workSource is not null, blame is given to it, otherwise blame is
     * given to callingUid.
     *
     * @param callingUid The uid initiating the wifi scan. Blame will be given here unless
     *                   workSource is specified.
     * @param workSource If not null, blame is given to workSource.
     * @param settings   Scan settings, see {@link ScanSettings}.
     */
    public void startScan(int callingUid, int scanCounter,
                          ScanSettings settings, WorkSource workSource) {
        Bundle bundle = new Bundle();
        bundle.putParcelable(CUSTOMIZED_SCAN_SETTING, settings);
        bundle.putParcelable(CUSTOMIZED_SCAN_WORKSOURCE, workSource);
        bundle.putLong(SCAN_REQUEST_TIME, System.currentTimeMillis());
        sendMessage(CMD_START_SCAN, callingUid, scanCounter, bundle);
    }

    // called from BroadcastListener

    /**
     * Start reading new scan data
     * Data comes in as:
     * "scancount=5\n"
     * "nextcount=5\n"
     * "apcount=3\n"
     * "trunc\n" (optional)
     * "bssid=...\n"
     * "ssid=...\n"
     * "freq=...\n" (in Mhz)
     * "level=...\n"
     * "dist=...\n" (in cm)
     * "distsd=...\n" (standard deviation, in cm)
     * "===="
     * "bssid=...\n"
     * etc
     * "===="
     * "bssid=...\n"
     * etc
     * "%%%%"
     * "apcount=2\n"
     * "bssid=...\n"
     * etc
     * "%%%%
     * etc
     * "----"
     */
    private final static boolean DEBUG_PARSE = false;

    private long mDisconnectedTimeStamp = 0;

    public long getDisconnectedTimeMilli() {
        if (getCurrentState() == mDisconnectedState
                && mDisconnectedTimeStamp != 0) {
            long now_ms = System.currentTimeMillis();
            return now_ms - mDisconnectedTimeStamp;
        }
        return 0;
    }

    // Last connect attempt is used to prevent scan requests:
    //  - for a period of 10 seconds after attempting to connect
    private long lastConnectAttemptTimestamp = 0;
    private Set<Integer> lastScanFreqs = null;

    // For debugging, keep track of last message status handling
    // TODO, find an equivalent mechanism as part of parent class
    private static int MESSAGE_HANDLING_STATUS_PROCESSED = 2;
    private static int MESSAGE_HANDLING_STATUS_OK = 1;
    private static int MESSAGE_HANDLING_STATUS_UNKNOWN = 0;
    private static int MESSAGE_HANDLING_STATUS_REFUSED = -1;
    private static int MESSAGE_HANDLING_STATUS_FAIL = -2;
    private static int MESSAGE_HANDLING_STATUS_OBSOLETE = -3;
    private static int MESSAGE_HANDLING_STATUS_DEFERRED = -4;
    private static int MESSAGE_HANDLING_STATUS_DISCARD = -5;
    private static int MESSAGE_HANDLING_STATUS_LOOPED = -6;
    private static int MESSAGE_HANDLING_STATUS_HANDLING_ERROR = -7;

    private int messageHandlingStatus = 0;

    //TODO: this is used only to track connection attempts, however the link state and packet per
    //TODO: second logic should be folded into that
    private boolean checkOrDeferScanAllowed(Message msg) {
        long now = System.currentTimeMillis();
        if (lastConnectAttemptTimestamp != 0 && (now - lastConnectAttemptTimestamp) < 10000) {
            Message dmsg = Message.obtain(msg);
            sendMessageDelayed(dmsg, 11000 - (now - lastConnectAttemptTimestamp));
            return false;
        }
        return true;
    }

    private int mOnTime = 0;
    private int mTxTime = 0;
    private int mRxTime = 0;

    private int mOnTimeScreenStateChange = 0;
    private long lastOntimeReportTimeStamp = 0;
    private long lastScreenStateChangeTimeStamp = 0;
    private int mOnTimeLastReport = 0;
    private int mTxTimeLastReport = 0;
    private int mRxTimeLastReport = 0;

    private long lastLinkLayerStatsUpdate = 0;

    String reportOnTime() {
        long now = System.currentTimeMillis();
        StringBuilder sb = new StringBuilder();
        // Report stats since last report
        int on = mOnTime - mOnTimeLastReport;
        mOnTimeLastReport = mOnTime;
        int tx = mTxTime - mTxTimeLastReport;
        mTxTimeLastReport = mTxTime;
        int rx = mRxTime - mRxTimeLastReport;
        mRxTimeLastReport = mRxTime;
        int period = (int) (now - lastOntimeReportTimeStamp);
        lastOntimeReportTimeStamp = now;
        sb.append(String.format("[on:%d tx:%d rx:%d period:%d]", on, tx, rx, period));
        // Report stats since Screen State Changed
        on = mOnTime - mOnTimeScreenStateChange;
        period = (int) (now - lastScreenStateChangeTimeStamp);
        sb.append(String.format(" from screen [on:%d period:%d]", on, period));
        return sb.toString();
    }

    WifiLinkLayerStats getWifiLinkLayerStats(boolean dbg) {
        WifiLinkLayerStats stats = null;
        if (mWifiLinkLayerStatsSupported > 0) {
            String name = "wlan0";
            stats = mWifiNative.getWifiLinkLayerStats(name);
            if (name != null && stats == null && mWifiLinkLayerStatsSupported > 0) {
                mWifiLinkLayerStatsSupported -= 1;
            } else if (stats != null) {
                lastLinkLayerStatsUpdate = System.currentTimeMillis();
                mOnTime = stats.on_time;
                mTxTime = stats.tx_time;
                mRxTime = stats.rx_time;
                mRunningBeaconCount = stats.beacon_rx;
            }
        }
        if (stats == null || mWifiLinkLayerStatsSupported <= 0) {
            long mTxPkts = mFacade.getTxPackets(mInterfaceName);
            long mRxPkts = mFacade.getRxPackets(mInterfaceName);
            mWifiInfo.updatePacketRates(mTxPkts, mRxPkts);
        } else {
            mWifiInfo.updatePacketRates(stats);
        }
        return stats;
    }

    int startWifiIPPacketOffload(int slot, KeepalivePacketData packetData, int intervalSeconds) {
        int ret = mWifiNative.startSendingOffloadedPacket(slot, packetData, intervalSeconds * 1000);
        if (ret != 0) {
            loge("startWifiIPPacketOffload(" + slot + ", " + intervalSeconds +
                    "): hardware error " + ret);
            return ConnectivityManager.PacketKeepalive.ERROR_HARDWARE_ERROR;
        } else {
            return ConnectivityManager.PacketKeepalive.SUCCESS;
        }
    }

    int stopWifiIPPacketOffload(int slot) {
        int ret = mWifiNative.stopSendingOffloadedPacket(slot);
        if (ret != 0) {
            loge("stopWifiIPPacketOffload(" + slot + "): hardware error " + ret);
            return ConnectivityManager.PacketKeepalive.ERROR_HARDWARE_ERROR;
        } else {
            return ConnectivityManager.PacketKeepalive.SUCCESS;
        }
    }

    int startRssiMonitoringOffload(byte maxRssi, byte minRssi) {
        return mWifiNative.startRssiMonitoring(maxRssi, minRssi, WifiStateMachine.this);
    }

    int stopRssiMonitoringOffload() {
        return mWifiNative.stopRssiMonitoring();
    }

    private void handleScanRequest(Message message) {
        ScanSettings settings = null;
        WorkSource workSource = null;

        // unbundle parameters
        Bundle bundle = (Bundle) message.obj;

        if (bundle != null) {
            settings = bundle.getParcelable(CUSTOMIZED_SCAN_SETTING);
            workSource = bundle.getParcelable(CUSTOMIZED_SCAN_WORKSOURCE);
        }

        Set<Integer> freqs = null;
        if (settings != null && settings.channelSet != null) {
            freqs = new HashSet<Integer>();
            for (WifiChannel channel : settings.channelSet) {
                freqs.add(channel.freqMHz);
            }
        }

        // Retrieve the list of hidden networkId's to scan for.
        Set<Integer> hiddenNetworkIds = mWifiConfigManager.getHiddenConfiguredNetworkIds();

        // call wifi native to start the scan
        if (startScanNative(freqs, hiddenNetworkIds, workSource)) {
            // a full scan covers everything, clearing scan request buffer
            if (freqs == null)
                mBufferedScanMsg.clear();
            messageHandlingStatus = MESSAGE_HANDLING_STATUS_OK;
            if (workSource != null) {
                // External worksource was passed along the scan request,
                // hence always send a broadcast
                mSendScanResultsBroadcast = true;
            }
            return;
        }

        // if reach here, scan request is rejected

        if (!mIsScanOngoing) {
            // if rejection is NOT due to ongoing scan (e.g. bad scan parameters),

            // discard this request and pop up the next one
            if (mBufferedScanMsg.size() > 0) {
                sendMessage(mBufferedScanMsg.remove());
            }
            messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
        } else if (!mIsFullScanOngoing) {
            // if rejection is due to an ongoing scan, and the ongoing one is NOT a full scan,
            // buffer the scan request to make sure specified channels will be scanned eventually
            if (freqs == null)
                mBufferedScanMsg.clear();
            if (mBufferedScanMsg.size() < SCAN_REQUEST_BUFFER_MAX_SIZE) {
                Message msg = obtainMessage(CMD_START_SCAN,
                        message.arg1, message.arg2, bundle);
                mBufferedScanMsg.add(msg);
            } else {
                // if too many requests in buffer, combine them into a single full scan
                bundle = new Bundle();
                bundle.putParcelable(CUSTOMIZED_SCAN_SETTING, null);
                bundle.putParcelable(CUSTOMIZED_SCAN_WORKSOURCE, workSource);
                Message msg = obtainMessage(CMD_START_SCAN, message.arg1, message.arg2, bundle);
                mBufferedScanMsg.clear();
                mBufferedScanMsg.add(msg);
            }
            messageHandlingStatus = MESSAGE_HANDLING_STATUS_LOOPED;
        } else {
            // mIsScanOngoing and mIsFullScanOngoing
            messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
        }
    }


    // TODO this is a temporary measure to bridge between WifiScanner and WifiStateMachine until
    // scan functionality is refactored out of WifiStateMachine.
    /**
     * return true iff scan request is accepted
     */
    private boolean startScanNative(final Set<Integer> freqs, Set<Integer> hiddenNetworkIds,
            WorkSource workSource) {
        WifiScanner.ScanSettings settings = new WifiScanner.ScanSettings();
        if (freqs == null) {
            settings.band = WifiScanner.WIFI_BAND_BOTH_WITH_DFS;
        } else {
            settings.band = WifiScanner.WIFI_BAND_UNSPECIFIED;
            int index = 0;
            settings.channels = new WifiScanner.ChannelSpec[freqs.size()];
            for (Integer freq : freqs) {
                settings.channels[index++] = new WifiScanner.ChannelSpec(freq);
            }
        }
        settings.reportEvents = WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN
                | WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT;
        if (hiddenNetworkIds != null && hiddenNetworkIds.size() > 0) {
            int i = 0;
            settings.hiddenNetworkIds = new int[hiddenNetworkIds.size()];
            for (Integer netId : hiddenNetworkIds) {
                settings.hiddenNetworkIds[i++] = netId;
            }
        }
        WifiScanner.ScanListener nativeScanListener = new WifiScanner.ScanListener() {
                // ignore all events since WifiStateMachine is registered for the supplicant events
                public void onSuccess() {
                }
                public void onFailure(int reason, String description) {
                    mIsScanOngoing = false;
                    mIsFullScanOngoing = false;
                }
                public void onResults(WifiScanner.ScanData[] results) {
                }
                public void onFullResult(ScanResult fullScanResult) {
                }
                public void onPeriodChanged(int periodInMs) {
                }
            };
        mWifiScanner.startScan(settings, nativeScanListener, workSource);
        mIsScanOngoing = true;
        mIsFullScanOngoing = (freqs == null);
        lastScanFreqs = freqs;
        return true;
    }

    /**
     * TODO: doc
     */
    public void setSupplicantRunning(boolean enable) {
        if (enable) {
            sendMessage(CMD_START_SUPPLICANT);
        } else {
            sendMessage(CMD_STOP_SUPPLICANT);
        }
    }

    /**
     * TODO: doc
     */
    public void setHostApRunning(WifiConfiguration wifiConfig, boolean enable) {
        if (enable) {
            sendMessage(CMD_START_AP, wifiConfig);
        } else {
            sendMessage(CMD_STOP_AP);
        }
    }

    public void setWifiApConfiguration(WifiConfiguration config) {
        mWifiApConfigStore.setApConfiguration(config);
    }

    public WifiConfiguration syncGetWifiApConfiguration() {
        return mWifiApConfigStore.getApConfiguration();
    }

    /**
     * TODO: doc
     */
    public int syncGetWifiState() {
        return mWifiState.get();
    }

    /**
     * TODO: doc
     */
    public String syncGetWifiStateByName() {
        switch (mWifiState.get()) {
            case WIFI_STATE_DISABLING:
                return "disabling";
            case WIFI_STATE_DISABLED:
                return "disabled";
            case WIFI_STATE_ENABLING:
                return "enabling";
            case WIFI_STATE_ENABLED:
                return "enabled";
            case WIFI_STATE_UNKNOWN:
                return "unknown state";
            default:
                return "[invalid state]";
        }
    }

    /**
     * TODO: doc
     */
    public int syncGetWifiApState() {
        return mWifiApState.get();
    }

    /**
     * TODO: doc
     */
    public String syncGetWifiApStateByName() {
        switch (mWifiApState.get()) {
            case WIFI_AP_STATE_DISABLING:
                return "disabling";
            case WIFI_AP_STATE_DISABLED:
                return "disabled";
            case WIFI_AP_STATE_ENABLING:
                return "enabling";
            case WIFI_AP_STATE_ENABLED:
                return "enabled";
            case WIFI_AP_STATE_FAILED:
                return "failed";
            default:
                return "[invalid state]";
        }
    }

    public boolean isConnected() {
        return getCurrentState() == mConnectedState;
    }

    public boolean isDisconnected() {
        return getCurrentState() == mDisconnectedState;
    }

    public boolean isSupplicantTransientState() {
        SupplicantState SupplicantState = mWifiInfo.getSupplicantState();
        if (SupplicantState == SupplicantState.ASSOCIATING
                || SupplicantState == SupplicantState.AUTHENTICATING
                || SupplicantState == SupplicantState.FOUR_WAY_HANDSHAKE
                || SupplicantState == SupplicantState.GROUP_HANDSHAKE) {

            if (DBG) {
                Log.d(TAG, "Supplicant is under transient state: " + SupplicantState);
            }
            return true;
        } else {
            if (DBG) {
                Log.d(TAG, "Supplicant is under steady state: " + SupplicantState);
            }
        }

        return false;
    }

    public boolean isLinkDebouncing() {
        return linkDebouncing;
    }

    /**
     * Get status information for the current connection, if any.
     *
     * @return a {@link WifiInfo} object containing information about the current connection
     */
    public WifiInfo syncRequestConnectionInfo() {
        return getWiFiInfoForUid(Binder.getCallingUid());
    }

    public WifiInfo getWifiInfo() {
        return mWifiInfo;
    }

    public DhcpResults syncGetDhcpResults() {
        synchronized (mDhcpResultsLock) {
            return new DhcpResults(mDhcpResults);
        }
    }

    /**
     * TODO: doc
     */
    public void setDriverStart(boolean enable) {
        if (enable) {
            sendMessage(CMD_START_DRIVER);
        } else {
            sendMessage(CMD_STOP_DRIVER);
        }
    }

    /**
     * TODO: doc
     */
    public void setOperationalMode(int mode) {
        if (DBG) log("setting operational mode to " + String.valueOf(mode));
        sendMessage(CMD_SET_OPERATIONAL_MODE, mode, 0);
    }

    /**
     * Allow tests to confirm the operational mode for WSM.
     */
    @VisibleForTesting
    protected int getOperationalModeForTest() {
        return mOperationalMode;
    }

    /**
     * TODO: doc
     */
    public List<ScanResult> syncGetScanResultsList() {
        synchronized (mScanResultsLock) {
            List<ScanResult> scanList = new ArrayList<ScanResult>();
            for (ScanDetail result : mScanResults) {
                scanList.add(new ScanResult(result.getScanResult()));
            }
            return scanList;
        }
    }

    public int syncAddPasspointManagementObject(AsyncChannel channel, String managementObject) {
        Message resultMsg =
                channel.sendMessageSynchronously(CMD_ADD_PASSPOINT_MO, managementObject);
        int result = resultMsg.arg1;
        resultMsg.recycle();
        return result;
    }

    public int syncModifyPasspointManagementObject(AsyncChannel channel, String fqdn,
                                                   List<PasspointManagementObjectDefinition>
                                                           managementObjectDefinitions) {
        Bundle bundle = new Bundle();
        bundle.putString("FQDN", fqdn);
        bundle.putParcelableList("MOS", managementObjectDefinitions);
        Message resultMsg = channel.sendMessageSynchronously(CMD_MODIFY_PASSPOINT_MO, bundle);
        int result = resultMsg.arg1;
        resultMsg.recycle();
        return result;
    }

    public boolean syncQueryPasspointIcon(AsyncChannel channel, long bssid, String fileName) {
        Bundle bundle = new Bundle();
        bundle.putLong("BSSID", bssid);
        bundle.putString("FILENAME", fileName);
        Message resultMsg = channel.sendMessageSynchronously(CMD_QUERY_OSU_ICON, bundle);
        int result = resultMsg.arg1;
        resultMsg.recycle();
        return result == 1;
    }

    public int matchProviderWithCurrentNetwork(AsyncChannel channel, String fqdn) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_MATCH_PROVIDER_NETWORK, fqdn);
        int result = resultMsg.arg1;
        resultMsg.recycle();
        return result;
    }

    /**
     * Deauthenticate and set the re-authentication hold off time for the current network
     * @param holdoff hold off time in milliseconds
     * @param ess set if the hold off pertains to an ESS rather than a BSS
     */
    public void deauthenticateNetwork(AsyncChannel channel, long holdoff, boolean ess) {
        // TODO: This needs an implementation
    }

    public void disableEphemeralNetwork(String SSID) {
        if (SSID != null) {
            sendMessage(CMD_DISABLE_EPHEMERAL_NETWORK, SSID);
        }
    }

    /**
     * Disconnect from Access Point
     */
    public void disconnectCommand() {
        sendMessage(CMD_DISCONNECT);
    }

    public void disconnectCommand(int uid, int reason) {
        sendMessage(CMD_DISCONNECT, uid, reason);
    }

    /**
     * Initiate a reconnection to AP
     */
    public void reconnectCommand() {
        sendMessage(CMD_RECONNECT);
    }

    /**
     * Initiate a re-association to AP
     */
    public void reassociateCommand() {
        sendMessage(CMD_REASSOCIATE);
    }

    /**
     * Reload networks and then reconnect; helps load correct data for TLS networks
     */

    public void reloadTlsNetworksAndReconnect() {
        sendMessage(CMD_RELOAD_TLS_AND_RECONNECT);
    }

    /**
     * Add a network synchronously
     *
     * @return network id of the new network
     */
    public int syncAddOrUpdateNetwork(AsyncChannel channel, WifiConfiguration config) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_ADD_OR_UPDATE_NETWORK, config);
        int result = resultMsg.arg1;
        resultMsg.recycle();
        return result;
    }

    /**
     * Get configured networks synchronously
     *
     * @param channel
     * @return
     */

    public List<WifiConfiguration> syncGetConfiguredNetworks(int uuid, AsyncChannel channel) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_GET_CONFIGURED_NETWORKS, uuid);
        List<WifiConfiguration> result = (List<WifiConfiguration>) resultMsg.obj;
        resultMsg.recycle();
        return result;
    }

    public List<WifiConfiguration> syncGetPrivilegedConfiguredNetwork(AsyncChannel channel) {
        Message resultMsg = channel.sendMessageSynchronously(
                CMD_GET_PRIVILEGED_CONFIGURED_NETWORKS);
        List<WifiConfiguration> result = (List<WifiConfiguration>) resultMsg.obj;
        resultMsg.recycle();
        return result;
    }

    public WifiConfiguration syncGetMatchingWifiConfig(ScanResult scanResult, AsyncChannel channel) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_GET_MATCHING_CONFIG, scanResult);
        return (WifiConfiguration) resultMsg.obj;
    }

    /**
     * Get connection statistics synchronously
     *
     * @param channel
     * @return
     */

    public WifiConnectionStatistics syncGetConnectionStatistics(AsyncChannel channel) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_GET_CONNECTION_STATISTICS);
        WifiConnectionStatistics result = (WifiConnectionStatistics) resultMsg.obj;
        resultMsg.recycle();
        return result;
    }

    /**
     * Get adaptors synchronously
     */

    public int syncGetSupportedFeatures(AsyncChannel channel) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_GET_SUPPORTED_FEATURES);
        int supportedFeatureSet = resultMsg.arg1;
        resultMsg.recycle();
        return supportedFeatureSet;
    }

    /**
     * Get link layers stats for adapter synchronously
     */
    public WifiLinkLayerStats syncGetLinkLayerStats(AsyncChannel channel) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_GET_LINK_LAYER_STATS);
        WifiLinkLayerStats result = (WifiLinkLayerStats) resultMsg.obj;
        resultMsg.recycle();
        return result;
    }

    /**
     * Delete a network
     *
     * @param networkId id of the network to be removed
     */
    public boolean syncRemoveNetwork(AsyncChannel channel, int networkId) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_REMOVE_NETWORK, networkId);
        boolean result = (resultMsg.arg1 != FAILURE);
        resultMsg.recycle();
        return result;
    }

    /**
     * Enable a network
     *
     * @param netId         network id of the network
     * @param disableOthers true, if all other networks have to be disabled
     * @return {@code true} if the operation succeeds, {@code false} otherwise
     */
    public boolean syncEnableNetwork(AsyncChannel channel, int netId, boolean disableOthers) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_ENABLE_NETWORK, netId,
                disableOthers ? 1 : 0);
        boolean result = (resultMsg.arg1 != FAILURE);
        resultMsg.recycle();
        return result;
    }

    /**
     * Disable a network
     *
     * @param netId network id of the network
     * @return {@code true} if the operation succeeds, {@code false} otherwise
     */
    public boolean syncDisableNetwork(AsyncChannel channel, int netId) {
        Message resultMsg = channel.sendMessageSynchronously(WifiManager.DISABLE_NETWORK, netId);
        boolean result = (resultMsg.arg1 != WifiManager.DISABLE_NETWORK_FAILED);
        resultMsg.recycle();
        return result;
    }

    /**
     * Retrieves a WPS-NFC configuration token for the specified network
     *
     * @return a hex string representation of the WPS-NFC configuration token
     */
    public String syncGetWpsNfcConfigurationToken(int netId) {
        return mWifiNative.getNfcWpsConfigurationToken(netId);
    }

    /**
     * Blacklist a BSSID. This will avoid the AP if there are
     * alternate APs to connect
     *
     * @param bssid BSSID of the network
     */
    public void addToBlacklist(String bssid) {
        sendMessage(CMD_BLACKLIST_NETWORK, bssid);
    }

    /**
     * Clear the blacklist list
     */
    public void clearBlacklist() {
        sendMessage(CMD_CLEAR_BLACKLIST);
    }

    public void enableRssiPolling(boolean enabled) {
        sendMessage(CMD_ENABLE_RSSI_POLL, enabled ? 1 : 0, 0);
    }

    public void enableAllNetworks() {
        sendMessage(CMD_ENABLE_ALL_NETWORKS);
    }

    /**
     * Start filtering Multicast v4 packets
     */
    public void startFilteringMulticastPackets() {
        mIpManager.setMulticastFilter(true);
    }

    /**
     * Stop filtering Multicast v4 packets
     */
    public void stopFilteringMulticastPackets() {
        mIpManager.setMulticastFilter(false);
    }

    /**
     * Set high performance mode of operation.
     * Enabling would set active power mode and disable suspend optimizations;
     * disabling would set auto power mode and enable suspend optimizations
     *
     * @param enable true if enable, false otherwise
     */
    public void setHighPerfModeEnabled(boolean enable) {
        sendMessage(CMD_SET_HIGH_PERF_MODE, enable ? 1 : 0, 0);
    }


    /**
     * reset cached SIM credential data
     */
    public synchronized void resetSimAuthNetworks(boolean simPresent) {
        sendMessage(CMD_RESET_SIM_NETWORKS, simPresent ? 1 : 0);
    }

    /**
     * Get Network object of current wifi network
     * @return Network object of current wifi network
     */
    public Network getCurrentNetwork() {
        if (mNetworkAgent != null) {
            return new Network(mNetworkAgent.netId);
        } else {
            return null;
        }
    }


    /**
     * Set the operational frequency band
     *
     * @param band
     * @param persist {@code true} if the setting should be remembered.
     */
    public void setFrequencyBand(int band, boolean persist) {
        if (persist) {
            Settings.Global.putInt(mContext.getContentResolver(),
                    Settings.Global.WIFI_FREQUENCY_BAND,
                    band);
        }
        sendMessage(CMD_SET_FREQUENCY_BAND, band, 0);
    }

    /**
     * Enable TDLS for a specific MAC address
     */
    public void enableTdls(String remoteMacAddress, boolean enable) {
        int enabler = enable ? 1 : 0;
        sendMessage(CMD_ENABLE_TDLS, enabler, 0, remoteMacAddress);
    }

    /**
     * Returns the operational frequency band
     */
    public int getFrequencyBand() {
        return mFrequencyBand.get();
    }

    /**
     * Returns the wifi configuration file
     */
    public String getConfigFile() {
        return mWifiConfigManager.getConfigFile();
    }

    /**
     * Send a message indicating bluetooth adapter connection state changed
     */
    public void sendBluetoothAdapterStateChange(int state) {
        sendMessage(CMD_BLUETOOTH_ADAPTER_STATE_CHANGE, state, 0);
    }

    /**
     * Send a message indicating a package has been uninstalled.
     */
    public void removeAppConfigs(String packageName, int uid) {
        // Build partial AppInfo manually - package may not exist in database any more
        ApplicationInfo ai = new ApplicationInfo();
        ai.packageName = packageName;
        ai.uid = uid;
        sendMessage(CMD_REMOVE_APP_CONFIGURATIONS, ai);
    }

    /**
     * Send a message indicating a user has been removed.
     */
    public void removeUserConfigs(int userId) {
        sendMessage(CMD_REMOVE_USER_CONFIGURATIONS, userId);
    }

    /**
     * Save configuration on supplicant
     *
     * @return {@code true} if the operation succeeds, {@code false} otherwise
     * <p/>
     * TODO: deprecate this
     */
    public boolean syncSaveConfig(AsyncChannel channel) {
        Message resultMsg = channel.sendMessageSynchronously(CMD_SAVE_CONFIG);
        boolean result = (resultMsg.arg1 != FAILURE);
        resultMsg.recycle();
        return result;
    }

    public void updateBatteryWorkSource(WorkSource newSource) {
        synchronized (mRunningWifiUids) {
            try {
                if (newSource != null) {
                    mRunningWifiUids.set(newSource);
                }
                if (mIsRunning) {
                    if (mReportedRunning) {
                        // If the work source has changed since last time, need
                        // to remove old work from battery stats.
                        if (mLastRunningWifiUids.diff(mRunningWifiUids)) {
                            mBatteryStats.noteWifiRunningChanged(mLastRunningWifiUids,
                                    mRunningWifiUids);
                            mLastRunningWifiUids.set(mRunningWifiUids);
                        }
                    } else {
                        // Now being started, report it.
                        mBatteryStats.noteWifiRunning(mRunningWifiUids);
                        mLastRunningWifiUids.set(mRunningWifiUids);
                        mReportedRunning = true;
                    }
                } else {
                    if (mReportedRunning) {
                        // Last reported we were running, time to stop.
                        mBatteryStats.noteWifiStopped(mLastRunningWifiUids);
                        mLastRunningWifiUids.clear();
                        mReportedRunning = false;
                    }
                }
                mWakeLock.setWorkSource(newSource);
            } catch (RemoteException ignore) {
            }
        }
    }

    public void dumpIpManager(FileDescriptor fd, PrintWriter pw, String[] args) {
        mIpManager.dump(fd, pw, args);
    }

    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        if (args.length > 1 && WifiMetrics.PROTO_DUMP_ARG.equals(args[0])
                && WifiMetrics.CLEAN_DUMP_ARG.equals(args[1])) {
            // Dump only wifi metrics serialized proto bytes (base64)
            updateWifiMetrics();
            mWifiMetrics.dump(fd, pw, args);
            return;
        }
        super.dump(fd, pw, args);
        mSupplicantStateTracker.dump(fd, pw, args);
        pw.println("mLinkProperties " + mLinkProperties);
        pw.println("mWifiInfo " + mWifiInfo);
        pw.println("mDhcpResults " + mDhcpResults);
        pw.println("mNetworkInfo " + mNetworkInfo);
        pw.println("mLastSignalLevel " + mLastSignalLevel);
        pw.println("mLastBssid " + mLastBssid);
        pw.println("mLastNetworkId " + mLastNetworkId);
        pw.println("mOperationalMode " + mOperationalMode);
        pw.println("mUserWantsSuspendOpt " + mUserWantsSuspendOpt);
        pw.println("mSuspendOptNeedsDisabled " + mSuspendOptNeedsDisabled);
        pw.println("Supplicant status " + mWifiNative.status(true));
        if (mCountryCode.getCountryCodeSentToDriver() != null) {
            pw.println("CountryCode sent to driver " + mCountryCode.getCountryCodeSentToDriver());
        } else {
            if (mCountryCode.getCountryCode() != null) {
                pw.println("CountryCode: " +
                        mCountryCode.getCountryCode() + " was not sent to driver");
            } else {
                pw.println("CountryCode was not initialized");
            }
        }
        pw.println("mConnectedModeGScanOffloadStarted " + mConnectedModeGScanOffloadStarted);
        pw.println("mGScanPeriodMilli " + mGScanPeriodMilli);
        if (mWhiteListedSsids != null && mWhiteListedSsids.length > 0) {
            pw.println("SSID whitelist :" );
            for (int i=0; i < mWhiteListedSsids.length; i++) {
                pw.println("       " + mWhiteListedSsids[i]);
            }
        }
        if (mNetworkFactory != null) {
            mNetworkFactory.dump(fd, pw, args);
        } else {
            pw.println("mNetworkFactory is not initialized");
        }

        if (mUntrustedNetworkFactory != null) {
            mUntrustedNetworkFactory.dump(fd, pw, args);
        } else {
            pw.println("mUntrustedNetworkFactory is not initialized");
        }
        pw.println("Wlan Wake Reasons:" + mWifiNative.getWlanWakeReasonCount());
        pw.println();
        updateWifiMetrics();
        mWifiMetrics.dump(fd, pw, args);
        pw.println();

        mWifiConfigManager.dump(fd, pw, args);
        pw.println();
        mWifiLogger.captureBugReportData(WifiLogger.REPORT_REASON_USER_ACTION);
        mWifiLogger.dump(fd, pw, args);
        mWifiQualifiedNetworkSelector.dump(fd, pw, args);
        dumpIpManager(fd, pw, args);
        if (mWifiConnectivityManager != null) {
            mWifiConnectivityManager.dump(fd, pw, args);
        }
    }

    public void handleUserSwitch(int userId) {
        sendMessage(CMD_USER_SWITCH, userId);
    }

    /**
     * ******************************************************
     * Internal private functions
     * ******************************************************
     */

    private void logStateAndMessage(Message message, State state) {
        messageHandlingStatus = 0;
        if (DBG) {
            logd(" " + state.getClass().getSimpleName() + " " + getLogRecString(message));
        }
    }

    /**
     * helper, prints the milli time since boot wi and w/o suspended time
     */
    String printTime() {
        StringBuilder sb = new StringBuilder();
        sb.append(" rt=").append(SystemClock.uptimeMillis());
        sb.append("/").append(SystemClock.elapsedRealtime());
        return sb.toString();
    }

    /**
     * Return the additional string to be logged by LogRec, default
     *
     * @param msg that was processed
     * @return information to be logged as a String
     */
    protected String getLogRecString(Message msg) {
        WifiConfiguration config;
        Long now;
        String report;
        String key;
        StringBuilder sb = new StringBuilder();
        if (mScreenOn) {
            sb.append("!");
        }
        if (messageHandlingStatus != MESSAGE_HANDLING_STATUS_UNKNOWN) {
            sb.append("(").append(messageHandlingStatus).append(")");
        }
        sb.append(smToString(msg));
        if (msg.sendingUid > 0 && msg.sendingUid != Process.WIFI_UID) {
            sb.append(" uid=" + msg.sendingUid);
        }
        sb.append(" ").append(printTime());
        switch (msg.what) {
            case CMD_UPDATE_ASSOCIATED_SCAN_PERMISSION:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" autojoinAllowed=");
                sb.append(mWifiConfigManager.getEnableAutoJoinWhenAssociated());
                sb.append(" withTraffic=").append(getAllowScansWithTraffic());
                sb.append(" tx=").append(mWifiInfo.txSuccessRate);
                sb.append("/").append(mWifiConfigManager.MAX_TX_PACKET_FOR_FULL_SCANS);
                sb.append(" rx=").append(mWifiInfo.rxSuccessRate);
                sb.append("/").append(mWifiConfigManager.MAX_RX_PACKET_FOR_FULL_SCANS);
                sb.append(" -> ").append(mConnectedModeGScanOffloadStarted);
                break;
            case CMD_START_SCAN:
                now = System.currentTimeMillis();
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" ic=");
                sb.append(Integer.toString(sScanAlarmIntentCount));
                if (msg.obj != null) {
                    Bundle bundle = (Bundle) msg.obj;
                    Long request = bundle.getLong(SCAN_REQUEST_TIME, 0);
                    if (request != 0) {
                        sb.append(" proc(ms):").append(now - request);
                    }
                }
                if (mIsScanOngoing) sb.append(" onGoing");
                if (mIsFullScanOngoing) sb.append(" full");
                sb.append(" rssi=").append(mWifiInfo.getRssi());
                sb.append(" f=").append(mWifiInfo.getFrequency());
                sb.append(" sc=").append(mWifiInfo.score);
                sb.append(" link=").append(mWifiInfo.getLinkSpeed());
                sb.append(String.format(" tx=%.1f,", mWifiInfo.txSuccessRate));
                sb.append(String.format(" %.1f,", mWifiInfo.txRetriesRate));
                sb.append(String.format(" %.1f ", mWifiInfo.txBadRate));
                sb.append(String.format(" rx=%.1f", mWifiInfo.rxSuccessRate));
                if (lastScanFreqs != null) {
                    sb.append(" list=");
                    for(int freq : lastScanFreqs) {
                        sb.append(freq).append(",");
                    }
                }
                report = reportOnTime();
                if (report != null) {
                    sb.append(" ").append(report);
                }
                break;
            case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                StateChangeResult stateChangeResult = (StateChangeResult) msg.obj;
                if (stateChangeResult != null) {
                    sb.append(stateChangeResult.toString());
                }
                break;
            case WifiManager.SAVE_NETWORK:
            case WifiStateMachine.CMD_AUTO_SAVE_NETWORK:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (lastSavedConfigurationAttempt != null) {
                    sb.append(" ").append(lastSavedConfigurationAttempt.configKey());
                    sb.append(" nid=").append(lastSavedConfigurationAttempt.networkId);
                    if (lastSavedConfigurationAttempt.hiddenSSID) {
                        sb.append(" hidden");
                    }
                    if (lastSavedConfigurationAttempt.preSharedKey != null
                            && !lastSavedConfigurationAttempt.preSharedKey.equals("*")) {
                        sb.append(" hasPSK");
                    }
                    if (lastSavedConfigurationAttempt.ephemeral) {
                        sb.append(" ephemeral");
                    }
                    if (lastSavedConfigurationAttempt.selfAdded) {
                        sb.append(" selfAdded");
                    }
                    sb.append(" cuid=").append(lastSavedConfigurationAttempt.creatorUid);
                    sb.append(" suid=").append(lastSavedConfigurationAttempt.lastUpdateUid);
                }
                break;
            case WifiManager.FORGET_NETWORK:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (lastForgetConfigurationAttempt != null) {
                    sb.append(" ").append(lastForgetConfigurationAttempt.configKey());
                    sb.append(" nid=").append(lastForgetConfigurationAttempt.networkId);
                    if (lastForgetConfigurationAttempt.hiddenSSID) {
                        sb.append(" hidden");
                    }
                    if (lastForgetConfigurationAttempt.preSharedKey != null) {
                        sb.append(" hasPSK");
                    }
                    if (lastForgetConfigurationAttempt.ephemeral) {
                        sb.append(" ephemeral");
                    }
                    if (lastForgetConfigurationAttempt.selfAdded) {
                        sb.append(" selfAdded");
                    }
                    sb.append(" cuid=").append(lastForgetConfigurationAttempt.creatorUid);
                    sb.append(" suid=").append(lastForgetConfigurationAttempt.lastUpdateUid);
                    WifiConfiguration.NetworkSelectionStatus netWorkSelectionStatus =
                            lastForgetConfigurationAttempt.getNetworkSelectionStatus();
                    sb.append(" ajst=").append(
                            netWorkSelectionStatus.getNetworkStatusString());
                }
                break;
            case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                String bssid = (String) msg.obj;
                if (bssid != null && bssid.length() > 0) {
                    sb.append(" ");
                    sb.append(bssid);
                }
                sb.append(" blacklist=" + Boolean.toString(didBlackListBSSID));
                break;
            case WifiMonitor.SCAN_RESULTS_EVENT:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (mScanResults != null) {
                    sb.append(" found=");
                    sb.append(mScanResults.size());
                }
                sb.append(" known=").append(mNumScanResultsKnown);
                sb.append(" got=").append(mNumScanResultsReturned);
                sb.append(String.format(" bcn=%d", mRunningBeaconCount));
                sb.append(String.format(" con=%d", mConnectionReqCount));
                sb.append(String.format(" untrustedcn=%d", mUntrustedReqCount));
                key = mWifiConfigManager.getLastSelectedConfiguration();
                if (key != null) {
                    sb.append(" last=").append(key);
                }
                break;
            case WifiMonitor.SCAN_FAILED_EVENT:
                break;
            case WifiMonitor.NETWORK_CONNECTION_EVENT:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" ").append(mLastBssid);
                sb.append(" nid=").append(mLastNetworkId);
                config = getCurrentWifiConfiguration();
                if (config != null) {
                    sb.append(" ").append(config.configKey());
                }
                key = mWifiConfigManager.getLastSelectedConfiguration();
                if (key != null) {
                    sb.append(" last=").append(key);
                }
                break;
            case CMD_TARGET_BSSID:
            case CMD_ASSOCIATED_BSSID:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (msg.obj != null) {
                    sb.append(" BSSID=").append((String) msg.obj);
                }
                if (mTargetRoamBSSID != null) {
                    sb.append(" Target=").append(mTargetRoamBSSID);
                }
                sb.append(" roam=").append(Boolean.toString(mAutoRoaming));
                break;
            case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                if (msg.obj != null) {
                    sb.append(" ").append((String) msg.obj);
                }
                sb.append(" nid=").append(msg.arg1);
                sb.append(" reason=").append(msg.arg2);
                if (mLastBssid != null) {
                    sb.append(" lastbssid=").append(mLastBssid);
                }
                if (mWifiInfo.getFrequency() != -1) {
                    sb.append(" freq=").append(mWifiInfo.getFrequency());
                    sb.append(" rssi=").append(mWifiInfo.getRssi());
                }
                if (linkDebouncing) {
                    sb.append(" debounce");
                }
                break;
            case WifiMonitor.SSID_TEMP_DISABLED:
            case WifiMonitor.SSID_REENABLED:
                sb.append(" nid=").append(msg.arg1);
                if (msg.obj != null) {
                    sb.append(" ").append((String) msg.obj);
                }
                config = getCurrentWifiConfiguration();
                if (config != null) {
                    WifiConfiguration.NetworkSelectionStatus netWorkSelectionStatus =
                            config.getNetworkSelectionStatus();
                    sb.append(" cur=").append(config.configKey());
                    sb.append(" ajst=").append(netWorkSelectionStatus.getNetworkStatusString());
                    if (config.selfAdded) {
                        sb.append(" selfAdded");
                    }
                    if (config.status != 0) {
                        sb.append(" st=").append(config.status);
                        sb.append(" rs=").append(
                                netWorkSelectionStatus.getNetworkDisableReasonString());
                    }
                    if (config.lastConnected != 0) {
                        now = System.currentTimeMillis();
                        sb.append(" lastconn=").append(now - config.lastConnected).append("(ms)");
                    }
                    if (mLastBssid != null) {
                        sb.append(" lastbssid=").append(mLastBssid);
                    }
                    if (mWifiInfo.getFrequency() != -1) {
                        sb.append(" freq=").append(mWifiInfo.getFrequency());
                        sb.append(" rssi=").append(mWifiInfo.getRssi());
                        sb.append(" bssid=").append(mWifiInfo.getBSSID());
                    }
                }
                break;
            case CMD_RSSI_POLL:
            case CMD_UNWANTED_NETWORK:
            case WifiManager.RSSI_PKTCNT_FETCH:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (mWifiInfo.getSSID() != null)
                    if (mWifiInfo.getSSID() != null)
                        sb.append(" ").append(mWifiInfo.getSSID());
                if (mWifiInfo.getBSSID() != null)
                    sb.append(" ").append(mWifiInfo.getBSSID());
                sb.append(" rssi=").append(mWifiInfo.getRssi());
                sb.append(" f=").append(mWifiInfo.getFrequency());
                sb.append(" sc=").append(mWifiInfo.score);
                sb.append(" link=").append(mWifiInfo.getLinkSpeed());
                sb.append(String.format(" tx=%.1f,", mWifiInfo.txSuccessRate));
                sb.append(String.format(" %.1f,", mWifiInfo.txRetriesRate));
                sb.append(String.format(" %.1f ", mWifiInfo.txBadRate));
                sb.append(String.format(" rx=%.1f", mWifiInfo.rxSuccessRate));
                sb.append(String.format(" bcn=%d", mRunningBeaconCount));
                report = reportOnTime();
                if (report != null) {
                    sb.append(" ").append(report);
                }
                if (mWifiScoreReport != null) {
                    sb.append(mWifiScoreReport.getReport());
                }
                if (mConnectedModeGScanOffloadStarted) {
                    sb.append(" offload-started periodMilli " + mGScanPeriodMilli);
                } else {
                    sb.append(" offload-stopped");
                }
                break;
            case CMD_AUTO_CONNECT:
            case WifiManager.CONNECT_NETWORK:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                config = mWifiConfigManager.getWifiConfiguration(msg.arg1);
                if (config != null) {
                    sb.append(" ").append(config.configKey());
                    if (config.visibility != null) {
                        sb.append(" ").append(config.visibility.toString());
                    }
                }
                if (mTargetRoamBSSID != null) {
                    sb.append(" ").append(mTargetRoamBSSID);
                }
                sb.append(" roam=").append(Boolean.toString(mAutoRoaming));
                config = getCurrentWifiConfiguration();
                if (config != null) {
                    sb.append(config.configKey());
                    if (config.visibility != null) {
                        sb.append(" ").append(config.visibility.toString());
                    }
                }
                break;
            case CMD_AUTO_ROAM:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                ScanResult result = (ScanResult) msg.obj;
                if (result != null) {
                    now = System.currentTimeMillis();
                    sb.append(" bssid=").append(result.BSSID);
                    sb.append(" rssi=").append(result.level);
                    sb.append(" freq=").append(result.frequency);
                    if (result.seen > 0 && result.seen < now) {
                        sb.append(" seen=").append(now - result.seen);
                    } else {
                        // Somehow the timestamp for this scan result is inconsistent
                        sb.append(" !seen=").append(result.seen);
                    }
                }
                if (mTargetRoamBSSID != null) {
                    sb.append(" ").append(mTargetRoamBSSID);
                }
                sb.append(" roam=").append(Boolean.toString(mAutoRoaming));
                sb.append(" fail count=").append(Integer.toString(mRoamFailCount));
                break;
            case CMD_ADD_OR_UPDATE_NETWORK:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (msg.obj != null) {
                    config = (WifiConfiguration) msg.obj;
                    sb.append(" ").append(config.configKey());
                    sb.append(" prio=").append(config.priority);
                    sb.append(" status=").append(config.status);
                    if (config.BSSID != null) {
                        sb.append(" ").append(config.BSSID);
                    }
                    WifiConfiguration curConfig = getCurrentWifiConfiguration();
                    if (curConfig != null) {
                        if (curConfig.configKey().equals(config.configKey())) {
                            sb.append(" is current");
                        } else {
                            sb.append(" current=").append(curConfig.configKey());
                            sb.append(" prio=").append(curConfig.priority);
                            sb.append(" status=").append(curConfig.status);
                        }
                    }
                }
                break;
            case WifiManager.DISABLE_NETWORK:
            case CMD_ENABLE_NETWORK:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                key = mWifiConfigManager.getLastSelectedConfiguration();
                if (key != null) {
                    sb.append(" last=").append(key);
                }
                config = mWifiConfigManager.getWifiConfiguration(msg.arg1);
                if (config != null && (key == null || !config.configKey().equals(key))) {
                    sb.append(" target=").append(key);
                }
                break;
            case CMD_GET_CONFIGURED_NETWORKS:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" num=").append(mWifiConfigManager.getConfiguredNetworksSize());
                break;
            case DhcpClient.CMD_PRE_DHCP_ACTION:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" txpkts=").append(mWifiInfo.txSuccess);
                sb.append(",").append(mWifiInfo.txBad);
                sb.append(",").append(mWifiInfo.txRetries);
                break;
            case DhcpClient.CMD_POST_DHCP_ACTION:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (msg.arg1 == DhcpClient.DHCP_SUCCESS) {
                    sb.append(" OK ");
                } else if (msg.arg1 == DhcpClient.DHCP_FAILURE) {
                    sb.append(" FAIL ");
                }
                if (mLinkProperties != null) {
                    sb.append(" ");
                    sb.append(getLinkPropertiesSummary(mLinkProperties));
                }
                break;
            case WifiP2pServiceImpl.P2P_CONNECTION_CHANGED:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (msg.obj != null) {
                    NetworkInfo info = (NetworkInfo) msg.obj;
                    NetworkInfo.State state = info.getState();
                    NetworkInfo.DetailedState detailedState = info.getDetailedState();
                    if (state != null) {
                        sb.append(" st=").append(state);
                    }
                    if (detailedState != null) {
                        sb.append("/").append(detailedState);
                    }
                }
                break;
            case CMD_IP_CONFIGURATION_LOST:
                int count = -1;
                WifiConfiguration c = getCurrentWifiConfiguration();
                if (c != null) {
                    count = c.getNetworkSelectionStatus().getDisableReasonCounter(
                            WifiConfiguration.NetworkSelectionStatus.DISABLED_DHCP_FAILURE);
                }
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" failures: ");
                sb.append(Integer.toString(count));
                sb.append("/");
                sb.append(Integer.toString(mWifiConfigManager.getMaxDhcpRetries()));
                if (mWifiInfo.getBSSID() != null) {
                    sb.append(" ").append(mWifiInfo.getBSSID());
                }
                sb.append(String.format(" bcn=%d", mRunningBeaconCount));
                break;
            case CMD_UPDATE_LINKPROPERTIES:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                if (mLinkProperties != null) {
                    sb.append(" ");
                    sb.append(getLinkPropertiesSummary(mLinkProperties));
                }
                break;
            case CMD_IP_REACHABILITY_LOST:
                if (msg.obj != null) {
                    sb.append(" ").append((String) msg.obj);
                }
                break;
            case CMD_INSTALL_PACKET_FILTER:
                sb.append(" len=" + ((byte[])msg.obj).length);
                break;
            case CMD_SET_FALLBACK_PACKET_FILTERING:
                sb.append(" enabled=" + (boolean)msg.obj);
                break;
            case CMD_ROAM_WATCHDOG_TIMER:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" cur=").append(roamWatchdogCount);
                break;
            case CMD_DISCONNECTING_WATCHDOG_TIMER:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                sb.append(" cur=").append(disconnectingWatchdogCount);
                break;
            case CMD_START_RSSI_MONITORING_OFFLOAD:
            case CMD_STOP_RSSI_MONITORING_OFFLOAD:
            case CMD_RSSI_THRESHOLD_BREACH:
                sb.append(" rssi=");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" thresholds=");
                sb.append(Arrays.toString(mRssiRanges));
                break;
            case CMD_USER_SWITCH:
                sb.append(" userId=");
                sb.append(Integer.toString(msg.arg1));
                break;
            case CMD_IPV4_PROVISIONING_SUCCESS:
                sb.append(" ");
                if (msg.arg1 == DhcpClient.DHCP_SUCCESS) {
                    sb.append("DHCP_OK");
                } else if (msg.arg1 == CMD_STATIC_IP_SUCCESS) {
                    sb.append("STATIC_OK");
                } else {
                    sb.append(Integer.toString(msg.arg1));
                }
                break;
            case CMD_IPV4_PROVISIONING_FAILURE:
                sb.append(" ");
                if (msg.arg1 == DhcpClient.DHCP_FAILURE) {
                    sb.append("DHCP_FAIL");
                } else if (msg.arg1 == CMD_STATIC_IP_FAILURE) {
                    sb.append("STATIC_FAIL");
                } else {
                    sb.append(Integer.toString(msg.arg1));
                }
                break;
            default:
                sb.append(" ");
                sb.append(Integer.toString(msg.arg1));
                sb.append(" ");
                sb.append(Integer.toString(msg.arg2));
                break;
        }

        return sb.toString();
    }

    private void handleScreenStateChanged(boolean screenOn) {
        mScreenOn = screenOn;
        if (DBG) {
            logd(" handleScreenStateChanged Enter: screenOn=" + screenOn
                    + " mUserWantsSuspendOpt=" + mUserWantsSuspendOpt
                    + " state " + getCurrentState().getName()
                    + " suppState:" + mSupplicantStateTracker.getSupplicantStateName());
        }
        enableRssiPolling(screenOn);
        if (mUserWantsSuspendOpt.get()) {
            int shouldReleaseWakeLock = 0;
            if (screenOn) {
                sendMessage(CMD_SET_SUSPEND_OPT_ENABLED, 0, shouldReleaseWakeLock);
            } else {
                if (isConnected()) {
                    // Allow 2s for suspend optimizations to be set
                    mSuspendWakeLock.acquire(2000);
                    shouldReleaseWakeLock = 1;
                }
                sendMessage(CMD_SET_SUSPEND_OPT_ENABLED, 1, shouldReleaseWakeLock);
            }
        }
        mScreenBroadcastReceived.set(true);

        getWifiLinkLayerStats(false);
        mOnTimeScreenStateChange = mOnTime;
        lastScreenStateChangeTimeStamp = lastLinkLayerStatsUpdate;

        mWifiMetrics.setScreenState(screenOn);

        if (mWifiConnectivityManager != null) {
            mWifiConnectivityManager.handleScreenStateChanged(screenOn);
        }

        if (DBG) log("handleScreenStateChanged Exit: " + screenOn);
    }

    private void checkAndSetConnectivityInstance() {
        if (mCm == null) {
            mCm = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        }
    }


    /**
     * Set the frequency band from the system setting value, if any.
     */
    private void setFrequencyBand() {
        int band = WifiManager.WIFI_FREQUENCY_BAND_AUTO;

        if (mWifiNative.setBand(band)) {
            mFrequencyBand.set(band);
            if (mWifiConnectivityManager != null) {
                mWifiConnectivityManager.setUserPreferredBand(band);
            }
            if (DBG) {
                logd("done set frequency band " + band);
            }
        } else {
            loge("Failed to set frequency band " + band);
        }
    }

    private void setSuspendOptimizationsNative(int reason, boolean enabled) {
        if (DBG) {
            log("setSuspendOptimizationsNative: " + reason + " " + enabled
                    + " -want " + mUserWantsSuspendOpt.get()
                    + " stack:" + Thread.currentThread().getStackTrace()[2].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[3].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[4].getMethodName()
                    + " - " + Thread.currentThread().getStackTrace()[5].getMethodName());
        }
        //mWifiNative.setSuspendOptimizations(enabled);

        if (enabled) {
            mSuspendOptNeedsDisabled &= ~reason;
            /* None of dhcp, screen or highperf need it disabled and user wants it enabled */
            if (mSuspendOptNeedsDisabled == 0 && mUserWantsSuspendOpt.get()) {
                if (DBG) {
                    log("setSuspendOptimizationsNative do it " + reason + " " + enabled
                            + " stack:" + Thread.currentThread().getStackTrace()[2].getMethodName()
                            + " - " + Thread.currentThread().getStackTrace()[3].getMethodName()
                            + " - " + Thread.currentThread().getStackTrace()[4].getMethodName()
                            + " - " + Thread.currentThread().getStackTrace()[5].getMethodName());
                }
                mWifiNative.setSuspendOptimizations(true);
            }
        } else {
            mSuspendOptNeedsDisabled |= reason;
            mWifiNative.setSuspendOptimizations(false);
        }
    }

    private void setSuspendOptimizations(int reason, boolean enabled) {
        if (DBG) log("setSuspendOptimizations: " + reason + " " + enabled);
        if (enabled) {
            mSuspendOptNeedsDisabled &= ~reason;
        } else {
            mSuspendOptNeedsDisabled |= reason;
        }
        if (DBG) log("mSuspendOptNeedsDisabled " + mSuspendOptNeedsDisabled);
    }

    private void setWifiState(int wifiState) {
        final int previousWifiState = mWifiState.get();

        try {
            if (wifiState == WIFI_STATE_ENABLED) {
                mBatteryStats.noteWifiOn();
            } else if (wifiState == WIFI_STATE_DISABLED) {
                mBatteryStats.noteWifiOff();
            }
        } catch (RemoteException e) {
            loge("Failed to note battery stats in wifi");
        }

        mWifiState.set(wifiState);

        if (DBG) log("setWifiState: " + syncGetWifiStateByName());

        final Intent intent = new Intent(WifiManager.WIFI_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_WIFI_STATE, wifiState);
        intent.putExtra(WifiManager.EXTRA_PREVIOUS_WIFI_STATE, previousWifiState);
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    private void setWifiApState(int wifiApState, int reason) {
        final int previousWifiApState = mWifiApState.get();

        try {
            if (wifiApState == WIFI_AP_STATE_ENABLED) {
                mBatteryStats.noteWifiOn();
            } else if (wifiApState == WIFI_AP_STATE_DISABLED) {
                mBatteryStats.noteWifiOff();
            }
        } catch (RemoteException e) {
            loge("Failed to note battery stats in wifi");
        }

        // Update state
        mWifiApState.set(wifiApState);

        if (DBG) log("setWifiApState: " + syncGetWifiApStateByName());

        final Intent intent = new Intent(WifiManager.WIFI_AP_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_WIFI_AP_STATE, wifiApState);
        intent.putExtra(WifiManager.EXTRA_PREVIOUS_WIFI_AP_STATE, previousWifiApState);
        if (wifiApState == WifiManager.WIFI_AP_STATE_FAILED) {
            //only set reason number when softAP start failed
            intent.putExtra(WifiManager.EXTRA_WIFI_AP_FAILURE_REASON, reason);
        }

        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    private void setScanResults() {
        mNumScanResultsKnown = 0;
        mNumScanResultsReturned = 0;

        ArrayList<ScanDetail> scanResults = mWifiNative.getScanResults();

        if (scanResults.isEmpty()) {
            mScanResults = new ArrayList<>();
            return;
        }

        mWifiConfigManager.trimANQPCache(false);

        boolean connected = mLastBssid != null;
        long activeBssid = 0L;
        if (connected) {
            try {
                activeBssid = Utils.parseMac(mLastBssid);
            } catch (IllegalArgumentException iae) {
                connected = false;
            }
        }

        synchronized (mScanResultsLock) {
            ScanDetail activeScanDetail = null;
            mScanResults = scanResults;
            mNumScanResultsReturned = mScanResults.size();
            for (ScanDetail resultDetail : mScanResults) {
                if (connected && resultDetail.getNetworkDetail().getBSSID() == activeBssid) {
                    if (activeScanDetail == null
                            || activeScanDetail.getNetworkDetail().getBSSID() != activeBssid
                            || activeScanDetail.getNetworkDetail().getANQPElements() == null) {
                        activeScanDetail = resultDetail;
                    }
                }
                // Cache DTIM values parsed from the beacon frame Traffic Indication Map (TIM)
                // Information Element (IE), into the associated WifiConfigurations. Most of the
                // time there is no TIM IE in the scan result (Probe Response instead of Beacon
                // Frame), these scanResult DTIM's are negative and ignored.
                // <TODO> Cache these per BSSID, since dtim can change vary
                NetworkDetail networkDetail = resultDetail.getNetworkDetail();
                if (networkDetail != null && networkDetail.getDtimInterval() > 0) {
                    List<WifiConfiguration> associatedWifiConfigurations =
                            mWifiConfigManager.getSavedNetworkFromScanDetail(resultDetail);
                    if (associatedWifiConfigurations != null) {
                        for (WifiConfiguration associatedConf : associatedWifiConfigurations) {
                            if (associatedConf != null) {
                                associatedConf.dtimInterval = networkDetail.getDtimInterval();
                            }
                        }
                    }
                }
            }
            mWifiConfigManager.setActiveScanDetail(activeScanDetail);
        }

        if (linkDebouncing) {
            // If debouncing, we dont re-select a SSID or BSSID hence
            // there is no need to call the network selection code
            // in WifiAutoJoinController, instead,
            // just try to reconnect to the same SSID by triggering a roam
            // The third parameter 1 means roam not from network selection but debouncing
            sendMessage(CMD_AUTO_ROAM, mLastNetworkId, 1, null);
        }
    }

    /*
     * Fetch RSSI, linkspeed, and frequency on current connection
     */
    private void fetchRssiLinkSpeedAndFrequencyNative() {
        Integer newRssi = null;
        Integer newLinkSpeed = null;
        Integer newFrequency = null;

        String signalPoll = mWifiNative.signalPoll();

        if (signalPoll != null) {
            String[] lines = signalPoll.split("\n");
            for (String line : lines) {
                String[] prop = line.split("=");
                if (prop.length < 2) continue;
                try {
                    if (prop[0].equals("RSSI")) {
                        newRssi = Integer.parseInt(prop[1]);
                    } else if (prop[0].equals("LINKSPEED")) {
                        newLinkSpeed = Integer.parseInt(prop[1]);
                    } else if (prop[0].equals("FREQUENCY")) {
                        newFrequency = Integer.parseInt(prop[1]);
                    }
                } catch (NumberFormatException e) {
                    //Ignore, defaults on rssi and linkspeed are assigned
                }
            }
        }

        if (DBG) {
            logd("fetchRssiLinkSpeedAndFrequencyNative rssi=" + newRssi +
                 " linkspeed=" + newLinkSpeed + " freq=" + newFrequency);
        }

        if (newRssi != null && newRssi > WifiInfo.INVALID_RSSI && newRssi < WifiInfo.MAX_RSSI) {
            // screen out invalid values
            /* some implementations avoid negative values by adding 256
             * so we need to adjust for that here.
             */
            if (newRssi > 0) newRssi -= 256;
            mWifiInfo.setRssi(newRssi);
            /*
             * Log the rssi poll value in metrics
             */
            mWifiMetrics.incrementRssiPollRssiCount(newRssi);
            /*
             * Rather then sending the raw RSSI out every time it
             * changes, we precalculate the signal level that would
             * be displayed in the status bar, and only send the
             * broadcast if that much more coarse-grained number
             * changes. This cuts down greatly on the number of
             * broadcasts, at the cost of not informing others
             * interested in RSSI of all the changes in signal
             * level.
             */
            int newSignalLevel = WifiManager.calculateSignalLevel(newRssi, WifiManager.RSSI_LEVELS);
            if (newSignalLevel != mLastSignalLevel) {
                updateCapabilities(getCurrentWifiConfiguration());
                sendRssiChangeBroadcast(newRssi);
            }
            mLastSignalLevel = newSignalLevel;
        } else {
            mWifiInfo.setRssi(WifiInfo.INVALID_RSSI);
            updateCapabilities(getCurrentWifiConfiguration());
        }

        if (newLinkSpeed != null) {
            mWifiInfo.setLinkSpeed(newLinkSpeed);
        }
        if (newFrequency != null && newFrequency > 0) {
            if (ScanResult.is5GHz(newFrequency)) {
                mWifiConnectionStatistics.num5GhzConnected++;
            }
            if (ScanResult.is24GHz(newFrequency)) {
                mWifiConnectionStatistics.num24GhzConnected++;
            }
            mWifiInfo.setFrequency(newFrequency);
        }
        mWifiConfigManager.updateConfiguration(mWifiInfo);
    }

    // Polling has completed, hence we wont have a score anymore
    private void cleanWifiScore() {
        mWifiInfo.txBadRate = 0;
        mWifiInfo.txSuccessRate = 0;
        mWifiInfo.txRetriesRate = 0;
        mWifiInfo.rxSuccessRate = 0;
        mWifiScoreReport = null;
    }

    // Object holding most recent wifi score report and bad Linkspeed count
    WifiScoreReport mWifiScoreReport = null;

    public double getTxPacketRate() {
        return mWifiInfo.txSuccessRate;
    }

    public double getRxPacketRate() {
        return mWifiInfo.rxSuccessRate;
    }

    /**
     * Fetch TX packet counters on current connection
     */
    private void fetchPktcntNative(RssiPacketCountInfo info) {
        String pktcntPoll = mWifiNative.pktcntPoll();

        if (pktcntPoll != null) {
            String[] lines = pktcntPoll.split("\n");
            for (String line : lines) {
                String[] prop = line.split("=");
                if (prop.length < 2) continue;
                try {
                    if (prop[0].equals("TXGOOD")) {
                        info.txgood = Integer.parseInt(prop[1]);
                    } else if (prop[0].equals("TXBAD")) {
                        info.txbad = Integer.parseInt(prop[1]);
                    }
                } catch (NumberFormatException e) {
                    // Ignore
                }
            }
        }
    }

    private void updateLinkProperties(LinkProperties newLp) {
        if (DBG) {
            log("Link configuration changed for netId: " + mLastNetworkId
                    + " old: " + mLinkProperties + " new: " + newLp);
        }
        // We own this instance of LinkProperties because IpManager passes us a copy.
        mLinkProperties = newLp;
        if (mNetworkAgent != null) {
            mNetworkAgent.sendLinkProperties(mLinkProperties);
        }

        if (getNetworkDetailedState() == DetailedState.CONNECTED) {
            // If anything has changed and we're already connected, send out a notification.
            // TODO: Update all callers to use NetworkCallbacks and delete this.
            sendLinkConfigurationChangedBroadcast();
        }

        if (DBG) {
            StringBuilder sb = new StringBuilder();
            sb.append("updateLinkProperties nid: " + mLastNetworkId);
            sb.append(" state: " + getNetworkDetailedState());

            if (mLinkProperties != null) {
                sb.append(" ");
                sb.append(getLinkPropertiesSummary(mLinkProperties));
            }
            logd(sb.toString());
        }
    }

    /**
     * Clears all our link properties.
     */
    private void clearLinkProperties() {
        // Clear the link properties obtained from DHCP. The only caller of this
        // function has already called IpManager#stop(), which clears its state.
        synchronized (mDhcpResultsLock) {
            if (mDhcpResults != null) {
                mDhcpResults.clear();
            }
        }

        // Now clear the merged link properties.
        mLinkProperties.clear();
        if (mNetworkAgent != null) mNetworkAgent.sendLinkProperties(mLinkProperties);
    }

    /**
     * try to update default route MAC address.
     */
    private String updateDefaultRouteMacAddress(int timeout) {
        String address = null;
        for (RouteInfo route : mLinkProperties.getRoutes()) {
            if (route.isDefaultRoute() && route.hasGateway()) {
                InetAddress gateway = route.getGateway();
                if (gateway instanceof Inet4Address) {
                    if (DBG) {
                        logd("updateDefaultRouteMacAddress found Ipv4 default :"
                                + gateway.getHostAddress());
                    }
                    address = macAddressFromRoute(gateway.getHostAddress());
                    /* The gateway's MAC address is known */
                    if ((address == null) && (timeout > 0)) {
                        boolean reachable = false;
                        try {
                            reachable = gateway.isReachable(timeout);
                        } catch (Exception e) {
                            loge("updateDefaultRouteMacAddress exception reaching :"
                                    + gateway.getHostAddress());

                        } finally {
                            if (reachable == true) {

                                address = macAddressFromRoute(gateway.getHostAddress());
                                if (DBG) {
                                    logd("updateDefaultRouteMacAddress reachable (tried again) :"
                                            + gateway.getHostAddress() + " found " + address);
                                }
                            }
                        }
                    }
                    if (address != null) {
                        mWifiConfigManager.setDefaultGwMacAddress(mLastNetworkId, address);
                    }
                }
            }
        }
        return address;
    }

    void sendScanResultsAvailableBroadcast(boolean scanSucceeded) {
        Intent intent = new Intent(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_RESULTS_UPDATED, scanSucceeded);
        mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
    }

    private void sendRssiChangeBroadcast(final int newRssi) {
        try {
            mBatteryStats.noteWifiRssiChanged(newRssi);
        } catch (RemoteException e) {
            // Won't happen.
        }
        Intent intent = new Intent(WifiManager.RSSI_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_NEW_RSSI, newRssi);
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    private void sendNetworkStateChangeBroadcast(String bssid) {
        Intent intent = new Intent(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_NETWORK_INFO, new NetworkInfo(mNetworkInfo));
        intent.putExtra(WifiManager.EXTRA_LINK_PROPERTIES, new LinkProperties(mLinkProperties));
        if (bssid != null)
            intent.putExtra(WifiManager.EXTRA_BSSID, bssid);
        if (mNetworkInfo.getDetailedState() == DetailedState.VERIFYING_POOR_LINK ||
                mNetworkInfo.getDetailedState() == DetailedState.CONNECTED) {
            // We no longer report MAC address to third-parties and our code does
            // not rely on this broadcast, so just send the default MAC address.
            fetchRssiLinkSpeedAndFrequencyNative();
            WifiInfo sentWifiInfo = new WifiInfo(mWifiInfo);
            sentWifiInfo.setMacAddress(WifiInfo.DEFAULT_MAC_ADDRESS);
            intent.putExtra(WifiManager.EXTRA_WIFI_INFO, sentWifiInfo);
        }
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    private WifiInfo getWiFiInfoForUid(int uid) {
        if (Binder.getCallingUid() == Process.myUid()) {
            return mWifiInfo;
        }

        WifiInfo result = new WifiInfo(mWifiInfo);
        result.setMacAddress(WifiInfo.DEFAULT_MAC_ADDRESS);

        IBinder binder = mFacade.getService("package");
        IPackageManager packageManager = IPackageManager.Stub.asInterface(binder);

        try {
            if (packageManager.checkUidPermission(Manifest.permission.LOCAL_MAC_ADDRESS,
                    uid) == PackageManager.PERMISSION_GRANTED) {
                result.setMacAddress(mWifiInfo.getMacAddress());
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Error checking receiver permission", e);
        }

        return result;
    }

    private void sendLinkConfigurationChangedBroadcast() {
        Intent intent = new Intent(WifiManager.LINK_CONFIGURATION_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_LINK_PROPERTIES, new LinkProperties(mLinkProperties));
        mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
    }

    private void sendSupplicantConnectionChangedBroadcast(boolean connected) {
        Intent intent = new Intent(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
        intent.putExtra(WifiManager.EXTRA_SUPPLICANT_CONNECTED, connected);
        mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
    }

    /**
     * Record the detailed state of a network.
     *
     * @param state the new {@code DetailedState}
     */
    private boolean setNetworkDetailedState(NetworkInfo.DetailedState state) {
        boolean hidden = false;

        if (linkDebouncing || isRoaming()) {
            // There is generally a confusion in the system about colluding
            // WiFi Layer 2 state (as reported by supplicant) and the Network state
            // which leads to multiple confusion.
            //
            // If link is de-bouncing or roaming, we already have an IP address
            // as well we were connected and are doing L2 cycles of
            // reconnecting or renewing IP address to check that we still have it
            // This L2 link flapping should ne be reflected into the Network state
            // which is the state of the WiFi Network visible to Layer 3 and applications
            // Note that once debouncing and roaming are completed, we will
            // set the Network state to where it should be, or leave it as unchanged
            //
            hidden = true;
        }
        if (DBG) {
            log("setDetailed state, old ="
                    + mNetworkInfo.getDetailedState() + " and new state=" + state
                    + " hidden=" + hidden);
        }
        if (mNetworkInfo.getExtraInfo() != null && mWifiInfo.getSSID() != null
                && !mWifiInfo.getSSID().equals(WifiSsid.NONE)) {
            // Always indicate that SSID has changed
            if (!mNetworkInfo.getExtraInfo().equals(mWifiInfo.getSSID())) {
                if (DBG) {
                    log("setDetailed state send new extra info" + mWifiInfo.getSSID());
                }
                mNetworkInfo.setExtraInfo(mWifiInfo.getSSID());
                sendNetworkStateChangeBroadcast(null);
            }
        }
        if (hidden == true) {
            return false;
        }

        if (state != mNetworkInfo.getDetailedState()) {
            mNetworkInfo.setDetailedState(state, null, mWifiInfo.getSSID());
            if (mNetworkAgent != null) {
                mNetworkAgent.sendNetworkInfo(mNetworkInfo);
            }
            sendNetworkStateChangeBroadcast(null);
            return true;
        }
        return false;
    }

    private DetailedState getNetworkDetailedState() {
        return mNetworkInfo.getDetailedState();
    }

    private SupplicantState handleSupplicantStateChange(Message message) {
        StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
        SupplicantState state = stateChangeResult.state;
        // Supplicant state change
        // [31-13] Reserved for future use
        // [8 - 0] Supplicant state (as defined in SupplicantState.java)
        // 50023 supplicant_state_changed (custom|1|5)
        mWifiInfo.setSupplicantState(state);
        // If we receive a supplicant state change with an empty SSID,
        // this implies that wpa_supplicant is already disconnected.
        // We should pretend we are still connected when linkDebouncing is on.
        if ((stateChangeResult.wifiSsid == null
                || stateChangeResult.wifiSsid.toString().isEmpty()) && linkDebouncing) {
            return state;
        }
        // Network id is only valid when we start connecting
        if (SupplicantState.isConnecting(state)) {
            mWifiInfo.setNetworkId(stateChangeResult.networkId);
        } else {
            mWifiInfo.setNetworkId(WifiConfiguration.INVALID_NETWORK_ID);
        }

        mWifiInfo.setBSSID(stateChangeResult.BSSID);

        if (mWhiteListedSsids != null
                && mWhiteListedSsids.length > 0
                && stateChangeResult.wifiSsid != null) {
            String SSID = stateChangeResult.wifiSsid.toString();
            String currentSSID = mWifiInfo.getSSID();
            if (SSID != null && currentSSID != null && !SSID.equals(WifiSsid.NONE)) {
                // Remove quote before comparing
                if (SSID.length() >= 2 && SSID.charAt(0) == '"'
                        && SSID.charAt(SSID.length() - 1) == '"') {
                    SSID = SSID.substring(1, SSID.length() - 1);
                }
                if (currentSSID.length() >= 2 && currentSSID.charAt(0) == '"'
                        && currentSSID.charAt(currentSSID.length() - 1) == '"') {
                    currentSSID = currentSSID.substring(1, currentSSID.length() - 1);
                }
                if ((!SSID.equals(currentSSID)) && (getCurrentState() == mConnectedState)) {
                    lastConnectAttemptTimestamp = System.currentTimeMillis();
                    targetWificonfiguration =
                            mWifiConfigManager.getWifiConfiguration(mWifiInfo.getNetworkId());
                    transitionTo(mRoamingState);
                }
            }
        }

        mWifiInfo.setSSID(stateChangeResult.wifiSsid);
        mWifiInfo.setEphemeral(mWifiConfigManager.isEphemeral(mWifiInfo.getNetworkId()));
        if (!mWifiInfo.getMeteredHint()) { // don't override the value if already set.
            mWifiInfo.setMeteredHint(mWifiConfigManager.getMeteredHint(mWifiInfo.getNetworkId()));
        }

        mSupplicantStateTracker.sendMessage(Message.obtain(message));

        return state;
    }

    /**
     * Resets the Wi-Fi Connections by clearing any state, resetting any sockets
     * using the interface, stopping DHCP & disabling interface
     */
    private void handleNetworkDisconnect() {
        if (DBG) log("handleNetworkDisconnect: Stopping DHCP and clearing IP"
                + " stack:" + Thread.currentThread().getStackTrace()[2].getMethodName()
                + " - " + Thread.currentThread().getStackTrace()[3].getMethodName()
                + " - " + Thread.currentThread().getStackTrace()[4].getMethodName()
                + " - " + Thread.currentThread().getStackTrace()[5].getMethodName());

        stopRssiMonitoringOffload();

        clearCurrentConfigBSSID("handleNetworkDisconnect");

        stopIpManager();

        /* Reset data structures */
        mWifiScoreReport = null;
        mWifiInfo.reset();
        linkDebouncing = false;
        /* Reset roaming parameters */
        mAutoRoaming = false;

        setNetworkDetailedState(DetailedState.DISCONNECTED);
        if (mNetworkAgent != null) {
            mNetworkAgent.sendNetworkInfo(mNetworkInfo);
            mNetworkAgent = null;
        }
        mWifiConfigManager.updateStatus(mLastNetworkId, DetailedState.DISCONNECTED);

        /* Clear network properties */
        clearLinkProperties();

        /* Cend event to CM & network change broadcast */
        sendNetworkStateChangeBroadcast(mLastBssid);

        /* Cancel auto roam requests */
        autoRoamSetBSSID(mLastNetworkId, "any");
        mLastBssid = null;
        registerDisconnected();
        mLastNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
    }

    private void handleSupplicantConnectionLoss(boolean killSupplicant) {
        /* Socket connection can be lost when we do a graceful shutdown
        * or when the driver is hung. Ensure supplicant is stopped here.
        */
        if (killSupplicant) {
            mWifiMonitor.killSupplicant(mP2pSupported);
        }
        mWifiNative.closeSupplicantConnection();
        sendSupplicantConnectionChangedBroadcast(false);
        setWifiState(WIFI_STATE_DISABLED);
    }

    void handlePreDhcpSetup() {
        if (!mBluetoothConnectionActive) {
            /*
             * There are problems setting the Wi-Fi driver's power
             * mode to active when bluetooth coexistence mode is
             * enabled or sense.
             * <p>
             * We set Wi-Fi to active mode when
             * obtaining an IP address because we've found
             * compatibility issues with some routers with low power
             * mode.
             * <p>
             * In order for this active power mode to properly be set,
             * we disable coexistence mode until we're done with
             * obtaining an IP address.  One exception is if we
             * are currently connected to a headset, since disabling
             * coexistence would interrupt that connection.
             */
            // Disable the coexistence mode
            mWifiNative.setBluetoothCoexistenceMode(
                    mWifiNative.BLUETOOTH_COEXISTENCE_MODE_DISABLED);
        }

        // Disable power save and suspend optimizations during DHCP
        // Note: The order here is important for now. Brcm driver changes
        // power settings when we control suspend mode optimizations.
        // TODO: Remove this comment when the driver is fixed.
        setSuspendOptimizationsNative(SUSPEND_DUE_TO_DHCP, false);
        mWifiNative.setPowerSave(false);

        // Update link layer stats
        getWifiLinkLayerStats(false);

        /* P2p discovery breaks dhcp, shut it down in order to get through this */
        Message msg = new Message();
        msg.what = WifiP2pServiceImpl.BLOCK_DISCOVERY;
        msg.arg1 = WifiP2pServiceImpl.ENABLED;
        msg.arg2 = DhcpClient.CMD_PRE_DHCP_ACTION_COMPLETE;
        msg.obj = WifiStateMachine.this;
        mWifiP2pChannel.sendMessage(msg);
    }

    void handlePostDhcpSetup() {
        /* Restore power save and suspend optimizations */
        setSuspendOptimizationsNative(SUSPEND_DUE_TO_DHCP, true);
        mWifiNative.setPowerSave(true);

        mWifiP2pChannel.sendMessage(WifiP2pServiceImpl.BLOCK_DISCOVERY,
                WifiP2pServiceImpl.DISABLED);

        // Set the coexistence mode back to its default value
        mWifiNative.setBluetoothCoexistenceMode(
                mWifiNative.BLUETOOTH_COEXISTENCE_MODE_SENSE);
    }

    /**
     * Inform other components (WifiMetrics, WifiLogger, etc.) that the current connection attempt
     * has concluded.
     */
    private void reportConnectionAttemptEnd(int level2FailureCode, int connectivityFailureCode) {
        mWifiMetrics.endConnectionEvent(level2FailureCode, connectivityFailureCode);
        switch (level2FailureCode) {
            case WifiMetrics.ConnectionEvent.FAILURE_NONE:
            case WifiMetrics.ConnectionEvent.FAILURE_REDUNDANT_CONNECTION_ATTEMPT:
                // WifiLogger doesn't care about success, or pre-empted connections.
                break;
            default:
                mWifiLogger.reportConnectionFailure();
        }
    }

    private void handleIPv4Success(DhcpResults dhcpResults) {
        if (DBG) {
            logd("handleIPv4Success <" + dhcpResults.toString() + ">");
            logd("link address " + dhcpResults.ipAddress);
        }

        Inet4Address addr;
        synchronized (mDhcpResultsLock) {
            mDhcpResults = dhcpResults;
            addr = (Inet4Address) dhcpResults.ipAddress.getAddress();
        }

        if (isRoaming()) {
            int previousAddress = mWifiInfo.getIpAddress();
            int newAddress = NetworkUtils.inetAddressToInt(addr);
            if (previousAddress != newAddress) {
                logd("handleIPv4Success, roaming and address changed" +
                        mWifiInfo + " got: " + addr);
            }
        }
        mWifiInfo.setInetAddress(addr);
        if (!mWifiInfo.getMeteredHint()) { // don't override the value if already set.
            mWifiInfo.setMeteredHint(dhcpResults.hasMeteredHint());
            updateCapabilities(getCurrentWifiConfiguration());
        }
    }

    private void handleSuccessfulIpConfiguration() {
        mLastSignalLevel = -1; // Force update of signal strength
        WifiConfiguration c = getCurrentWifiConfiguration();
        if (c != null) {
            // Reset IP failure tracking
            c.getNetworkSelectionStatus().clearDisableReasonCounter(
                    WifiConfiguration.NetworkSelectionStatus.DISABLED_DHCP_FAILURE);

            // Tell the framework whether the newly connected network is trusted or untrusted.
            updateCapabilities(c);
        }
        if (c != null) {
            ScanResult result = getCurrentScanResult();
            if (result == null) {
                logd("WifiStateMachine: handleSuccessfulIpConfiguration and no scan results" +
                        c.configKey());
            } else {
                // Clear the per BSSID failure count
                result.numIpConfigFailures = 0;
                // Clear the WHOLE BSSID blacklist, which means supplicant is free to retry
                // any BSSID, even though it may already have a non zero ip failure count,
                // this will typically happen if the user walks away and come back to his arrea
                // TODO: implement blacklisting based on a timer, i.e. keep BSSID blacklisted
                // in supplicant for a couple of hours or a day
                mWifiConfigManager.clearBssidBlacklist();
            }
        }
    }

    private void handleIPv4Failure() {
        // TODO: Move this to provisioning failure, not DHCP failure.
        // DHCPv4 failure is expected on an IPv6-only network.
        mWifiLogger.captureBugReportData(WifiLogger.REPORT_REASON_DHCP_FAILURE);
        if (DBG) {
            int count = -1;
            WifiConfiguration config = getCurrentWifiConfiguration();
            if (config != null) {
                count = config.getNetworkSelectionStatus().getDisableReasonCounter(
                        WifiConfiguration.NetworkSelectionStatus.DISABLED_DHCP_FAILURE);
            }
            log("DHCP failure count=" + count);
        }
        reportConnectionAttemptEnd(
                WifiMetrics.ConnectionEvent.FAILURE_DHCP,
                WifiMetricsProto.ConnectionEvent.HLF_DHCP);
        synchronized(mDhcpResultsLock) {
             if (mDhcpResults != null) {
                 mDhcpResults.clear();
             }
        }
        if (DBG) {
            logd("handleIPv4Failure");
        }
    }

    private void handleIpConfigurationLost() {
        mWifiInfo.setInetAddress(null);
        mWifiInfo.setMeteredHint(false);

        mWifiConfigManager.updateNetworkSelectionStatus(mLastNetworkId,
                WifiConfiguration.NetworkSelectionStatus.DISABLED_DHCP_FAILURE);

        /* DHCP times out after about 30 seconds, we do a
         * disconnect thru supplicant, we will let autojoin retry connecting to the network
         */
        mWifiNative.disconnect();
    }

    // TODO: De-duplicated this and handleIpConfigurationLost().
    private void handleIpReachabilityLost() {
        mWifiInfo.setInetAddress(null);
        mWifiInfo.setMeteredHint(false);

        // TODO: Determine whether to call some form of mWifiConfigManager.handleSSIDStateChange().

        // Disconnect via supplicant, and let autojoin retry connecting to the network.
        mWifiNative.disconnect();
    }

    private int convertFrequencyToChannelNumber(int frequency) {
        if (frequency >= 2412 && frequency <= 2484) {
            return (frequency -2412) / 5 + 1;
        } else if (frequency >= 5170  &&  frequency <=5825) {
            //DFS is included
            return (frequency -5170) / 5 + 34;
        } else {
            return 0;
        }
    }

    private int chooseApChannel(int apBand) {
        int apChannel;
        int[] channel;

        if (apBand == 0)  {
            ArrayList<Integer> allowed2GChannel =
                    mWifiApConfigStore.getAllowed2GChannel();
            if (allowed2GChannel == null || allowed2GChannel.size() == 0) {
                //most safe channel to use
                if (DBG) {
                    Log.d(TAG, "No specified 2G allowed channel list");
                }
                apChannel = 6;
            } else {
                int index = mRandom.nextInt(allowed2GChannel.size());
                apChannel = allowed2GChannel.get(index).intValue();
            }
        } else {
            //5G without DFS
            channel = mWifiNative.getChannelsForBand(2);
            if (channel != null && channel.length > 0) {
                apChannel = channel[mRandom.nextInt(channel.length)];
                apChannel = convertFrequencyToChannelNumber(apChannel);
            } else {
                Log.e(TAG, "SoftAp do not get available channel list");
                apChannel = 0;
            }
        }

        if (DBG) {
            Log.d(TAG, "SoftAp set on channel " + apChannel);
        }

        return apChannel;
    }

    /* Driver/firmware setup for soft AP. */
    private boolean setupDriverForSoftAp() {
        if (!mWifiNative.loadDriver()) {
            Log.e(TAG, "Failed to load driver for softap");
            return false;
        }

        int index = mWifiNative.queryInterfaceIndex(mInterfaceName);
        if (index != -1) {
            if (!mWifiNative.setInterfaceUp(false)) {
                Log.e(TAG, "toggleInterface failed");
                return false;
            }
        } else {
            if (DBG) Log.d(TAG, "No interfaces to bring down");
        }

        try {
            mNwService.wifiFirmwareReload(mInterfaceName, "AP");
            if (DBG) Log.d(TAG, "Firmware reloaded in AP mode");
        } catch (Exception e) {
            Log.e(TAG, "Failed to reload AP firmware " + e);
        }

        if (!mWifiNative.startHal()) {
            /* starting HAL is optional */
            Log.e(TAG, "Failed to start HAL");
        }
        return true;
    }

    private byte[] macAddressFromString(String macString) {
        String[] macBytes = macString.split(":");
        if (macBytes.length != 6) {
            throw new IllegalArgumentException("MAC address should be 6 bytes long!");
        }
        byte[] mac = new byte[6];
        for (int i = 0; i < macBytes.length; i++) {
            Integer hexVal = Integer.parseInt(macBytes[i], 16);
            mac[i] = hexVal.byteValue();
        }
        return mac;
    }

    /*
     * Read a MAC address in /proc/arp/table, used by WifistateMachine
     * so as to record MAC address of default gateway.
     **/
    private String macAddressFromRoute(String ipAddress) {
        String macAddress = null;
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader("/proc/net/arp"));

            // Skip over the line bearing colum titles
            String line = reader.readLine();

            while ((line = reader.readLine()) != null) {
                String[] tokens = line.split("[ ]+");
                if (tokens.length < 6) {
                    continue;
                }

                // ARP column format is
                // Address HWType HWAddress Flags Mask IFace
                String ip = tokens[0];
                String mac = tokens[3];

                if (ipAddress.equals(ip)) {
                    macAddress = mac;
                    break;
                }
            }

            if (macAddress == null) {
                loge("Did not find remoteAddress {" + ipAddress + "} in " +
                        "/proc/net/arp");
            }

        } catch (FileNotFoundException e) {
            loge("Could not open /proc/net/arp to lookup mac address");
        } catch (IOException e) {
            loge("Could not read /proc/net/arp to lookup mac address");
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (IOException e) {
                // Do nothing
            }
        }
        return macAddress;

    }

    private class WifiNetworkFactory extends NetworkFactory {
        public WifiNetworkFactory(Looper l, Context c, String TAG, NetworkCapabilities f) {
            super(l, c, TAG, f);
        }

        @Override
        protected void needNetworkFor(NetworkRequest networkRequest, int score) {
            synchronized (mWifiReqCountLock) {
                if (++mConnectionReqCount == 1) {
                    if (mWifiConnectivityManager != null && mUntrustedReqCount == 0) {
                        mWifiConnectivityManager.enable(true);
                    }
                }
            }
        }

        @Override
        protected void releaseNetworkFor(NetworkRequest networkRequest) {
            synchronized (mWifiReqCountLock) {
                if (--mConnectionReqCount == 0) {
                    if (mWifiConnectivityManager != null && mUntrustedReqCount == 0) {
                        mWifiConnectivityManager.enable(false);
                    }
                }
            }
        }

        public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
            pw.println("mConnectionReqCount " + mConnectionReqCount);
        }

    }

    private class UntrustedWifiNetworkFactory extends NetworkFactory {
        public UntrustedWifiNetworkFactory(Looper l, Context c, String tag, NetworkCapabilities f) {
            super(l, c, tag, f);
        }

        @Override
        protected void needNetworkFor(NetworkRequest networkRequest, int score) {
            if (!networkRequest.networkCapabilities.hasCapability(
                    NetworkCapabilities.NET_CAPABILITY_TRUSTED)) {
                synchronized (mWifiReqCountLock) {
                    if (++mUntrustedReqCount == 1) {
                        if (mWifiConnectivityManager != null) {
                            if (mConnectionReqCount == 0) {
                                mWifiConnectivityManager.enable(true);
                            }
                            mWifiConnectivityManager.setUntrustedConnectionAllowed(true);
                        }
                    }
                }
            }
        }

        @Override
        protected void releaseNetworkFor(NetworkRequest networkRequest) {
            if (!networkRequest.networkCapabilities.hasCapability(
                    NetworkCapabilities.NET_CAPABILITY_TRUSTED)) {
                synchronized (mWifiReqCountLock) {
                    if (--mUntrustedReqCount == 0) {
                        if (mWifiConnectivityManager != null) {
                            mWifiConnectivityManager.setUntrustedConnectionAllowed(false);
                            if (mConnectionReqCount == 0) {
                                mWifiConnectivityManager.enable(false);
                            }
                        }
                    }
                }
            }
        }

        public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
            pw.println("mUntrustedReqCount " + mUntrustedReqCount);
        }
    }

    void maybeRegisterNetworkFactory() {
        if (mNetworkFactory == null) {
            checkAndSetConnectivityInstance();
            if (mCm != null) {
                mNetworkFactory = new WifiNetworkFactory(getHandler().getLooper(), mContext,
                        NETWORKTYPE, mNetworkCapabilitiesFilter);
                mNetworkFactory.setScoreFilter(60);
                mNetworkFactory.register();

                // We can't filter untrusted network in the capabilities filter because a trusted
                // network would still satisfy a request that accepts untrusted ones.
                mUntrustedNetworkFactory = new UntrustedWifiNetworkFactory(getHandler().getLooper(),
                        mContext, NETWORKTYPE_UNTRUSTED, mNetworkCapabilitiesFilter);
                mUntrustedNetworkFactory.setScoreFilter(Integer.MAX_VALUE);
                mUntrustedNetworkFactory.register();
            }
        }
    }

    /********************************************************
     * HSM states
     *******************************************************/

    class DefaultState extends State {
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch (message.what) {
                case AsyncChannel.CMD_CHANNEL_HALF_CONNECTED: {
                    AsyncChannel ac = (AsyncChannel) message.obj;
                    if (ac == mWifiP2pChannel) {
                        if (message.arg1 == AsyncChannel.STATUS_SUCCESSFUL) {
                            mWifiP2pChannel.sendMessage(AsyncChannel.CMD_CHANNEL_FULL_CONNECTION);
                        } else {
                            loge("WifiP2pService connection failure, error=" + message.arg1);
                        }
                    } else {
                        loge("got HALF_CONNECTED for unknown channel");
                    }
                    break;
                }
                case AsyncChannel.CMD_CHANNEL_DISCONNECTED: {
                    AsyncChannel ac = (AsyncChannel) message.obj;
                    if (ac == mWifiP2pChannel) {
                        loge("WifiP2pService channel lost, message.arg1 =" + message.arg1);
                        //TODO: Re-establish connection to state machine after a delay
                        // mWifiP2pChannel.connect(mContext, getHandler(),
                        // mWifiP2pManager.getMessenger());
                    }
                    break;
                }
                case CMD_BLUETOOTH_ADAPTER_STATE_CHANGE:
                    mBluetoothConnectionActive = (message.arg1 !=
                            BluetoothAdapter.STATE_DISCONNECTED);
                    break;
                    /* Synchronous call returns */
                case CMD_PING_SUPPLICANT:
                case CMD_ENABLE_NETWORK:
                case CMD_ADD_OR_UPDATE_NETWORK:
                case CMD_REMOVE_NETWORK:
                case CMD_SAVE_CONFIG:
                    replyToMessage(message, message.what, FAILURE);
                    break;
                case CMD_GET_CAPABILITY_FREQ:
                    replyToMessage(message, message.what, null);
                    break;
                case CMD_GET_CONFIGURED_NETWORKS:
                    replyToMessage(message, message.what, (List<WifiConfiguration>) null);
                    break;
                case CMD_GET_PRIVILEGED_CONFIGURED_NETWORKS:
                    replyToMessage(message, message.what, (List<WifiConfiguration>) null);
                    break;
                case CMD_ENABLE_RSSI_POLL:
                    mEnableRssiPolling = (message.arg1 == 1);
                    break;
                case CMD_SET_HIGH_PERF_MODE:
                    if (message.arg1 == 1) {
                        setSuspendOptimizations(SUSPEND_DUE_TO_HIGH_PERF, false);
                    } else {
                        setSuspendOptimizations(SUSPEND_DUE_TO_HIGH_PERF, true);
                    }
                    break;
                case CMD_BOOT_COMPLETED:
                    maybeRegisterNetworkFactory();
                    break;
                case CMD_SCREEN_STATE_CHANGED:
                    handleScreenStateChanged(message.arg1 != 0);
                    break;
                    /* Discard */
                case CMD_START_SCAN:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    break;
                case CMD_START_SUPPLICANT:
                case CMD_STOP_SUPPLICANT:
                case CMD_STOP_SUPPLICANT_FAILED:
                case CMD_START_DRIVER:
                case CMD_STOP_DRIVER:
                case CMD_DRIVER_START_TIMED_OUT:
                case CMD_START_AP:
                case CMD_START_AP_FAILURE:
                case CMD_STOP_AP:
                case CMD_AP_STOPPED:
                case CMD_DISCONNECT:
                case CMD_RECONNECT:
                case CMD_REASSOCIATE:
                case CMD_RELOAD_TLS_AND_RECONNECT:
                case WifiMonitor.SUP_CONNECTION_EVENT:
                case WifiMonitor.SUP_DISCONNECTION_EVENT:
                case WifiMonitor.NETWORK_CONNECTION_EVENT:
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                case WifiMonitor.SCAN_RESULTS_EVENT:
                case WifiMonitor.SCAN_FAILED_EVENT:
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                case WifiMonitor.AUTHENTICATION_FAILURE_EVENT:
                case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                case WifiMonitor.WPS_OVERLAP_EVENT:
                case CMD_BLACKLIST_NETWORK:
                case CMD_CLEAR_BLACKLIST:
                case CMD_SET_OPERATIONAL_MODE:
                case CMD_SET_FREQUENCY_BAND:
                case CMD_RSSI_POLL:
                case CMD_ENABLE_ALL_NETWORKS:
                case DhcpClient.CMD_PRE_DHCP_ACTION:
                case DhcpClient.CMD_PRE_DHCP_ACTION_COMPLETE:
                case DhcpClient.CMD_POST_DHCP_ACTION:
                case CMD_NO_NETWORKS_PERIODIC_SCAN:
                case CMD_DISABLE_P2P_RSP:
                case WifiMonitor.SUP_REQUEST_IDENTITY:
                case CMD_TEST_NETWORK_DISCONNECT:
                case CMD_OBTAINING_IP_ADDRESS_WATCHDOG_TIMER:
                case WifiMonitor.SUP_REQUEST_SIM_AUTH:
                case CMD_TARGET_BSSID:
                case CMD_AUTO_CONNECT:
                case CMD_AUTO_ROAM:
                case CMD_AUTO_SAVE_NETWORK:
                case CMD_ASSOCIATED_BSSID:
                case CMD_UNWANTED_NETWORK:
                case CMD_DISCONNECTING_WATCHDOG_TIMER:
                case CMD_ROAM_WATCHDOG_TIMER:
                case CMD_DISABLE_EPHEMERAL_NETWORK:
                case CMD_UPDATE_ASSOCIATED_SCAN_PERMISSION:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    break;
                case CMD_SET_SUSPEND_OPT_ENABLED:
                    if (message.arg1 == 1) {
                        if (message.arg2 == 1) {
                            mSuspendWakeLock.release();
                        }
                        setSuspendOptimizations(SUSPEND_DUE_TO_SCREEN, true);
                    } else {
                        setSuspendOptimizations(SUSPEND_DUE_TO_SCREEN, false);
                    }
                    break;
                case WifiMonitor.DRIVER_HUNG_EVENT:
                    setSupplicantRunning(false);
                    setSupplicantRunning(true);
                    break;
                case WifiManager.CONNECT_NETWORK:
                    replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                            WifiManager.BUSY);
                    break;
                case WifiManager.FORGET_NETWORK:
                    replyToMessage(message, WifiManager.FORGET_NETWORK_FAILED,
                            WifiManager.BUSY);
                    break;
                case WifiManager.SAVE_NETWORK:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                    replyToMessage(message, WifiManager.SAVE_NETWORK_FAILED,
                            WifiManager.BUSY);
                    break;
                case WifiManager.START_WPS:
                    replyToMessage(message, WifiManager.WPS_FAILED,
                            WifiManager.BUSY);
                    break;
                case WifiManager.CANCEL_WPS:
                    replyToMessage(message, WifiManager.CANCEL_WPS_FAILED,
                            WifiManager.BUSY);
                    break;
                case WifiManager.DISABLE_NETWORK:
                    replyToMessage(message, WifiManager.DISABLE_NETWORK_FAILED,
                            WifiManager.BUSY);
                    break;
                case WifiManager.RSSI_PKTCNT_FETCH:
                    replyToMessage(message, WifiManager.RSSI_PKTCNT_FETCH_FAILED,
                            WifiManager.BUSY);
                    break;
                case CMD_GET_SUPPORTED_FEATURES:
                    int featureSet = mWifiNative.getSupportedFeatureSet();
                    replyToMessage(message, message.what, featureSet);
                    break;
                case CMD_FIRMWARE_ALERT:
                    if (mWifiLogger != null) {
                        byte[] buffer = (byte[])message.obj;
                        int alertReason = message.arg1;
                        mWifiLogger.captureAlertData(alertReason, buffer);
                        mWifiMetrics.incrementAlertReasonCount(alertReason);
                    }
                    break;
                case CMD_GET_LINK_LAYER_STATS:
                    // Not supported hence reply with error message
                    replyToMessage(message, message.what, null);
                    break;
                case WifiP2pServiceImpl.P2P_CONNECTION_CHANGED:
                    NetworkInfo info = (NetworkInfo) message.obj;
                    mP2pConnected.set(info.isConnected());
                    break;
                case WifiP2pServiceImpl.DISCONNECT_WIFI_REQUEST:
                    mTemporarilyDisconnectWifi = (message.arg1 == 1);
                    replyToMessage(message, WifiP2pServiceImpl.DISCONNECT_WIFI_RESPONSE);
                    break;
                /* Link configuration (IP address, DNS, ...) changes notified via netlink */
                case CMD_UPDATE_LINKPROPERTIES:
                    updateLinkProperties((LinkProperties) message.obj);
                    break;
                case CMD_GET_MATCHING_CONFIG:
                    replyToMessage(message, message.what);
                    break;
                case CMD_IP_CONFIGURATION_SUCCESSFUL:
                case CMD_IP_CONFIGURATION_LOST:
                case CMD_IP_REACHABILITY_LOST:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    break;
                case CMD_GET_CONNECTION_STATISTICS:
                    replyToMessage(message, message.what, mWifiConnectionStatistics);
                    break;
                case CMD_REMOVE_APP_CONFIGURATIONS:
                    deferMessage(message);
                    break;
                case CMD_REMOVE_USER_CONFIGURATIONS:
                    deferMessage(message);
                    break;
                case CMD_START_IP_PACKET_OFFLOAD:
                    if (mNetworkAgent != null) mNetworkAgent.onPacketKeepaliveEvent(
                            message.arg1,
                            ConnectivityManager.PacketKeepalive.ERROR_INVALID_NETWORK);
                    break;
                case CMD_STOP_IP_PACKET_OFFLOAD:
                    if (mNetworkAgent != null) mNetworkAgent.onPacketKeepaliveEvent(
                            message.arg1,
                            ConnectivityManager.PacketKeepalive.ERROR_INVALID_NETWORK);
                    break;
                case CMD_START_RSSI_MONITORING_OFFLOAD:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    break;
                case CMD_STOP_RSSI_MONITORING_OFFLOAD:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    break;
                case CMD_USER_SWITCH:
                    mWifiConfigManager.handleUserSwitch(message.arg1);
                    break;
                case CMD_ADD_PASSPOINT_MO:
                case CMD_MODIFY_PASSPOINT_MO:
                case CMD_QUERY_OSU_ICON:
                case CMD_MATCH_PROVIDER_NETWORK:
                    /* reply with arg1 = 0 - it returns API failure to the calling app
                     * (message.what is not looked at)
                     */
                    replyToMessage(message, message.what);
                    break;
                case CMD_RESET_SIM_NETWORKS:
                    /* Defer this message until supplicant is started. */
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DEFERRED;
                    deferMessage(message);
                    break;
                case CMD_INSTALL_PACKET_FILTER:
                    mWifiNative.installPacketFilter((byte[]) message.obj);
                    break;
                case CMD_SET_FALLBACK_PACKET_FILTERING:
                    if ((boolean) message.obj) {
                        mWifiNative.startFilteringMulticastV4Packets();
                    } else {
                        mWifiNative.stopFilteringMulticastV4Packets();
                    }
                    break;
                default:
                    loge("Error! unhandled message" + message);
                    break;
            }
            return HANDLED;
        }
    }

    class InitialState extends State {
        @Override
        public void enter() {
            mWifiNative.stopHal();
            mWifiNative.unloadDriver();
            if (mWifiP2pChannel == null) {
                mWifiP2pChannel = new AsyncChannel();
                mWifiP2pChannel.connect(mContext, getHandler(),
                    mWifiP2pServiceImpl.getP2pStateMachineMessenger());
            }

            if (mWifiApConfigStore == null) {
                mWifiApConfigStore =
                        mFacade.makeApConfigStore(mContext, mBackupManagerProxy);
            }
        }
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);
            switch (message.what) {
                case CMD_START_SUPPLICANT:
                    if (mWifiNative.loadDriver()) {
                        try {
                            mNwService.wifiFirmwareReload(mInterfaceName, "STA");
                        } catch (Exception e) {
                            loge("Failed to reload STA firmware " + e);
                            setWifiState(WifiManager.WIFI_STATE_UNKNOWN);
                            return HANDLED;
                        }

                        try {
                            // A runtime crash can leave the interface up and
                            // IP addresses configured, and this affects
                            // connectivity when supplicant starts up.
                            // Ensure interface is down and we have no IP
                            // addresses before a supplicant start.
                            mNwService.setInterfaceDown(mInterfaceName);
                            mNwService.clearInterfaceAddresses(mInterfaceName);

                            // Set privacy extensions
                            mNwService.setInterfaceIpv6PrivacyExtensions(mInterfaceName, true);

                            // IPv6 is enabled only as long as access point is connected since:
                            // - IPv6 addresses and routes stick around after disconnection
                            // - kernel is unaware when connected and fails to start IPv6 negotiation
                            // - kernel can start autoconfiguration when 802.1x is not complete
                            mNwService.disableIpv6(mInterfaceName);
                        } catch (RemoteException re) {
                            loge("Unable to change interface settings: " + re);
                        } catch (IllegalStateException ie) {
                            loge("Unable to change interface settings: " + ie);
                        }

                       /* Stop a running supplicant after a runtime restart
                        * Avoids issues with drivers that do not handle interface down
                        * on a running supplicant properly.
                        */
                        mWifiMonitor.killSupplicant(mP2pSupported);

                        if (mWifiNative.startHal() == false) {
                            /* starting HAL is optional */
                            loge("Failed to start HAL");
                        }

                        if (mWifiNative.startSupplicant(mP2pSupported)) {
                            setSupplicantLogLevel();
                            setWifiState(WIFI_STATE_ENABLING);
                            if (DBG) log("Supplicant start successful");
                            mWifiMonitor.startMonitoring(mInterfaceName);
                            transitionTo(mSupplicantStartingState);
                        } else {
                            loge("Failed to start supplicant!");
                            setWifiState(WifiManager.WIFI_STATE_UNKNOWN);
                        }
                    } else {
                        loge("Failed to load driver");
                        setWifiState(WifiManager.WIFI_STATE_UNKNOWN);
                    }
                    break;
                case CMD_START_AP:
                    if (setupDriverForSoftAp()) {
                        transitionTo(mSoftApState);
                    } else {
                        setWifiApState(WIFI_AP_STATE_FAILED,
                                WifiManager.SAP_START_FAILURE_GENERAL);
                        /**
                         * Transition to InitialState (current state) to reset the
                         * driver/HAL back to the initial state.
                         */
                        transitionTo(mInitialState);
                    }
                    break;
                case CMD_SET_OPERATIONAL_MODE:
                    mOperationalMode = message.arg1;
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    class SupplicantStartingState extends State {
        private void initializeWpsDetails() {
            String detail;
            detail = mPropertyService.get("ro.product.name", "");
            if (!mWifiNative.setDeviceName(detail)) {
                loge("Failed to set device name " +  detail);
            }
            detail = mPropertyService.get("ro.product.manufacturer", "");
            if (!mWifiNative.setManufacturer(detail)) {
                loge("Failed to set manufacturer " + detail);
            }
            detail = mPropertyService.get("ro.product.model", "");
            if (!mWifiNative.setModelName(detail)) {
                loge("Failed to set model name " + detail);
            }
            detail = mPropertyService.get("ro.product.model", "");
            if (!mWifiNative.setModelNumber(detail)) {
                loge("Failed to set model number " + detail);
            }
            detail = mPropertyService.get("ro.serialno", "");
            if (!mWifiNative.setSerialNumber(detail)) {
                loge("Failed to set serial number " + detail);
            }
            if (!mWifiNative.setConfigMethods("physical_display virtual_push_button")) {
                loge("Failed to set WPS config methods");
            }
            if (!mWifiNative.setDeviceType(mPrimaryDeviceType)) {
                loge("Failed to set primary device type " + mPrimaryDeviceType);
            }
        }

        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
                case WifiMonitor.SUP_CONNECTION_EVENT:
                    if (DBG) log("Supplicant connection established");
                    setWifiState(WIFI_STATE_ENABLED);
                    mSupplicantRestartCount = 0;
                    /* Reset the supplicant state to indicate the supplicant
                     * state is not known at this time */
                    mSupplicantStateTracker.sendMessage(CMD_RESET_SUPPLICANT_STATE);
                    /* Initialize data structures */
                    mLastBssid = null;
                    mLastNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
                    mLastSignalLevel = -1;

                    mWifiInfo.setMacAddress(mWifiNative.getMacAddress());
                    /* set frequency band of operation */
                    setFrequencyBand();
                    mWifiNative.enableSaveConfig();
                    mWifiConfigManager.loadAndEnableAllNetworks();
                    if (mWifiConfigManager.mEnableVerboseLogging.get() > 0) {
                        enableVerboseLogging(mWifiConfigManager.mEnableVerboseLogging.get());
                    }
                    initializeWpsDetails();

                    sendSupplicantConnectionChangedBroadcast(true);
                    transitionTo(mDriverStartedState);
                    break;
                case WifiMonitor.SUP_DISCONNECTION_EVENT:
                    if (++mSupplicantRestartCount <= SUPPLICANT_RESTART_TRIES) {
                        loge("Failed to setup control channel, restart supplicant");
                        mWifiMonitor.killSupplicant(mP2pSupported);
                        transitionTo(mInitialState);
                        sendMessageDelayed(CMD_START_SUPPLICANT, SUPPLICANT_RESTART_INTERVAL_MSECS);
                    } else {
                        loge("Failed " + mSupplicantRestartCount +
                                " times to start supplicant, unload driver");
                        mSupplicantRestartCount = 0;
                        setWifiState(WIFI_STATE_UNKNOWN);
                        transitionTo(mInitialState);
                    }
                    break;
                case CMD_START_SUPPLICANT:
                case CMD_STOP_SUPPLICANT:
                case CMD_START_AP:
                case CMD_STOP_AP:
                case CMD_START_DRIVER:
                case CMD_STOP_DRIVER:
                case CMD_SET_OPERATIONAL_MODE:
                case CMD_SET_FREQUENCY_BAND:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DEFERRED;
                    deferMessage(message);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    class SupplicantStartedState extends State {
        @Override
        public void enter() {
            /* Wifi is available as long as we have a connection to supplicant */
            mNetworkInfo.setIsAvailable(true);
            if (mNetworkAgent != null) mNetworkAgent.sendNetworkInfo(mNetworkInfo);

            int defaultInterval = mContext.getResources().getInteger(
                    R.integer.config_wifi_supplicant_scan_interval);

            mSupplicantScanIntervalMs = mFacade.getLongSetting(mContext,
                    Settings.Global.WIFI_SUPPLICANT_SCAN_INTERVAL_MS,
                    defaultInterval);

            mWifiNative.setScanInterval((int)mSupplicantScanIntervalMs / 1000);
            mWifiNative.setExternalSim(true);

            /* turn on use of DFS channels */
            mWifiNative.setDfsFlag(true);

            setRandomMacOui();
            mWifiNative.enableAutoConnect(false);
            mCountryCode.setReadyForChange(true);
        }

        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
                case CMD_STOP_SUPPLICANT:   /* Supplicant stopped by user */
                    if (mP2pSupported) {
                        transitionTo(mWaitForP2pDisableState);
                    } else {
                        transitionTo(mSupplicantStoppingState);
                    }
                    break;
                case WifiMonitor.SUP_DISCONNECTION_EVENT:  /* Supplicant connection lost */
                    loge("Connection lost, restart supplicant");
                    handleSupplicantConnectionLoss(true);
                    handleNetworkDisconnect();
                    mSupplicantStateTracker.sendMessage(CMD_RESET_SUPPLICANT_STATE);
                    if (mP2pSupported) {
                        transitionTo(mWaitForP2pDisableState);
                    } else {
                        transitionTo(mInitialState);
                    }
                    sendMessageDelayed(CMD_START_SUPPLICANT, SUPPLICANT_RESTART_INTERVAL_MSECS);
                    break;
                case WifiMonitor.SCAN_RESULTS_EVENT:
                case WifiMonitor.SCAN_FAILED_EVENT:
                    maybeRegisterNetworkFactory(); // Make sure our NetworkFactory is registered
                    setScanResults();
                    if (mIsFullScanOngoing || mSendScanResultsBroadcast) {
                        /* Just updated results from full scan, let apps know about this */
                        boolean scanSucceeded = message.what == WifiMonitor.SCAN_RESULTS_EVENT;
                        sendScanResultsAvailableBroadcast(scanSucceeded);
                    }
                    mSendScanResultsBroadcast = false;
                    mIsScanOngoing = false;
                    mIsFullScanOngoing = false;
                    if (mBufferedScanMsg.size() > 0)
                        sendMessage(mBufferedScanMsg.remove());
                    break;
                case CMD_PING_SUPPLICANT:
                    boolean ok = mWifiNative.ping();
                    replyToMessage(message, message.what, ok ? SUCCESS : FAILURE);
                    break;
                case CMD_GET_CAPABILITY_FREQ:
                    String freqs = mWifiNative.getFreqCapability();
                    replyToMessage(message, message.what, freqs);
                    break;
                case CMD_START_AP:
                    /* Cannot start soft AP while in client mode */
                    loge("Failed to start soft AP with a running supplicant");
                    setWifiApState(WIFI_AP_STATE_FAILED, WifiManager.SAP_START_FAILURE_GENERAL);
                    break;
                case CMD_SET_OPERATIONAL_MODE:
                    mOperationalMode = message.arg1;
                    mWifiConfigManager.
                            setAndEnableLastSelectedConfiguration(
                                    WifiConfiguration.INVALID_NETWORK_ID);
                    break;
                case CMD_TARGET_BSSID:
                    // Trying to associate to this BSSID
                    if (message.obj != null) {
                        mTargetRoamBSSID = (String) message.obj;
                    }
                    break;
                case CMD_GET_LINK_LAYER_STATS:
                    WifiLinkLayerStats stats = getWifiLinkLayerStats(DBG);
                    replyToMessage(message, message.what, stats);
                    break;
                case CMD_RESET_SIM_NETWORKS:
                    log("resetting EAP-SIM/AKA/AKA' networks since SIM was changed");
                    mWifiConfigManager.resetSimNetworks();
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void exit() {
            mNetworkInfo.setIsAvailable(false);
            if (mNetworkAgent != null) mNetworkAgent.sendNetworkInfo(mNetworkInfo);
            mCountryCode.setReadyForChange(false);
        }
    }

    class SupplicantStoppingState extends State {
        @Override
        public void enter() {
            /* Send any reset commands to supplicant before shutting it down */
            handleNetworkDisconnect();

            String suppState = System.getProperty("init.svc.wpa_supplicant");
            if (suppState == null) suppState = "unknown";
            String p2pSuppState = System.getProperty("init.svc.p2p_supplicant");
            if (p2pSuppState == null) p2pSuppState = "unknown";

            logd("SupplicantStoppingState: stopSupplicant "
                    + " init.svc.wpa_supplicant=" + suppState
                    + " init.svc.p2p_supplicant=" + p2pSuppState);
            mWifiMonitor.stopSupplicant();

            /* Send ourselves a delayed message to indicate failure after a wait time */
            sendMessageDelayed(obtainMessage(CMD_STOP_SUPPLICANT_FAILED,
                    ++mSupplicantStopFailureToken, 0), SUPPLICANT_RESTART_INTERVAL_MSECS);
            setWifiState(WIFI_STATE_DISABLING);
            mSupplicantStateTracker.sendMessage(CMD_RESET_SUPPLICANT_STATE);
        }
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
                case WifiMonitor.SUP_CONNECTION_EVENT:
                    loge("Supplicant connection received while stopping");
                    break;
                case WifiMonitor.SUP_DISCONNECTION_EVENT:
                    if (DBG) log("Supplicant connection lost");
                    handleSupplicantConnectionLoss(false);
                    transitionTo(mInitialState);
                    break;
                case CMD_STOP_SUPPLICANT_FAILED:
                    if (message.arg1 == mSupplicantStopFailureToken) {
                        loge("Timed out on a supplicant stop, kill and proceed");
                        handleSupplicantConnectionLoss(true);
                        transitionTo(mInitialState);
                    }
                    break;
                case CMD_START_SUPPLICANT:
                case CMD_STOP_SUPPLICANT:
                case CMD_START_AP:
                case CMD_STOP_AP:
                case CMD_START_DRIVER:
                case CMD_STOP_DRIVER:
                case CMD_SET_OPERATIONAL_MODE:
                case CMD_SET_FREQUENCY_BAND:
                    deferMessage(message);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    class DriverStartingState extends State {
        private int mTries;
        @Override
        public void enter() {
            mTries = 1;
            /* Send ourselves a delayed message to start driver a second time */
            sendMessageDelayed(obtainMessage(CMD_DRIVER_START_TIMED_OUT,
                        ++mDriverStartToken, 0), DRIVER_START_TIME_OUT_MSECS);
        }
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
               case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    SupplicantState state = handleSupplicantStateChange(message);
                    /* If suplicant is exiting out of INTERFACE_DISABLED state into
                     * a state that indicates driver has started, it is ready to
                     * receive driver commands
                     */
                    if (SupplicantState.isDriverActive(state)) {
                        transitionTo(mDriverStartedState);
                    }
                    break;
                case CMD_DRIVER_START_TIMED_OUT:
                    if (message.arg1 == mDriverStartToken) {
                        if (mTries >= 2) {
                            loge("Failed to start driver after " + mTries);
                            setSupplicantRunning(false);
                            setSupplicantRunning(true);
                        } else {
                            loge("Driver start failed, retrying");
                            mWakeLock.acquire();
                            mWifiNative.startDriver();
                            mWakeLock.release();

                            ++mTries;
                            /* Send ourselves a delayed message to start driver again */
                            sendMessageDelayed(obtainMessage(CMD_DRIVER_START_TIMED_OUT,
                                        ++mDriverStartToken, 0), DRIVER_START_TIME_OUT_MSECS);
                        }
                    }
                    break;
                    /* Queue driver commands & connection events */
                case CMD_START_DRIVER:
                case CMD_STOP_DRIVER:
                case WifiMonitor.NETWORK_CONNECTION_EVENT:
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                case WifiMonitor.AUTHENTICATION_FAILURE_EVENT:
                case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                case WifiMonitor.WPS_OVERLAP_EVENT:
                case CMD_SET_FREQUENCY_BAND:
                case CMD_START_SCAN:
                case CMD_DISCONNECT:
                case CMD_REASSOCIATE:
                case CMD_RECONNECT:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DEFERRED;
                    deferMessage(message);
                    break;
                case WifiMonitor.SCAN_RESULTS_EVENT:
                case WifiMonitor.SCAN_FAILED_EVENT:
                    // Loose scan results obtained in Driver Starting state, they can only confuse
                    // the state machine
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    class DriverStartedState extends State {
        @Override
        public void enter() {
            if (DBG) {
                logd("DriverStartedState enter");
            }

            // We can't do this in the constructor because WifiStateMachine is created before the
            // wifi scanning service is initialized
            if (mWifiScanner == null) {
                mWifiScanner = mFacade.makeWifiScanner(mContext, getHandler().getLooper());

                synchronized (mWifiReqCountLock) {
                    mWifiConnectivityManager = new WifiConnectivityManager(mContext,
                        WifiStateMachine.this, mWifiScanner, mWifiConfigManager, mWifiInfo,
                        mWifiQualifiedNetworkSelector, mWifiInjector,
                        getHandler().getLooper(), hasConnectionRequests());
                    mWifiConnectivityManager.setUntrustedConnectionAllowed(mUntrustedReqCount > 0);
                }
            }

            mWifiLogger.startLogging(DBG);
            mIsRunning = true;
            updateBatteryWorkSource(null);
            /**
             * Enable bluetooth coexistence scan mode when bluetooth connection is active.
             * When this mode is on, some of the low-level scan parameters used by the
             * driver are changed to reduce interference with bluetooth
             */
            mWifiNative.setBluetoothCoexistenceScanMode(mBluetoothConnectionActive);
            /* initialize network state */
            setNetworkDetailedState(DetailedState.DISCONNECTED);

            // Disable legacy multicast filtering, which on some chipsets defaults to enabled.
            // Legacy IPv6 multicast filtering blocks ICMPv6 router advertisements which breaks IPv6
            // provisioning. Legacy IPv4 multicast filtering may be re-enabled later via
            // IpManager.Callback.setFallbackMulticastFilter()
            mWifiNative.stopFilteringMulticastV4Packets();
            mWifiNative.stopFilteringMulticastV6Packets();

            if (mOperationalMode != CONNECT_MODE) {
                mWifiNative.disconnect();
                mWifiConfigManager.disableAllNetworksNative();
                if (mOperationalMode == SCAN_ONLY_WITH_WIFI_OFF_MODE) {
                    setWifiState(WIFI_STATE_DISABLED);
                }
                transitionTo(mScanModeState);
            } else {

                // Status pulls in the current supplicant state and network connection state
                // events over the monitor connection. This helps framework sync up with
                // current supplicant state
                // TODO: actually check th supplicant status string and make sure the supplicant
                // is in disconnecte4d state.
                mWifiNative.status();
                // Transitioning to Disconnected state will trigger a scan and subsequently AutoJoin
                transitionTo(mDisconnectedState);
                transitionTo(mDisconnectedState);
            }

            // We may have missed screen update at boot
            if (mScreenBroadcastReceived.get() == false) {
                PowerManager powerManager = (PowerManager)mContext.getSystemService(
                        Context.POWER_SERVICE);
                handleScreenStateChanged(powerManager.isScreenOn());
            } else {
                // Set the right suspend mode settings
                mWifiNative.setSuspendOptimizations(mSuspendOptNeedsDisabled == 0
                        && mUserWantsSuspendOpt.get());

                // Inform WifiConnectivtyManager the screen state in case
                // WifiConnectivityManager missed the last screen update because
                // it was not started yet.
                mWifiConnectivityManager.handleScreenStateChanged(mScreenOn);
            }
            mWifiNative.setPowerSave(true);

            if (mP2pSupported) {
                if (mOperationalMode == CONNECT_MODE) {
                    mWifiP2pChannel.sendMessage(WifiStateMachine.CMD_ENABLE_P2P);
                } else {
                    // P2P statemachine starts in disabled state, and is not enabled until
                    // CMD_ENABLE_P2P is sent from here; so, nothing needs to be done to
                    // keep it disabled.
                }
            }

            final Intent intent = new Intent(WifiManager.WIFI_SCAN_AVAILABLE);
            intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
            intent.putExtra(WifiManager.EXTRA_SCAN_AVAILABLE, WIFI_STATE_ENABLED);
            mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);

            // Enable link layer stats gathering
            mWifiNative.setWifiLinkLayerStats("wlan0", 1);
        }

        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
                case CMD_START_SCAN:
                    handleScanRequest(message);
                    break;
                case CMD_SET_FREQUENCY_BAND:
                    int band =  message.arg1;
                    if (DBG) log("set frequency band " + band);
                    if (mWifiNative.setBand(band)) {

                        if (DBG)  logd("did set frequency band " + band);

                        mFrequencyBand.set(band);
                        // Flush old data - like scan results
                        mWifiNative.bssFlush();

                        if (DBG)  logd("done set frequency band " + band);

                    } else {
                        loge("Failed to set frequency band " + band);
                    }
                    break;
                case CMD_BLUETOOTH_ADAPTER_STATE_CHANGE:
                    mBluetoothConnectionActive = (message.arg1 !=
                            BluetoothAdapter.STATE_DISCONNECTED);
                    mWifiNative.setBluetoothCoexistenceScanMode(mBluetoothConnectionActive);
                    break;
                case CMD_STOP_DRIVER:
                    int mode = message.arg1;

                    log("stop driver");
                    mWifiConfigManager.disableAllNetworksNative();

                    if (getCurrentState() != mDisconnectedState) {
                        mWifiNative.disconnect();
                        handleNetworkDisconnect();
                    }
                    mWakeLock.acquire();
                    mWifiNative.stopDriver();
                    mWakeLock.release();
                    if (mP2pSupported) {
                        transitionTo(mWaitForP2pDisableState);
                    } else {
                        transitionTo(mDriverStoppingState);
                    }
                    break;
                case CMD_START_DRIVER:
                    if (mOperationalMode == CONNECT_MODE) {
                        mWifiConfigManager.enableAllNetworks();
                    }
                    break;
                case CMD_SET_SUSPEND_OPT_ENABLED:
                    if (message.arg1 == 1) {
                        setSuspendOptimizationsNative(SUSPEND_DUE_TO_SCREEN, true);
                        if (message.arg2 == 1) {
                            mSuspendWakeLock.release();
                        }
                    } else {
                        setSuspendOptimizationsNative(SUSPEND_DUE_TO_SCREEN, false);
                    }
                    break;
                case CMD_SET_HIGH_PERF_MODE:
                    if (message.arg1 == 1) {
                        setSuspendOptimizationsNative(SUSPEND_DUE_TO_HIGH_PERF, false);
                    } else {
                        setSuspendOptimizationsNative(SUSPEND_DUE_TO_HIGH_PERF, true);
                    }
                    break;
                case CMD_ENABLE_TDLS:
                    if (message.obj != null) {
                        String remoteAddress = (String) message.obj;
                        boolean enable = (message.arg1 == 1);
                        mWifiNative.startTdls(remoteAddress, enable);
                    }
                    break;
                case WifiMonitor.ANQP_DONE_EVENT:
                    mWifiConfigManager.notifyANQPDone((Long) message.obj, message.arg1 != 0);
                    break;
                case CMD_STOP_IP_PACKET_OFFLOAD: {
                    int slot = message.arg1;
                    int ret = stopWifiIPPacketOffload(slot);
                    if (mNetworkAgent != null) {
                        mNetworkAgent.onPacketKeepaliveEvent(slot, ret);
                    }
                    break;
                }
                case WifiMonitor.RX_HS20_ANQP_ICON_EVENT:
                    mWifiConfigManager.notifyIconReceived((IconEvent) message.obj);
                    break;
                case WifiMonitor.HS20_REMEDIATION_EVENT:
                    wnmFrameReceived((WnmData) message.obj);
                    break;
                case CMD_CONFIG_ND_OFFLOAD:
                    final boolean enabled = (message.arg1 > 0);
                    mWifiNative.configureNeighborDiscoveryOffload(enabled);
                    break;
                case CMD_ENABLE_WIFI_CONNECTIVITY_MANAGER:
                    if (mWifiConnectivityManager != null) {
                        mWifiConnectivityManager.enable(message.arg1 == 1 ? true : false);
                    }
                    break;
                case CMD_ENABLE_AUTOJOIN_WHEN_ASSOCIATED:
                    final boolean allowed = (message.arg1 > 0);
                    boolean old_state = mWifiConfigManager.getEnableAutoJoinWhenAssociated();
                    mWifiConfigManager.setEnableAutoJoinWhenAssociated(allowed);
                    if (!old_state && allowed && mScreenOn
                            && getCurrentState() == mConnectedState) {
                        if (mWifiConnectivityManager != null) {
                            mWifiConnectivityManager.forceConnectivityScan();
                        }
                    }
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
        @Override
        public void exit() {

            mWifiLogger.stopLogging();

            mIsRunning = false;
            updateBatteryWorkSource(null);
            mScanResults = new ArrayList<>();

            final Intent intent = new Intent(WifiManager.WIFI_SCAN_AVAILABLE);
            intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
            intent.putExtra(WifiManager.EXTRA_SCAN_AVAILABLE, WIFI_STATE_DISABLED);
            mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
            mBufferedScanMsg.clear();
        }
    }

    class WaitForP2pDisableState extends State {
        private State mTransitionToState;
        @Override
        public void enter() {
            switch (getCurrentMessage().what) {
                case WifiMonitor.SUP_DISCONNECTION_EVENT:
                    mTransitionToState = mInitialState;
                    break;
                case CMD_STOP_DRIVER:
                    mTransitionToState = mDriverStoppingState;
                    break;
                case CMD_STOP_SUPPLICANT:
                    mTransitionToState = mSupplicantStoppingState;
                    break;
                default:
                    mTransitionToState = mDriverStoppingState;
                    break;
            }
            mWifiP2pChannel.sendMessage(WifiStateMachine.CMD_DISABLE_P2P_REQ);
        }
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
                case WifiStateMachine.CMD_DISABLE_P2P_RSP:
                    transitionTo(mTransitionToState);
                    break;
                /* Defer wifi start/shut and driver commands */
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                case CMD_START_SUPPLICANT:
                case CMD_STOP_SUPPLICANT:
                case CMD_START_AP:
                case CMD_STOP_AP:
                case CMD_START_DRIVER:
                case CMD_STOP_DRIVER:
                case CMD_SET_OPERATIONAL_MODE:
                case CMD_SET_FREQUENCY_BAND:
                case CMD_START_SCAN:
                case CMD_DISCONNECT:
                case CMD_REASSOCIATE:
                case CMD_RECONNECT:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DEFERRED;
                    deferMessage(message);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    class DriverStoppingState extends State {
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    SupplicantState state = handleSupplicantStateChange(message);
                    if (state == SupplicantState.INTERFACE_DISABLED) {
                        transitionTo(mDriverStoppedState);
                    }
                    break;
                    /* Queue driver commands */
                case CMD_START_DRIVER:
                case CMD_STOP_DRIVER:
                case CMD_SET_FREQUENCY_BAND:
                case CMD_START_SCAN:
                case CMD_DISCONNECT:
                case CMD_REASSOCIATE:
                case CMD_RECONNECT:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DEFERRED;
                    deferMessage(message);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    class DriverStoppedState extends State {
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);
            switch (message.what) {
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
                    SupplicantState state = stateChangeResult.state;
                    // A WEXT bug means that we can be back to driver started state
                    // unexpectedly
                    if (SupplicantState.isDriverActive(state)) {
                        transitionTo(mDriverStartedState);
                    }
                    break;
                case CMD_START_DRIVER:
                    mWakeLock.acquire();
                    mWifiNative.startDriver();
                    mWakeLock.release();
                    transitionTo(mDriverStartingState);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    class ScanModeState extends State {
        private int mLastOperationMode;
        @Override
        public void enter() {
            mLastOperationMode = mOperationalMode;
        }
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
                case CMD_SET_OPERATIONAL_MODE:
                    if (message.arg1 == CONNECT_MODE) {

                        if (mLastOperationMode == SCAN_ONLY_WITH_WIFI_OFF_MODE) {
                            setWifiState(WIFI_STATE_ENABLED);
                            // Load and re-enable networks when going back to enabled state
                            // This is essential for networks to show up after restore
                            mWifiConfigManager.loadAndEnableAllNetworks();
                            mWifiP2pChannel.sendMessage(CMD_ENABLE_P2P);
                        } else {
                            mWifiConfigManager.enableAllNetworks();
                        }

                        // Loose last selection choice since user toggled WiFi
                        mWifiConfigManager.
                                setAndEnableLastSelectedConfiguration(
                                        WifiConfiguration.INVALID_NETWORK_ID);

                        mOperationalMode = CONNECT_MODE;
                        transitionTo(mDisconnectedState);
                    } else {
                        // Nothing to do
                        return HANDLED;
                    }
                    break;
                // Handle scan. All the connection related commands are
                // handled only in ConnectModeState
                case CMD_START_SCAN:
                    handleScanRequest(message);
                    break;
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    SupplicantState state = handleSupplicantStateChange(message);
                    if (DBG) log("SupplicantState= " + state);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }


    String smToString(Message message) {
        return smToString(message.what);
    }

    String smToString(int what) {
        String s = sSmToString.get(what);
        if (s != null) {
            return s;
        }
        switch (what) {
            case WifiMonitor.DRIVER_HUNG_EVENT:
                s = "DRIVER_HUNG_EVENT";
                break;
            case AsyncChannel.CMD_CHANNEL_HALF_CONNECTED:
                s = "AsyncChannel.CMD_CHANNEL_HALF_CONNECTED";
                break;
            case AsyncChannel.CMD_CHANNEL_DISCONNECTED:
                s = "AsyncChannel.CMD_CHANNEL_DISCONNECTED";
                break;
            case WifiP2pServiceImpl.DISCONNECT_WIFI_REQUEST:
                s = "WifiP2pServiceImpl.DISCONNECT_WIFI_REQUEST";
                break;
            case WifiManager.DISABLE_NETWORK:
                s = "WifiManager.DISABLE_NETWORK";
                break;
            case WifiManager.CONNECT_NETWORK:
                s = "CONNECT_NETWORK";
                break;
            case WifiManager.SAVE_NETWORK:
                s = "SAVE_NETWORK";
                break;
            case WifiManager.FORGET_NETWORK:
                s = "FORGET_NETWORK";
                break;
            case WifiMonitor.SUP_CONNECTION_EVENT:
                s = "SUP_CONNECTION_EVENT";
                break;
            case WifiMonitor.SUP_DISCONNECTION_EVENT:
                s = "SUP_DISCONNECTION_EVENT";
                break;
            case WifiMonitor.SCAN_RESULTS_EVENT:
                s = "SCAN_RESULTS_EVENT";
                break;
            case WifiMonitor.SCAN_FAILED_EVENT:
                s = "SCAN_FAILED_EVENT";
                break;
            case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                s = "SUPPLICANT_STATE_CHANGE_EVENT";
                break;
            case WifiMonitor.AUTHENTICATION_FAILURE_EVENT:
                s = "AUTHENTICATION_FAILURE_EVENT";
                break;
            case WifiMonitor.SSID_TEMP_DISABLED:
                s = "SSID_TEMP_DISABLED";
                break;
            case WifiMonitor.SSID_REENABLED:
                s = "SSID_REENABLED";
                break;
            case WifiMonitor.WPS_SUCCESS_EVENT:
                s = "WPS_SUCCESS_EVENT";
                break;
            case WifiMonitor.WPS_FAIL_EVENT:
                s = "WPS_FAIL_EVENT";
                break;
            case WifiMonitor.SUP_REQUEST_IDENTITY:
                s = "SUP_REQUEST_IDENTITY";
                break;
            case WifiMonitor.NETWORK_CONNECTION_EVENT:
                s = "NETWORK_CONNECTION_EVENT";
                break;
            case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                s = "NETWORK_DISCONNECTION_EVENT";
                break;
            case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                s = "ASSOCIATION_REJECTION_EVENT";
                break;
            case WifiMonitor.ANQP_DONE_EVENT:
                s = "WifiMonitor.ANQP_DONE_EVENT";
                break;
            case WifiMonitor.RX_HS20_ANQP_ICON_EVENT:
                s = "WifiMonitor.RX_HS20_ANQP_ICON_EVENT";
                break;
            case WifiMonitor.GAS_QUERY_DONE_EVENT:
                s = "WifiMonitor.GAS_QUERY_DONE_EVENT";
                break;
            case WifiMonitor.HS20_REMEDIATION_EVENT:
                s = "WifiMonitor.HS20_REMEDIATION_EVENT";
                break;
            case WifiMonitor.GAS_QUERY_START_EVENT:
                s = "WifiMonitor.GAS_QUERY_START_EVENT";
                break;
            case WifiP2pServiceImpl.GROUP_CREATING_TIMED_OUT:
                s = "GROUP_CREATING_TIMED_OUT";
                break;
            case WifiP2pServiceImpl.P2P_CONNECTION_CHANGED:
                s = "P2P_CONNECTION_CHANGED";
                break;
            case WifiP2pServiceImpl.DISCONNECT_WIFI_RESPONSE:
                s = "P2P.DISCONNECT_WIFI_RESPONSE";
                break;
            case WifiP2pServiceImpl.SET_MIRACAST_MODE:
                s = "P2P.SET_MIRACAST_MODE";
                break;
            case WifiP2pServiceImpl.BLOCK_DISCOVERY:
                s = "P2P.BLOCK_DISCOVERY";
                break;
            case WifiManager.CANCEL_WPS:
                s = "CANCEL_WPS";
                break;
            case WifiManager.CANCEL_WPS_FAILED:
                s = "CANCEL_WPS_FAILED";
                break;
            case WifiManager.CANCEL_WPS_SUCCEDED:
                s = "CANCEL_WPS_SUCCEDED";
                break;
            case WifiManager.START_WPS:
                s = "START_WPS";
                break;
            case WifiManager.START_WPS_SUCCEEDED:
                s = "START_WPS_SUCCEEDED";
                break;
            case WifiManager.WPS_FAILED:
                s = "WPS_FAILED";
                break;
            case WifiManager.WPS_COMPLETED:
                s = "WPS_COMPLETED";
                break;
            case WifiManager.RSSI_PKTCNT_FETCH:
                s = "RSSI_PKTCNT_FETCH";
                break;
            default:
                s = "what:" + Integer.toString(what);
                break;
        }
        return s;
    }

    void registerConnected() {
        if (mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID) {
            WifiConfiguration config = mWifiConfigManager.getWifiConfiguration(mLastNetworkId);
            if (config != null) {
                //Here we will clear all disable counters once a network is connected
                //records how long this network is connected in future
                config.lastConnected = System.currentTimeMillis();
                config.numAssociation++;
                WifiConfiguration.NetworkSelectionStatus networkSelectionStatus =
                        config.getNetworkSelectionStatus();
                networkSelectionStatus.clearDisableReasonCounter();
                networkSelectionStatus.setHasEverConnected(true);
            }
            // On connect, reset wifiScoreReport
            mWifiScoreReport = null;
       }
    }

    void registerDisconnected() {
        if (mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID) {
            // We are switching away from this configuration,
            // hence record the time we were connected last
            WifiConfiguration config = mWifiConfigManager.getWifiConfiguration(mLastNetworkId);
            if (config != null) {
                config.lastDisconnected = System.currentTimeMillis();
                if (config.ephemeral) {
                    // Remove ephemeral WifiConfigurations from file
                    mWifiConfigManager.forgetNetwork(mLastNetworkId);
                }
            }
        }
    }

    void noteWifiDisabledWhileAssociated() {
        // We got disabled by user while we were associated, make note of it
        int rssi = mWifiInfo.getRssi();
        WifiConfiguration config = getCurrentWifiConfiguration();
        if (getCurrentState() == mConnectedState
                && rssi != WifiInfo.INVALID_RSSI
                && config != null) {
            boolean is24GHz = mWifiInfo.is24GHz();
            boolean isBadRSSI = (is24GHz && rssi < mWifiConfigManager.mThresholdMinimumRssi24.get())
                    || (!is24GHz && rssi < mWifiConfigManager.mThresholdMinimumRssi5.get());
            boolean isLowRSSI =
                    (is24GHz && rssi < mWifiConfigManager.mThresholdQualifiedRssi24.get())
                            || (!is24GHz && mWifiInfo.getRssi() <
                                    mWifiConfigManager.mThresholdQualifiedRssi5.get());
            boolean isHighRSSI = (is24GHz && rssi
                    >= mWifiConfigManager.mThresholdSaturatedRssi24.get())
                    || (!is24GHz && mWifiInfo.getRssi()
                    >= mWifiConfigManager.mThresholdSaturatedRssi5.get());
            if (isBadRSSI) {
                // Take note that we got disabled while RSSI was Bad
                config.numUserTriggeredWifiDisableLowRSSI++;
            } else if (isLowRSSI) {
                // Take note that we got disabled while RSSI was Low
                config.numUserTriggeredWifiDisableBadRSSI++;
            } else if (!isHighRSSI) {
                // Take note that we got disabled while RSSI was Not high
                config.numUserTriggeredWifiDisableNotHighRSSI++;
            }
        }
    }

    /**
     * Returns Wificonfiguration object correponding to the currently connected network, null if
     * not connected.
     */
    public WifiConfiguration getCurrentWifiConfiguration() {
        if (mLastNetworkId == WifiConfiguration.INVALID_NETWORK_ID) {
            return null;
        }
        return mWifiConfigManager.getWifiConfiguration(mLastNetworkId);
    }

    ScanResult getCurrentScanResult() {
        WifiConfiguration config = getCurrentWifiConfiguration();
        if (config == null) {
            return null;
        }
        String BSSID = mWifiInfo.getBSSID();
        if (BSSID == null) {
            BSSID = mTargetRoamBSSID;
        }
        ScanDetailCache scanDetailCache =
                mWifiConfigManager.getScanDetailCache(config);

        if (scanDetailCache == null) {
            return null;
        }

        return scanDetailCache.get(BSSID);
    }

    String getCurrentBSSID() {
        if (linkDebouncing) {
            return null;
        }
        return mLastBssid;
    }

    class ConnectModeState extends State {

        @Override
        public void enter() {
            // Inform WifiConnectivityManager that Wifi is enabled
            if (mWifiConnectivityManager != null) {
                mWifiConnectivityManager.setWifiEnabled(true);
            }
            // Inform metrics that Wifi is Enabled (but not yet connected)
            mWifiMetrics.setWifiState(WifiMetricsProto.WifiLog.WIFI_DISCONNECTED);


        }

        @Override
        public void exit() {
            // Inform WifiConnectivityManager that Wifi is disabled
            if (mWifiConnectivityManager != null) {
                mWifiConnectivityManager.setWifiEnabled(false);
            }
            // Inform metrics that Wifi is being disabled (Toggled, airplane enabled, etc)
            mWifiMetrics.setWifiState(WifiMetricsProto.WifiLog.WIFI_DISABLED);
        }

        @Override
        public boolean processMessage(Message message) {
            WifiConfiguration config;
            int netId;
            boolean ok;
            boolean didDisconnect;
            String bssid;
            String ssid;
            NetworkUpdateResult result;
            logStateAndMessage(message, this);

            switch (message.what) {
                case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                    mWifiLogger.captureBugReportData(WifiLogger.REPORT_REASON_ASSOC_FAILURE);
                    didBlackListBSSID = false;
                    bssid = (String) message.obj;
                    if (bssid == null || TextUtils.isEmpty(bssid)) {
                        // If BSSID is null, use the target roam BSSID
                        bssid = mTargetRoamBSSID;
                    }
                    if (bssid != null) {
                        // If we have a BSSID, tell configStore to black list it
                        if (mWifiConnectivityManager != null) {
                            didBlackListBSSID = mWifiConnectivityManager.trackBssid(bssid,
                                    false);
                        }
                    }

                    mWifiConfigManager.updateNetworkSelectionStatus(mTargetNetworkId,
                            WifiConfiguration.NetworkSelectionStatus
                            .DISABLED_ASSOCIATION_REJECTION);

                    mSupplicantStateTracker.sendMessage(WifiMonitor.ASSOCIATION_REJECTION_EVENT);
                    //If rejection occurred while Metrics is tracking a ConnnectionEvent, end it.
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_ASSOCIATION_REJECTION,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE);
                    mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(getTargetSsid(),
                            bssid,
                            WifiLastResortWatchdog.FAILURE_CODE_ASSOCIATION);
                    break;
                case WifiMonitor.AUTHENTICATION_FAILURE_EVENT:
                    mWifiLogger.captureBugReportData(WifiLogger.REPORT_REASON_AUTH_FAILURE);
                    mSupplicantStateTracker.sendMessage(WifiMonitor.AUTHENTICATION_FAILURE_EVENT);
                    if (mTargetNetworkId != WifiConfiguration.INVALID_NETWORK_ID) {
                        mWifiConfigManager.updateNetworkSelectionStatus(mTargetNetworkId,
                                WifiConfiguration.NetworkSelectionStatus
                                        .DISABLED_AUTHENTICATION_FAILURE);
                    }
                    //If failure occurred while Metrics is tracking a ConnnectionEvent, end it.
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_AUTHENTICATION_FAILURE,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE);
                    mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(getTargetSsid(),
                            mTargetRoamBSSID,
                            WifiLastResortWatchdog.FAILURE_CODE_AUTHENTICATION);
                    break;
                case WifiMonitor.SSID_TEMP_DISABLED:
                    Log.e(TAG, "Supplicant SSID temporary disabled:"
                            + mWifiConfigManager.getWifiConfiguration(message.arg1));
                    mWifiConfigManager.updateNetworkSelectionStatus(
                            message.arg1,
                            WifiConfiguration.NetworkSelectionStatus
                            .DISABLED_AUTHENTICATION_FAILURE);
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_SSID_TEMP_DISABLED,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE);
                    mWifiLastResortWatchdog.noteConnectionFailureAndTriggerIfNeeded(getTargetSsid(),
                            mTargetRoamBSSID,
                            WifiLastResortWatchdog.FAILURE_CODE_AUTHENTICATION);
                    break;
                case WifiMonitor.SSID_REENABLED:
                    Log.d(TAG, "Supplicant SSID reenable:"
                            + mWifiConfigManager.getWifiConfiguration(message.arg1));
                    // Do not re-enable it in Quality Network Selection since framework has its own
                    // Algorithm of disable/enable
                    break;
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    SupplicantState state = handleSupplicantStateChange(message);
                    // A driver/firmware hang can now put the interface in a down state.
                    // We detect the interface going down and recover from it
                    if (!SupplicantState.isDriverActive(state)) {
                        if (mNetworkInfo.getState() != NetworkInfo.State.DISCONNECTED) {
                            handleNetworkDisconnect();
                        }
                        log("Detected an interface down, restart driver");
                        transitionTo(mDriverStoppedState);
                        sendMessage(CMD_START_DRIVER);
                        break;
                    }

                    // Supplicant can fail to report a NETWORK_DISCONNECTION_EVENT
                    // when authentication times out after a successful connection,
                    // we can figure this from the supplicant state. If supplicant
                    // state is DISCONNECTED, but the mNetworkInfo says we are not
                    // disconnected, we need to handle a disconnection
                    if (!linkDebouncing && state == SupplicantState.DISCONNECTED &&
                            mNetworkInfo.getState() != NetworkInfo.State.DISCONNECTED) {
                        if (DBG) log("Missed CTRL-EVENT-DISCONNECTED, disconnect");
                        handleNetworkDisconnect();
                        transitionTo(mDisconnectedState);
                    }

                    // If we have COMPLETED a connection to a BSSID, start doing
                    // DNAv4/DNAv6 -style probing for on-link neighbors of
                    // interest (e.g. routers); harmless if none are configured.
                    if (state == SupplicantState.COMPLETED) {
                        mIpManager.confirmConfiguration();
                    }
                    break;
                case WifiP2pServiceImpl.DISCONNECT_WIFI_REQUEST:
                    if (message.arg1 == 1) {
                        mWifiNative.disconnect();
                        mTemporarilyDisconnectWifi = true;
                    } else {
                        mWifiNative.reconnect();
                        mTemporarilyDisconnectWifi = false;
                    }
                    break;
                case CMD_ADD_OR_UPDATE_NETWORK:
                    // Only the current foreground user can modify networks.
                    if (!mWifiConfigManager.isCurrentUserProfile(
                            UserHandle.getUserId(message.sendingUid))) {
                        loge("Only the current foreground user can modify networks "
                                + " currentUserId=" + mWifiConfigManager.getCurrentUserId()
                                + " sendingUserId=" + UserHandle.getUserId(message.sendingUid));
                        replyToMessage(message, message.what, FAILURE);
                        break;
                    }

                    config = (WifiConfiguration) message.obj;

                    if (!recordUidIfAuthorized(config, message.sendingUid,
                            /* onlyAnnotate */ false)) {
                        logw("Not authorized to update network "
                             + " config=" + config.SSID
                             + " cnid=" + config.networkId
                             + " uid=" + message.sendingUid);
                        replyToMessage(message, message.what, FAILURE);
                        break;
                    }

                    int res = mWifiConfigManager.addOrUpdateNetwork(config, message.sendingUid);
                    if (res < 0) {
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                    } else {
                        WifiConfiguration curConfig = getCurrentWifiConfiguration();
                        if (curConfig != null && config != null) {
                            WifiConfiguration.NetworkSelectionStatus networkStatus =
                                    config.getNetworkSelectionStatus();
                            if (curConfig.priority < config.priority && networkStatus != null
                                    && !networkStatus.isNetworkPermanentlyDisabled()) {
                                // Interpret this as a connect attempt
                                // Set the last selected configuration so as to allow the system to
                                // stick the last user choice without persisting the choice
                                mWifiConfigManager.setAndEnableLastSelectedConfiguration(res);
                                mWifiConfigManager.updateLastConnectUid(config, message.sendingUid);
                                boolean persist = mWifiConfigManager
                                        .checkConfigOverridePermission(message.sendingUid);
                                if (mWifiConnectivityManager != null) {
                                    mWifiConnectivityManager.connectToUserSelectNetwork(res,
                                            persist);
                                }

                                // Remember time of last connection attempt
                                lastConnectAttemptTimestamp = System.currentTimeMillis();
                                mWifiConnectionStatistics.numWifiManagerJoinAttempt++;

                                // As a courtesy to the caller, trigger a scan now
                                startScan(ADD_OR_UPDATE_SOURCE, 0, null, WIFI_WORK_SOURCE);
                            }
                        }
                    }
                    replyToMessage(message, CMD_ADD_OR_UPDATE_NETWORK, res);
                    break;
                case CMD_REMOVE_NETWORK:
                    // Only the current foreground user can modify networks.
                    if (!mWifiConfigManager.isCurrentUserProfile(
                            UserHandle.getUserId(message.sendingUid))) {
                        loge("Only the current foreground user can modify networks "
                                + " currentUserId=" + mWifiConfigManager.getCurrentUserId()
                                + " sendingUserId=" + UserHandle.getUserId(message.sendingUid));
                        replyToMessage(message, message.what, FAILURE);
                        break;
                    }
                    netId = message.arg1;

                    if (!mWifiConfigManager.canModifyNetwork(message.sendingUid, netId,
                            /* onlyAnnotate */ false)) {
                        logw("Not authorized to remove network "
                             + " cnid=" + netId
                             + " uid=" + message.sendingUid);
                        replyToMessage(message, message.what, FAILURE);
                        break;
                    }

                    ok = mWifiConfigManager.removeNetwork(message.arg1);
                    if (!ok) {
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                    }
                    replyToMessage(message, message.what, ok ? SUCCESS : FAILURE);
                    break;
                case CMD_ENABLE_NETWORK:
                    // Only the current foreground user can modify networks.
                    if (!mWifiConfigManager.isCurrentUserProfile(
                            UserHandle.getUserId(message.sendingUid))) {
                        loge("Only the current foreground user can modify networks "
                                + " currentUserId=" + mWifiConfigManager.getCurrentUserId()
                                + " sendingUserId=" + UserHandle.getUserId(message.sendingUid));
                        replyToMessage(message, message.what, FAILURE);
                        break;
                    }

                    boolean disableOthers = message.arg2 == 1;
                    netId = message.arg1;
                    config = mWifiConfigManager.getWifiConfiguration(netId);
                    if (config == null) {
                        loge("No network with id = " + netId);
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                        replyToMessage(message, message.what, FAILURE);
                        break;
                    }
                    // disable other only means select this network, does not mean all other
                    // networks need to be disabled
                    if (disableOthers) {
                        // Remember time of last connection attempt
                        lastConnectAttemptTimestamp = System.currentTimeMillis();
                        mWifiConnectionStatistics.numWifiManagerJoinAttempt++;
                    }
                    // Cancel auto roam requests
                    autoRoamSetBSSID(netId, "any");

                    ok = mWifiConfigManager.enableNetwork(
                            config, disableOthers, message.sendingUid);
                    if (!ok) {
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                    } else {
                        if (disableOthers) {
                            mTargetNetworkId = netId;
                        }
                        mWifiConnectivityManager.forceConnectivityScan();
                    }

                    replyToMessage(message, message.what, ok ? SUCCESS : FAILURE);
                    break;
                case CMD_ENABLE_ALL_NETWORKS:
                    long time = android.os.SystemClock.elapsedRealtime();
                    if (time - mLastEnableAllNetworksTime > MIN_INTERVAL_ENABLE_ALL_NETWORKS_MS) {
                        mWifiConfigManager.enableAllNetworks();
                        mLastEnableAllNetworksTime = time;
                    }
                    break;
                case WifiManager.DISABLE_NETWORK:
                    if (mWifiConfigManager.updateNetworkSelectionStatus(message.arg1,
                            WifiConfiguration.NetworkSelectionStatus.DISABLED_BY_WIFI_MANAGER)) {
                        replyToMessage(message, WifiManager.DISABLE_NETWORK_SUCCEEDED);
                    } else {
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                        replyToMessage(message, WifiManager.DISABLE_NETWORK_FAILED,
                                WifiManager.ERROR);
                    }
                    break;
                case CMD_DISABLE_EPHEMERAL_NETWORK:
                    config = mWifiConfigManager.disableEphemeralNetwork((String)message.obj);
                    if (config != null) {
                        if (config.networkId == mLastNetworkId) {
                            // Disconnect and let autojoin reselect a new network
                            sendMessage(CMD_DISCONNECT);
                        }
                    }
                    break;
                case CMD_BLACKLIST_NETWORK:
                    mWifiConfigManager.blackListBssid((String) message.obj);
                    break;
                case CMD_CLEAR_BLACKLIST:
                    mWifiConfigManager.clearBssidBlacklist();
                    break;
                case CMD_SAVE_CONFIG:
                    ok = mWifiConfigManager.saveConfig();

                    if (DBG) logd("did save config " + ok);
                    replyToMessage(message, CMD_SAVE_CONFIG, ok ? SUCCESS : FAILURE);

                    // Inform the backup manager about a data change
                    mBackupManagerProxy.notifyDataChanged();
                    break;
                case CMD_GET_CONFIGURED_NETWORKS:
                    replyToMessage(message, message.what,
                            mWifiConfigManager.getSavedNetworks());
                    break;
                case WifiMonitor.SUP_REQUEST_IDENTITY:
                    int networkId = message.arg2;
                    boolean identitySent = false;
                    int eapMethod = WifiEnterpriseConfig.Eap.NONE;

                    if (targetWificonfiguration != null
                            && targetWificonfiguration.enterpriseConfig != null) {
                        eapMethod = targetWificonfiguration.enterpriseConfig.getEapMethod();
                    }

                    // For SIM & AKA/AKA' EAP method Only, get identity from ICC
                    if (targetWificonfiguration != null
                            && targetWificonfiguration.networkId == networkId
                            && targetWificonfiguration.allowedKeyManagement
                                    .get(WifiConfiguration.KeyMgmt.IEEE8021X)
                            && TelephonyUtil.isSimEapMethod(eapMethod)) {
                        String identity = TelephonyUtil.getSimIdentity(mContext, eapMethod);
                        if (identity != null) {
                            mWifiNative.simIdentityResponse(networkId, identity);
                            identitySent = true;
                        }
                    }
                    if (!identitySent) {
                        // Supplicant lacks credentials to connect to that network, hence black list
                        ssid = (String) message.obj;
                        if (targetWificonfiguration != null && ssid != null
                                && targetWificonfiguration.SSID != null
                                && targetWificonfiguration.SSID.equals("\"" + ssid + "\"")) {
                            mWifiConfigManager.updateNetworkSelectionStatus(targetWificonfiguration,
                                    WifiConfiguration.NetworkSelectionStatus
                                            .DISABLED_AUTHENTICATION_NO_CREDENTIALS);
                        }
                        // Disconnect now, as we don't have any way to fullfill
                        // the  supplicant request.
                        mWifiConfigManager.setAndEnableLastSelectedConfiguration(
                                WifiConfiguration.INVALID_NETWORK_ID);
                        mWifiNative.disconnect();
                    }
                    break;
                case WifiMonitor.SUP_REQUEST_SIM_AUTH:
                    logd("Received SUP_REQUEST_SIM_AUTH");
                    SimAuthRequestData requestData = (SimAuthRequestData) message.obj;
                    if (requestData != null) {
                        if (requestData.protocol == WifiEnterpriseConfig.Eap.SIM) {
                            handleGsmAuthRequest(requestData);
                        } else if (requestData.protocol == WifiEnterpriseConfig.Eap.AKA
                            || requestData.protocol == WifiEnterpriseConfig.Eap.AKA_PRIME) {
                            handle3GAuthRequest(requestData);
                        }
                    } else {
                        loge("Invalid sim auth request");
                    }
                    break;
                case CMD_GET_PRIVILEGED_CONFIGURED_NETWORKS:
                    replyToMessage(message, message.what,
                            mWifiConfigManager.getPrivilegedSavedNetworks());
                    break;
                case CMD_GET_MATCHING_CONFIG:
                    replyToMessage(message, message.what,
                            mWifiConfigManager.getMatchingConfig((ScanResult)message.obj));
                    break;
                case CMD_RECONNECT:
                    if (mWifiConnectivityManager != null) {
                        mWifiConnectivityManager.forceConnectivityScan();
                    }
                    break;
                case CMD_REASSOCIATE:
                    lastConnectAttemptTimestamp = System.currentTimeMillis();
                    mWifiNative.reassociate();
                    break;
                case CMD_RELOAD_TLS_AND_RECONNECT:
                    if (mWifiConfigManager.needsUnlockedKeyStore()) {
                        logd("Reconnecting to give a chance to un-connected TLS networks");
                        mWifiNative.disconnect();
                        lastConnectAttemptTimestamp = System.currentTimeMillis();
                        mWifiNative.reconnect();
                    }
                    break;
                case CMD_AUTO_ROAM:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    return HANDLED;
                case CMD_AUTO_CONNECT:
                    /* Work Around: wpa_supplicant can get in a bad state where it returns a non
                     * associated status to the STATUS command but somehow-someplace still thinks
                     * it is associated and thus will ignore select/reconnect command with
                     * following message:
                     * "Already associated with the selected network - do nothing"
                     *
                     * Hence, sends a disconnect to supplicant first.
                     */
                    didDisconnect = false;
                    if (getCurrentState() != mDisconnectedState) {
                        /** Supplicant will ignore the reconnect if we are currently associated,
                         * hence trigger a disconnect
                         */
                        didDisconnect = true;
                        mWifiNative.disconnect();
                    }

                    /* connect command coming from auto-join */
                    netId = message.arg1;
                    mTargetNetworkId = netId;
                    mTargetRoamBSSID = (String) message.obj;
                    config = mWifiConfigManager.getWifiConfiguration(netId);
                    logd("CMD_AUTO_CONNECT sup state "
                            + mSupplicantStateTracker.getSupplicantStateName()
                            + " my state " + getCurrentState().getName()
                            + " nid=" + Integer.toString(netId)
                            + " roam=" + Boolean.toString(mAutoRoaming));
                    if (config == null) {
                        loge("AUTO_CONNECT and no config, bail out...");
                        break;
                    }

                    /* Make sure we cancel any previous roam request */
                    setTargetBssid(config, mTargetRoamBSSID);

                    /* Save the network config */
                    logd("CMD_AUTO_CONNECT will save config -> " + config.SSID
                            + " nid=" + Integer.toString(netId));
                    result = mWifiConfigManager.saveNetwork(config, WifiConfiguration.UNKNOWN_UID);
                    netId = result.getNetworkId();
                    logd("CMD_AUTO_CONNECT did save config -> "
                            + " nid=" + Integer.toString(netId));

                    // Since we updated the config,read it back from config store:
                    config = mWifiConfigManager.getWifiConfiguration(netId);
                    if (config == null) {
                        loge("CMD_AUTO_CONNECT couldn't update the config, got null config");
                        break;
                    }
                    if (netId != config.networkId) {
                        loge("CMD_AUTO_CONNECT couldn't update the config, want"
                                + " nid=" + Integer.toString(netId) + " but got" + config.networkId);
                        break;
                    }

                    if (deferForUserInput(message, netId, false)) {
                        break;
                    } else if (mWifiConfigManager.getWifiConfiguration(netId).userApproved ==
                                                                   WifiConfiguration.USER_BANNED) {
                        replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                                WifiManager.NOT_AUTHORIZED);
                        break;
                    }

                    // If we're autojoining a network that the user or an app explicitly selected,
                    // keep track of the UID that selected it.
                    // TODO(b/26786318): Keep track of the lastSelectedConfiguration and the
                    // lastConnectUid on a per-user basis.
                    int lastConnectUid = WifiConfiguration.UNKNOWN_UID;

                    //Start a new ConnectionEvent due to auto_connect, assume we are connecting
                    //between different networks due to QNS, setting ROAM_UNRELATED
                    mWifiMetrics.startConnectionEvent(config, mTargetRoamBSSID,
                            WifiMetricsProto.ConnectionEvent.ROAM_UNRELATED);
                    if (!didDisconnect) {
                        //If we were originally disconnected, then this was not any kind of ROAM
                        mWifiMetrics.setConnectionEventRoamType(
                                WifiMetricsProto.ConnectionEvent.ROAM_NONE);
                    }
                    //Determine if this CONNECTION is for a user selection
                    if (mWifiConfigManager.isLastSelectedConfiguration(config)
                            && mWifiConfigManager.isCurrentUserProfile(
                                    UserHandle.getUserId(config.lastConnectUid))) {
                        lastConnectUid = config.lastConnectUid;
                        mWifiMetrics.setConnectionEventRoamType(
                                WifiMetricsProto.ConnectionEvent.ROAM_USER_SELECTED);
                    }
                    if (mWifiConfigManager.selectNetwork(config, /* updatePriorities = */ false,
                            lastConnectUid) && mWifiNative.reconnect()) {
                        lastConnectAttemptTimestamp = System.currentTimeMillis();
                        targetWificonfiguration = mWifiConfigManager.getWifiConfiguration(netId);
                        config = mWifiConfigManager.getWifiConfiguration(netId);
                        if (config != null
                                && !mWifiConfigManager.isLastSelectedConfiguration(config)) {
                            // If we autojoined a different config than the user selected one,
                            // it means we could not see the last user selection,
                            // or that the last user selection was faulty and ended up blacklisted
                            // for some reason (in which case the user is notified with an error
                            // message in the Wifi picker), and thus we managed to auto-join away
                            // from the selected  config. -> in that case we need to forget
                            // the selection because we don't want to abruptly switch back to it.
                            //
                            // Note that the user selection is also forgotten after a period of time
                            // during which the device has been disconnected.
                            // The default value is 30 minutes : see the code path at bottom of
                            // setScanResults() function.
                            mWifiConfigManager.
                                 setAndEnableLastSelectedConfiguration(
                                         WifiConfiguration.INVALID_NETWORK_ID);
                        }
                        mAutoRoaming = false;
                        if (isRoaming() || linkDebouncing) {
                            transitionTo(mRoamingState);
                        } else if (didDisconnect) {
                            transitionTo(mDisconnectingState);
                        }
                    } else {
                        loge("Failed to connect config: " + config + " netId: " + netId);
                        replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                                WifiManager.ERROR);
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_CONNECT_NETWORK_FAILED,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE);
                        break;
                    }
                    break;
                case CMD_REMOVE_APP_CONFIGURATIONS:
                    mWifiConfigManager.removeNetworksForApp((ApplicationInfo) message.obj);
                    break;
                case CMD_REMOVE_USER_CONFIGURATIONS:
                    mWifiConfigManager.removeNetworksForUser(message.arg1);
                    break;
                case WifiManager.CONNECT_NETWORK:
                    // Only the current foreground user and System UI (which runs as user 0 but acts
                    // on behalf of the current foreground user) can modify networks.
                    if (!mWifiConfigManager.isCurrentUserProfile(
                            UserHandle.getUserId(message.sendingUid)) &&
                            message.sendingUid != mSystemUiUid) {
                        loge("Only the current foreground user can modify networks "
                                + " currentUserId=" + mWifiConfigManager.getCurrentUserId()
                                + " sendingUserId=" + UserHandle.getUserId(message.sendingUid));
                        replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                                       WifiManager.NOT_AUTHORIZED);
                        break;
                    }

                    /**
                     *  The connect message can contain a network id passed as arg1 on message or
                     * or a config passed as obj on message.
                     * For a new network, a config is passed to create and connect.
                     * For an existing network, a network id is passed
                     */
                    netId = message.arg1;
                    config = (WifiConfiguration) message.obj;
                    mWifiConnectionStatistics.numWifiManagerJoinAttempt++;
                    boolean updatedExisting = false;

                    /* Save the network config */
                    if (config != null) {
                        // When connecting to an access point, WifiStateMachine wants to update the
                        // relevant config with administrative data. This update should not be
                        // considered a 'real' update, therefore lockdown by Device Owner must be
                        // disregarded.
                        if (!recordUidIfAuthorized(config, message.sendingUid,
                                /* onlyAnnotate */ true)) {
                            logw("Not authorized to update network "
                                 + " config=" + config.SSID
                                 + " cnid=" + config.networkId
                                 + " uid=" + message.sendingUid);
                            replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                                           WifiManager.NOT_AUTHORIZED);
                            break;
                        }
                        String configKey = config.configKey(true /* allowCached */);
                        WifiConfiguration savedConfig =
                                mWifiConfigManager.getWifiConfiguration(configKey);
                        if (savedConfig != null) {
                            // There is an existing config with this netId, but it wasn't exposed
                            // (either AUTO_JOIN_DELETED or ephemeral; see WifiConfigManager#
                            // getConfiguredNetworks). Remove those bits and update the config.
                            config = savedConfig;
                            logd("CONNECT_NETWORK updating existing config with id=" +
                                    config.networkId + " configKey=" + configKey);
                            config.ephemeral = false;
                            mWifiConfigManager.updateNetworkSelectionStatus(config,
                                    WifiConfiguration.NetworkSelectionStatus
                                    .NETWORK_SELECTION_ENABLE);
                            updatedExisting = true;
                        }

                        result = mWifiConfigManager.saveNetwork(config, message.sendingUid);
                        netId = result.getNetworkId();
                    }
                    config = mWifiConfigManager.getWifiConfiguration(netId);
                    if (config == null) {
                        logd("CONNECT_NETWORK no config for id=" + Integer.toString(netId) + " "
                                + mSupplicantStateTracker.getSupplicantStateName() + " my state "
                                + getCurrentState().getName());
                        replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                                WifiManager.ERROR);
                        break;
                    }
                    mTargetNetworkId = netId;
                    autoRoamSetBSSID(netId, "any");
                    if (message.sendingUid == Process.WIFI_UID
                        || message.sendingUid == Process.SYSTEM_UID) {
                        // As a sanity measure, clear the BSSID in the supplicant network block.
                        // If system or Wifi Settings want to connect, they will not
                        // specify the BSSID.
                        // If an app however had added a BSSID to this configuration, and the BSSID
                        // was wrong, Then we would forever fail to connect until that BSSID
                        // is cleaned up.
                        clearConfigBSSID(config, "CONNECT_NETWORK");
                    }

                    if (deferForUserInput(message, netId, true)) {
                        break;
                    } else if (mWifiConfigManager.getWifiConfiguration(netId).userApproved ==
                                                                    WifiConfiguration.USER_BANNED) {
                        replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                                WifiManager.NOT_AUTHORIZED);
                        break;
                    }

                    mAutoRoaming = false;

                    /* Tell network selection the user did try to connect to that network if from
                    settings */
                    boolean persist =
                        mWifiConfigManager.checkConfigOverridePermission(message.sendingUid);


                    mWifiConfigManager.setAndEnableLastSelectedConfiguration(netId);
                    if (mWifiConnectivityManager != null) {
                        mWifiConnectivityManager.connectToUserSelectNetwork(netId, persist);
                    }
                    didDisconnect = false;
                    if (mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID
                            && mLastNetworkId != netId) {
                        /** Supplicant will ignore the reconnect if we are currently associated,
                         * hence trigger a disconnect
                         */
                        didDisconnect = true;
                        mWifiNative.disconnect();
                    }

                    //Start a new ConnectionEvent due to connect_network, this is always user
                    //selected
                    mWifiMetrics.startConnectionEvent(config, mTargetRoamBSSID,
                            WifiMetricsProto.ConnectionEvent.ROAM_USER_SELECTED);
                    if (mWifiConfigManager.selectNetwork(config, /* updatePriorities = */ true,
                            message.sendingUid) && mWifiNative.reconnect()) {
                        lastConnectAttemptTimestamp = System.currentTimeMillis();
                        targetWificonfiguration = mWifiConfigManager.getWifiConfiguration(netId);

                        /* The state tracker handles enabling networks upon completion/failure */
                        mSupplicantStateTracker.sendMessage(WifiManager.CONNECT_NETWORK);
                        replyToMessage(message, WifiManager.CONNECT_NETWORK_SUCCEEDED);
                        if (didDisconnect) {
                            /* Expect a disconnection from the old connection */
                            transitionTo(mDisconnectingState);
                        } else if (updatedExisting && getCurrentState() == mConnectedState &&
                                getCurrentWifiConfiguration().networkId == netId) {
                            // Update the current set of network capabilities, but stay in the
                            // current state.
                            updateCapabilities(config);
                        } else {
                            /**
                             * Directly go to disconnected state where we
                             * process the connection events from supplicant
                             */
                            transitionTo(mDisconnectedState);
                        }
                    } else {
                        loge("Failed to connect config: " + config + " netId: " + netId);
                        replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                                WifiManager.ERROR);
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_CONNECT_NETWORK_FAILED,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE);
                        break;
                    }
                    break;
                case WifiManager.SAVE_NETWORK:
                    mWifiConnectionStatistics.numWifiManagerJoinAttempt++;
                    // Fall thru
                case WifiStateMachine.CMD_AUTO_SAVE_NETWORK:
                    // Only the current foreground user can modify networks.
                    if (!mWifiConfigManager.isCurrentUserProfile(
                            UserHandle.getUserId(message.sendingUid))) {
                        loge("Only the current foreground user can modify networks "
                                + " currentUserId=" + mWifiConfigManager.getCurrentUserId()
                                + " sendingUserId=" + UserHandle.getUserId(message.sendingUid));
                        replyToMessage(message, WifiManager.SAVE_NETWORK_FAILED,
                                WifiManager.NOT_AUTHORIZED);
                        break;
                    }

                    lastSavedConfigurationAttempt = null; // Used for debug
                    config = (WifiConfiguration) message.obj;
                    if (config == null) {
                        loge("ERROR: SAVE_NETWORK with null configuration"
                                + mSupplicantStateTracker.getSupplicantStateName()
                                + " my state " + getCurrentState().getName());
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                        replyToMessage(message, WifiManager.SAVE_NETWORK_FAILED,
                                WifiManager.ERROR);
                        break;
                    }
                    lastSavedConfigurationAttempt = new WifiConfiguration(config);
                    int nid = config.networkId;
                    logd("SAVE_NETWORK id=" + Integer.toString(nid)
                                + " config=" + config.SSID
                                + " nid=" + config.networkId
                                + " supstate=" + mSupplicantStateTracker.getSupplicantStateName()
                                + " my state " + getCurrentState().getName());

                    // Only record the uid if this is user initiated
                    boolean checkUid = (message.what == WifiManager.SAVE_NETWORK);
                    if (checkUid && !recordUidIfAuthorized(config, message.sendingUid,
                            /* onlyAnnotate */ false)) {
                        logw("Not authorized to update network "
                             + " config=" + config.SSID
                             + " cnid=" + config.networkId
                             + " uid=" + message.sendingUid);
                        replyToMessage(message, WifiManager.SAVE_NETWORK_FAILED,
                                       WifiManager.NOT_AUTHORIZED);
                        break;
                    }

                    result = mWifiConfigManager.saveNetwork(config, WifiConfiguration.UNKNOWN_UID);
                    if (result.getNetworkId() != WifiConfiguration.INVALID_NETWORK_ID) {
                        if (mWifiInfo.getNetworkId() == result.getNetworkId()) {
                            if (result.hasIpChanged()) {
                                // The currently connection configuration was changed
                                // We switched from DHCP to static or from static to DHCP, or the
                                // static IP address has changed.
                                log("Reconfiguring IP on connection");
                                // TODO: clear addresses and disable IPv6
                                // to simplify obtainingIpState.
                                transitionTo(mObtainingIpState);
                            }
                            if (result.hasProxyChanged()) {
                                log("Reconfiguring proxy on connection");
                                mIpManager.setHttpProxy(
                                        mWifiConfigManager.getProxyProperties(mLastNetworkId));
                            }
                        }
                        replyToMessage(message, WifiManager.SAVE_NETWORK_SUCCEEDED);
                        broadcastWifiCredentialChanged(WifiManager.WIFI_CREDENTIAL_SAVED, config);

                        if (DBG) {
                           logd("Success save network nid="
                                    + Integer.toString(result.getNetworkId()));
                        }

                        /**
                         * If the command comes from WifiManager, then
                         * tell autojoin the user did try to modify and save that network,
                         * and interpret the SAVE_NETWORK as a request to connect
                         */
                        boolean user = message.what == WifiManager.SAVE_NETWORK;

                        // Did this connect come from settings
                        boolean persistConnect =
                                mWifiConfigManager.checkConfigOverridePermission(
                                        message.sendingUid);

                        if (user) {
                            mWifiConfigManager.updateLastConnectUid(config, message.sendingUid);
                            mWifiConfigManager.writeKnownNetworkHistory();
                        }

                        if (mWifiConnectivityManager != null) {
                            mWifiConnectivityManager.connectToUserSelectNetwork(
                                    result.getNetworkId(), persistConnect);
                        }
                    } else {
                        loge("Failed to save network");
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                        replyToMessage(message, WifiManager.SAVE_NETWORK_FAILED,
                                WifiManager.ERROR);
                    }
                    break;
                case WifiManager.FORGET_NETWORK:
                    // Only the current foreground user can modify networks.
                    if (!mWifiConfigManager.isCurrentUserProfile(
                            UserHandle.getUserId(message.sendingUid))) {
                        loge("Only the current foreground user can modify networks "
                                + " currentUserId=" + mWifiConfigManager.getCurrentUserId()
                                + " sendingUserId=" + UserHandle.getUserId(message.sendingUid));
                        replyToMessage(message, WifiManager.FORGET_NETWORK_FAILED,
                                WifiManager.NOT_AUTHORIZED);
                        break;
                    }

                    // Debug only, remember last configuration that was forgotten
                    WifiConfiguration toRemove
                            = mWifiConfigManager.getWifiConfiguration(message.arg1);
                    if (toRemove == null) {
                        lastForgetConfigurationAttempt = null;
                    } else {
                        lastForgetConfigurationAttempt = new WifiConfiguration(toRemove);
                    }
                    // check that the caller owns this network
                    netId = message.arg1;

                    if (!mWifiConfigManager.canModifyNetwork(message.sendingUid, netId,
                            /* onlyAnnotate */ false)) {
                        logw("Not authorized to forget network "
                             + " cnid=" + netId
                             + " uid=" + message.sendingUid);
                        replyToMessage(message, WifiManager.FORGET_NETWORK_FAILED,
                                WifiManager.NOT_AUTHORIZED);
                        break;
                    }

                    if (mWifiConfigManager.forgetNetwork(message.arg1)) {
                        replyToMessage(message, WifiManager.FORGET_NETWORK_SUCCEEDED);
                        broadcastWifiCredentialChanged(WifiManager.WIFI_CREDENTIAL_FORGOT,
                                (WifiConfiguration) message.obj);
                    } else {
                        loge("Failed to forget network");
                        replyToMessage(message, WifiManager.FORGET_NETWORK_FAILED,
                                WifiManager.ERROR);
                    }
                    break;
                case WifiManager.START_WPS:
                    WpsInfo wpsInfo = (WpsInfo) message.obj;
                    WpsResult wpsResult;
                    switch (wpsInfo.setup) {
                        case WpsInfo.PBC:
                            wpsResult = mWifiConfigManager.startWpsPbc(wpsInfo);
                            break;
                        case WpsInfo.KEYPAD:
                            wpsResult = mWifiConfigManager.startWpsWithPinFromAccessPoint(wpsInfo);
                            break;
                        case WpsInfo.DISPLAY:
                            wpsResult = mWifiConfigManager.startWpsWithPinFromDevice(wpsInfo);
                            break;
                        default:
                            wpsResult = new WpsResult(Status.FAILURE);
                            loge("Invalid setup for WPS");
                            break;
                    }
                    mWifiConfigManager.setAndEnableLastSelectedConfiguration
                            (WifiConfiguration.INVALID_NETWORK_ID);
                    if (wpsResult.status == Status.SUCCESS) {
                        replyToMessage(message, WifiManager.START_WPS_SUCCEEDED, wpsResult);
                        transitionTo(mWpsRunningState);
                    } else {
                        loge("Failed to start WPS with config " + wpsInfo.toString());
                        replyToMessage(message, WifiManager.WPS_FAILED, WifiManager.ERROR);
                    }
                    break;
                case CMD_ASSOCIATED_BSSID:
                    // This is where we can confirm the connection BSSID. Use it to find the
                    // right ScanDetail to populate metrics.
                    String someBssid = (String) message.obj;
                    if (someBssid != null) {
                        //Get the config associated with this connection attempt
                        WifiConfiguration someConf =
                                mWifiConfigManager.getWifiConfiguration(mTargetNetworkId);
                        // Get the ScanDetail associated with this BSSID
                        ScanDetailCache scanDetailCache = mWifiConfigManager.getScanDetailCache(
                                someConf);
                        if (scanDetailCache != null) {
                            mWifiMetrics.setConnectionScanDetail(scanDetailCache.getScanDetail(
                                    someBssid));
                        }
                    }
                    return NOT_HANDLED;
                case WifiMonitor.NETWORK_CONNECTION_EVENT:
                    if (DBG) log("Network connection established");
                    mLastNetworkId = message.arg1;
                    mLastBssid = (String) message.obj;

                    mWifiInfo.setBSSID(mLastBssid);
                    mWifiInfo.setNetworkId(mLastNetworkId);
                    mWifiQualifiedNetworkSelector
                            .enableBssidForQualityNetworkSelection(mLastBssid, true);
                    sendNetworkStateChangeBroadcast(mLastBssid);
                    transitionTo(mObtainingIpState);
                    break;
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                    // Calling handleNetworkDisconnect here is redundant because we might already
                    // have called it when leaving L2ConnectedState to go to disconnecting state
                    // or thru other path
                    // We should normally check the mWifiInfo or mLastNetworkId so as to check
                    // if they are valid, and only in this case call handleNEtworkDisconnect,
                    // TODO: this should be fixed for a L MR release
                    // The side effect of calling handleNetworkDisconnect twice is that a bunch of
                    // idempotent commands are executed twice (stopping Dhcp, enabling the SPS mode
                    // at the chip etc...
                    if (DBG) log("ConnectModeState: Network connection lost ");
                    handleNetworkDisconnect();
                    transitionTo(mDisconnectedState);
                    break;
                case CMD_ADD_PASSPOINT_MO:
                    res = mWifiConfigManager.addPasspointManagementObject((String) message.obj);
                    replyToMessage(message, message.what, res);
                    break;
                case CMD_MODIFY_PASSPOINT_MO:
                    if (message.obj != null) {
                        Bundle bundle = (Bundle) message.obj;
                        ArrayList<PasspointManagementObjectDefinition> mos =
                                bundle.getParcelableArrayList("MOS");
                        res = mWifiConfigManager.modifyPasspointMo(bundle.getString("FQDN"), mos);
                    } else {
                        res = 0;
                    }
                    replyToMessage(message, message.what, res);

                    break;
                case CMD_QUERY_OSU_ICON:
                    if (mWifiConfigManager.queryPasspointIcon(
                            ((Bundle) message.obj).getLong("BSSID"),
                            ((Bundle) message.obj).getString("FILENAME"))) {
                        res = 1;
                    } else {
                        res = 0;
                    }
                    replyToMessage(message, message.what, res);
                    break;
                case CMD_MATCH_PROVIDER_NETWORK:
                    res = mWifiConfigManager.matchProviderWithCurrentNetwork((String) message.obj);
                    replyToMessage(message, message.what, res);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    private void updateCapabilities(WifiConfiguration config) {
        NetworkCapabilities networkCapabilities = new NetworkCapabilities(mDfltNetworkCapabilities);
        if (config != null) {
            if (config.ephemeral) {
                networkCapabilities.removeCapability(
                        NetworkCapabilities.NET_CAPABILITY_TRUSTED);
            } else {
                networkCapabilities.addCapability(
                        NetworkCapabilities.NET_CAPABILITY_TRUSTED);
            }

            networkCapabilities.setSignalStrength(
                    (mWifiInfo.getRssi() != WifiInfo.INVALID_RSSI)
                    ? mWifiInfo.getRssi()
                    : NetworkCapabilities.SIGNAL_STRENGTH_UNSPECIFIED);
        }

        if (mWifiInfo.getMeteredHint()) {
            networkCapabilities.removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED);
        }

        mNetworkAgent.sendNetworkCapabilities(networkCapabilities);
    }

    private class WifiNetworkAgent extends NetworkAgent {
        public WifiNetworkAgent(Looper l, Context c, String TAG, NetworkInfo ni,
                NetworkCapabilities nc, LinkProperties lp, int score, NetworkMisc misc) {
            super(l, c, TAG, ni, nc, lp, score, misc);
        }
        protected void unwanted() {
            // Ignore if we're not the current networkAgent.
            if (this != mNetworkAgent) return;
            if (DBG) log("WifiNetworkAgent -> Wifi unwanted score "
                    + Integer.toString(mWifiInfo.score));
            unwantedNetwork(NETWORK_STATUS_UNWANTED_DISCONNECT);
        }

        @Override
        protected void networkStatus(int status, String redirectUrl) {
            if (this != mNetworkAgent) return;
            if (status == NetworkAgent.INVALID_NETWORK) {
                if (DBG) log("WifiNetworkAgent -> Wifi networkStatus invalid, score="
                        + Integer.toString(mWifiInfo.score));
                unwantedNetwork(NETWORK_STATUS_UNWANTED_VALIDATION_FAILED);
            } else if (status == NetworkAgent.VALID_NETWORK) {
                if (DBG) {
                    log("WifiNetworkAgent -> Wifi networkStatus valid, score= "
                            + Integer.toString(mWifiInfo.score));
                }
                doNetworkStatus(status);
            }
        }

        @Override
        protected void saveAcceptUnvalidated(boolean accept) {
            if (this != mNetworkAgent) return;
            WifiStateMachine.this.sendMessage(CMD_ACCEPT_UNVALIDATED, accept ? 1 : 0);
        }

        @Override
        protected void startPacketKeepalive(Message msg) {
            WifiStateMachine.this.sendMessage(
                    CMD_START_IP_PACKET_OFFLOAD, msg.arg1, msg.arg2, msg.obj);
        }

        @Override
        protected void stopPacketKeepalive(Message msg) {
            WifiStateMachine.this.sendMessage(
                    CMD_STOP_IP_PACKET_OFFLOAD, msg.arg1, msg.arg2, msg.obj);
        }

        @Override
        protected void setSignalStrengthThresholds(int[] thresholds) {
            // 0. If there are no thresholds, or if the thresholds are invalid, stop RSSI monitoring.
            // 1. Tell the hardware to start RSSI monitoring here, possibly adding MIN_VALUE and
            //    MAX_VALUE at the start/end of the thresholds array if necessary.
            // 2. Ensure that when the hardware event fires, we fetch the RSSI from the hardware
            //    event, call mWifiInfo.setRssi() with it, and call updateCapabilities(), and then
            //    re-arm the hardware event. This needs to be done on the state machine thread to
            //    avoid race conditions. The RSSI used to re-arm the event (and perhaps also the one
            //    sent in the NetworkCapabilities) must be the one received from the hardware event
            //    received, or we might skip callbacks.
            // 3. Ensure that when we disconnect, RSSI monitoring is stopped.
            log("Received signal strength thresholds: " + Arrays.toString(thresholds));
            if (thresholds.length == 0) {
                WifiStateMachine.this.sendMessage(CMD_STOP_RSSI_MONITORING_OFFLOAD,
                        mWifiInfo.getRssi());
                return;
            }
            int [] rssiVals = Arrays.copyOf(thresholds, thresholds.length + 2);
            rssiVals[rssiVals.length - 2] = Byte.MIN_VALUE;
            rssiVals[rssiVals.length - 1] = Byte.MAX_VALUE;
            Arrays.sort(rssiVals);
            byte[] rssiRange = new byte[rssiVals.length];
            for (int i = 0; i < rssiVals.length; i++) {
                int val = rssiVals[i];
                if (val <= Byte.MAX_VALUE && val >= Byte.MIN_VALUE) {
                    rssiRange[i] = (byte) val;
                } else {
                    Log.e(TAG, "Illegal value " + val + " for RSSI thresholds: "
                            + Arrays.toString(rssiVals));
                    WifiStateMachine.this.sendMessage(CMD_STOP_RSSI_MONITORING_OFFLOAD,
                            mWifiInfo.getRssi());
                    return;
                }
            }
            // TODO: Do we quash rssi values in this sorted array which are very close?
            mRssiRanges = rssiRange;
            WifiStateMachine.this.sendMessage(CMD_START_RSSI_MONITORING_OFFLOAD,
                    mWifiInfo.getRssi());
        }

        @Override
        protected void preventAutomaticReconnect() {
            if (this != mNetworkAgent) return;
            unwantedNetwork(NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN);
        }
    }

    void unwantedNetwork(int reason) {
        sendMessage(CMD_UNWANTED_NETWORK, reason);
    }

    void doNetworkStatus(int status) {
        sendMessage(CMD_NETWORK_STATUS, status);
    }

    // rfc4186 & rfc4187:
    // create Permanent Identity base on IMSI,
    // identity = usernam@realm
    // with username = prefix | IMSI
    // and realm is derived MMC/MNC tuple according 3GGP spec(TS23.003)
    private String buildIdentity(int eapMethod, String imsi, String mccMnc) {
        String mcc;
        String mnc;
        String prefix;

        if (imsi == null || imsi.isEmpty())
            return "";

        if (eapMethod == WifiEnterpriseConfig.Eap.SIM)
            prefix = "1";
        else if (eapMethod == WifiEnterpriseConfig.Eap.AKA)
            prefix = "0";
        else if (eapMethod == WifiEnterpriseConfig.Eap.AKA_PRIME)
            prefix = "6";
        else  // not a valide EapMethod
            return "";

        /* extract mcc & mnc from mccMnc */
        if (mccMnc != null && !mccMnc.isEmpty()) {
            mcc = mccMnc.substring(0, 3);
            mnc = mccMnc.substring(3);
            if (mnc.length() == 2)
                mnc = "0" + mnc;
        } else {
            // extract mcc & mnc from IMSI, assume mnc size is 3
            mcc = imsi.substring(0, 3);
            mnc = imsi.substring(3, 6);
        }

        return prefix + imsi + "@wlan.mnc" + mnc + ".mcc" + mcc + ".3gppnetwork.org";
    }

    boolean startScanForConfiguration(WifiConfiguration config) {
        if (config == null)
            return false;

        // We are still seeing a fairly high power consumption triggered by autojoin scans
        // Hence do partial scans only for PSK configuration that are roamable since the
        // primary purpose of the partial scans is roaming.
        // Full badn scans with exponential backoff for the purpose or extended roaming and
        // network switching are performed unconditionally.
        ScanDetailCache scanDetailCache =
                mWifiConfigManager.getScanDetailCache(config);
        if (scanDetailCache == null
                || !config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.WPA_PSK)
                || scanDetailCache.size() > 6) {
            //return true but to not trigger the scan
            return true;
        }
        HashSet<Integer> freqs = mWifiConfigManager.makeChannelList(config, ONE_HOUR_MILLI);
        if (freqs != null && freqs.size() != 0) {
            //if (DBG) {
            logd("starting scan for " + config.configKey() + " with " + freqs);
            //}
            Set<Integer> hiddenNetworkIds = new HashSet<>();
            if (config.hiddenSSID) {
                hiddenNetworkIds.add(config.networkId);
            }
            // Call wifi native to start the scan
            if (startScanNative(freqs, hiddenNetworkIds, WIFI_WORK_SOURCE)) {
                messageHandlingStatus = MESSAGE_HANDLING_STATUS_OK;
            } else {
                // used for debug only, mark scan as failed
                messageHandlingStatus = MESSAGE_HANDLING_STATUS_HANDLING_ERROR;
            }
            return true;
        } else {
            if (DBG) logd("no channels for " + config.configKey());
            return false;
        }
    }

    void clearCurrentConfigBSSID(String dbg) {
        // Clear the bssid in the current config's network block
        WifiConfiguration config = getCurrentWifiConfiguration();
        if (config == null)
            return;
        clearConfigBSSID(config, dbg);
    }
    void clearConfigBSSID(WifiConfiguration config, String dbg) {
        if (config == null)
            return;
        if (DBG) {
            logd(dbg + " " + mTargetRoamBSSID + " config " + config.configKey()
                    + " config.NetworkSelectionStatus.mNetworkSelectionBSSID "
                    + config.getNetworkSelectionStatus().getNetworkSelectionBSSID());
        }
        if (DBG) {
           logd(dbg + " " + config.SSID
                    + " nid=" + Integer.toString(config.networkId));
        }
        mWifiConfigManager.saveWifiConfigBSSID(config, "any");
    }

    class L2ConnectedState extends State {
        @Override
        public void enter() {
            mRssiPollToken++;
            if (mEnableRssiPolling) {
                sendMessage(CMD_RSSI_POLL, mRssiPollToken, 0);
            }
            if (mNetworkAgent != null) {
                loge("Have NetworkAgent when entering L2Connected");
                setNetworkDetailedState(DetailedState.DISCONNECTED);
            }
            setNetworkDetailedState(DetailedState.CONNECTING);

            mNetworkAgent = new WifiNetworkAgent(getHandler().getLooper(), mContext,
                    "WifiNetworkAgent", mNetworkInfo, mNetworkCapabilitiesFilter,
                    mLinkProperties, 60, mNetworkMisc);

            // We must clear the config BSSID, as the wifi chipset may decide to roam
            // from this point on and having the BSSID specified in the network block would
            // cause the roam to faile and the device to disconnect
            clearCurrentConfigBSSID("L2ConnectedState");
            mCountryCode.setReadyForChange(false);
            mWifiMetrics.setWifiState(WifiMetricsProto.WifiLog.WIFI_ASSOCIATED);
        }

        @Override
        public void exit() {
            mIpManager.stop();

            // This is handled by receiving a NETWORK_DISCONNECTION_EVENT in ConnectModeState
            // Bug: 15347363
            // For paranoia's sake, call handleNetworkDisconnect
            // only if BSSID is null or last networkId
            // is not invalid.
            if (DBG) {
                StringBuilder sb = new StringBuilder();
                sb.append("leaving L2ConnectedState state nid=" + Integer.toString(mLastNetworkId));
                if (mLastBssid !=null) {
                    sb.append(" ").append(mLastBssid);
                }
            }
            if (mLastBssid != null || mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID) {
                handleNetworkDisconnect();
            }
            mCountryCode.setReadyForChange(true);
            mWifiMetrics.setWifiState(WifiMetricsProto.WifiLog.WIFI_DISCONNECTED);
        }

        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch (message.what) {
                case DhcpClient.CMD_PRE_DHCP_ACTION:
                    handlePreDhcpSetup();
                    break;
                case DhcpClient.CMD_PRE_DHCP_ACTION_COMPLETE:
                    mIpManager.completedPreDhcpAction();
                    break;
                case DhcpClient.CMD_POST_DHCP_ACTION:
                    handlePostDhcpSetup();
                    // We advance to mConnectedState because IpManager will also send a
                    // CMD_IPV4_PROVISIONING_SUCCESS message, which calls handleIPv4Success(),
                    // which calls updateLinkProperties, which then sends
                    // CMD_IP_CONFIGURATION_SUCCESSFUL.
                    //
                    // In the event of failure, we transition to mDisconnectingState
                    // similarly--via messages sent back from IpManager.
                    break;
                case CMD_IPV4_PROVISIONING_SUCCESS: {
                    handleIPv4Success((DhcpResults) message.obj);
                    sendNetworkStateChangeBroadcast(mLastBssid);
                    break;
                }
                case CMD_IPV4_PROVISIONING_FAILURE: {
                    handleIPv4Failure();
                    break;
                }
                case CMD_IP_CONFIGURATION_SUCCESSFUL:
                    handleSuccessfulIpConfiguration();
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_NONE,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE);
                    sendConnectedState();
                    transitionTo(mConnectedState);
                    break;
                case CMD_IP_CONFIGURATION_LOST:
                    // Get Link layer stats so that we get fresh tx packet counters.
                    getWifiLinkLayerStats(true);
                    handleIpConfigurationLost();
                    transitionTo(mDisconnectingState);
                    break;
                case CMD_IP_REACHABILITY_LOST:
                    if (DBG && message.obj != null) log((String) message.obj);
                    handleIpReachabilityLost();
                    transitionTo(mDisconnectingState);
                    break;
                case CMD_DISCONNECT:
                    mWifiNative.disconnect();
                    transitionTo(mDisconnectingState);
                    break;
                case WifiP2pServiceImpl.DISCONNECT_WIFI_REQUEST:
                    if (message.arg1 == 1) {
                        mWifiNative.disconnect();
                        mTemporarilyDisconnectWifi = true;
                        transitionTo(mDisconnectingState);
                    }
                    break;
                case CMD_SET_OPERATIONAL_MODE:
                    if (message.arg1 != CONNECT_MODE) {
                        sendMessage(CMD_DISCONNECT);
                        deferMessage(message);
                        if (message.arg1 == SCAN_ONLY_WITH_WIFI_OFF_MODE) {
                            noteWifiDisabledWhileAssociated();
                        }
                    }
                    mWifiConfigManager.
                                setAndEnableLastSelectedConfiguration(
                                        WifiConfiguration.INVALID_NETWORK_ID);
                    break;
                    /* Ignore connection to same network */
                case WifiManager.CONNECT_NETWORK:
                    int netId = message.arg1;
                    if (mWifiInfo.getNetworkId() == netId) {
                        break;
                    }
                    return NOT_HANDLED;
                case WifiMonitor.NETWORK_CONNECTION_EVENT:
                    mWifiInfo.setBSSID((String) message.obj);
                    mLastNetworkId = message.arg1;
                    mWifiInfo.setNetworkId(mLastNetworkId);
                    if(!mLastBssid.equals((String) message.obj)) {
                        mLastBssid = (String) message.obj;
                        sendNetworkStateChangeBroadcast(mLastBssid);
                    }
                    break;
                case CMD_RSSI_POLL:
                    if (message.arg1 == mRssiPollToken) {
                        if (mWifiConfigManager.mEnableChipWakeUpWhenAssociated.get()) {
                            if (DBG) log(" get link layer stats " + mWifiLinkLayerStatsSupported);
                            WifiLinkLayerStats stats = getWifiLinkLayerStats(DBG);
                            if (stats != null) {
                                // Sanity check the results provided by driver
                                if (mWifiInfo.getRssi() != WifiInfo.INVALID_RSSI
                                        && (stats.rssi_mgmt == 0
                                        || stats.beacon_rx == 0)) {
                                    stats = null;
                                }
                            }
                            // Get Info and continue polling
                            fetchRssiLinkSpeedAndFrequencyNative();
                            mWifiScoreReport =
                                    WifiScoreReport.calculateScore(mWifiInfo,
                                                                   getCurrentWifiConfiguration(),
                                                                   mWifiConfigManager,
                                                                   mNetworkAgent,
                                                                   mWifiScoreReport,
                                                                   mAggressiveHandover,
                                                                   mWifiMetrics);
                        }
                        sendMessageDelayed(obtainMessage(CMD_RSSI_POLL,
                                mRssiPollToken, 0), POLL_RSSI_INTERVAL_MSECS);
                        if (DBG) sendRssiChangeBroadcast(mWifiInfo.getRssi());
                    } else {
                        // Polling has completed
                    }
                    break;
                case CMD_ENABLE_RSSI_POLL:
                    cleanWifiScore();
                    if (mWifiConfigManager.mEnableRssiPollWhenAssociated.get()) {
                        mEnableRssiPolling = (message.arg1 == 1);
                    } else {
                        mEnableRssiPolling = false;
                    }
                    mRssiPollToken++;
                    if (mEnableRssiPolling) {
                        // First poll
                        fetchRssiLinkSpeedAndFrequencyNative();
                        sendMessageDelayed(obtainMessage(CMD_RSSI_POLL,
                                mRssiPollToken, 0), POLL_RSSI_INTERVAL_MSECS);
                    }
                    break;
                case WifiManager.RSSI_PKTCNT_FETCH:
                    RssiPacketCountInfo info = new RssiPacketCountInfo();
                    fetchRssiLinkSpeedAndFrequencyNative();
                    info.rssi = mWifiInfo.getRssi();
                    fetchPktcntNative(info);
                    replyToMessage(message, WifiManager.RSSI_PKTCNT_FETCH_SUCCEEDED, info);
                    break;
                case CMD_DELAYED_NETWORK_DISCONNECT:
                    if (!linkDebouncing && mWifiConfigManager.mEnableLinkDebouncing) {

                        // Ignore if we are not debouncing
                        logd("CMD_DELAYED_NETWORK_DISCONNECT and not debouncing - ignore "
                                + message.arg1);
                        return HANDLED;
                    } else {
                        logd("CMD_DELAYED_NETWORK_DISCONNECT and debouncing - disconnect "
                                + message.arg1);

                        linkDebouncing = false;
                        // If we are still debouncing while this message comes,
                        // it means we were not able to reconnect within the alloted time
                        // = LINK_FLAPPING_DEBOUNCE_MSEC
                        // and thus, trigger a real disconnect
                        handleNetworkDisconnect();
                        transitionTo(mDisconnectedState);
                    }
                    break;
                case CMD_ASSOCIATED_BSSID:
                    if ((String) message.obj == null) {
                        logw("Associated command w/o BSSID");
                        break;
                    }
                    mLastBssid = (String) message.obj;
                    if (mLastBssid != null && (mWifiInfo.getBSSID() == null
                            || !mLastBssid.equals(mWifiInfo.getBSSID()))) {
                        mWifiInfo.setBSSID((String) message.obj);
                        sendNetworkStateChangeBroadcast(mLastBssid);
                    }
                    break;
                case CMD_START_RSSI_MONITORING_OFFLOAD:
                case CMD_RSSI_THRESHOLD_BREACH:
                    byte currRssi = (byte) message.arg1;
                    processRssiThreshold(currRssi, message.what);
                    break;
                case CMD_STOP_RSSI_MONITORING_OFFLOAD:
                    stopRssiMonitoringOffload();
                    break;
                case CMD_RESET_SIM_NETWORKS:
                    if (message.arg1 == 0 // sim was removed
                            && mLastNetworkId != WifiConfiguration.INVALID_NETWORK_ID) {
                        WifiConfiguration config =
                                mWifiConfigManager.getWifiConfiguration(mLastNetworkId);
                        if (TelephonyUtil.isSimConfig(config)) {
                            mWifiNative.disconnect();
                            transitionTo(mDisconnectingState);
                        }
                    }
                    /* allow parent state to reset data for other networks */
                    return NOT_HANDLED;
                default:
                    return NOT_HANDLED;
            }

            return HANDLED;
        }
    }

    class ObtainingIpState extends State {
        @Override
        public void enter() {
            if (DBG) {
                String key = "";
                if (getCurrentWifiConfiguration() != null) {
                    key = getCurrentWifiConfiguration().configKey();
                }
                log("enter ObtainingIpState netId=" + Integer.toString(mLastNetworkId)
                        + " " + key + " "
                        + " roam=" + mAutoRoaming
                        + " static=" + mWifiConfigManager.isUsingStaticIp(mLastNetworkId)
                        + " watchdog= " + obtainingIpWatchdogCount);
            }

            // Reset link Debouncing, indicating we have successfully re-connected to the AP
            // We might still be roaming
            linkDebouncing = false;

            // Send event to CM & network change broadcast
            setNetworkDetailedState(DetailedState.OBTAINING_IPADDR);

            // We must clear the config BSSID, as the wifi chipset may decide to roam
            // from this point on and having the BSSID specified in the network block would
            // cause the roam to fail and the device to disconnect.
            clearCurrentConfigBSSID("ObtainingIpAddress");

            // Stop IpManager in case we're switching from DHCP to static
            // configuration or vice versa.
            //
            // TODO: Only ever enter this state the first time we connect to a
            // network, never on switching between static configuration and
            // DHCP. When we transition from static configuration to DHCP in
            // particular, we must tell ConnectivityService that we're
            // disconnected, because DHCP might take a long time during which
            // connectivity APIs such as getActiveNetworkInfo should not return
            // CONNECTED.
            stopIpManager();

            mIpManager.setHttpProxy(mWifiConfigManager.getProxyProperties(mLastNetworkId));
            if (!TextUtils.isEmpty(mTcpBufferSizes)) {
                mIpManager.setTcpBufferSizes(mTcpBufferSizes);
            }

            if (!mWifiConfigManager.isUsingStaticIp(mLastNetworkId)) {
                final IpManager.ProvisioningConfiguration prov =
                        mIpManager.buildProvisioningConfiguration()
                            .withPreDhcpAction()
                            .withApfCapabilities(mWifiNative.getApfCapabilities())
                            .build();
                mIpManager.startProvisioning(prov);
                obtainingIpWatchdogCount++;
                logd("Start Dhcp Watchdog " + obtainingIpWatchdogCount);
                // Get Link layer stats so as we get fresh tx packet counters
                getWifiLinkLayerStats(true);
                sendMessageDelayed(obtainMessage(CMD_OBTAINING_IP_ADDRESS_WATCHDOG_TIMER,
                        obtainingIpWatchdogCount, 0), OBTAINING_IP_ADDRESS_GUARD_TIMER_MSEC);
            } else {
                StaticIpConfiguration config = mWifiConfigManager.getStaticIpConfiguration(
                        mLastNetworkId);
                if (config.ipAddress == null) {
                    logd("Static IP lacks address");
                    sendMessage(CMD_IPV4_PROVISIONING_FAILURE);
                } else {
                    final IpManager.ProvisioningConfiguration prov =
                            mIpManager.buildProvisioningConfiguration()
                                .withStaticConfiguration(config)
                                .withApfCapabilities(mWifiNative.getApfCapabilities())
                                .build();
                    mIpManager.startProvisioning(prov);
                }
            }
        }

        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
                case CMD_AUTO_CONNECT:
                case CMD_AUTO_ROAM:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    break;
                case WifiManager.SAVE_NETWORK:
                case WifiStateMachine.CMD_AUTO_SAVE_NETWORK:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DEFERRED;
                    deferMessage(message);
                    break;
                    /* Defer any power mode changes since we must keep active power mode at DHCP */

                case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_NETWORK_DISCONNECTION,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE);
                    return NOT_HANDLED;
                case CMD_SET_HIGH_PERF_MODE:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DEFERRED;
                    deferMessage(message);
                    break;
                    /* Defer scan request since we should not switch to other channels at DHCP */
                case CMD_START_SCAN:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DEFERRED;
                    deferMessage(message);
                    break;
                case CMD_OBTAINING_IP_ADDRESS_WATCHDOG_TIMER:
                    if (message.arg1 == obtainingIpWatchdogCount) {
                        logd("ObtainingIpAddress: Watchdog Triggered, count="
                                + obtainingIpWatchdogCount);
                        handleIpConfigurationLost();
                        transitionTo(mDisconnectingState);
                        break;
                    }
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    private void sendConnectedState() {
        // If this network was explicitly selected by the user, evaluate whether to call
        // explicitlySelected() so the system can treat it appropriately.
        WifiConfiguration config = getCurrentWifiConfiguration();
        if (mWifiConfigManager.isLastSelectedConfiguration(config)) {
            boolean prompt =
                    mWifiConfigManager.checkConfigOverridePermission(config.lastConnectUid);
            if (DBG) {
                log("Network selected by UID " + config.lastConnectUid + " prompt=" + prompt);
            }
            if (prompt) {
                // Selected by the user via Settings or QuickSettings. If this network has Internet
                // access, switch to it. Otherwise, switch to it only if the user confirms that they
                // really want to switch, or has already confirmed and selected "Don't ask again".
                if (DBG) {
                    log("explictlySelected acceptUnvalidated=" + config.noInternetAccessExpected);
                }
                mNetworkAgent.explicitlySelected(config.noInternetAccessExpected);
            }
        }

        setNetworkDetailedState(DetailedState.CONNECTED);
        mWifiConfigManager.updateStatus(mLastNetworkId, DetailedState.CONNECTED);
        sendNetworkStateChangeBroadcast(mLastBssid);
    }

    class RoamingState extends State {
        boolean mAssociated;
        @Override
        public void enter() {
            if (DBG) {
                log("RoamingState Enter"
                        + " mScreenOn=" + mScreenOn );
            }

            // Make sure we disconnect if roaming fails
            roamWatchdogCount++;
            logd("Start Roam Watchdog " + roamWatchdogCount);
            sendMessageDelayed(obtainMessage(CMD_ROAM_WATCHDOG_TIMER,
                    roamWatchdogCount, 0), ROAM_GUARD_TIMER_MSEC);
            mAssociated = false;
        }
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);
            WifiConfiguration config;
            switch (message.what) {
                case CMD_IP_CONFIGURATION_LOST:
                    config = getCurrentWifiConfiguration();
                    if (config != null) {
                        mWifiLogger.captureBugReportData(WifiLogger.REPORT_REASON_AUTOROAM_FAILURE);
                        mWifiConfigManager.noteRoamingFailure(config,
                                WifiConfiguration.ROAMING_FAILURE_IP_CONFIG);
                    }
                    return NOT_HANDLED;
                case CMD_UNWANTED_NETWORK:
                    if (DBG) log("Roaming and CS doesnt want the network -> ignore");
                    return HANDLED;
                case CMD_SET_OPERATIONAL_MODE:
                    if (message.arg1 != CONNECT_MODE) {
                        deferMessage(message);
                    }
                    break;
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    /**
                     * If we get a SUPPLICANT_STATE_CHANGE_EVENT indicating a DISCONNECT
                     * before NETWORK_DISCONNECTION_EVENT
                     * And there is an associated BSSID corresponding to our target BSSID, then
                     * we have missed the network disconnection, transition to mDisconnectedState
                     * and handle the rest of the events there.
                     */
                    StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
                    if (stateChangeResult.state == SupplicantState.DISCONNECTED
                            || stateChangeResult.state == SupplicantState.INACTIVE
                            || stateChangeResult.state == SupplicantState.INTERFACE_DISABLED) {
                        if (DBG) {
                            log("STATE_CHANGE_EVENT in roaming state "
                                    + stateChangeResult.toString() );
                        }
                        if (stateChangeResult.BSSID != null
                                && stateChangeResult.BSSID.equals(mTargetRoamBSSID)) {
                            handleNetworkDisconnect();
                            transitionTo(mDisconnectedState);
                        }
                    }
                    if (stateChangeResult.state == SupplicantState.ASSOCIATED) {
                        // We completed the layer2 roaming part
                        mAssociated = true;
                        if (stateChangeResult.BSSID != null) {
                            mTargetRoamBSSID = (String) stateChangeResult.BSSID;
                        }
                    }
                    break;
                case CMD_ROAM_WATCHDOG_TIMER:
                    if (roamWatchdogCount == message.arg1) {
                        if (DBG) log("roaming watchdog! -> disconnect");
                        mWifiMetrics.endConnectionEvent(
                                WifiMetrics.ConnectionEvent.FAILURE_ROAM_TIMEOUT,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE);
                        mRoamFailCount++;
                        handleNetworkDisconnect();
                        mWifiNative.disconnect();
                        transitionTo(mDisconnectedState);
                    }
                    break;
                case WifiMonitor.NETWORK_CONNECTION_EVENT:
                    if (mAssociated) {
                        if (DBG) log("roaming and Network connection established");
                        mLastNetworkId = message.arg1;
                        mLastBssid = (String) message.obj;
                        mWifiInfo.setBSSID(mLastBssid);
                        mWifiInfo.setNetworkId(mLastNetworkId);
                        if (mWifiConnectivityManager != null) {
                            mWifiConnectivityManager.trackBssid(mLastBssid, true);
                        }
                        sendNetworkStateChangeBroadcast(mLastBssid);

                        // Successful framework roam! (probably)
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_NONE,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE);

                        // We must clear the config BSSID, as the wifi chipset may decide to roam
                        // from this point on and having the BSSID specified by QNS would cause
                        // the roam to fail and the device to disconnect.
                        // When transition from RoamingState to DisconnectingState or
                        // DisconnectedState, the config BSSID is cleared by
                        // handleNetworkDisconnect().
                        clearCurrentConfigBSSID("RoamingCompleted");

                        // We used to transition to ObtainingIpState in an
                        // attempt to do DHCPv4 RENEWs on framework roams.
                        // DHCP can take too long to time out, and we now rely
                        // upon IpManager's use of IpReachabilityMonitor to
                        // confirm our current network configuration.
                        //
                        // mIpManager.confirmConfiguration() is called within
                        // the handling of SupplicantState.COMPLETED.
                        transitionTo(mConnectedState);
                    } else {
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    }
                    break;
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                    // Throw away but only if it corresponds to the network we're roaming to
                    String bssid = (String) message.obj;
                    if (true) {
                        String target = "";
                        if (mTargetRoamBSSID != null) target = mTargetRoamBSSID;
                        log("NETWORK_DISCONNECTION_EVENT in roaming state"
                                + " BSSID=" + bssid
                                + " target=" + target);
                    }
                    if (bssid != null && bssid.equals(mTargetRoamBSSID)) {
                        handleNetworkDisconnect();
                        transitionTo(mDisconnectedState);
                    }
                    break;
                case WifiMonitor.SSID_TEMP_DISABLED:
                    // Auth error while roaming
                    logd("SSID_TEMP_DISABLED nid=" + Integer.toString(mLastNetworkId)
                            + " id=" + Integer.toString(message.arg1)
                            + " isRoaming=" + isRoaming()
                            + " roam=" + mAutoRoaming);
                    if (message.arg1 == mLastNetworkId) {
                        config = getCurrentWifiConfiguration();
                        if (config != null) {
                            mWifiLogger.captureBugReportData(
                                    WifiLogger.REPORT_REASON_AUTOROAM_FAILURE);
                            mWifiConfigManager.noteRoamingFailure(config,
                                    WifiConfiguration.ROAMING_FAILURE_AUTH_FAILURE);
                        }
                        handleNetworkDisconnect();
                        transitionTo(mDisconnectingState);
                    }
                    return NOT_HANDLED;
                case CMD_START_SCAN:
                    deferMessage(message);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void exit() {
            logd("WifiStateMachine: Leaving Roaming state");
        }
    }

    class ConnectedState extends State {
        @Override
        public void enter() {
            String address;
            updateDefaultRouteMacAddress(1000);
            if (DBG) {
                log("Enter ConnectedState "
                       + " mScreenOn=" + mScreenOn);
            }

            if (mWifiConnectivityManager != null) {
                mWifiConnectivityManager.handleConnectionStateChanged(
                        WifiConnectivityManager.WIFI_STATE_CONNECTED);
            }
            registerConnected();
            lastConnectAttemptTimestamp = 0;
            targetWificonfiguration = null;
            // Paranoia
            linkDebouncing = false;

            // Not roaming anymore
            mAutoRoaming = false;

            if (testNetworkDisconnect) {
                testNetworkDisconnectCounter++;
                logd("ConnectedState Enter start disconnect test " +
                        testNetworkDisconnectCounter);
                sendMessageDelayed(obtainMessage(CMD_TEST_NETWORK_DISCONNECT,
                        testNetworkDisconnectCounter, 0), 15000);
            }

            // Reenable all networks, allow for hidden networks to be scanned
            mWifiConfigManager.enableAllNetworks();

            mLastDriverRoamAttempt = 0;
            mTargetNetworkId = WifiConfiguration.INVALID_NETWORK_ID;
            mWifiLastResortWatchdog.connectedStateTransition(true);
        }
        @Override
        public boolean processMessage(Message message) {
            WifiConfiguration config = null;
            logStateAndMessage(message, this);

            switch (message.what) {
                case CMD_UPDATE_ASSOCIATED_SCAN_PERMISSION:
                    updateAssociatedScanPermission();
                    break;
                case CMD_UNWANTED_NETWORK:
                    if (message.arg1 == NETWORK_STATUS_UNWANTED_DISCONNECT) {
                        mWifiNative.disconnect();
                        transitionTo(mDisconnectingState);
                    } else if (message.arg1 == NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN ||
                            message.arg1 == NETWORK_STATUS_UNWANTED_VALIDATION_FAILED) {
                        Log.d(TAG, (message.arg1 == NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN
                                ? "NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN"
                                : "NETWORK_STATUS_UNWANTED_VALIDATION_FAILED"));
                        config = getCurrentWifiConfiguration();
                        if (config != null) {
                            // Disable autojoin
                            if (message.arg1 == NETWORK_STATUS_UNWANTED_DISABLE_AUTOJOIN) {
                                config.validatedInternetAccess = false;
                                // Clear last-selected status, as being last-selected also avoids
                                // disabling auto-join.
                                if (mWifiConfigManager.isLastSelectedConfiguration(config)) {
                                    mWifiConfigManager.setAndEnableLastSelectedConfiguration(
                                        WifiConfiguration.INVALID_NETWORK_ID);
                                }
                                mWifiConfigManager.updateNetworkSelectionStatus(config,
                                        WifiConfiguration.NetworkSelectionStatus
                                        .DISABLED_NO_INTERNET);
                            }
                            config.numNoInternetAccessReports += 1;
                            mWifiConfigManager.writeKnownNetworkHistory();
                        }
                    }
                    return HANDLED;
                case CMD_NETWORK_STATUS:
                    if (message.arg1 == NetworkAgent.VALID_NETWORK) {
                        config = getCurrentWifiConfiguration();
                        if (config != null) {
                            // re-enable autojoin
                            config.numNoInternetAccessReports = 0;
                            config.validatedInternetAccess = true;
                            mWifiConfigManager.writeKnownNetworkHistory();
                        }
                    }
                    return HANDLED;
                case CMD_ACCEPT_UNVALIDATED:
                    boolean accept = (message.arg1 != 0);
                    config = getCurrentWifiConfiguration();
                    if (config != null) {
                        config.noInternetAccessExpected = accept;
                        mWifiConfigManager.writeKnownNetworkHistory();
                    }
                    return HANDLED;
                case CMD_TEST_NETWORK_DISCONNECT:
                    // Force a disconnect
                    if (message.arg1 == testNetworkDisconnectCounter) {
                        mWifiNative.disconnect();
                    }
                    break;
                case CMD_ASSOCIATED_BSSID:
                    // ASSOCIATING to a new BSSID while already connected, indicates
                    // that driver is roaming
                    mLastDriverRoamAttempt = System.currentTimeMillis();
                    return NOT_HANDLED;
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                    long lastRoam = 0;
                    reportConnectionAttemptEnd(
                            WifiMetrics.ConnectionEvent.FAILURE_NETWORK_DISCONNECTION,
                            WifiMetricsProto.ConnectionEvent.HLF_NONE);
                    if (mLastDriverRoamAttempt != 0) {
                        // Calculate time since last driver roam attempt
                        lastRoam = System.currentTimeMillis() - mLastDriverRoamAttempt;
                        mLastDriverRoamAttempt = 0;
                    }
                    if (unexpectedDisconnectedReason(message.arg2)) {
                        mWifiLogger.captureBugReportData(
                                WifiLogger.REPORT_REASON_UNEXPECTED_DISCONNECT);
                    }
                    config = getCurrentWifiConfiguration();
                    if (mScreenOn
                            && !linkDebouncing
                            && config != null
                            && config.getNetworkSelectionStatus().isNetworkEnabled()
                            && !mWifiConfigManager.isLastSelectedConfiguration(config)
                            && (message.arg2 != 3 /* reason cannot be 3, i.e. locally generated */
                                || (lastRoam > 0 && lastRoam < 2000) /* unless driver is roaming */)
                            && ((ScanResult.is24GHz(mWifiInfo.getFrequency())
                                    && mWifiInfo.getRssi() >
                                    WifiQualifiedNetworkSelector.QUALIFIED_RSSI_24G_BAND)
                                    || (ScanResult.is5GHz(mWifiInfo.getFrequency())
                                    && mWifiInfo.getRssi() >
                                    mWifiConfigManager.mThresholdQualifiedRssi5.get()))) {
                        // Start de-bouncing the L2 disconnection:
                        // this L2 disconnection might be spurious.
                        // Hence we allow 4 seconds for the state machine to try
                        // to reconnect, go thru the
                        // roaming cycle and enter Obtaining IP address
                        // before signalling the disconnect to ConnectivityService and L3
                        startScanForConfiguration(getCurrentWifiConfiguration());
                        linkDebouncing = true;

                        sendMessageDelayed(obtainMessage(CMD_DELAYED_NETWORK_DISCONNECT,
                                0, mLastNetworkId), LINK_FLAPPING_DEBOUNCE_MSEC);
                        if (DBG) {
                            log("NETWORK_DISCONNECTION_EVENT in connected state"
                                    + " BSSID=" + mWifiInfo.getBSSID()
                                    + " RSSI=" + mWifiInfo.getRssi()
                                    + " freq=" + mWifiInfo.getFrequency()
                                    + " reason=" + message.arg2
                                    + " -> debounce");
                        }
                        return HANDLED;
                    } else {
                        if (DBG) {
                            log("NETWORK_DISCONNECTION_EVENT in connected state"
                                    + " BSSID=" + mWifiInfo.getBSSID()
                                    + " RSSI=" + mWifiInfo.getRssi()
                                    + " freq=" + mWifiInfo.getFrequency()
                                    + " was debouncing=" + linkDebouncing
                                    + " reason=" + message.arg2
                                    + " Network Selection Status=" + (config == null ? "Unavailable"
                                    : config.getNetworkSelectionStatus().getNetworkStatusString()));
                        }
                    }
                    break;
                case CMD_AUTO_ROAM:
                    // Clear the driver roam indication since we are attempting a framework roam
                    mLastDriverRoamAttempt = 0;

                    /*<TODO> 2016-02-24
                        Fix CMD_AUTO_ROAM to use the candidate (message.arg1) networkID, rather than
                        the old networkID.
                        The current code only handles roaming between BSSIDs on the same networkID,
                        and will break for roams between different (but linked) networkIDs. This
                        case occurs for DBDC roaming, and the CMD_AUTO_ROAM sent due to it will
                        fail.
                    */
                    /* Connect command coming from auto-join */
                    ScanResult candidate = (ScanResult)message.obj;
                    String bssid = "any";
                    if (candidate != null) {
                        bssid = candidate.BSSID;
                    }
                    int netId = message.arg1;
                    if (netId == WifiConfiguration.INVALID_NETWORK_ID) {
                        loge("AUTO_ROAM and no config, bail out...");
                        break;
                    } else {
                        config = mWifiConfigManager.getWifiConfiguration(netId);
                    }

                    setTargetBssid(config, bssid);
                    mTargetNetworkId = netId;

                    logd("CMD_AUTO_ROAM sup state "
                            + mSupplicantStateTracker.getSupplicantStateName()
                            + " my state " + getCurrentState().getName()
                            + " nid=" + Integer.toString(netId)
                            + " config " + config.configKey()
                            + " targetRoamBSSID " + mTargetRoamBSSID);

                    /* Determine if this is a regular roam (between BSSIDs sharing the same SSID),
                       or a DBDC roam (between 2.4 & 5GHz networks on different SSID's, but with
                       matching 16 byte BSSID prefixes):
                     */
                    WifiConfiguration currentConfig = getCurrentWifiConfiguration();
                    if (currentConfig != null && currentConfig.isLinked(config)) {
                        // This is dual band roaming
                        mWifiMetrics.startConnectionEvent(config, mTargetRoamBSSID,
                                WifiMetricsProto.ConnectionEvent.ROAM_DBDC);
                    } else {
                        // This is regular roaming
                        mWifiMetrics.startConnectionEvent(config, mTargetRoamBSSID,
                                WifiMetricsProto.ConnectionEvent.ROAM_ENTERPRISE);
                    }

                    if (deferForUserInput(message, netId, false)) {
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_CONNECT_NETWORK_FAILED,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE);
                        break;
                    } else if (mWifiConfigManager.getWifiConfiguration(netId).userApproved ==
                            WifiConfiguration.USER_BANNED) {
                        replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                                WifiManager.NOT_AUTHORIZED);
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_CONNECT_NETWORK_FAILED,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE);
                        break;
                    }

                    boolean ret = false;
                    if (mLastNetworkId != netId) {
                        if (mWifiConfigManager.selectNetwork(config, /* updatePriorities = */ false,
                                WifiConfiguration.UNKNOWN_UID) && mWifiNative.reconnect()) {
                            ret = true;
                        }
                    } else {
                        ret = mWifiNative.reassociate();
                    }
                    if (ret) {
                        lastConnectAttemptTimestamp = System.currentTimeMillis();
                        targetWificonfiguration = mWifiConfigManager.getWifiConfiguration(netId);

                        // replyToMessage(message, WifiManager.CONNECT_NETWORK_SUCCEEDED);
                        mAutoRoaming = true;
                        transitionTo(mRoamingState);

                    } else {
                        loge("Failed to connect config: " + config + " netId: " + netId);
                        replyToMessage(message, WifiManager.CONNECT_NETWORK_FAILED,
                                WifiManager.ERROR);
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_FAIL;
                        reportConnectionAttemptEnd(
                                WifiMetrics.ConnectionEvent.FAILURE_CONNECT_NETWORK_FAILED,
                                WifiMetricsProto.ConnectionEvent.HLF_NONE);
                        break;
                    }
                    break;
                case CMD_START_IP_PACKET_OFFLOAD: {
                        int slot = message.arg1;
                        int intervalSeconds = message.arg2;
                        KeepalivePacketData pkt = (KeepalivePacketData) message.obj;
                        byte[] dstMac;
                        try {
                            InetAddress gateway = RouteInfo.selectBestRoute(
                                    mLinkProperties.getRoutes(), pkt.dstAddress).getGateway();
                            String dstMacStr = macAddressFromRoute(gateway.getHostAddress());
                            dstMac = macAddressFromString(dstMacStr);
                        } catch (NullPointerException|IllegalArgumentException e) {
                            loge("Can't find MAC address for next hop to " + pkt.dstAddress);
                            mNetworkAgent.onPacketKeepaliveEvent(slot,
                                    ConnectivityManager.PacketKeepalive.ERROR_INVALID_IP_ADDRESS);
                            break;
                        }
                        pkt.dstMac = dstMac;
                        int result = startWifiIPPacketOffload(slot, pkt, intervalSeconds);
                        mNetworkAgent.onPacketKeepaliveEvent(slot, result);
                        break;
                    }
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void exit() {
            logd("WifiStateMachine: Leaving Connected state");
            if (mWifiConnectivityManager != null) {
                mWifiConnectivityManager.handleConnectionStateChanged(
                         WifiConnectivityManager.WIFI_STATE_TRANSITIONING);
            }

            mLastDriverRoamAttempt = 0;
            mWhiteListedSsids = null;
            mWifiLastResortWatchdog.connectedStateTransition(false);
        }
    }

    class DisconnectingState extends State {

        @Override
        public void enter() {

            if (DBG) {
                logd(" Enter DisconnectingState State screenOn=" + mScreenOn);
            }

            // Make sure we disconnect: we enter this state prior to connecting to a new
            // network, waiting for either a DISCONNECT event or a SUPPLICANT_STATE_CHANGE
            // event which in this case will be indicating that supplicant started to associate.
            // In some cases supplicant doesn't ignore the connect requests (it might not
            // find the target SSID in its cache),
            // Therefore we end up stuck that state, hence the need for the watchdog.
            disconnectingWatchdogCount++;
            logd("Start Disconnecting Watchdog " + disconnectingWatchdogCount);
            sendMessageDelayed(obtainMessage(CMD_DISCONNECTING_WATCHDOG_TIMER,
                    disconnectingWatchdogCount, 0), DISCONNECTING_GUARD_TIMER_MSEC);
        }

        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);
            switch (message.what) {
                case CMD_SET_OPERATIONAL_MODE:
                    if (message.arg1 != CONNECT_MODE) {
                        deferMessage(message);
                    }
                    break;
                case CMD_START_SCAN:
                    deferMessage(message);
                    return HANDLED;
                case CMD_DISCONNECT:
                    if (DBG) log("Ignore CMD_DISCONNECT when already disconnecting.");
                    break;
                case CMD_DISCONNECTING_WATCHDOG_TIMER:
                    if (disconnectingWatchdogCount == message.arg1) {
                        if (DBG) log("disconnecting watchdog! -> disconnect");
                        handleNetworkDisconnect();
                        transitionTo(mDisconnectedState);
                    }
                    break;
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    /**
                     * If we get a SUPPLICANT_STATE_CHANGE_EVENT before NETWORK_DISCONNECTION_EVENT
                     * we have missed the network disconnection, transition to mDisconnectedState
                     * and handle the rest of the events there
                     */
                    deferMessage(message);
                    handleNetworkDisconnect();
                    transitionTo(mDisconnectedState);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    class DisconnectedState extends State {
        @Override
        public void enter() {
            // We dont scan frequently if this is a temporary disconnect
            // due to p2p
            if (mTemporarilyDisconnectWifi) {
                mWifiP2pChannel.sendMessage(WifiP2pServiceImpl.DISCONNECT_WIFI_RESPONSE);
                return;
            }

            if (DBG) {
                logd(" Enter DisconnectedState screenOn=" + mScreenOn);
            }

            /** clear the roaming state, if we were roaming, we failed */
            mAutoRoaming = false;

            if (mWifiConnectivityManager != null) {
                mWifiConnectivityManager.handleConnectionStateChanged(
                        WifiConnectivityManager.WIFI_STATE_DISCONNECTED);
            }

            /**
             * If we have no networks saved, the supplicant stops doing the periodic scan.
             * The scans are useful to notify the user of the presence of an open network.
             * Note that these are not wake up scans.
             */
            if (mNoNetworksPeriodicScan != 0 && !mP2pConnected.get()
                    && mWifiConfigManager.getSavedNetworks().size() == 0) {
                sendMessageDelayed(obtainMessage(CMD_NO_NETWORKS_PERIODIC_SCAN,
                        ++mPeriodicScanToken, 0), mNoNetworksPeriodicScan);
            }

            mDisconnectedTimeStamp = System.currentTimeMillis();
        }
        @Override
        public boolean processMessage(Message message) {
            boolean ret = HANDLED;

            logStateAndMessage(message, this);

            switch (message.what) {
                case CMD_NO_NETWORKS_PERIODIC_SCAN:
                    if (mP2pConnected.get()) break;
                    if (mNoNetworksPeriodicScan != 0 && message.arg1 == mPeriodicScanToken &&
                            mWifiConfigManager.getSavedNetworks().size() == 0) {
                        startScan(UNKNOWN_SCAN_SOURCE, -1, null, WIFI_WORK_SOURCE);
                        sendMessageDelayed(obtainMessage(CMD_NO_NETWORKS_PERIODIC_SCAN,
                                    ++mPeriodicScanToken, 0), mNoNetworksPeriodicScan);
                    }
                    break;
                case WifiManager.FORGET_NETWORK:
                case CMD_REMOVE_NETWORK:
                case CMD_REMOVE_APP_CONFIGURATIONS:
                case CMD_REMOVE_USER_CONFIGURATIONS:
                    // Set up a delayed message here. After the forget/remove is handled
                    // the handled delayed message will determine if there is a need to
                    // scan and continue
                    sendMessageDelayed(obtainMessage(CMD_NO_NETWORKS_PERIODIC_SCAN,
                                ++mPeriodicScanToken, 0), mNoNetworksPeriodicScan);
                    ret = NOT_HANDLED;
                    break;
                case CMD_SET_OPERATIONAL_MODE:
                    if (message.arg1 != CONNECT_MODE) {
                        mOperationalMode = message.arg1;
                        mWifiConfigManager.disableAllNetworksNative();
                        if (mOperationalMode == SCAN_ONLY_WITH_WIFI_OFF_MODE) {
                            mWifiP2pChannel.sendMessage(CMD_DISABLE_P2P_REQ);
                            setWifiState(WIFI_STATE_DISABLED);
                        }
                        transitionTo(mScanModeState);
                    }
                    mWifiConfigManager.
                            setAndEnableLastSelectedConfiguration(
                                    WifiConfiguration.INVALID_NETWORK_ID);
                    break;
                case CMD_DISCONNECT:
                    if (SupplicantState.isConnecting(mWifiInfo.getSupplicantState())) {
                        if (DBG) {
                            log("CMD_DISCONNECT when supplicant is connecting - do not ignore");
                        }
                        mWifiConfigManager.setAndEnableLastSelectedConfiguration(
                                WifiConfiguration.INVALID_NETWORK_ID);
                        mWifiNative.disconnect();
                        break;
                    }
                    if (DBG) log("Ignore CMD_DISCONNECT when already disconnected.");
                    break;
                /* Ignore network disconnect */
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                    // Interpret this as an L2 connection failure
                    break;
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
                    if (DBG) {
                        logd("SUPPLICANT_STATE_CHANGE_EVENT state=" + stateChangeResult.state +
                                " -> state= " + WifiInfo.getDetailedStateOf(stateChangeResult.state)
                                + " debouncing=" + linkDebouncing);
                    }
                    setNetworkDetailedState(WifiInfo.getDetailedStateOf(stateChangeResult.state));
                    /* ConnectModeState does the rest of the handling */
                    ret = NOT_HANDLED;
                    break;
                case CMD_START_SCAN:
                    if (!checkOrDeferScanAllowed(message)) {
                        // The scan request was rescheduled
                        messageHandlingStatus = MESSAGE_HANDLING_STATUS_REFUSED;
                        return HANDLED;
                    }

                    ret = NOT_HANDLED;
                    break;
                case WifiP2pServiceImpl.P2P_CONNECTION_CHANGED:
                    NetworkInfo info = (NetworkInfo) message.obj;
                    mP2pConnected.set(info.isConnected());
                    if (mP2pConnected.get()) {
                        int defaultInterval = mContext.getResources().getInteger(
                                R.integer.config_wifi_scan_interval_p2p_connected);
                        long scanIntervalMs = mFacade.getLongSetting(mContext,
                                Settings.Global.WIFI_SCAN_INTERVAL_WHEN_P2P_CONNECTED_MS,
                                defaultInterval);
                        mWifiNative.setScanInterval((int) scanIntervalMs/1000);
                    } else if (mWifiConfigManager.getSavedNetworks().size() == 0) {
                        if (DBG) log("Turn on scanning after p2p disconnected");
                        sendMessageDelayed(obtainMessage(CMD_NO_NETWORKS_PERIODIC_SCAN,
                                    ++mPeriodicScanToken, 0), mNoNetworksPeriodicScan);
                    }
                    break;
                case CMD_RECONNECT:
                case CMD_REASSOCIATE:
                    if (mTemporarilyDisconnectWifi) {
                        // Drop a third party reconnect/reassociate if STA is
                        // temporarily disconnected for p2p
                        break;
                    } else {
                        // ConnectModeState handles it
                        ret = NOT_HANDLED;
                    }
                    break;
                case CMD_SCREEN_STATE_CHANGED:
                    handleScreenStateChanged(message.arg1 != 0);
                    break;
                default:
                    ret = NOT_HANDLED;
            }
            return ret;
        }

        @Override
        public void exit() {
            if (mWifiConnectivityManager != null) {
                mWifiConnectivityManager.handleConnectionStateChanged(
                         WifiConnectivityManager.WIFI_STATE_TRANSITIONING);
            }
        }
    }

    class WpsRunningState extends State {
        // Tracks the source to provide a reply
        private Message mSourceMessage;
        @Override
        public void enter() {
            mSourceMessage = Message.obtain(getCurrentMessage());
        }
        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch (message.what) {
                case WifiMonitor.WPS_SUCCESS_EVENT:
                    // Ignore intermediate success, wait for full connection
                    break;
                case WifiMonitor.NETWORK_CONNECTION_EVENT:
                    replyToMessage(mSourceMessage, WifiManager.WPS_COMPLETED);
                    mSourceMessage.recycle();
                    mSourceMessage = null;
                    deferMessage(message);
                    transitionTo(mDisconnectedState);
                    break;
                case WifiMonitor.WPS_OVERLAP_EVENT:
                    replyToMessage(mSourceMessage, WifiManager.WPS_FAILED,
                            WifiManager.WPS_OVERLAP_ERROR);
                    mSourceMessage.recycle();
                    mSourceMessage = null;
                    transitionTo(mDisconnectedState);
                    break;
                case WifiMonitor.WPS_FAIL_EVENT:
                    // Arg1 has the reason for the failure
                    if ((message.arg1 != WifiManager.ERROR) || (message.arg2 != 0)) {
                        replyToMessage(mSourceMessage, WifiManager.WPS_FAILED, message.arg1);
                        mSourceMessage.recycle();
                        mSourceMessage = null;
                        transitionTo(mDisconnectedState);
                    } else {
                        if (DBG) log("Ignore unspecified fail event during WPS connection");
                    }
                    break;
                case WifiMonitor.WPS_TIMEOUT_EVENT:
                    replyToMessage(mSourceMessage, WifiManager.WPS_FAILED,
                            WifiManager.WPS_TIMED_OUT);
                    mSourceMessage.recycle();
                    mSourceMessage = null;
                    transitionTo(mDisconnectedState);
                    break;
                case WifiManager.START_WPS:
                    replyToMessage(message, WifiManager.WPS_FAILED, WifiManager.IN_PROGRESS);
                    break;
                case WifiManager.CANCEL_WPS:
                    if (mWifiNative.cancelWps()) {
                        replyToMessage(message, WifiManager.CANCEL_WPS_SUCCEDED);
                    } else {
                        replyToMessage(message, WifiManager.CANCEL_WPS_FAILED, WifiManager.ERROR);
                    }
                    transitionTo(mDisconnectedState);
                    break;
                /**
                 * Defer all commands that can cause connections to a different network
                 * or put the state machine out of connect mode
                 */
                case CMD_STOP_DRIVER:
                case CMD_SET_OPERATIONAL_MODE:
                case WifiManager.CONNECT_NETWORK:
                case CMD_ENABLE_NETWORK:
                case CMD_RECONNECT:
                case CMD_REASSOCIATE:
                case CMD_ENABLE_ALL_NETWORKS:
                    deferMessage(message);
                    break;
                case CMD_AUTO_CONNECT:
                case CMD_AUTO_ROAM:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    return HANDLED;
                case CMD_START_SCAN:
                    messageHandlingStatus = MESSAGE_HANDLING_STATUS_DISCARD;
                    return HANDLED;
                case WifiMonitor.NETWORK_DISCONNECTION_EVENT:
                    if (DBG) log("Network connection lost");
                    handleNetworkDisconnect();
                    break;
                case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                    if (DBG) log("Ignore Assoc reject event during WPS Connection");
                    break;
                case WifiMonitor.AUTHENTICATION_FAILURE_EVENT:
                    // Disregard auth failure events during WPS connection. The
                    // EAP sequence is retried several times, and there might be
                    // failures (especially for wps pin). We will get a WPS_XXX
                    // event at the end of the sequence anyway.
                    if (DBG) log("Ignore auth failure during WPS connection");
                    break;
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    // Throw away supplicant state changes when WPS is running.
                    // We will start getting supplicant state changes once we get
                    // a WPS success or failure
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }

        @Override
        public void exit() {
            mWifiConfigManager.enableAllNetworks();
            mWifiConfigManager.loadConfiguredNetworks();
        }
    }

    class SoftApState extends State {
        private SoftApManager mSoftApManager;

        private class SoftApListener implements SoftApManager.Listener {
            @Override
            public void onStateChanged(int state, int reason) {
                if (state == WIFI_AP_STATE_DISABLED) {
                    sendMessage(CMD_AP_STOPPED);
                } else if (state == WIFI_AP_STATE_FAILED) {
                    sendMessage(CMD_START_AP_FAILURE);
                }

                setWifiApState(state, reason);
            }
        }

        @Override
        public void enter() {
            final Message message = getCurrentMessage();
            if (message.what == CMD_START_AP) {
                WifiConfiguration config = (WifiConfiguration) message.obj;

                if (config == null) {
                    /**
                     * Configuration not provided in the command, fallback to use the current
                     * configuration.
                     */
                    config = mWifiApConfigStore.getApConfiguration();
                } else {
                    /* Update AP configuration. */
                    mWifiApConfigStore.setApConfiguration(config);
                }

                checkAndSetConnectivityInstance();
                mSoftApManager = mFacade.makeSoftApManager(
                        mContext, getHandler().getLooper(), mWifiNative, mNwService,
                        mCm, mCountryCode.getCountryCode(),
                        mWifiApConfigStore.getAllowed2GChannel(),
                        new SoftApListener());
                mSoftApManager.start(config);
            } else {
                throw new RuntimeException("Illegal transition to SoftApState: " + message);
            }
        }

        @Override
        public void exit() {
            mSoftApManager = null;
        }

        @Override
        public boolean processMessage(Message message) {
            logStateAndMessage(message, this);

            switch(message.what) {
                case CMD_START_AP:
                    /* Ignore start command when it is starting/started. */
                    break;
                case CMD_STOP_AP:
                    mSoftApManager.stop();
                    break;
                case CMD_START_AP_FAILURE:
                    transitionTo(mInitialState);
                    break;
                case CMD_AP_STOPPED:
                    transitionTo(mInitialState);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    /**
     * State machine initiated requests can have replyTo set to null indicating
     * there are no recepients, we ignore those reply actions.
     */
    private void replyToMessage(Message msg, int what) {
        if (msg.replyTo == null) return;
        Message dstMsg = obtainMessageWithWhatAndArg2(msg, what);
        mReplyChannel.replyToMessage(msg, dstMsg);
    }

    private void replyToMessage(Message msg, int what, int arg1) {
        if (msg.replyTo == null) return;
        Message dstMsg = obtainMessageWithWhatAndArg2(msg, what);
        dstMsg.arg1 = arg1;
        mReplyChannel.replyToMessage(msg, dstMsg);
    }

    private void replyToMessage(Message msg, int what, Object obj) {
        if (msg.replyTo == null) return;
        Message dstMsg = obtainMessageWithWhatAndArg2(msg, what);
        dstMsg.obj = obj;
        mReplyChannel.replyToMessage(msg, dstMsg);
    }

    /**
     * arg2 on the source message has a unique id that needs to be retained in replies
     * to match the request
     * <p>see WifiManager for details
     */
    private Message obtainMessageWithWhatAndArg2(Message srcMsg, int what) {
        Message msg = Message.obtain();
        msg.what = what;
        msg.arg2 = srcMsg.arg2;
        return msg;
    }

    /**
     * @param wifiCredentialEventType WIFI_CREDENTIAL_SAVED or WIFI_CREDENTIAL_FORGOT
     * @param msg Must have a WifiConfiguration obj to succeed
     */
    private void broadcastWifiCredentialChanged(int wifiCredentialEventType,
            WifiConfiguration config) {
        if (config != null && config.preSharedKey != null) {
            Intent intent = new Intent(WifiManager.WIFI_CREDENTIAL_CHANGED_ACTION);
            intent.putExtra(WifiManager.EXTRA_WIFI_CREDENTIAL_SSID, config.SSID);
            intent.putExtra(WifiManager.EXTRA_WIFI_CREDENTIAL_EVENT_TYPE,
                    wifiCredentialEventType);
            mContext.sendBroadcastAsUser(intent, UserHandle.CURRENT,
                    android.Manifest.permission.RECEIVE_WIFI_CREDENTIAL_CHANGE);
        }
    }

    private static int parseHex(char ch) {
        if ('0' <= ch && ch <= '9') {
            return ch - '0';
        } else if ('a' <= ch && ch <= 'f') {
            return ch - 'a' + 10;
        } else if ('A' <= ch && ch <= 'F') {
            return ch - 'A' + 10;
        } else {
            throw new NumberFormatException("" + ch + " is not a valid hex digit");
        }
    }

    private byte[] parseHex(String hex) {
        /* This only works for good input; don't throw bad data at it */
        if (hex == null) {
            return new byte[0];
        }

        if (hex.length() % 2 != 0) {
            throw new NumberFormatException(hex + " is not a valid hex string");
        }

        byte[] result = new byte[(hex.length())/2 + 1];
        result[0] = (byte) ((hex.length())/2);
        for (int i = 0, j = 1; i < hex.length(); i += 2, j++) {
            int val = parseHex(hex.charAt(i)) * 16 + parseHex(hex.charAt(i+1));
            byte b = (byte) (val & 0xFF);
            result[j] = b;
        }

        return result;
    }

    private static String makeHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }

    private static String makeHex(byte[] bytes, int from, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            sb.append(String.format("%02x", bytes[from+i]));
        }
        return sb.toString();
    }

    private static byte[] concat(byte[] array1, byte[] array2, byte[] array3) {

        int len = array1.length + array2.length + array3.length;

        if (array1.length != 0) {
            len++;                      /* add another byte for size */
        }

        if (array2.length != 0) {
            len++;                      /* add another byte for size */
        }

        if (array3.length != 0) {
            len++;                      /* add another byte for size */
        }

        byte[] result = new byte[len];

        int index = 0;
        if (array1.length != 0) {
            result[index] = (byte) (array1.length & 0xFF);
            index++;
            for (byte b : array1) {
                result[index] = b;
                index++;
            }
        }

        if (array2.length != 0) {
            result[index] = (byte) (array2.length & 0xFF);
            index++;
            for (byte b : array2) {
                result[index] = b;
                index++;
            }
        }

        if (array3.length != 0) {
            result[index] = (byte) (array3.length & 0xFF);
            index++;
            for (byte b : array3) {
                result[index] = b;
                index++;
            }
        }
        return result;
    }

    private static byte[] concatHex(byte[] array1, byte[] array2) {

        int len = array1.length + array2.length;

        byte[] result = new byte[len];

        int index = 0;
        if (array1.length != 0) {
            for (byte b : array1) {
                result[index] = b;
                index++;
            }
        }

        if (array2.length != 0) {
            for (byte b : array2) {
                result[index] = b;
                index++;
            }
        }

        return result;
    }

    // TODO move to TelephonyUtil, same with utilities above
    String getGsmSimAuthResponse(String[] requestData, TelephonyManager tm) {
        StringBuilder sb = new StringBuilder();
        for (String challenge : requestData) {
            if (challenge == null || challenge.isEmpty()) {
                continue;
            }
            logd("RAND = " + challenge);

            byte[] rand = null;
            try {
                rand = parseHex(challenge);
            } catch (NumberFormatException e) {
                loge("malformed challenge");
                continue;
            }

            String base64Challenge = android.util.Base64.encodeToString(
                    rand, android.util.Base64.NO_WRAP);

            // Try USIM first for authentication.
            String tmResponse = tm.getIccAuthentication(tm.APPTYPE_USIM,
                    tm.AUTHTYPE_EAP_SIM, base64Challenge);
            if (tmResponse == null) {
                /* Then, in case of failure, issue may be due to sim type, retry as a simple sim
                 */
                tmResponse = tm.getIccAuthentication(tm.APPTYPE_SIM,
                        tm.AUTHTYPE_EAP_SIM, base64Challenge);
            }
            logv("Raw Response - " + tmResponse);

            if (tmResponse == null || tmResponse.length() <= 4) {
                loge("bad response - " + tmResponse);
                return null;
            }

            byte[] result = android.util.Base64.decode(tmResponse, android.util.Base64.DEFAULT);
            logv("Hex Response -" + makeHex(result));
            int sres_len = result[0];
            if (sres_len >= result.length) {
                loge("malfomed response - " + tmResponse);
                return null;
            }
            String sres = makeHex(result, 1, sres_len);
            int kc_offset = 1 + sres_len;
            if (kc_offset >= result.length) {
                loge("malfomed response - " + tmResponse);
                return null;
            }
            int kc_len = result[kc_offset];
            if (kc_offset + kc_len > result.length) {
                loge("malfomed response - " + tmResponse);
                return null;
            }
            String kc = makeHex(result, 1 + kc_offset, kc_len);
            sb.append(":" + kc + ":" + sres);
            logv("kc:" + kc + " sres:" + sres);
        }

        return sb.toString();
    }

    // TODO move to TelephonyUtil
    void handleGsmAuthRequest(SimAuthRequestData requestData) {
        if (targetWificonfiguration == null
                || targetWificonfiguration.networkId == requestData.networkId) {
            logd("id matches targetWifiConfiguration");
        } else {
            logd("id does not match targetWifiConfiguration");
            return;
        }

        TelephonyManager tm = (TelephonyManager)
                mContext.getSystemService(Context.TELEPHONY_SERVICE);

        if (tm == null) {
            loge("could not get telephony manager");
            mWifiNative.simAuthFailedResponse(requestData.networkId);
            return;
        }

        String response = getGsmSimAuthResponse(requestData.data, tm);
        if (response == null) {
            mWifiNative.simAuthFailedResponse(requestData.networkId);
        } else {
            logv("Supplicant Response -" + response);
            mWifiNative.simAuthResponse(requestData.networkId, "GSM-AUTH", response);
        }
    }

    // TODO move to TelephonyUtil
    void handle3GAuthRequest(SimAuthRequestData requestData) {
        StringBuilder sb = new StringBuilder();
        byte[] rand = null;
        byte[] authn = null;
        String res_type = "UMTS-AUTH";

        if (targetWificonfiguration == null
                || targetWificonfiguration.networkId == requestData.networkId) {
            logd("id matches targetWifiConfiguration");
        } else {
            logd("id does not match targetWifiConfiguration");
            return;
        }
        if (requestData.data.length == 2) {
            try {
                rand = parseHex(requestData.data[0]);
                authn = parseHex(requestData.data[1]);
            } catch (NumberFormatException e) {
                loge("malformed challenge");
            }
        } else {
               loge("malformed challenge");
        }

        String tmResponse = "";
        if (rand != null && authn != null) {
            String base64Challenge = android.util.Base64.encodeToString(
                    concatHex(rand,authn), android.util.Base64.NO_WRAP);

            TelephonyManager tm = (TelephonyManager)
                    mContext.getSystemService(Context.TELEPHONY_SERVICE);
            if (tm != null) {
                tmResponse = tm.getIccAuthentication(tm.APPTYPE_USIM,
                        tm.AUTHTYPE_EAP_AKA, base64Challenge);
                logv("Raw Response - " + tmResponse);
            } else {
                loge("could not get telephony manager");
            }
        }

        boolean good_response = false;
        if (tmResponse != null && tmResponse.length() > 4) {
            byte[] result = android.util.Base64.decode(tmResponse,
                    android.util.Base64.DEFAULT);
            loge("Hex Response - " + makeHex(result));
            byte tag = result[0];
            if (tag == (byte) 0xdb) {
                logv("successful 3G authentication ");
                int res_len = result[1];
                String res = makeHex(result, 2, res_len);
                int ck_len = result[res_len + 2];
                String ck = makeHex(result, res_len + 3, ck_len);
                int ik_len = result[res_len + ck_len + 3];
                String ik = makeHex(result, res_len + ck_len + 4, ik_len);
                sb.append(":" + ik + ":" + ck + ":" + res);
                logv("ik:" + ik + "ck:" + ck + " res:" + res);
                good_response = true;
            } else if (tag == (byte) 0xdc) {
                loge("synchronisation failure");
                int auts_len = result[1];
                String auts = makeHex(result, 2, auts_len);
                res_type = "UMTS-AUTS";
                sb.append(":" + auts);
                logv("auts:" + auts);
                good_response = true;
            } else {
                loge("bad response - unknown tag = " + tag);
            }
        } else {
            loge("bad response - " + tmResponse);
        }

        if (good_response) {
            String response = sb.toString();
            logv("Supplicant Response -" + response);
            mWifiNative.simAuthResponse(requestData.networkId, res_type, response);
        } else {
            mWifiNative.umtsAuthFailedResponse(requestData.networkId);
        }
    }

    /**
     * Automatically connect to the network specified
     *
     * @param networkId ID of the network to connect to
     * @param bssid BSSID of the network
     */
    public void autoConnectToNetwork(int networkId, String bssid) {
        synchronized (mWifiReqCountLock) {
            if (hasConnectionRequests()) {
                sendMessage(CMD_AUTO_CONNECT, networkId, 0, bssid);
            }
        }
    }

    /**
     * Automatically roam to the network specified
     *
     * @param networkId ID of the network to roam to
     * @param scanResult scan result which identifies the network to roam to
     */
    public void autoRoamToNetwork(int networkId, ScanResult scanResult) {
        sendMessage(CMD_AUTO_ROAM, networkId, 0, scanResult);
    }

    /**
     * Dynamically turn on/off WifiConnectivityManager
     *
     * @param enabled true-enable; false-disable
     */
    public void enableWifiConnectivityManager(boolean enabled) {
        sendMessage(CMD_ENABLE_WIFI_CONNECTIVITY_MANAGER, enabled ? 1 : 0);
    }

    /**
     * @param reason reason code from supplicant on network disconnected event
     * @return true if this is a suspicious disconnect
     */
    static boolean unexpectedDisconnectedReason(int reason) {
        return reason == 2              // PREV_AUTH_NOT_VALID
                || reason == 6          // CLASS2_FRAME_FROM_NONAUTH_STA
                || reason == 7          // FRAME_FROM_NONASSOC_STA
                || reason == 8          // STA_HAS_LEFT
                || reason == 9          // STA_REQ_ASSOC_WITHOUT_AUTH
                || reason == 14         // MICHAEL_MIC_FAILURE
                || reason == 15         // 4WAY_HANDSHAKE_TIMEOUT
                || reason == 16         // GROUP_KEY_UPDATE_TIMEOUT
                || reason == 18         // GROUP_CIPHER_NOT_VALID
                || reason == 19         // PAIRWISE_CIPHER_NOT_VALID
                || reason == 23         // IEEE_802_1X_AUTH_FAILED
                || reason == 34;        // DISASSOC_LOW_ACK
    }

    /**
     * Update WifiMetrics before dumping
     */
    void updateWifiMetrics() {
        int numSavedNetworks = mWifiConfigManager.getConfiguredNetworksSize();
        int numOpenNetworks = 0;
        int numPersonalNetworks = 0;
        int numEnterpriseNetworks = 0;
        int numNetworksAddedByUser = 0;
        int numNetworksAddedByApps = 0;
        int numHiddenNetworks = 0;
        int numPasspoint = 0;
        for (WifiConfiguration config : mWifiConfigManager.getSavedNetworks()) {
            if (config.allowedKeyManagement.get(WifiConfiguration.KeyMgmt.NONE)) {
                numOpenNetworks++;
            } else if (config.isEnterprise()) {
                numEnterpriseNetworks++;
            } else {
                numPersonalNetworks++;
            }
            if (config.selfAdded) {
                numNetworksAddedByUser++;
            } else {
                numNetworksAddedByApps++;
            }
            if (config.hiddenSSID) {
                numHiddenNetworks++;
            }
            if (config.isPasspoint()) {
                numPasspoint++;
            }
        }
        mWifiMetrics.setNumSavedNetworks(numSavedNetworks);
        mWifiMetrics.setNumOpenNetworks(numOpenNetworks);
        mWifiMetrics.setNumPersonalNetworks(numPersonalNetworks);
        mWifiMetrics.setNumEnterpriseNetworks(numEnterpriseNetworks);
        mWifiMetrics.setNumNetworksAddedByUser(numNetworksAddedByUser);
        mWifiMetrics.setNumNetworksAddedByApps(numNetworksAddedByApps);
        mWifiMetrics.setNumHiddenNetworks(numHiddenNetworks);
        mWifiMetrics.setNumPasspointNetworks(numPasspoint);
    }

    private static String getLinkPropertiesSummary(LinkProperties lp) {
        List<String> attributes = new ArrayList(6);
        if (lp.hasIPv4Address()) {
            attributes.add("v4");
        }
        if (lp.hasIPv4DefaultRoute()) {
            attributes.add("v4r");
        }
        if (lp.hasIPv4DnsServer()) {
            attributes.add("v4dns");
        }
        if (lp.hasGlobalIPv6Address()) {
            attributes.add("v6");
        }
        if (lp.hasIPv6DefaultRoute()) {
            attributes.add("v6r");
        }
        if (lp.hasIPv6DnsServer()) {
            attributes.add("v6dns");
        }

        return TextUtils.join(" ", attributes);
    }

    private void wnmFrameReceived(WnmData event) {
        // %012x HS20-SUBSCRIPTION-REMEDIATION "%u %s", osu_method, url
        // %012x HS20-DEAUTH-IMMINENT-NOTICE "%u %u %s", code, reauth_delay, url

        Intent intent = new Intent(WifiManager.PASSPOINT_WNM_FRAME_RECEIVED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);

        intent.putExtra(WifiManager.EXTRA_PASSPOINT_WNM_BSSID, event.getBssid());
        intent.putExtra(WifiManager.EXTRA_PASSPOINT_WNM_URL, event.getUrl());

        if (event.isDeauthEvent()) {
            intent.putExtra(WifiManager.EXTRA_PASSPOINT_WNM_ESS, event.isEss());
            intent.putExtra(WifiManager.EXTRA_PASSPOINT_WNM_DELAY, event.getDelay());
        } else {
            intent.putExtra(WifiManager.EXTRA_PASSPOINT_WNM_METHOD, event.getMethod());
            WifiConfiguration config = getCurrentWifiConfiguration();
            if (config != null && config.FQDN != null) {
                intent.putExtra(WifiManager.EXTRA_PASSPOINT_WNM_PPOINT_MATCH,
                        mWifiConfigManager.matchProviderWithCurrentNetwork(config.FQDN));
            }
        }
        mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
    }

    /**
     * Gets the SSID from the WifiConfiguration pointed at by 'mTargetNetworkId'
     * This should match the network config framework is attempting to connect to.
     */
    private String getTargetSsid() {
        WifiConfiguration currentConfig = mWifiConfigManager.getWifiConfiguration(mTargetNetworkId);
        if (currentConfig != null) {
            return currentConfig.SSID;
        }
        return null;
    }

    /**
     * Check if there is any connection request for WiFi network.
     * Note, caller of this helper function must acquire mWifiReqCountLock.
     */
    private boolean hasConnectionRequests() {
        return mConnectionReqCount > 0 || mUntrustedReqCount > 0;
    }
}
