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

package com.android.server.wifi.scanner;

import android.Manifest;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.wifi.IWifiScanner;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiScanner;
import android.net.wifi.WifiScanner.BssidInfo;
import android.net.wifi.WifiScanner.ChannelSpec;
import android.net.wifi.WifiScanner.PnoSettings;
import android.net.wifi.WifiScanner.ScanData;
import android.net.wifi.WifiScanner.ScanSettings;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.os.WorkSource;
import android.util.ArrayMap;
import android.util.LocalLog;
import android.util.Log;
import android.util.Pair;

import com.android.internal.app.IBatteryStats;
import com.android.internal.util.AsyncChannel;
import com.android.internal.util.Protocol;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.server.wifi.Clock;
import com.android.server.wifi.WifiInjector;
import com.android.server.wifi.WifiMetrics;
import com.android.server.wifi.WifiMetricsProto;
import com.android.server.wifi.WifiNative;
import com.android.server.wifi.WifiStateMachine;
import com.android.server.wifi.scanner.ChannelHelper.ChannelCollection;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

public class WifiScanningServiceImpl extends IWifiScanner.Stub {

    private static final String TAG = WifiScanningService.TAG;
    private static final boolean DBG = false;

    private static final int MIN_PERIOD_PER_CHANNEL_MS = 200;               // DFS needs 120 ms
    private static final int UNKNOWN_PID = -1;

    private final LocalLog mLocalLog = new LocalLog(512);

    private void localLog(String message) {
        mLocalLog.log(message);
    }

    private void logw(String message) {
        Log.w(TAG, message);
        mLocalLog.log(message);
    }

    private void loge(String message) {
        Log.e(TAG, message);
        mLocalLog.log(message);
    }

    private WifiScannerImpl mScannerImpl;

    @Override
    public Messenger getMessenger() {
        if (mClientHandler != null) {
            return new Messenger(mClientHandler);
        } else {
            loge("WifiScanningServiceImpl trying to get messenger w/o initialization");
            return null;
        }
    }

    @Override
    public Bundle getAvailableChannels(int band) {
        mChannelHelper.updateChannels();
        ChannelSpec[] channelSpecs = mChannelHelper.getAvailableScanChannels(band);
        ArrayList<Integer> list = new ArrayList<Integer>(channelSpecs.length);
        for (ChannelSpec channelSpec : channelSpecs) {
            list.add(channelSpec.frequency);
        }
        Bundle b = new Bundle();
        b.putIntegerArrayList(WifiScanner.GET_AVAILABLE_CHANNELS_EXTRA, list);
        return b;
    }

    private void enforceLocationHardwarePermission(int uid) {
        mContext.enforcePermission(
                Manifest.permission.LOCATION_HARDWARE,
                UNKNOWN_PID, uid,
                "LocationHardware");
    }

    private class ClientHandler extends Handler {

        ClientHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case AsyncChannel.CMD_CHANNEL_FULL_CONNECTION: {
                    ExternalClientInfo client = (ExternalClientInfo) mClients.get(msg.replyTo);
                    if (client != null) {
                        logw("duplicate client connection: " + msg.sendingUid + ", messenger="
                                + msg.replyTo);
                        client.mChannel.replyToMessage(msg, AsyncChannel.CMD_CHANNEL_FULLY_CONNECTED,
                                AsyncChannel.STATUS_FULL_CONNECTION_REFUSED_ALREADY_CONNECTED);
                        return;
                    }

                    AsyncChannel ac = new AsyncChannel();
                    ac.connected(mContext, this, msg.replyTo);

                    client = new ExternalClientInfo(msg.sendingUid, msg.replyTo, ac);
                    client.register();

                    ac.replyToMessage(msg, AsyncChannel.CMD_CHANNEL_FULLY_CONNECTED,
                            AsyncChannel.STATUS_SUCCESSFUL);

                    localLog("client connected: " + client);
                    return;
                }
                case AsyncChannel.CMD_CHANNEL_DISCONNECT: {
                    ExternalClientInfo client = (ExternalClientInfo) mClients.get(msg.replyTo);
                    if (client != null) {
                        client.mChannel.disconnect();
                    }
                    return;
                }
                case AsyncChannel.CMD_CHANNEL_DISCONNECTED: {
                    ExternalClientInfo client = (ExternalClientInfo) mClients.get(msg.replyTo);
                    if (client != null && msg.arg1 != AsyncChannel.STATUS_SEND_UNSUCCESSFUL
                            && msg.arg1
                            != AsyncChannel.STATUS_FULL_CONNECTION_REFUSED_ALREADY_CONNECTED) {
                        localLog("client disconnected: " + client + ", reason: " + msg.arg1);
                        client.cleanup();
                    }
                    return;
                }
            }

            try {
                enforceLocationHardwarePermission(msg.sendingUid);
            } catch (SecurityException e) {
                localLog("failed to authorize app: " + e);
                replyFailed(msg, WifiScanner.REASON_NOT_AUTHORIZED, "Not authorized");
                return;
            }

            // Since this message is sent from WifiScanner using |sendMessageSynchronously| which
            // doesn't set the correct |msg.replyTo| field.
            if (msg.what == WifiScanner.CMD_GET_SCAN_RESULTS) {
                mBackgroundScanStateMachine.sendMessage(Message.obtain(msg));
                return;
            }

            ClientInfo ci = mClients.get(msg.replyTo);
            if (ci == null) {
                loge("Could not find client info for message " + msg.replyTo);
                replyFailed(msg, WifiScanner.REASON_INVALID_LISTENER, "Could not find listener");
                return;
            }

            switch (msg.what) {
                case WifiScanner.CMD_START_BACKGROUND_SCAN:
                case WifiScanner.CMD_STOP_BACKGROUND_SCAN:
                case WifiScanner.CMD_SET_HOTLIST:
                case WifiScanner.CMD_RESET_HOTLIST:
                    mBackgroundScanStateMachine.sendMessage(Message.obtain(msg));
                    break;
                case WifiScanner.CMD_START_PNO_SCAN:
                case WifiScanner.CMD_STOP_PNO_SCAN:
                    mPnoScanStateMachine.sendMessage(Message.obtain(msg));
                    break;
                case WifiScanner.CMD_START_SINGLE_SCAN:
                case WifiScanner.CMD_STOP_SINGLE_SCAN:
                    mSingleScanStateMachine.sendMessage(Message.obtain(msg));
                    break;
                case WifiScanner.CMD_CONFIGURE_WIFI_CHANGE:
                case WifiScanner.CMD_START_TRACKING_CHANGE:
                case WifiScanner.CMD_STOP_TRACKING_CHANGE:
                    mWifiChangeStateMachine.sendMessage(Message.obtain(msg));
                    break;
                case WifiScanner.CMD_REGISTER_SCAN_LISTENER:
                    logScanRequest("registerScanListener", ci, msg.arg2, null, null, null);
                    mSingleScanListeners.addRequest(ci, msg.arg2, null, null);
                    replySucceeded(msg);
                    break;
                case WifiScanner.CMD_DEREGISTER_SCAN_LISTENER:
                    logScanRequest("deregisterScanListener", ci, msg.arg2, null, null, null);
                    mSingleScanListeners.removeRequest(ci, msg.arg2);
                    break;
                default:
                    replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "Invalid request");
                    break;
            }
        }
    }

    private static final int BASE = Protocol.BASE_WIFI_SCANNER_SERVICE;

    private static final int CMD_SCAN_RESULTS_AVAILABLE              = BASE + 0;
    private static final int CMD_FULL_SCAN_RESULTS                   = BASE + 1;
    private static final int CMD_HOTLIST_AP_FOUND                    = BASE + 2;
    private static final int CMD_HOTLIST_AP_LOST                     = BASE + 3;
    private static final int CMD_WIFI_CHANGE_DETECTED                = BASE + 4;
    private static final int CMD_WIFI_CHANGE_TIMEOUT                 = BASE + 5;
    private static final int CMD_DRIVER_LOADED                       = BASE + 6;
    private static final int CMD_DRIVER_UNLOADED                     = BASE + 7;
    private static final int CMD_SCAN_PAUSED                         = BASE + 8;
    private static final int CMD_SCAN_RESTARTED                      = BASE + 9;
    private static final int CMD_SCAN_FAILED                         = BASE + 10;
    private static final int CMD_PNO_NETWORK_FOUND                   = BASE + 11;
    private static final int CMD_PNO_SCAN_FAILED                     = BASE + 12;

    private final Context mContext;
    private final Looper mLooper;
    private final WifiScannerImpl.WifiScannerImplFactory mScannerImplFactory;
    private final ArrayMap<Messenger, ClientInfo> mClients;

    private final RequestList<Void> mSingleScanListeners = new RequestList<>();

    private ChannelHelper mChannelHelper;
    private BackgroundScanScheduler mBackgroundScheduler;
    private WifiNative.ScanSettings mPreviousSchedule;

    private WifiBackgroundScanStateMachine mBackgroundScanStateMachine;
    private WifiSingleScanStateMachine mSingleScanStateMachine;
    private WifiChangeStateMachine mWifiChangeStateMachine;
    private WifiPnoScanStateMachine mPnoScanStateMachine;
    private ClientHandler mClientHandler;
    private final IBatteryStats mBatteryStats;
    private final AlarmManager mAlarmManager;
    private final WifiMetrics mWifiMetrics;
    private final Clock mClock;

    WifiScanningServiceImpl(Context context, Looper looper,
            WifiScannerImpl.WifiScannerImplFactory scannerImplFactory, IBatteryStats batteryStats,
            WifiInjector wifiInjector) {
        mContext = context;
        mLooper = looper;
        mScannerImplFactory = scannerImplFactory;
        mBatteryStats = batteryStats;
        mClients = new ArrayMap<>();
        mAlarmManager = (AlarmManager) mContext.getSystemService(Context.ALARM_SERVICE);
        mWifiMetrics = wifiInjector.getWifiMetrics();
        mClock = wifiInjector.getClock();

        mPreviousSchedule = null;
    }

    public void startService() {
        mClientHandler = new ClientHandler(mLooper);
        mBackgroundScanStateMachine = new WifiBackgroundScanStateMachine(mLooper);
        mWifiChangeStateMachine = new WifiChangeStateMachine(mLooper);
        mSingleScanStateMachine = new WifiSingleScanStateMachine(mLooper);
        mPnoScanStateMachine = new WifiPnoScanStateMachine(mLooper);

        mContext.registerReceiver(
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        int state = intent.getIntExtra(
                                WifiManager.EXTRA_SCAN_AVAILABLE, WifiManager.WIFI_STATE_DISABLED);
                        if (DBG) localLog("SCAN_AVAILABLE : " + state);
                        if (state == WifiManager.WIFI_STATE_ENABLED) {
                            mBackgroundScanStateMachine.sendMessage(CMD_DRIVER_LOADED);
                            mSingleScanStateMachine.sendMessage(CMD_DRIVER_LOADED);
                            mPnoScanStateMachine.sendMessage(CMD_DRIVER_LOADED);
                        } else if (state == WifiManager.WIFI_STATE_DISABLED) {
                            mBackgroundScanStateMachine.sendMessage(CMD_DRIVER_UNLOADED);
                            mSingleScanStateMachine.sendMessage(CMD_DRIVER_UNLOADED);
                            mPnoScanStateMachine.sendMessage(CMD_DRIVER_UNLOADED);
                        }
                    }
                }, new IntentFilter(WifiManager.WIFI_SCAN_AVAILABLE));

        mBackgroundScanStateMachine.start();
        mWifiChangeStateMachine.start();
        mSingleScanStateMachine.start();
        mPnoScanStateMachine.start();
    }

    private static boolean isWorkSourceValid(WorkSource workSource) {
        return workSource != null && workSource.size() > 0 && workSource.get(0) >= 0;
    }

    private WorkSource computeWorkSource(ClientInfo ci, WorkSource requestedWorkSource) {
        if (requestedWorkSource != null) {
            if (isWorkSourceValid(requestedWorkSource)) {
                // Wifi currently doesn't use names, so need to clear names out of the
                // supplied WorkSource to allow future WorkSource combining.
                requestedWorkSource.clearNames();
                return requestedWorkSource;
            } else {
                loge("Got invalid work source request: " + requestedWorkSource.toString() +
                        " from " + ci);
            }
        }
        WorkSource callingWorkSource = new WorkSource(ci.getUid());
        if (isWorkSourceValid(callingWorkSource)) {
            return callingWorkSource;
        } else {
            loge("Client has invalid work source: " + callingWorkSource);
            return new WorkSource();
        }
    }

    private class RequestInfo<T> {
        final ClientInfo clientInfo;
        final int handlerId;
        final WorkSource workSource;
        final T settings;

        RequestInfo(ClientInfo clientInfo, int handlerId, WorkSource requestedWorkSource,
                T settings) {
            this.clientInfo = clientInfo;
            this.handlerId = handlerId;
            this.settings = settings;
            this.workSource = computeWorkSource(clientInfo, requestedWorkSource);
        }

        void reportEvent(int what, int arg1, Object obj) {
            clientInfo.reportEvent(what, arg1, handlerId, obj);
        }
    }

    private class RequestList<T> extends ArrayList<RequestInfo<T>> {
        void addRequest(ClientInfo ci, int handler, WorkSource reqworkSource, T settings) {
            add(new RequestInfo<T>(ci, handler, reqworkSource, settings));
        }

        T removeRequest(ClientInfo ci, int handlerId) {
            T removed = null;
            Iterator<RequestInfo<T>> iter = iterator();
            while (iter.hasNext()) {
                RequestInfo<T> entry = iter.next();
                if (entry.clientInfo == ci && entry.handlerId == handlerId) {
                    removed = entry.settings;
                    iter.remove();
                }
            }
            return removed;
        }

        Collection<T> getAllSettings() {
            ArrayList<T> settingsList = new ArrayList<>();
            Iterator<RequestInfo<T>> iter = iterator();
            while (iter.hasNext()) {
                RequestInfo<T> entry = iter.next();
                settingsList.add(entry.settings);
            }
            return settingsList;
        }

        Collection<T> getAllSettingsForClient(ClientInfo ci) {
            ArrayList<T> settingsList = new ArrayList<>();
            Iterator<RequestInfo<T>> iter = iterator();
            while (iter.hasNext()) {
                RequestInfo<T> entry = iter.next();
                if (entry.clientInfo == ci) {
                    settingsList.add(entry.settings);
                }
            }
            return settingsList;
        }

        void removeAllForClient(ClientInfo ci) {
            Iterator<RequestInfo<T>> iter = iterator();
            while (iter.hasNext()) {
                RequestInfo<T> entry = iter.next();
                if (entry.clientInfo == ci) {
                    iter.remove();
                }
            }
        }

        WorkSource createMergedWorkSource() {
            WorkSource mergedSource = new WorkSource();
            for (RequestInfo<T> entry : this) {
                mergedSource.add(entry.workSource);
            }
            return mergedSource;
        }
    }

    /**
     * State machine that holds the state of single scans. Scans should only be active in the
     * ScanningState. The pending scans and active scans maps are swaped when entering
     * ScanningState. Any requests queued while scanning will be placed in the pending queue and
     * executed after transitioning back to IdleState.
     */
    class WifiSingleScanStateMachine extends StateMachine implements WifiNative.ScanEventHandler {
        private final DefaultState mDefaultState = new DefaultState();
        private final DriverStartedState mDriverStartedState = new DriverStartedState();
        private final IdleState  mIdleState  = new IdleState();
        private final ScanningState  mScanningState  = new ScanningState();

        private WifiNative.ScanSettings mActiveScanSettings = null;
        private RequestList<ScanSettings> mActiveScans = new RequestList<>();
        private RequestList<ScanSettings> mPendingScans = new RequestList<>();

        WifiSingleScanStateMachine(Looper looper) {
            super("WifiSingleScanStateMachine", looper);

            setLogRecSize(128);
            setLogOnlyTransitions(false);

            // CHECKSTYLE:OFF IndentationCheck
            addState(mDefaultState);
                addState(mDriverStartedState, mDefaultState);
                    addState(mIdleState, mDriverStartedState);
                    addState(mScanningState, mDriverStartedState);
            // CHECKSTYLE:ON IndentationCheck

            setInitialState(mDefaultState);
        }

        /**
         * Called to indicate a change in state for the current scan.
         * Will dispatch a coresponding event to the state machine
         */
        @Override
        public void onScanStatus(int event) {
            if (DBG) localLog("onScanStatus event received, event=" + event);
            switch(event) {
                case WifiNative.WIFI_SCAN_RESULTS_AVAILABLE:
                case WifiNative.WIFI_SCAN_THRESHOLD_NUM_SCANS:
                case WifiNative.WIFI_SCAN_THRESHOLD_PERCENT:
                    sendMessage(CMD_SCAN_RESULTS_AVAILABLE);
                    break;
                case WifiNative.WIFI_SCAN_FAILED:
                    sendMessage(CMD_SCAN_FAILED);
                    break;
                default:
                    Log.e(TAG, "Unknown scan status event: " + event);
                    break;
            }
        }

        /**
         * Called for each full scan result if requested
         */
        @Override
        public void onFullScanResult(ScanResult fullScanResult, int bucketsScanned) {
            if (DBG) localLog("onFullScanResult received");
            sendMessage(CMD_FULL_SCAN_RESULTS, 0, bucketsScanned, fullScanResult);
        }

        @Override
        public void onScanPaused(ScanData[] scanData) {
            // should not happen for single scan
            Log.e(TAG, "Got scan paused for single scan");
        }

        @Override
        public void onScanRestarted() {
            // should not happen for single scan
            Log.e(TAG, "Got scan restarted for single scan");
        }

        class DefaultState extends State {
            @Override
            public void enter() {
                mActiveScans.clear();
                mPendingScans.clear();
            }
            @Override
            public boolean processMessage(Message msg) {
                switch (msg.what) {
                    case CMD_DRIVER_LOADED:
                        transitionTo(mIdleState);
                        return HANDLED;
                    case CMD_DRIVER_UNLOADED:
                        transitionTo(mDefaultState);
                        return HANDLED;
                    case WifiScanner.CMD_START_SINGLE_SCAN:
                    case WifiScanner.CMD_STOP_SINGLE_SCAN:
                        replyFailed(msg, WifiScanner.REASON_UNSPECIFIED, "not available");
                        return HANDLED;
                    case CMD_SCAN_RESULTS_AVAILABLE:
                        if (DBG) localLog("ignored scan results available event");
                        return HANDLED;
                    case CMD_FULL_SCAN_RESULTS:
                        if (DBG) localLog("ignored full scan result event");
                        return HANDLED;
                    default:
                        return NOT_HANDLED;
                }

            }
        }

        /**
         * State representing when the driver is running. This state is not meant to be transitioned
         * directly, but is instead indented as a parent state of ScanningState and IdleState
         * to hold common functionality and handle cleaning up scans when the driver is shut down.
         */
        class DriverStartedState extends State {
            @Override
            public void exit() {
                mWifiMetrics.incrementScanReturnEntry(
                        WifiMetricsProto.WifiLog.SCAN_FAILURE_INTERRUPTED,
                        mPendingScans.size());
                sendOpFailedToAllAndClear(mPendingScans, WifiScanner.REASON_UNSPECIFIED,
                        "Scan was interrupted");
            }

            @Override
            public boolean processMessage(Message msg) {
                ClientInfo ci = mClients.get(msg.replyTo);

                switch (msg.what) {
                    case WifiScanner.CMD_START_SINGLE_SCAN:
                        mWifiMetrics.incrementOneshotScanCount();
                        int handler = msg.arg2;
                        Bundle scanParams = (Bundle) msg.obj;
                        if (scanParams == null) {
                            logCallback("singleScanInvalidRequest",  ci, handler, "null params");
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "params null");
                            return HANDLED;
                        }
                        scanParams.setDefusable(true);
                        ScanSettings scanSettings =
                                scanParams.getParcelable(WifiScanner.SCAN_PARAMS_SCAN_SETTINGS_KEY);
                        WorkSource workSource =
                                scanParams.getParcelable(WifiScanner.SCAN_PARAMS_WORK_SOURCE_KEY);
                        if (validateScanRequest(ci, handler, scanSettings, workSource)) {
                            logScanRequest("addSingleScanRequest", ci, handler, workSource,
                                    scanSettings, null);
                            replySucceeded(msg);

                            // If there is an active scan that will fulfill the scan request then
                            // mark this request as an active scan, otherwise mark it pending.
                            // If were not currently scanning then try to start a scan. Otherwise
                            // this scan will be scheduled when transitioning back to IdleState
                            // after finishing the current scan.
                            if (getCurrentState() == mScanningState) {
                                if (activeScanSatisfies(scanSettings)) {
                                    mActiveScans.addRequest(ci, handler, workSource, scanSettings);
                                } else {
                                    mPendingScans.addRequest(ci, handler, workSource, scanSettings);
                                }
                            } else {
                                mPendingScans.addRequest(ci, handler, workSource, scanSettings);
                                tryToStartNewScan();
                            }
                        } else {
                            logCallback("singleScanInvalidRequest",  ci, handler, "bad request");
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "bad request");
                            mWifiMetrics.incrementScanReturnEntry(
                                    WifiMetricsProto.WifiLog.SCAN_FAILURE_INVALID_CONFIGURATION, 1);
                        }
                        return HANDLED;
                    case WifiScanner.CMD_STOP_SINGLE_SCAN:
                        removeSingleScanRequest(ci, msg.arg2);
                        return HANDLED;
                    default:
                        return NOT_HANDLED;
                }
            }
        }

        class IdleState extends State {
            @Override
            public void enter() {
                tryToStartNewScan();
            }

            @Override
            public boolean processMessage(Message msg) {
                return NOT_HANDLED;
            }
        }

        class ScanningState extends State {
            private WorkSource mScanWorkSource;

            @Override
            public void enter() {
                mScanWorkSource = mActiveScans.createMergedWorkSource();
                try {
                    mBatteryStats.noteWifiScanStartedFromSource(mScanWorkSource);
                } catch (RemoteException e) {
                    loge(e.toString());
                }
            }

            @Override
            public void exit() {
                mActiveScanSettings = null;
                try {
                    mBatteryStats.noteWifiScanStoppedFromSource(mScanWorkSource);
                } catch (RemoteException e) {
                    loge(e.toString());
                }

                // if any scans are still active (never got results available then indicate failure)
                mWifiMetrics.incrementScanReturnEntry(
                                WifiMetricsProto.WifiLog.SCAN_UNKNOWN,
                                mActiveScans.size());
                sendOpFailedToAllAndClear(mActiveScans, WifiScanner.REASON_UNSPECIFIED,
                        "Scan was interrupted");
            }

            @Override
            public boolean processMessage(Message msg) {
                switch (msg.what) {
                    case CMD_SCAN_RESULTS_AVAILABLE:
                        mWifiMetrics.incrementScanReturnEntry(
                                WifiMetricsProto.WifiLog.SCAN_SUCCESS,
                                mActiveScans.size());
                        reportScanResults(mScannerImpl.getLatestSingleScanResults());
                        mActiveScans.clear();
                        transitionTo(mIdleState);
                        return HANDLED;
                    case CMD_FULL_SCAN_RESULTS:
                        reportFullScanResult((ScanResult) msg.obj, /* bucketsScanned */ msg.arg2);
                        return HANDLED;
                    case CMD_SCAN_FAILED:
                        mWifiMetrics.incrementScanReturnEntry(
                                WifiMetricsProto.WifiLog.SCAN_UNKNOWN, mActiveScans.size());
                        sendOpFailedToAllAndClear(mActiveScans, WifiScanner.REASON_UNSPECIFIED,
                                "Scan failed");
                        transitionTo(mIdleState);
                        return HANDLED;
                    default:
                        return NOT_HANDLED;
                }
            }
        }

        boolean validateScanRequest(ClientInfo ci, int handler, ScanSettings settings,
                WorkSource workSource) {
            if (ci == null) {
                Log.d(TAG, "Failing single scan request ClientInfo not found " + handler);
                return false;
            }
            if (settings.band == WifiScanner.WIFI_BAND_UNSPECIFIED) {
                if (settings.channels == null || settings.channels.length == 0) {
                    Log.d(TAG, "Failing single scan because channel list was empty");
                    return false;
                }
            }
            return true;
        }

        boolean activeScanSatisfies(ScanSettings settings) {
            if (mActiveScanSettings == null) {
                return false;
            }

            // there is always one bucket for a single scan
            WifiNative.BucketSettings activeBucket = mActiveScanSettings.buckets[0];

            // validate that all requested channels are being scanned
            ChannelCollection activeChannels = mChannelHelper.createChannelCollection();
            activeChannels.addChannels(activeBucket);
            if (!activeChannels.containsSettings(settings)) {
                return false;
            }

            // if the request is for a full scan, but there is no ongoing full scan
            if ((settings.reportEvents & WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT) != 0
                    && (activeBucket.report_events & WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT)
                    == 0) {
                return false;
            }

            if (settings.hiddenNetworkIds != null) {
                if (mActiveScanSettings.hiddenNetworkIds == null) {
                    return false;
                }
                Set<Integer> activeHiddenNetworkIds = new HashSet<>();
                for (int id : mActiveScanSettings.hiddenNetworkIds) {
                    activeHiddenNetworkIds.add(id);
                }
                for (int id : settings.hiddenNetworkIds) {
                    if (!activeHiddenNetworkIds.contains(id)) {
                        return false;
                    }
                }
            }

            return true;
        }

        void removeSingleScanRequest(ClientInfo ci, int handler) {
            if (ci != null) {
                logScanRequest("removeSingleScanRequest", ci, handler, null, null, null);
                mPendingScans.removeRequest(ci, handler);
                mActiveScans.removeRequest(ci, handler);
            }
        }

        void removeSingleScanRequests(ClientInfo ci) {
            if (ci != null) {
                logScanRequest("removeSingleScanRequests", ci, -1, null, null, null);
                mPendingScans.removeAllForClient(ci);
                mActiveScans.removeAllForClient(ci);
            }
        }

        void tryToStartNewScan() {
            if (mPendingScans.size() == 0) { // no pending requests
                return;
            }
            mChannelHelper.updateChannels();
            // TODO move merging logic to a scheduler
            WifiNative.ScanSettings settings = new WifiNative.ScanSettings();
            settings.num_buckets = 1;
            WifiNative.BucketSettings bucketSettings = new WifiNative.BucketSettings();
            bucketSettings.bucket = 0;
            bucketSettings.period_ms = 0;
            bucketSettings.report_events = WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN;

            ChannelCollection channels = mChannelHelper.createChannelCollection();
            HashSet<Integer> hiddenNetworkIdSet = new HashSet<>();
            for (RequestInfo<ScanSettings> entry : mPendingScans) {
                channels.addChannels(entry.settings);
                if (entry.settings.hiddenNetworkIds != null) {
                    for (int i = 0; i < entry.settings.hiddenNetworkIds.length; i++) {
                        hiddenNetworkIdSet.add(entry.settings.hiddenNetworkIds[i]);
                    }
                }
                if ((entry.settings.reportEvents & WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT)
                        != 0) {
                    bucketSettings.report_events |= WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT;
                }
            }
            if (hiddenNetworkIdSet.size() > 0) {
                settings.hiddenNetworkIds = new int[hiddenNetworkIdSet.size()];
                int numHiddenNetworks = 0;
                for (Integer hiddenNetworkId : hiddenNetworkIdSet) {
                    settings.hiddenNetworkIds[numHiddenNetworks++] = hiddenNetworkId;
                }
            }

            channels.fillBucketSettings(bucketSettings, Integer.MAX_VALUE);

            settings.buckets = new WifiNative.BucketSettings[] {bucketSettings};
            if (mScannerImpl.startSingleScan(settings, this)) {
                // store the active scan settings
                mActiveScanSettings = settings;
                // swap pending and active scan requests
                RequestList<ScanSettings> tmp = mActiveScans;
                mActiveScans = mPendingScans;
                mPendingScans = tmp;
                // make sure that the pending list is clear
                mPendingScans.clear();
                transitionTo(mScanningState);
            } else {
                mWifiMetrics.incrementScanReturnEntry(
                        WifiMetricsProto.WifiLog.SCAN_UNKNOWN, mPendingScans.size());
                // notify and cancel failed scans
                sendOpFailedToAllAndClear(mPendingScans, WifiScanner.REASON_UNSPECIFIED,
                        "Failed to start single scan");
            }
        }

        void sendOpFailedToAllAndClear(RequestList<?> clientHandlers, int reason,
                String description) {
            for (RequestInfo<?> entry : clientHandlers) {
                logCallback("singleScanFailed",  entry.clientInfo, entry.handlerId,
                        "reason=" + reason + ", " + description);
                entry.reportEvent(WifiScanner.CMD_OP_FAILED, 0,
                        new WifiScanner.OperationResult(reason, description));
            }
            clientHandlers.clear();
        }

        void reportFullScanResult(ScanResult result, int bucketsScanned) {
            for (RequestInfo<ScanSettings> entry : mActiveScans) {
                if (ScanScheduleUtil.shouldReportFullScanResultForSettings(mChannelHelper,
                                result, bucketsScanned, entry.settings, -1)) {
                    entry.reportEvent(WifiScanner.CMD_FULL_SCAN_RESULT, 0, result);
                }
            }

            for (RequestInfo<Void> entry : mSingleScanListeners) {
                entry.reportEvent(WifiScanner.CMD_FULL_SCAN_RESULT, 0, result);
            }
        }

        void reportScanResults(ScanData results) {
            if (results != null && results.getResults() != null) {
                if (results.getResults().length > 0) {
                    mWifiMetrics.incrementNonEmptyScanResultCount();
                } else {
                    mWifiMetrics.incrementEmptyScanResultCount();
                }
            }
            ScanData[] allResults = new ScanData[] {results};
            for (RequestInfo<ScanSettings> entry : mActiveScans) {
                ScanData[] resultsToDeliver = ScanScheduleUtil.filterResultsForSettings(
                        mChannelHelper, allResults, entry.settings, -1);
                WifiScanner.ParcelableScanData parcelableResultsToDeliver =
                        new WifiScanner.ParcelableScanData(resultsToDeliver);
                logCallback("singleScanResults",  entry.clientInfo, entry.handlerId,
                        describeForLog(resultsToDeliver));
                entry.reportEvent(WifiScanner.CMD_SCAN_RESULT, 0, parcelableResultsToDeliver);
                // make sure the handler is removed
                entry.reportEvent(WifiScanner.CMD_SINGLE_SCAN_COMPLETED, 0, null);
            }

            WifiScanner.ParcelableScanData parcelableAllResults =
                    new WifiScanner.ParcelableScanData(allResults);
            for (RequestInfo<Void> entry : mSingleScanListeners) {
                logCallback("singleScanResults",  entry.clientInfo, entry.handlerId,
                        describeForLog(allResults));
                entry.reportEvent(WifiScanner.CMD_SCAN_RESULT, 0, parcelableAllResults);
            }
        }
    }

    class WifiBackgroundScanStateMachine extends StateMachine
            implements WifiNative.ScanEventHandler, WifiNative.HotlistEventHandler {

        private final DefaultState mDefaultState = new DefaultState();
        private final StartedState mStartedState = new StartedState();
        private final PausedState  mPausedState  = new PausedState();

        private final RequestList<ScanSettings> mActiveBackgroundScans = new RequestList<>();
        private final RequestList<WifiScanner.HotlistSettings> mActiveHotlistSettings =
                new RequestList<>();

        WifiBackgroundScanStateMachine(Looper looper) {
            super("WifiBackgroundScanStateMachine", looper);

            setLogRecSize(512);
            setLogOnlyTransitions(false);

            // CHECKSTYLE:OFF IndentationCheck
            addState(mDefaultState);
                addState(mStartedState, mDefaultState);
                addState(mPausedState, mDefaultState);
            // CHECKSTYLE:ON IndentationCheck

            setInitialState(mDefaultState);
        }

        public Collection<ScanSettings> getBackgroundScanSettings(ClientInfo ci) {
            return mActiveBackgroundScans.getAllSettingsForClient(ci);
        }

        public void removeBackgroundScanSettings(ClientInfo ci) {
            mActiveBackgroundScans.removeAllForClient(ci);
            updateSchedule();
        }

        public void removeHotlistSettings(ClientInfo ci) {
            mActiveHotlistSettings.removeAllForClient(ci);
            resetHotlist();
        }

        @Override
        public void onScanStatus(int event) {
            if (DBG) localLog("onScanStatus event received, event=" + event);
            switch(event) {
                case WifiNative.WIFI_SCAN_RESULTS_AVAILABLE:
                case WifiNative.WIFI_SCAN_THRESHOLD_NUM_SCANS:
                case WifiNative.WIFI_SCAN_THRESHOLD_PERCENT:
                    sendMessage(CMD_SCAN_RESULTS_AVAILABLE);
                    break;
                case WifiNative.WIFI_SCAN_FAILED:
                    sendMessage(CMD_SCAN_FAILED);
                    break;
                default:
                    Log.e(TAG, "Unknown scan status event: " + event);
                    break;
            }
        }

        @Override
        public void onFullScanResult(ScanResult fullScanResult, int bucketsScanned) {
            if (DBG) localLog("onFullScanResult received");
            sendMessage(CMD_FULL_SCAN_RESULTS, 0, bucketsScanned, fullScanResult);
        }

        @Override
        public void onScanPaused(ScanData scanData[]) {
            if (DBG) localLog("onScanPaused received");
            sendMessage(CMD_SCAN_PAUSED, scanData);
        }

        @Override
        public void onScanRestarted() {
            if (DBG) localLog("onScanRestarted received");
            sendMessage(CMD_SCAN_RESTARTED);
        }

        @Override
        public void onHotlistApFound(ScanResult[] results) {
            if (DBG) localLog("onHotlistApFound event received");
            sendMessage(CMD_HOTLIST_AP_FOUND, 0, 0, results);
        }

        @Override
        public void onHotlistApLost(ScanResult[] results) {
            if (DBG) localLog("onHotlistApLost event received");
            sendMessage(CMD_HOTLIST_AP_LOST, 0, 0, results);
        }

        class DefaultState extends State {
            @Override
            public void enter() {
                if (DBG) localLog("DefaultState");
                mActiveBackgroundScans.clear();
                mActiveHotlistSettings.clear();
            }

            @Override
            public boolean processMessage(Message msg) {
                switch (msg.what) {
                    case CMD_DRIVER_LOADED:
                        // TODO this should be moved to a common location since it is used outside
                        // of this state machine. It is ok right now because the driver loaded event
                        // is sent to this state machine first.
                        if (mScannerImpl == null) {
                            mScannerImpl = mScannerImplFactory.create(mContext, mLooper, mClock);
                            mChannelHelper = mScannerImpl.getChannelHelper();
                        }

                        mBackgroundScheduler = new BackgroundScanScheduler(mChannelHelper);

                        WifiNative.ScanCapabilities capabilities =
                                new WifiNative.ScanCapabilities();
                        if (!mScannerImpl.getScanCapabilities(capabilities)) {
                            loge("could not get scan capabilities");
                            return HANDLED;
                        }
                        mBackgroundScheduler.setMaxBuckets(capabilities.max_scan_buckets);
                        mBackgroundScheduler.setMaxApPerScan(capabilities.max_ap_cache_per_scan);

                        Log.i(TAG, "wifi driver loaded with scan capabilities: "
                                + "max buckets=" + capabilities.max_scan_buckets);

                        transitionTo(mStartedState);
                        return HANDLED;
                    case CMD_DRIVER_UNLOADED:
                        Log.i(TAG, "wifi driver unloaded");
                        transitionTo(mDefaultState);
                        break;
                    case WifiScanner.CMD_START_BACKGROUND_SCAN:
                    case WifiScanner.CMD_STOP_BACKGROUND_SCAN:
                    case WifiScanner.CMD_START_SINGLE_SCAN:
                    case WifiScanner.CMD_STOP_SINGLE_SCAN:
                    case WifiScanner.CMD_SET_HOTLIST:
                    case WifiScanner.CMD_RESET_HOTLIST:
                    case WifiScanner.CMD_GET_SCAN_RESULTS:
                        replyFailed(msg, WifiScanner.REASON_UNSPECIFIED, "not available");
                        break;

                    case CMD_SCAN_RESULTS_AVAILABLE:
                        if (DBG) localLog("ignored scan results available event");
                        break;

                    case CMD_FULL_SCAN_RESULTS:
                        if (DBG) localLog("ignored full scan result event");
                        break;

                    default:
                        break;
                }

                return HANDLED;
            }
        }

        class StartedState extends State {

            @Override
            public void enter() {
                if (DBG) localLog("StartedState");
            }

            @Override
            public void exit() {
                sendBackgroundScanFailedToAllAndClear(
                        WifiScanner.REASON_UNSPECIFIED, "Scan was interrupted");
                sendHotlistFailedToAllAndClear(
                        WifiScanner.REASON_UNSPECIFIED, "Scan was interrupted");
                mScannerImpl.cleanup();
            }

            @Override
            public boolean processMessage(Message msg) {
                ClientInfo ci = mClients.get(msg.replyTo);

                switch (msg.what) {
                    case CMD_DRIVER_LOADED:
                        return NOT_HANDLED;
                    case CMD_DRIVER_UNLOADED:
                        return NOT_HANDLED;
                    case WifiScanner.CMD_START_BACKGROUND_SCAN: {
                        mWifiMetrics.incrementBackgroundScanCount();
                        Bundle scanParams = (Bundle) msg.obj;
                        if (scanParams == null) {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "params null");
                            return HANDLED;
                        }
                        scanParams.setDefusable(true);
                        ScanSettings scanSettings =
                                scanParams.getParcelable(WifiScanner.SCAN_PARAMS_SCAN_SETTINGS_KEY);
                        WorkSource workSource =
                                scanParams.getParcelable(WifiScanner.SCAN_PARAMS_WORK_SOURCE_KEY);
                        if (addBackgroundScanRequest(ci, msg.arg2, scanSettings, workSource)) {
                            replySucceeded(msg);
                        } else {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "bad request");
                        }
                        break;
                    }
                    case WifiScanner.CMD_STOP_BACKGROUND_SCAN:
                        removeBackgroundScanRequest(ci, msg.arg2);
                        break;
                    case WifiScanner.CMD_GET_SCAN_RESULTS:
                        reportScanResults(mScannerImpl.getLatestBatchedScanResults(true));
                        replySucceeded(msg);
                        break;
                    case WifiScanner.CMD_SET_HOTLIST:
                        if (addHotlist(ci, msg.arg2, (WifiScanner.HotlistSettings) msg.obj)) {
                            replySucceeded(msg);
                        } else {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "bad request");
                        }
                        break;
                    case WifiScanner.CMD_RESET_HOTLIST:
                        removeHotlist(ci, msg.arg2);
                        break;
                    case CMD_SCAN_RESULTS_AVAILABLE:
                        reportScanResults(mScannerImpl.getLatestBatchedScanResults(true));
                        break;
                    case CMD_FULL_SCAN_RESULTS:
                        reportFullScanResult((ScanResult) msg.obj, /* bucketsScanned */ msg.arg2);
                        break;
                    case CMD_HOTLIST_AP_FOUND:
                        reportHotlistResults(WifiScanner.CMD_AP_FOUND, (ScanResult[]) msg.obj);
                        break;
                    case CMD_HOTLIST_AP_LOST:
                        reportHotlistResults(WifiScanner.CMD_AP_LOST, (ScanResult[]) msg.obj);
                        break;
                    case CMD_SCAN_PAUSED:
                        reportScanResults((ScanData[]) msg.obj);
                        transitionTo(mPausedState);
                        break;
                    case CMD_SCAN_FAILED:
                        Log.e(TAG, "WifiScanner background scan gave CMD_SCAN_FAILED");
                        sendBackgroundScanFailedToAllAndClear(
                                WifiScanner.REASON_UNSPECIFIED, "Background Scan failed");
                        break;
                    default:
                        return NOT_HANDLED;
                }

                return HANDLED;
            }
        }

        class PausedState extends State {
            @Override
            public void enter() {
                if (DBG) localLog("PausedState");
            }

            @Override
            public boolean processMessage(Message msg) {
                switch (msg.what) {
                    case CMD_SCAN_RESTARTED:
                        transitionTo(mStartedState);
                        break;
                    default:
                        deferMessage(msg);
                        break;
                }
                return HANDLED;
            }
        }

        private boolean addBackgroundScanRequest(ClientInfo ci, int handler,
                ScanSettings settings, WorkSource workSource) {
            // sanity check the input
            if (ci == null) {
                Log.d(TAG, "Failing scan request ClientInfo not found " + handler);
                return false;
            }
            if (settings.periodInMs < WifiScanner.MIN_SCAN_PERIOD_MS) {
                loge("Failing scan request because periodInMs is " + settings.periodInMs
                        + ", min scan period is: " + WifiScanner.MIN_SCAN_PERIOD_MS);
                return false;
            }

            if (settings.band == WifiScanner.WIFI_BAND_UNSPECIFIED && settings.channels == null) {
                loge("Channels was null with unspecified band");
                return false;
            }

            if (settings.band == WifiScanner.WIFI_BAND_UNSPECIFIED
                    && settings.channels.length == 0) {
                loge("No channels specified");
                return false;
            }

            int minSupportedPeriodMs = mChannelHelper.estimateScanDuration(settings);
            if (settings.periodInMs < minSupportedPeriodMs) {
                loge("Failing scan request because minSupportedPeriodMs is "
                        + minSupportedPeriodMs + " but the request wants " + settings.periodInMs);
                return false;
            }

            // check truncated binary exponential back off scan settings
            if (settings.maxPeriodInMs != 0 && settings.maxPeriodInMs != settings.periodInMs) {
                if (settings.maxPeriodInMs < settings.periodInMs) {
                    loge("Failing scan request because maxPeriodInMs is " + settings.maxPeriodInMs
                            + " but less than periodInMs " + settings.periodInMs);
                    return false;
                }
                if (settings.maxPeriodInMs > WifiScanner.MAX_SCAN_PERIOD_MS) {
                    loge("Failing scan request because maxSupportedPeriodMs is "
                            + WifiScanner.MAX_SCAN_PERIOD_MS + " but the request wants "
                            + settings.maxPeriodInMs);
                    return false;
                }
                if (settings.stepCount < 1) {
                    loge("Failing scan request because stepCount is " + settings.stepCount
                            + " which is less than 1");
                    return false;
                }
            }

            logScanRequest("addBackgroundScanRequest", ci, handler, null, settings, null);
            mActiveBackgroundScans.addRequest(ci, handler, workSource, settings);

            if (updateSchedule()) {
                return true;
            } else {
                mActiveBackgroundScans.removeRequest(ci, handler);
                localLog("Failing scan request because failed to reset scan");
                return false;
            }
        }

        private boolean updateSchedule() {
            if (mChannelHelper == null || mBackgroundScheduler == null || mScannerImpl == null) {
                loge("Failed to update schedule because WifiScanningService is not initialized");
                return false;
            }
            mChannelHelper.updateChannels();
            Collection<ScanSettings> settings = mActiveBackgroundScans.getAllSettings();

            mBackgroundScheduler.updateSchedule(settings);
            WifiNative.ScanSettings schedule = mBackgroundScheduler.getSchedule();

            if (ScanScheduleUtil.scheduleEquals(mPreviousSchedule, schedule)) {
                if (DBG) Log.d(TAG, "schedule updated with no change");
                return true;
            }

            mPreviousSchedule = schedule;

            if (schedule.num_buckets == 0) {
                mScannerImpl.stopBatchedScan();
                if (DBG) Log.d(TAG, "scan stopped");
                return true;
            } else {
                localLog("starting scan: "
                        + "base period=" + schedule.base_period_ms
                        + ", max ap per scan=" + schedule.max_ap_per_scan
                        + ", batched scans=" + schedule.report_threshold_num_scans);
                for (int b = 0; b < schedule.num_buckets; b++) {
                    WifiNative.BucketSettings bucket = schedule.buckets[b];
                    localLog("bucket " + bucket.bucket + " (" + bucket.period_ms + "ms)"
                            + "[" + bucket.report_events + "]: "
                            + ChannelHelper.toString(bucket));
                }

                if (mScannerImpl.startBatchedScan(schedule, this)) {
                    if (DBG) {
                        Log.d(TAG, "scan restarted with " + schedule.num_buckets
                                + " bucket(s) and base period: " + schedule.base_period_ms);
                    }
                    return true;
                } else {
                    mPreviousSchedule = null;
                    loge("error starting scan: "
                            + "base period=" + schedule.base_period_ms
                            + ", max ap per scan=" + schedule.max_ap_per_scan
                            + ", batched scans=" + schedule.report_threshold_num_scans);
                    for (int b = 0; b < schedule.num_buckets; b++) {
                        WifiNative.BucketSettings bucket = schedule.buckets[b];
                        loge("bucket " + bucket.bucket + " (" + bucket.period_ms + "ms)"
                                + "[" + bucket.report_events + "]: "
                                + ChannelHelper.toString(bucket));
                    }
                    return false;
                }
            }
        }

        private void removeBackgroundScanRequest(ClientInfo ci, int handler) {
            if (ci != null) {
                ScanSettings settings = mActiveBackgroundScans.removeRequest(ci, handler);
                logScanRequest("removeBackgroundScanRequest", ci, handler, null, settings, null);
                updateSchedule();
            }
        }

        private void reportFullScanResult(ScanResult result, int bucketsScanned) {
            for (RequestInfo<ScanSettings> entry : mActiveBackgroundScans) {
                ClientInfo ci = entry.clientInfo;
                int handler = entry.handlerId;
                ScanSettings settings = entry.settings;
                if (mBackgroundScheduler.shouldReportFullScanResultForSettings(
                                result, bucketsScanned, settings)) {
                    ScanResult newResult = new ScanResult(result);
                    if (result.informationElements != null) {
                        newResult.informationElements = result.informationElements.clone();
                    }
                    else {
                        newResult.informationElements = null;
                    }
                    ci.reportEvent(WifiScanner.CMD_FULL_SCAN_RESULT, 0, handler, newResult);
                }
            }
        }

        private void reportScanResults(ScanData[] results) {
            for (ScanData result : results) {
                if (result != null && result.getResults() != null) {
                    if (result.getResults().length > 0) {
                        mWifiMetrics.incrementNonEmptyScanResultCount();
                    } else {
                        mWifiMetrics.incrementEmptyScanResultCount();
                    }
                }
            }
            for (RequestInfo<ScanSettings> entry : mActiveBackgroundScans) {
                ClientInfo ci = entry.clientInfo;
                int handler = entry.handlerId;
                ScanSettings settings = entry.settings;
                ScanData[] resultsToDeliver =
                        mBackgroundScheduler.filterResultsForSettings(results, settings);
                if (resultsToDeliver != null) {
                    logCallback("backgroundScanResults", ci, handler,
                            describeForLog(resultsToDeliver));
                    WifiScanner.ParcelableScanData parcelableScanData =
                            new WifiScanner.ParcelableScanData(resultsToDeliver);
                    ci.reportEvent(WifiScanner.CMD_SCAN_RESULT, 0, handler, parcelableScanData);
                }
            }
        }

        private void sendBackgroundScanFailedToAllAndClear(int reason, String description) {
            for (RequestInfo<ScanSettings> entry : mActiveBackgroundScans) {
                ClientInfo ci = entry.clientInfo;
                int handler = entry.handlerId;
                ci.reportEvent(WifiScanner.CMD_OP_FAILED, 0, handler,
                        new WifiScanner.OperationResult(reason, description));
            }
            mActiveBackgroundScans.clear();
        }

        private boolean addHotlist(ClientInfo ci, int handler,
                WifiScanner.HotlistSettings settings) {
            if (ci == null) {
                Log.d(TAG, "Failing hotlist request ClientInfo not found " + handler);
                return false;
            }
            mActiveHotlistSettings.addRequest(ci, handler, null, settings);
            resetHotlist();
            return true;
        }

        private void removeHotlist(ClientInfo ci, int handler) {
            if (ci != null) {
                mActiveHotlistSettings.removeRequest(ci, handler);
                resetHotlist();
            }
        }

        private void resetHotlist() {
            if (mScannerImpl == null) {
                loge("Failed to update hotlist because WifiScanningService is not initialized");
                return;
            }

            Collection<WifiScanner.HotlistSettings> settings =
                    mActiveHotlistSettings.getAllSettings();
            int num_hotlist_ap = 0;

            for (WifiScanner.HotlistSettings s : settings) {
                num_hotlist_ap +=  s.bssidInfos.length;
            }

            if (num_hotlist_ap == 0) {
                mScannerImpl.resetHotlist();
            } else {
                BssidInfo[] bssidInfos = new BssidInfo[num_hotlist_ap];
                int apLostThreshold = Integer.MAX_VALUE;
                int index = 0;
                for (WifiScanner.HotlistSettings s : settings) {
                    for (int i = 0; i < s.bssidInfos.length; i++, index++) {
                        bssidInfos[index] = s.bssidInfos[i];
                    }
                    if (s.apLostThreshold < apLostThreshold) {
                        apLostThreshold = s.apLostThreshold;
                    }
                }

                WifiScanner.HotlistSettings mergedSettings = new WifiScanner.HotlistSettings();
                mergedSettings.bssidInfos = bssidInfos;
                mergedSettings.apLostThreshold = apLostThreshold;
                mScannerImpl.setHotlist(mergedSettings, this);
            }
        }

        private void reportHotlistResults(int what, ScanResult[] results) {
            if (DBG) localLog("reportHotlistResults " + what + " results " + results.length);
            for (RequestInfo<WifiScanner.HotlistSettings> entry : mActiveHotlistSettings) {
                ClientInfo ci = entry.clientInfo;
                int handler = entry.handlerId;
                WifiScanner.HotlistSettings settings = entry.settings;
                int num_results = 0;
                for (ScanResult result : results) {
                    for (BssidInfo BssidInfo : settings.bssidInfos) {
                        if (result.BSSID.equalsIgnoreCase(BssidInfo.bssid)) {
                            num_results++;
                            break;
                        }
                    }
                }
                if (num_results == 0) {
                    // nothing to report
                    return;
                }
                ScanResult[] results2 = new ScanResult[num_results];
                int index = 0;
                for (ScanResult result : results) {
                    for (BssidInfo BssidInfo : settings.bssidInfos) {
                        if (result.BSSID.equalsIgnoreCase(BssidInfo.bssid)) {
                            results2[index] = result;
                            index++;
                        }
                    }
                }
                WifiScanner.ParcelableScanResults parcelableScanResults =
                        new WifiScanner.ParcelableScanResults(results2);

                ci.reportEvent(what, 0, handler, parcelableScanResults);
            }
        }

        private void sendHotlistFailedToAllAndClear(int reason, String description) {
            for (RequestInfo<WifiScanner.HotlistSettings> entry : mActiveHotlistSettings) {
                ClientInfo ci = entry.clientInfo;
                int handler = entry.handlerId;
                ci.reportEvent(WifiScanner.CMD_OP_FAILED, 0, handler,
                        new WifiScanner.OperationResult(reason, description));
            }
            mActiveHotlistSettings.clear();
        }
    }

    /**
     * PNO scan state machine has 5 states:
     * -Default State
     *   -Started State
     *     -Hw Pno Scan state
     *       -Single Scan state
     *     -Sw Pno Scan state
     *
     * These are the main state transitions:
     * 1. Start at |Default State|
     * 2. Move to |Started State| when we get the |WIFI_SCAN_AVAILABLE| broadcast from WifiManager.
     * 3. When a new PNO scan request comes in:
     *   a.1. Switch to |Hw Pno Scan state| when the device supports HW PNO
     *        (This could either be HAL based ePNO or supplicant based PNO).
     *   a.2. In |Hw Pno Scan state| when PNO scan results are received, check if the result
     *        contains IE (information elements). If yes, send the results to the client, else
     *        switch to |Single Scan state| and send the result to the client when the scan result
     *        is obtained.
     *   b.1. Switch to |Sw Pno Scan state| when the device does not supports HW PNO
     *        (This is for older devices which do not support HW PNO and for connected PNO on
     *         devices which support supplicant based PNO)
     *   b.2. In |Sw Pno Scan state| send the result to the client when the background scan result
     *        is obtained
     *
     * Note: PNO scans only work for a single client today. We don't have support in HW to support
     * multiple requests at the same time, so will need non-trivial changes to support (if at all
     * possible) in WifiScanningService.
     */
    class WifiPnoScanStateMachine extends StateMachine implements WifiNative.PnoEventHandler {

        private final DefaultState mDefaultState = new DefaultState();
        private final StartedState mStartedState = new StartedState();
        private final HwPnoScanState mHwPnoScanState = new HwPnoScanState();
        private final SwPnoScanState mSwPnoScanState = new SwPnoScanState();
        private final SingleScanState mSingleScanState = new SingleScanState();
        private InternalClientInfo mInternalClientInfo;

        private final RequestList<Pair<PnoSettings, ScanSettings>> mActivePnoScans =
                new RequestList<>();

        WifiPnoScanStateMachine(Looper looper) {
            super("WifiPnoScanStateMachine", looper);

            setLogRecSize(512);
            setLogOnlyTransitions(false);

            // CHECKSTYLE:OFF IndentationCheck
            addState(mDefaultState);
                addState(mStartedState, mDefaultState);
                    addState(mHwPnoScanState, mStartedState);
                        addState(mSingleScanState, mHwPnoScanState);
                    addState(mSwPnoScanState, mStartedState);
            // CHECKSTYLE:ON IndentationCheck

            setInitialState(mDefaultState);
        }

        public void removePnoSettings(ClientInfo ci) {
            mActivePnoScans.removeAllForClient(ci);
            transitionTo(mStartedState);
        }

        @Override
        public void onPnoNetworkFound(ScanResult[] results) {
            if (DBG) localLog("onWifiPnoNetworkFound event received");
            sendMessage(CMD_PNO_NETWORK_FOUND, 0, 0, results);
        }

        @Override
        public void onPnoScanFailed() {
            if (DBG) localLog("onWifiPnoScanFailed event received");
            sendMessage(CMD_PNO_SCAN_FAILED, 0, 0, null);
        }

        class DefaultState extends State {
            @Override
            public void enter() {
                if (DBG) localLog("DefaultState");
            }

            @Override
            public boolean processMessage(Message msg) {
                switch (msg.what) {
                    case CMD_DRIVER_LOADED:
                        transitionTo(mStartedState);
                        break;
                    case CMD_DRIVER_UNLOADED:
                        transitionTo(mDefaultState);
                        break;
                    case WifiScanner.CMD_START_PNO_SCAN:
                    case WifiScanner.CMD_STOP_PNO_SCAN:
                        replyFailed(msg, WifiScanner.REASON_UNSPECIFIED, "not available");
                        break;
                    case CMD_PNO_NETWORK_FOUND:
                    case CMD_PNO_SCAN_FAILED:
                    case WifiScanner.CMD_SCAN_RESULT:
                    case WifiScanner.CMD_OP_FAILED:
                        loge("Unexpected message " + msg.what);
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

        class StartedState extends State {
            @Override
            public void enter() {
                if (DBG) localLog("StartedState");
            }

            @Override
            public void exit() {
                sendPnoScanFailedToAllAndClear(
                        WifiScanner.REASON_UNSPECIFIED, "Scan was interrupted");
            }

            @Override
            public boolean processMessage(Message msg) {
                ClientInfo ci = mClients.get(msg.replyTo);
                switch (msg.what) {
                    case WifiScanner.CMD_START_PNO_SCAN:
                        Bundle pnoParams = (Bundle) msg.obj;
                        if (pnoParams == null) {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "params null");
                            return HANDLED;
                        }
                        pnoParams.setDefusable(true);
                        PnoSettings pnoSettings =
                                pnoParams.getParcelable(WifiScanner.PNO_PARAMS_PNO_SETTINGS_KEY);
                        // This message is handled after the transition to SwPnoScan/HwPnoScan state
                        deferMessage(msg);
                        if (mScannerImpl.isHwPnoSupported(pnoSettings.isConnected)) {
                            transitionTo(mHwPnoScanState);
                        } else {
                            transitionTo(mSwPnoScanState);
                        }
                        break;
                    case WifiScanner.CMD_STOP_PNO_SCAN:
                        replyFailed(msg, WifiScanner.REASON_UNSPECIFIED, "no scan running");
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

        class HwPnoScanState extends State {
            @Override
            public void enter() {
                if (DBG) localLog("HwPnoScanState");
            }

            @Override
            public void exit() {
                // Reset PNO scan in ScannerImpl before we exit.
                mScannerImpl.resetHwPnoList();
                removeInternalClient();
            }

            @Override
            public boolean processMessage(Message msg) {
                ClientInfo ci = mClients.get(msg.replyTo);
                switch (msg.what) {
                    case WifiScanner.CMD_START_PNO_SCAN:
                        Bundle pnoParams = (Bundle) msg.obj;
                        if (pnoParams == null) {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "params null");
                            return HANDLED;
                        }
                        pnoParams.setDefusable(true);
                        PnoSettings pnoSettings =
                                pnoParams.getParcelable(WifiScanner.PNO_PARAMS_PNO_SETTINGS_KEY);
                        ScanSettings scanSettings =
                                pnoParams.getParcelable(WifiScanner.PNO_PARAMS_SCAN_SETTINGS_KEY);
                        if (addHwPnoScanRequest(ci, msg.arg2, scanSettings, pnoSettings)) {
                            replySucceeded(msg);
                        } else {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "bad request");
                            transitionTo(mStartedState);
                        }
                        break;
                    case WifiScanner.CMD_STOP_PNO_SCAN:
                        removeHwPnoScanRequest(ci, msg.arg2);
                        transitionTo(mStartedState);
                        break;
                    case CMD_PNO_NETWORK_FOUND:
                        ScanResult[] scanResults = ((ScanResult[]) msg.obj);
                        if (isSingleScanNeeded(scanResults)) {
                            ScanSettings activeScanSettings = getScanSettings();
                            if (activeScanSettings == null) {
                                sendPnoScanFailedToAllAndClear(
                                        WifiScanner.REASON_UNSPECIFIED,
                                        "couldn't retrieve setting");
                                transitionTo(mStartedState);
                            } else {
                                addSingleScanRequest(activeScanSettings);
                                transitionTo(mSingleScanState);
                            }
                        } else {
                            reportPnoNetworkFound((ScanResult[]) msg.obj);
                        }
                        break;
                    case CMD_PNO_SCAN_FAILED:
                        sendPnoScanFailedToAllAndClear(
                                WifiScanner.REASON_UNSPECIFIED, "pno scan failed");
                        transitionTo(mStartedState);
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

        class SingleScanState extends State {
            @Override
            public void enter() {
                if (DBG) localLog("SingleScanState");
            }

            @Override
            public boolean processMessage(Message msg) {
                ClientInfo ci = mClients.get(msg.replyTo);
                switch (msg.what) {
                    case WifiScanner.CMD_SCAN_RESULT:
                        WifiScanner.ParcelableScanData parcelableScanData =
                                (WifiScanner.ParcelableScanData) msg.obj;
                        ScanData[] scanDatas = parcelableScanData.getResults();
                        ScanData lastScanData = scanDatas[scanDatas.length - 1];
                        reportPnoNetworkFound(lastScanData.getResults());
                        transitionTo(mHwPnoScanState);
                        break;
                    case WifiScanner.CMD_OP_FAILED:
                        sendPnoScanFailedToAllAndClear(
                                WifiScanner.REASON_UNSPECIFIED, "single scan failed");
                        transitionTo(mStartedState);
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

        class SwPnoScanState extends State {
            private final ArrayList<ScanResult> mSwPnoFullScanResults = new ArrayList<>();

            @Override
            public void enter() {
                if (DBG) localLog("SwPnoScanState");
                mSwPnoFullScanResults.clear();
            }

            @Override
            public void exit() {
                removeInternalClient();
            }

            @Override
            public boolean processMessage(Message msg) {
                ClientInfo ci = mClients.get(msg.replyTo);
                switch (msg.what) {
                    case WifiScanner.CMD_START_PNO_SCAN:
                        Bundle pnoParams = (Bundle) msg.obj;
                        if (pnoParams == null) {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "params null");
                            return HANDLED;
                        }
                        pnoParams.setDefusable(true);
                        PnoSettings pnoSettings =
                                pnoParams.getParcelable(WifiScanner.PNO_PARAMS_PNO_SETTINGS_KEY);
                        ScanSettings scanSettings =
                                pnoParams.getParcelable(WifiScanner.PNO_PARAMS_SCAN_SETTINGS_KEY);
                        if (addSwPnoScanRequest(ci, msg.arg2, scanSettings, pnoSettings)) {
                            replySucceeded(msg);
                        } else {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "bad request");
                            transitionTo(mStartedState);
                        }
                        break;
                    case WifiScanner.CMD_STOP_PNO_SCAN:
                        removeSwPnoScanRequest(ci, msg.arg2);
                        transitionTo(mStartedState);
                        break;
                    case WifiScanner.CMD_FULL_SCAN_RESULT:
                        // Aggregate full scan results until we get the |CMD_SCAN_RESULT| message
                        mSwPnoFullScanResults.add((ScanResult) msg.obj);
                        break;
                    case WifiScanner.CMD_SCAN_RESULT:
                        ScanResult[] scanResults = mSwPnoFullScanResults.toArray(
                                new ScanResult[mSwPnoFullScanResults.size()]);
                        reportPnoNetworkFound(scanResults);
                        mSwPnoFullScanResults.clear();
                        break;
                    case WifiScanner.CMD_OP_FAILED:
                        sendPnoScanFailedToAllAndClear(
                                WifiScanner.REASON_UNSPECIFIED, "background scan failed");
                        transitionTo(mStartedState);
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

        private WifiNative.PnoSettings convertPnoSettingsToNative(PnoSettings pnoSettings) {
            WifiNative.PnoSettings nativePnoSetting = new WifiNative.PnoSettings();
            nativePnoSetting.min5GHzRssi = pnoSettings.min5GHzRssi;
            nativePnoSetting.min24GHzRssi = pnoSettings.min24GHzRssi;
            nativePnoSetting.initialScoreMax = pnoSettings.initialScoreMax;
            nativePnoSetting.currentConnectionBonus = pnoSettings.currentConnectionBonus;
            nativePnoSetting.sameNetworkBonus = pnoSettings.sameNetworkBonus;
            nativePnoSetting.secureBonus = pnoSettings.secureBonus;
            nativePnoSetting.band5GHzBonus = pnoSettings.band5GHzBonus;
            nativePnoSetting.isConnected = pnoSettings.isConnected;
            nativePnoSetting.networkList =
                    new WifiNative.PnoNetwork[pnoSettings.networkList.length];
            for (int i = 0; i < pnoSettings.networkList.length; i++) {
                nativePnoSetting.networkList[i] = new WifiNative.PnoNetwork();
                nativePnoSetting.networkList[i].ssid = pnoSettings.networkList[i].ssid;
                nativePnoSetting.networkList[i].networkId = pnoSettings.networkList[i].networkId;
                nativePnoSetting.networkList[i].priority = pnoSettings.networkList[i].priority;
                nativePnoSetting.networkList[i].flags = pnoSettings.networkList[i].flags;
                nativePnoSetting.networkList[i].auth_bit_field =
                        pnoSettings.networkList[i].authBitField;
            }
            return nativePnoSetting;
        }

        // Retrieve the only active scan settings.
        private ScanSettings getScanSettings() {
            for (Pair<PnoSettings, ScanSettings> settingsPair : mActivePnoScans.getAllSettings()) {
                return settingsPair.second;
            }
            return null;
        }

        private void removeInternalClient() {
            if (mInternalClientInfo != null) {
                mInternalClientInfo.cleanup();
                mInternalClientInfo = null;
            } else {
                Log.w(TAG, "No Internal client for PNO");
            }
        }

        private void addInternalClient(ClientInfo ci) {
            if (mInternalClientInfo == null) {
                mInternalClientInfo =
                        new InternalClientInfo(ci.getUid(), new Messenger(this.getHandler()));
                mInternalClientInfo.register();
            } else {
                Log.w(TAG, "Internal client for PNO already exists");
            }
        }

        private void addPnoScanRequest(ClientInfo ci, int handler, ScanSettings scanSettings,
                PnoSettings pnoSettings) {
            mActivePnoScans.addRequest(ci, handler, WifiStateMachine.WIFI_WORK_SOURCE,
                    Pair.create(pnoSettings, scanSettings));
            addInternalClient(ci);
        }

        private Pair<PnoSettings, ScanSettings> removePnoScanRequest(ClientInfo ci, int handler) {
            Pair<PnoSettings, ScanSettings> settings = mActivePnoScans.removeRequest(ci, handler);
            return settings;
        }

        private boolean addHwPnoScanRequest(ClientInfo ci, int handler, ScanSettings scanSettings,
                PnoSettings pnoSettings) {
            if (ci == null) {
                Log.d(TAG, "Failing scan request ClientInfo not found " + handler);
                return false;
            }
            if (!mActivePnoScans.isEmpty()) {
                loge("Failing scan request because there is already an active scan");
                return false;
            }
            WifiNative.PnoSettings nativePnoSettings = convertPnoSettingsToNative(pnoSettings);
            if (!mScannerImpl.setHwPnoList(nativePnoSettings, mPnoScanStateMachine)) {
                return false;
            }
            logScanRequest("addHwPnoScanRequest", ci, handler, null, scanSettings, pnoSettings);
            addPnoScanRequest(ci, handler, scanSettings, pnoSettings);
            // HW PNO is supported, check if we need a background scan running for this.
            if (mScannerImpl.shouldScheduleBackgroundScanForHwPno()) {
                addBackgroundScanRequest(scanSettings);
            }
            return true;
        }

        private void removeHwPnoScanRequest(ClientInfo ci, int handler) {
            if (ci != null) {
                Pair<PnoSettings, ScanSettings> settings = removePnoScanRequest(ci, handler);
                logScanRequest("removeHwPnoScanRequest", ci, handler, null,
                        settings.second, settings.first);
            }
        }

        private boolean addSwPnoScanRequest(ClientInfo ci, int handler, ScanSettings scanSettings,
                PnoSettings pnoSettings) {
            if (ci == null) {
                Log.d(TAG, "Failing scan request ClientInfo not found " + handler);
                return false;
            }
            if (!mActivePnoScans.isEmpty()) {
                loge("Failing scan request because there is already an active scan");
                return false;
            }
            logScanRequest("addSwPnoScanRequest", ci, handler, null, scanSettings, pnoSettings);
            addPnoScanRequest(ci, handler, scanSettings, pnoSettings);
            // HW PNO is not supported, we need to revert to normal background scans and
            // report events after each scan and we need full scan results to get the IE information
            scanSettings.reportEvents = WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN
                    | WifiScanner.REPORT_EVENT_FULL_SCAN_RESULT;
            addBackgroundScanRequest(scanSettings);
            return true;
        }

        private void removeSwPnoScanRequest(ClientInfo ci, int handler) {
            if (ci != null) {
                Pair<PnoSettings, ScanSettings> settings = removePnoScanRequest(ci, handler);
                logScanRequest("removeSwPnoScanRequest", ci, handler, null,
                        settings.second, settings.first);
            }
        }

        private void reportPnoNetworkFound(ScanResult[] results) {
            WifiScanner.ParcelableScanResults parcelableScanResults =
                    new WifiScanner.ParcelableScanResults(results);
            for (RequestInfo<Pair<PnoSettings, ScanSettings>> entry : mActivePnoScans) {
                ClientInfo ci = entry.clientInfo;
                int handler = entry.handlerId;
                logCallback("pnoNetworkFound", ci, handler, describeForLog(results));
                ci.reportEvent(
                        WifiScanner.CMD_PNO_NETWORK_FOUND, 0, handler, parcelableScanResults);
            }
        }

        private void sendPnoScanFailedToAllAndClear(int reason, String description) {
            for (RequestInfo<Pair<PnoSettings, ScanSettings>> entry : mActivePnoScans) {
                ClientInfo ci = entry.clientInfo;
                int handler = entry.handlerId;
                ci.reportEvent(WifiScanner.CMD_OP_FAILED, 0, handler,
                        new WifiScanner.OperationResult(reason, description));
            }
            mActivePnoScans.clear();
        }

        private void addBackgroundScanRequest(ScanSettings settings) {
            if (DBG) localLog("Starting background scan");
            if (mInternalClientInfo != null) {
                mInternalClientInfo.sendRequestToClientHandler(
                        WifiScanner.CMD_START_BACKGROUND_SCAN, settings,
                        WifiStateMachine.WIFI_WORK_SOURCE);
            }
        }

        private void addSingleScanRequest(ScanSettings settings) {
            if (DBG) localLog("Starting single scan");
            if (mInternalClientInfo != null) {
                mInternalClientInfo.sendRequestToClientHandler(
                        WifiScanner.CMD_START_SINGLE_SCAN, settings,
                        WifiStateMachine.WIFI_WORK_SOURCE);
            }
        }

        /**
         * Checks if IE are present in scan data, if no single scan is needed to report event to
         * client
         */
        private boolean isSingleScanNeeded(ScanResult[] scanResults) {
            for (ScanResult scanResult : scanResults) {
                if (scanResult.informationElements != null
                        && scanResult.informationElements.length > 0) {
                    return false;
                }
            }
            return true;
        }
    }

    private abstract class ClientInfo {
        private final int mUid;
        private final WorkSource mWorkSource;
        private boolean mScanWorkReported = false;
        protected final Messenger mMessenger;

        ClientInfo(int uid, Messenger messenger) {
            mUid = uid;
            mMessenger = messenger;
            mWorkSource = new WorkSource(uid);
        }

        /**
         * Register this client to main client map.
         */
        public void register() {
            mClients.put(mMessenger, this);
        }

        /**
         * Unregister this client from main client map.
         */
        private void unregister() {
            mClients.remove(mMessenger);
        }

        public void cleanup() {
            mSingleScanListeners.removeAllForClient(this);
            mSingleScanStateMachine.removeSingleScanRequests(this);
            mBackgroundScanStateMachine.removeBackgroundScanSettings(this);
            mBackgroundScanStateMachine.removeHotlistSettings(this);
            unregister();
            localLog("Successfully stopped all requests for client " + this);
        }

        public int getUid() {
            return mUid;
        }

        public void reportEvent(int what, int arg1, int arg2) {
            reportEvent(what, arg1, arg2, null);
        }

        // This has to be implemented by subclasses to report events back to clients.
        public abstract void reportEvent(int what, int arg1, int arg2, Object obj);

        // TODO(b/27903217): Blame scan on provided work source
        private void reportBatchedScanStart() {
            if (mUid == 0)
                return;

            int csph = getCsph();

            try {
                mBatteryStats.noteWifiBatchedScanStartedFromSource(mWorkSource, csph);
            } catch (RemoteException e) {
                logw("failed to report scan work: " + e.toString());
            }
        }

        private void reportBatchedScanStop() {
            if (mUid == 0)
                return;

            try {
                mBatteryStats.noteWifiBatchedScanStoppedFromSource(mWorkSource);
            } catch (RemoteException e) {
                logw("failed to cleanup scan work: " + e.toString());
            }
        }

        // TODO migrate batterystats to accept scan duration per hour instead of csph
        private int getCsph() {
            int totalScanDurationPerHour = 0;
            Collection<ScanSettings> settingsList =
                    mBackgroundScanStateMachine.getBackgroundScanSettings(this);
            for (ScanSettings settings : settingsList) {
                int scanDurationMs = mChannelHelper.estimateScanDuration(settings);
                int scans_per_Hour = settings.periodInMs == 0 ? 1 : (3600 * 1000) /
                        settings.periodInMs;
                totalScanDurationPerHour += scanDurationMs * scans_per_Hour;
            }

            return totalScanDurationPerHour / ChannelHelper.SCAN_PERIOD_PER_CHANNEL_MS;
        }

        public void reportScanWorkUpdate() {
            if (mScanWorkReported) {
                reportBatchedScanStop();
                mScanWorkReported = false;
            }
            if (mBackgroundScanStateMachine.getBackgroundScanSettings(this).isEmpty()) {
                reportBatchedScanStart();
                mScanWorkReported = true;
            }
        }

        @Override
        public String toString() {
            return "ClientInfo[uid=" + mUid + "," + mMessenger + "]";
        }
    }

    /**
     * This class is used to represent external clients to the WifiScanning Service.
     */
    private class ExternalClientInfo extends ClientInfo {
        private final AsyncChannel mChannel;
        /**
         * Indicates if the client is still connected
         * If the client is no longer connected then messages to it will be silently dropped
         */
        private boolean mDisconnected = false;

        ExternalClientInfo(int uid, Messenger messenger, AsyncChannel c) {
            super(uid, messenger);
            mChannel = c;
            if (DBG) localLog("New client, channel: " + c);
        }

        @Override
        public void reportEvent(int what, int arg1, int arg2, Object obj) {
            if (!mDisconnected) {
                mChannel.sendMessage(what, arg1, arg2, obj);
            }
        }

        @Override
        public void cleanup() {
            mDisconnected = true;
            // Internal clients should not have any wifi change requests. So, keeping this cleanup
            // only for external client because this will otherwise cause an infinite recursion
            // when the internal client in WifiChangeStateMachine is cleaned up.
            mWifiChangeStateMachine.removeWifiChangeHandler(this);
            mPnoScanStateMachine.removePnoSettings(this);
            super.cleanup();
        }
    }

    /**
     * This class is used to represent internal clients to the WifiScanning Service. This is needed
     * for communicating between State Machines.
     * This leaves the onReportEvent method unimplemented, so that the clients have the freedom
     * to handle the events as they need.
     */
    private class InternalClientInfo extends ClientInfo {
        private static final int INTERNAL_CLIENT_HANDLER = 0;

        /**
         * The UID here is used to proxy the original external requester UID.
         */
        InternalClientInfo(int requesterUid, Messenger messenger) {
            super(requesterUid, messenger);
        }

        @Override
        public void reportEvent(int what, int arg1, int arg2, Object obj) {
            Message message = Message.obtain();
            message.what = what;
            message.arg1 = arg1;
            message.arg2 = arg2;
            message.obj = obj;
            try {
                mMessenger.send(message);
            } catch (RemoteException e) {
                loge("Failed to send message: " + what);
            }
        }

        /**
         * Send a message to the client handler which should reroute the message to the appropriate
         * state machine.
         */
        public void sendRequestToClientHandler(int what, ScanSettings settings,
                WorkSource workSource) {
            Message msg = Message.obtain();
            msg.what = what;
            msg.arg2 = INTERNAL_CLIENT_HANDLER;
            if (settings != null) {
                Bundle bundle = new Bundle();
                bundle.putParcelable(WifiScanner.SCAN_PARAMS_SCAN_SETTINGS_KEY, settings);
                bundle.putParcelable(WifiScanner.SCAN_PARAMS_WORK_SOURCE_KEY, workSource);
                msg.obj = bundle;
            }
            msg.replyTo = mMessenger;
            msg.sendingUid = getUid();
            mClientHandler.sendMessage(msg);
        }

        /**
         * Send a message to the client handler which should reroute the message to the appropriate
         * state machine.
         */
        public void sendRequestToClientHandler(int what) {
            sendRequestToClientHandler(what, null, null);
        }

        @Override
        public String toString() {
            return "InternalClientInfo[]";
        }
    }

    void replySucceeded(Message msg) {
        if (msg.replyTo != null) {
            Message reply = Message.obtain();
            reply.what = WifiScanner.CMD_OP_SUCCEEDED;
            reply.arg2 = msg.arg2;
            try {
                msg.replyTo.send(reply);
            } catch (RemoteException e) {
                // There's not much we can do if reply can't be sent!
            }
        } else {
            // locally generated message; doesn't need a reply!
        }
    }

    void replyFailed(Message msg, int reason, String description) {
        if (msg.replyTo != null) {
            Message reply = Message.obtain();
            reply.what = WifiScanner.CMD_OP_FAILED;
            reply.arg2 = msg.arg2;
            reply.obj = new WifiScanner.OperationResult(reason, description);
            try {
                msg.replyTo.send(reply);
            } catch (RemoteException e) {
                // There's not much we can do if reply can't be sent!
            }
        } else {
            // locally generated message; doesn't need a reply!
        }
    }

    /**
     * Wifi Change state machine is used to handle any wifi change tracking requests.
     * TODO: This state machine doesn't handle driver loading/unloading yet.
     */
    class WifiChangeStateMachine extends StateMachine
            implements WifiNative.SignificantWifiChangeEventHandler {

        private static final int MAX_APS_TO_TRACK = 3;
        private static final int MOVING_SCAN_PERIOD_MS      = 10000;
        private static final int STATIONARY_SCAN_PERIOD_MS  =  5000;
        private static final int MOVING_STATE_TIMEOUT_MS    = 30000;

        State mDefaultState = new DefaultState();
        State mStationaryState = new StationaryState();
        State mMovingState = new MovingState();

        private static final String ACTION_TIMEOUT =
                "com.android.server.WifiScanningServiceImpl.action.TIMEOUT";
        private PendingIntent mTimeoutIntent;
        private ScanResult[] mCurrentBssids;
        private InternalClientInfo mInternalClientInfo;

        private final Set<Pair<ClientInfo, Integer>> mActiveWifiChangeHandlers = new HashSet<>();

        WifiChangeStateMachine(Looper looper) {
            super("SignificantChangeStateMachine", looper);

            // CHECKSTYLE:OFF IndentationCheck
            addState(mDefaultState);
                addState(mStationaryState, mDefaultState);
                addState(mMovingState, mDefaultState);
            // CHECKSTYLE:ON IndentationCheck

            setInitialState(mDefaultState);
        }

        public void removeWifiChangeHandler(ClientInfo ci) {
            Iterator<Pair<ClientInfo, Integer>> iter = mActiveWifiChangeHandlers.iterator();
            while (iter.hasNext()) {
                Pair<ClientInfo, Integer> entry = iter.next();
                if (entry.first == ci) {
                    iter.remove();
                }
            }
            untrackSignificantWifiChangeOnEmpty();
        }

        class DefaultState extends State {
            @Override
            public void enter() {
                if (DBG) localLog("Entering IdleState");
            }

            @Override
            public boolean processMessage(Message msg) {
                if (DBG) localLog("DefaultState state got " + msg);
                ClientInfo ci = mClients.get(msg.replyTo);
                switch (msg.what) {
                    case WifiScanner.CMD_START_TRACKING_CHANGE:
                        if (addWifiChangeHandler(ci, msg.arg2)) {
                            replySucceeded(msg);
                            transitionTo(mMovingState);
                        } else {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "bad request");
                        }
                        break;
                    case WifiScanner.CMD_STOP_TRACKING_CHANGE:
                        // nothing to do
                        break;
                    case WifiScanner.CMD_CONFIGURE_WIFI_CHANGE:
                        /* save configuration till we transition to moving state */
                        deferMessage(msg);
                        break;
                    case WifiScanner.CMD_SCAN_RESULT:
                        // nothing to do
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

        class StationaryState extends State {
            @Override
            public void enter() {
                if (DBG) localLog("Entering StationaryState");
                reportWifiStabilized(mCurrentBssids);
            }

            @Override
            public boolean processMessage(Message msg) {
                if (DBG) localLog("Stationary state got " + msg);
                ClientInfo ci = mClients.get(msg.replyTo);
                switch (msg.what) {
                    case WifiScanner.CMD_START_TRACKING_CHANGE:
                        if (addWifiChangeHandler(ci, msg.arg2)) {
                            replySucceeded(msg);
                        } else {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "bad request");
                        }
                        break;
                    case WifiScanner.CMD_STOP_TRACKING_CHANGE:
                        removeWifiChangeHandler(ci, msg.arg2);
                        break;
                    case WifiScanner.CMD_CONFIGURE_WIFI_CHANGE:
                        /* save configuration till we transition to moving state */
                        deferMessage(msg);
                        break;
                    case CMD_WIFI_CHANGE_DETECTED:
                        if (DBG) localLog("Got wifi change detected");
                        reportWifiChanged((ScanResult[]) msg.obj);
                        transitionTo(mMovingState);
                        break;
                    case WifiScanner.CMD_SCAN_RESULT:
                        // nothing to do
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

        class MovingState extends State {
            boolean mWifiChangeDetected = false;
            boolean mScanResultsPending = false;

            @Override
            public void enter() {
                if (DBG) localLog("Entering MovingState");
                if (mTimeoutIntent == null) {
                    Intent intent = new Intent(ACTION_TIMEOUT, null);
                    mTimeoutIntent = PendingIntent.getBroadcast(mContext, 0, intent, 0);

                    mContext.registerReceiver(
                            new BroadcastReceiver() {
                                @Override
                                public void onReceive(Context context, Intent intent) {
                                    sendMessage(CMD_WIFI_CHANGE_TIMEOUT);
                                }
                            }, new IntentFilter(ACTION_TIMEOUT));
                }
                issueFullScan();
            }

            @Override
            public boolean processMessage(Message msg) {
                if (DBG) localLog("MovingState state got " + msg);
                ClientInfo ci = mClients.get(msg.replyTo);
                switch (msg.what) {
                    case WifiScanner.CMD_START_TRACKING_CHANGE:
                        if (addWifiChangeHandler(ci, msg.arg2)) {
                            replySucceeded(msg);
                        } else {
                            replyFailed(msg, WifiScanner.REASON_INVALID_REQUEST, "bad request");
                        }
                        break;
                    case WifiScanner.CMD_STOP_TRACKING_CHANGE:
                        removeWifiChangeHandler(ci, msg.arg2);
                        break;
                    case WifiScanner.CMD_CONFIGURE_WIFI_CHANGE:
                        if (DBG) localLog("Got configuration from app");
                        WifiScanner.WifiChangeSettings settings =
                                (WifiScanner.WifiChangeSettings) msg.obj;
                        reconfigureScan(settings);
                        mWifiChangeDetected = false;
                        long unchangedDelay = settings.unchangedSampleSize * settings.periodInMs;
                        mAlarmManager.cancel(mTimeoutIntent);
                        mAlarmManager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                                mClock.elapsedRealtime() + unchangedDelay,
                                mTimeoutIntent);
                        break;
                    case WifiScanner.CMD_SCAN_RESULT:
                        if (DBG) localLog("Got scan results");
                        if (mScanResultsPending) {
                            if (DBG) localLog("reconfiguring scan");
                            reconfigureScan((ScanData[])msg.obj,
                                    STATIONARY_SCAN_PERIOD_MS);
                            mWifiChangeDetected = false;
                            mAlarmManager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                                    mClock.elapsedRealtime() + MOVING_STATE_TIMEOUT_MS,
                                    mTimeoutIntent);
                            mScanResultsPending = false;
                        }
                        break;
                    case CMD_WIFI_CHANGE_DETECTED:
                        if (DBG) localLog("Change detected");
                        mAlarmManager.cancel(mTimeoutIntent);
                        reportWifiChanged((ScanResult[])msg.obj);
                        mWifiChangeDetected = true;
                        issueFullScan();
                        break;
                    case CMD_WIFI_CHANGE_TIMEOUT:
                        if (DBG) localLog("Got timeout event");
                        if (mWifiChangeDetected == false) {
                            transitionTo(mStationaryState);
                        }
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }

            @Override
            public void exit() {
                mAlarmManager.cancel(mTimeoutIntent);
            }

            void issueFullScan() {
                if (DBG) localLog("Issuing full scan");
                ScanSettings settings = new ScanSettings();
                settings.band = WifiScanner.WIFI_BAND_BOTH;
                settings.periodInMs = MOVING_SCAN_PERIOD_MS;
                settings.reportEvents = WifiScanner.REPORT_EVENT_AFTER_EACH_SCAN;
                addScanRequest(settings);
                mScanResultsPending = true;
            }

        }

        private void reconfigureScan(ScanData[] results, int period) {
            // find brightest APs and set them as sentinels
            if (results.length < MAX_APS_TO_TRACK) {
                localLog("too few APs (" + results.length + ") available to track wifi change");
                return;
            }

            removeScanRequest();

            // remove duplicate BSSIDs
            HashMap<String, ScanResult> bssidToScanResult = new HashMap<String, ScanResult>();
            for (ScanResult result : results[0].getResults()) {
                ScanResult saved = bssidToScanResult.get(result.BSSID);
                if (saved == null) {
                    bssidToScanResult.put(result.BSSID, result);
                } else if (saved.level > result.level) {
                    bssidToScanResult.put(result.BSSID, result);
                }
            }

            // find brightest BSSIDs
            ScanResult brightest[] = new ScanResult[MAX_APS_TO_TRACK];
            Collection<ScanResult> results2 = bssidToScanResult.values();
            for (ScanResult result : results2) {
                for (int j = 0; j < brightest.length; j++) {
                    if (brightest[j] == null
                            || (brightest[j].level < result.level)) {
                        for (int k = brightest.length; k > (j + 1); k--) {
                            brightest[k - 1] = brightest[k - 2];
                        }
                        brightest[j] = result;
                        break;
                    }
                }
            }

            // Get channels to scan for
            ArrayList<Integer> channels = new ArrayList<Integer>();
            for (int i = 0; i < brightest.length; i++) {
                boolean found = false;
                for (int j = i + 1; j < brightest.length; j++) {
                    if (brightest[j].frequency == brightest[i].frequency) {
                        found = true;
                    }
                }
                if (!found) {
                    channels.add(brightest[i].frequency);
                }
            }

            if (DBG) localLog("Found " + channels.size() + " channels");

            // set scanning schedule
            ScanSettings settings = new ScanSettings();
            settings.band = WifiScanner.WIFI_BAND_UNSPECIFIED;
            settings.channels = new ChannelSpec[channels.size()];
            for (int i = 0; i < channels.size(); i++) {
                settings.channels[i] = new ChannelSpec(channels.get(i));
            }

            settings.periodInMs = period;
            addScanRequest(settings);

            WifiScanner.WifiChangeSettings settings2 = new WifiScanner.WifiChangeSettings();
            settings2.rssiSampleSize = 3;
            settings2.lostApSampleSize = 3;
            settings2.unchangedSampleSize = 3;
            settings2.minApsBreachingThreshold = 2;
            settings2.bssidInfos = new BssidInfo[brightest.length];

            for (int i = 0; i < brightest.length; i++) {
                BssidInfo BssidInfo = new BssidInfo();
                BssidInfo.bssid = brightest[i].BSSID;
                int threshold = (100 + brightest[i].level) / 32 + 2;
                BssidInfo.low = brightest[i].level - threshold;
                BssidInfo.high = brightest[i].level + threshold;
                settings2.bssidInfos[i] = BssidInfo;

                if (DBG) localLog("Setting bssid=" + BssidInfo.bssid + ", " +
                        "low=" + BssidInfo.low + ", high=" + BssidInfo.high);
            }

            trackSignificantWifiChange(settings2);
            mCurrentBssids = brightest;
        }

        private void reconfigureScan(WifiScanner.WifiChangeSettings settings) {

            if (settings.bssidInfos.length < MAX_APS_TO_TRACK) {
                localLog("too few APs (" + settings.bssidInfos.length
                        + ") available to track wifi change");
                return;
            }

            if (DBG) localLog("Setting configuration specified by app");

            mCurrentBssids = new ScanResult[settings.bssidInfos.length];
            HashSet<Integer> channels = new HashSet<Integer>();

            for (int i = 0; i < settings.bssidInfos.length; i++) {
                ScanResult result = new ScanResult();
                result.BSSID = settings.bssidInfos[i].bssid;
                mCurrentBssids[i] = result;
                channels.add(settings.bssidInfos[i].frequencyHint);
            }

            // cancel previous scan
            removeScanRequest();

            // set new scanning schedule
            ScanSettings settings2 = new ScanSettings();
            settings2.band = WifiScanner.WIFI_BAND_UNSPECIFIED;
            settings2.channels = new ChannelSpec[channels.size()];
            int i = 0;
            for (Integer channel : channels) {
                settings2.channels[i++] = new ChannelSpec(channel);
            }

            settings2.periodInMs = settings.periodInMs;
            addScanRequest(settings2);

            // start tracking new APs
            trackSignificantWifiChange(settings);
        }


        @Override
        public void onChangesFound(ScanResult results[]) {
            sendMessage(CMD_WIFI_CHANGE_DETECTED, 0, 0, results);
        }

        private void addScanRequest(ScanSettings settings) {
            if (DBG) localLog("Starting scans");
            if (mInternalClientInfo != null) {
                mInternalClientInfo.sendRequestToClientHandler(
                        WifiScanner.CMD_START_BACKGROUND_SCAN, settings, null);
            }
        }

        private void removeScanRequest() {
            if (DBG) localLog("Stopping scans");
            if (mInternalClientInfo != null) {
                mInternalClientInfo.sendRequestToClientHandler(
                        WifiScanner.CMD_STOP_BACKGROUND_SCAN);
            }
        }

        private void trackSignificantWifiChange(WifiScanner.WifiChangeSettings settings) {
            if (mScannerImpl != null) {
                mScannerImpl.untrackSignificantWifiChange();
                mScannerImpl.trackSignificantWifiChange(settings, this);
            }
        }

        private void untrackSignificantWifiChange() {
            if (mScannerImpl != null) {
                mScannerImpl.untrackSignificantWifiChange();
            }
        }

        private boolean addWifiChangeHandler(ClientInfo ci, int handler) {
            if (ci == null) {
                Log.d(TAG, "Failing wifi change request ClientInfo not found " + handler);
                return false;
            }
            mActiveWifiChangeHandlers.add(Pair.create(ci, handler));
            // Add an internal client to make background scan requests.
            if (mInternalClientInfo == null) {
                mInternalClientInfo =
                        new InternalClientInfo(ci.getUid(), new Messenger(this.getHandler()));
                mInternalClientInfo.register();
            }
            return true;
        }

        private void removeWifiChangeHandler(ClientInfo ci, int handler) {
            if (ci != null) {
                mActiveWifiChangeHandlers.remove(Pair.create(ci, handler));
                untrackSignificantWifiChangeOnEmpty();
            }
        }

        private void untrackSignificantWifiChangeOnEmpty() {
            if (mActiveWifiChangeHandlers.isEmpty()) {
                if (DBG) localLog("Got Disable Wifi Change");
                mCurrentBssids = null;
                untrackSignificantWifiChange();
                // Remove the internal client when there are no more external clients.
                if (mInternalClientInfo != null) {
                    mInternalClientInfo.cleanup();
                    mInternalClientInfo = null;
                }
                transitionTo(mDefaultState);
            }
        }

        private void reportWifiChanged(ScanResult[] results) {
            WifiScanner.ParcelableScanResults parcelableScanResults =
                    new WifiScanner.ParcelableScanResults(results);
            Iterator<Pair<ClientInfo, Integer>> it = mActiveWifiChangeHandlers.iterator();
            while (it.hasNext()) {
                Pair<ClientInfo, Integer> entry = it.next();
                ClientInfo ci = entry.first;
                int handler = entry.second;
                ci.reportEvent(WifiScanner.CMD_WIFI_CHANGE_DETECTED, 0, handler,
                        parcelableScanResults);
            }
        }

        private void reportWifiStabilized(ScanResult[] results) {
            WifiScanner.ParcelableScanResults parcelableScanResults =
                    new WifiScanner.ParcelableScanResults(results);
            Iterator<Pair<ClientInfo, Integer>> it = mActiveWifiChangeHandlers.iterator();
            while (it.hasNext()) {
                Pair<ClientInfo, Integer> entry = it.next();
                ClientInfo ci = entry.first;
                int handler = entry.second;
                ci.reportEvent(WifiScanner.CMD_WIFI_CHANGES_STABILIZED, 0, handler,
                        parcelableScanResults);
            }
        }
    }

    private static String toString(int uid, ScanSettings settings) {
        StringBuilder sb = new StringBuilder();
        sb.append("ScanSettings[uid=").append(uid);
        sb.append(", period=").append(settings.periodInMs);
        sb.append(", report=").append(settings.reportEvents);
        if (settings.reportEvents == WifiScanner.REPORT_EVENT_AFTER_BUFFER_FULL
                && settings.numBssidsPerScan > 0
                && settings.maxScansToCache > 1) {
            sb.append(", batch=").append(settings.maxScansToCache);
            sb.append(", numAP=").append(settings.numBssidsPerScan);
        }
        sb.append(", ").append(ChannelHelper.toString(settings));
        sb.append("]");

        return sb.toString();
    }

    @Override
    protected void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        if (mContext.checkCallingOrSelfPermission(android.Manifest.permission.DUMP)
                != PackageManager.PERMISSION_GRANTED) {
            pw.println("Permission Denial: can't dump WifiScanner from from pid="
                    + Binder.getCallingPid()
                    + ", uid=" + Binder.getCallingUid()
                    + " without permission "
                    + android.Manifest.permission.DUMP);
            return;
        }
        pw.println("WifiScanningService - Log Begin ----");
        mLocalLog.dump(fd, pw, args);
        pw.println("WifiScanningService - Log End ----");
        pw.println();
        pw.println("clients:");
        for (ClientInfo client : mClients.values()) {
            pw.println("  " + client);
        }
        pw.println("listeners:");
        for (ClientInfo client : mClients.values()) {
            Collection<ScanSettings> settingsList =
                    mBackgroundScanStateMachine.getBackgroundScanSettings(client);
            for (ScanSettings settings : settingsList) {
                pw.println("  " + toString(client.mUid, settings));
            }
        }
        if (mBackgroundScheduler != null) {
            WifiNative.ScanSettings schedule = mBackgroundScheduler.getSchedule();
            if (schedule != null) {
                pw.println("schedule:");
                pw.println("  base period: " + schedule.base_period_ms);
                pw.println("  max ap per scan: " + schedule.max_ap_per_scan);
                pw.println("  batched scans: " + schedule.report_threshold_num_scans);
                pw.println("  buckets:");
                for (int b = 0; b < schedule.num_buckets; b++) {
                    WifiNative.BucketSettings bucket = schedule.buckets[b];
                    pw.println("    bucket " + bucket.bucket + " (" + bucket.period_ms + "ms)["
                            + bucket.report_events + "]: "
                            + ChannelHelper.toString(bucket));
                }
            }
        }
        if (mPnoScanStateMachine != null) {
            mPnoScanStateMachine.dump(fd, pw, args);
        }
    }

    void logScanRequest(String request, ClientInfo ci, int id, WorkSource workSource,
            ScanSettings settings, PnoSettings pnoSettings) {
        StringBuilder sb = new StringBuilder();
        sb.append(request)
                .append(": ")
                .append((ci == null) ? "ClientInfo[unknown]" : ci.toString())
                .append(",Id=")
                .append(id);
        if (workSource != null) {
            sb.append(",").append(workSource);
        }
        if (settings != null) {
            sb.append(", ");
            describeTo(sb, settings);
        }
        if (pnoSettings != null) {
            sb.append(", ");
            describeTo(sb, pnoSettings);
        }
        localLog(sb.toString());
    }

    void logCallback(String callback, ClientInfo ci, int id, String extra) {
        StringBuilder sb = new StringBuilder();
        sb.append(callback)
                .append(": ")
                .append((ci == null) ? "ClientInfo[unknown]" : ci.toString())
                .append(",Id=")
                .append(id);
        if (extra != null) {
            sb.append(",").append(extra);
        }
        localLog(sb.toString());
    }

    static String describeForLog(ScanData[] results) {
        StringBuilder sb = new StringBuilder();
        sb.append("results=");
        for (int i = 0; i < results.length; ++i) {
            if (i > 0) sb.append(";");
            sb.append(results[i].getResults().length);
        }
        return sb.toString();
    }

    static String describeForLog(ScanResult[] results) {
        return "results=" + results.length;
    }

    static String describeTo(StringBuilder sb, ScanSettings scanSettings) {
        sb.append("ScanSettings { ")
          .append(" band:").append(scanSettings.band)
          .append(" period:").append(scanSettings.periodInMs)
          .append(" reportEvents:").append(scanSettings.reportEvents)
          .append(" numBssidsPerScan:").append(scanSettings.numBssidsPerScan)
          .append(" maxScansToCache:").append(scanSettings.maxScansToCache)
          .append(" channels:[ ");
        if (scanSettings.channels != null) {
            for (int i = 0; i < scanSettings.channels.length; i++) {
                sb.append(scanSettings.channels[i].frequency)
                  .append(" ");
            }
        }
        sb.append(" ] ")
          .append(" } ");
        return sb.toString();
    }

    static String describeTo(StringBuilder sb, PnoSettings pnoSettings) {
        sb.append("PnoSettings { ")
          .append(" min5GhzRssi:").append(pnoSettings.min5GHzRssi)
          .append(" min24GhzRssi:").append(pnoSettings.min24GHzRssi)
          .append(" initialScoreMax:").append(pnoSettings.initialScoreMax)
          .append(" currentConnectionBonus:").append(pnoSettings.currentConnectionBonus)
          .append(" sameNetworkBonus:").append(pnoSettings.sameNetworkBonus)
          .append(" secureBonus:").append(pnoSettings.secureBonus)
          .append(" band5GhzBonus:").append(pnoSettings.band5GHzBonus)
          .append(" isConnected:").append(pnoSettings.isConnected)
          .append(" networks:[ ");
        if (pnoSettings.networkList != null) {
            for (int i = 0; i < pnoSettings.networkList.length; i++) {
                sb.append(pnoSettings.networkList[i].ssid)
                  .append(",")
                  .append(pnoSettings.networkList[i].networkId)
                  .append(" ");
            }
        }
        sb.append(" ] ")
          .append(" } ");
        return sb.toString();
    }
}
