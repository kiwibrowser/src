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

import android.content.Context;
import android.content.Intent;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.BatteryStats;
import android.os.Handler;
import android.os.Message;
import android.os.Parcelable;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.util.Log;
import android.util.Slog;

import com.android.internal.app.IBatteryStats;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;

import java.io.FileDescriptor;
import java.io.PrintWriter;

/**
 * Tracks the state changes in supplicant and provides functionality
 * that is based on these state changes:
 * - detect a failed WPA handshake that loops indefinitely
 * - authentication failure handling
 */
public class SupplicantStateTracker extends StateMachine {

    private static final String TAG = "SupplicantStateTracker";
    private static boolean DBG = false;
    private final WifiConfigManager mWifiConfigManager;
    private final IBatteryStats mBatteryStats;
    /* Indicates authentication failure in supplicant broadcast.
     * TODO: enhance auth failure reporting to include notification
     * for all type of failures: EAP, WPS & WPA networks */
    private boolean mAuthFailureInSupplicantBroadcast = false;

    /* Maximum retries on a authentication failure notification */
    private static final int MAX_RETRIES_ON_AUTHENTICATION_FAILURE = 2;

    /* Maximum retries on assoc rejection events */
    private static final int MAX_RETRIES_ON_ASSOCIATION_REJECT = 16;

    /* Tracks if networks have been disabled during a connection */
    private boolean mNetworksDisabledDuringConnect = false;

    private final Context mContext;

    private final State mUninitializedState = new UninitializedState();
    private final State mDefaultState = new DefaultState();
    private final State mInactiveState = new InactiveState();
    private final State mDisconnectState = new DisconnectedState();
    private final State mScanState = new ScanState();
    private final State mHandshakeState = new HandshakeState();
    private final State mCompletedState = new CompletedState();
    private final State mDormantState = new DormantState();

    void enableVerboseLogging(int verbose) {
        if (verbose > 0) {
            DBG = true;
        } else {
            DBG = false;
        }
    }

    public String getSupplicantStateName() {
        return getCurrentState().getName();
    }

    public SupplicantStateTracker(Context c, WifiConfigManager wcs, Handler t) {
        super(TAG, t.getLooper());

        mContext = c;
        mWifiConfigManager = wcs;
        mBatteryStats = (IBatteryStats)ServiceManager.getService(BatteryStats.SERVICE_NAME);
        addState(mDefaultState);
            addState(mUninitializedState, mDefaultState);
            addState(mInactiveState, mDefaultState);
            addState(mDisconnectState, mDefaultState);
            addState(mScanState, mDefaultState);
            addState(mHandshakeState, mDefaultState);
            addState(mCompletedState, mDefaultState);
            addState(mDormantState, mDefaultState);

        setInitialState(mUninitializedState);
        setLogRecSize(50);
        setLogOnlyTransitions(true);
        //start the state machine
        start();
    }

    private void handleNetworkConnectionFailure(int netId, int disableReason) {
        if (DBG) {
            Log.d(TAG, "handleNetworkConnectionFailure netId=" + Integer.toString(netId)
                    + " reason " + Integer.toString(disableReason)
                    + " mNetworksDisabledDuringConnect=" + mNetworksDisabledDuringConnect);
        }

        /* If other networks disabled during connection, enable them */
        if (mNetworksDisabledDuringConnect) {
            mWifiConfigManager.enableAllNetworks();
            mNetworksDisabledDuringConnect = false;
        }
        /* update network status */
        mWifiConfigManager.updateNetworkSelectionStatus(netId, disableReason);
    }

    private void transitionOnSupplicantStateChange(StateChangeResult stateChangeResult) {
        SupplicantState supState = (SupplicantState) stateChangeResult.state;

        if (DBG) Log.d(TAG, "Supplicant state: " + supState.toString() + "\n");

        switch (supState) {
           case DISCONNECTED:
                transitionTo(mDisconnectState);
                break;
            case INTERFACE_DISABLED:
                //we should have received a disconnection already, do nothing
                break;
            case SCANNING:
                transitionTo(mScanState);
                break;
            case AUTHENTICATING:
            case ASSOCIATING:
            case ASSOCIATED:
            case FOUR_WAY_HANDSHAKE:
            case GROUP_HANDSHAKE:
                transitionTo(mHandshakeState);
                break;
            case COMPLETED:
                transitionTo(mCompletedState);
                break;
            case DORMANT:
                transitionTo(mDormantState);
                break;
            case INACTIVE:
                transitionTo(mInactiveState);
                break;
            case UNINITIALIZED:
            case INVALID:
                transitionTo(mUninitializedState);
                break;
            default:
                Log.e(TAG, "Unknown supplicant state " + supState);
                break;
        }
    }

    private void sendSupplicantStateChangedBroadcast(SupplicantState state, boolean failedAuth) {
        int supplState;
        switch (state) {
            case DISCONNECTED: supplState = BatteryStats.WIFI_SUPPL_STATE_DISCONNECTED; break;
            case INTERFACE_DISABLED:
                supplState = BatteryStats.WIFI_SUPPL_STATE_INTERFACE_DISABLED; break;
            case INACTIVE: supplState = BatteryStats.WIFI_SUPPL_STATE_INACTIVE; break;
            case SCANNING: supplState = BatteryStats.WIFI_SUPPL_STATE_SCANNING; break;
            case AUTHENTICATING: supplState = BatteryStats.WIFI_SUPPL_STATE_AUTHENTICATING; break;
            case ASSOCIATING: supplState = BatteryStats.WIFI_SUPPL_STATE_ASSOCIATING; break;
            case ASSOCIATED: supplState = BatteryStats.WIFI_SUPPL_STATE_ASSOCIATED; break;
            case FOUR_WAY_HANDSHAKE:
                supplState = BatteryStats.WIFI_SUPPL_STATE_FOUR_WAY_HANDSHAKE; break;
            case GROUP_HANDSHAKE: supplState = BatteryStats.WIFI_SUPPL_STATE_GROUP_HANDSHAKE; break;
            case COMPLETED: supplState = BatteryStats.WIFI_SUPPL_STATE_COMPLETED; break;
            case DORMANT: supplState = BatteryStats.WIFI_SUPPL_STATE_DORMANT; break;
            case UNINITIALIZED: supplState = BatteryStats.WIFI_SUPPL_STATE_UNINITIALIZED; break;
            case INVALID: supplState = BatteryStats.WIFI_SUPPL_STATE_INVALID; break;
            default:
                Slog.w(TAG, "Unknown supplicant state " + state);
                supplState = BatteryStats.WIFI_SUPPL_STATE_INVALID;
                break;
        }
        try {
            mBatteryStats.noteWifiSupplicantStateChanged(supplState, failedAuth);
        } catch (RemoteException e) {
            // Won't happen.
        }
        Intent intent = new Intent(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT
                | Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intent.putExtra(WifiManager.EXTRA_NEW_STATE, (Parcelable) state);
        if (failedAuth) {
            intent.putExtra(
                WifiManager.EXTRA_SUPPLICANT_ERROR,
                WifiManager.ERROR_AUTHENTICATING);
        }
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    /********************************************************
     * HSM states
     *******************************************************/

    class DefaultState extends State {
        @Override
         public void enter() {
             if (DBG) Log.d(TAG, getName() + "\n");
         }
        @Override
        public boolean processMessage(Message message) {
            if (DBG) Log.d(TAG, getName() + message.toString() + "\n");
            switch (message.what) {
                case WifiMonitor.AUTHENTICATION_FAILURE_EVENT:
                    mAuthFailureInSupplicantBroadcast = true;
                    break;
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
                    SupplicantState state = stateChangeResult.state;
                    sendSupplicantStateChangedBroadcast(state, mAuthFailureInSupplicantBroadcast);
                    mAuthFailureInSupplicantBroadcast = false;
                    transitionOnSupplicantStateChange(stateChangeResult);
                    break;
                case WifiStateMachine.CMD_RESET_SUPPLICANT_STATE:
                    transitionTo(mUninitializedState);
                    break;
                case WifiManager.CONNECT_NETWORK:
                    mNetworksDisabledDuringConnect = true;
                    break;
                case WifiMonitor.ASSOCIATION_REJECTION_EVENT:
                default:
                    Log.e(TAG, "Ignoring " + message);
                    break;
            }
            return HANDLED;
        }
    }

    /*
     * This indicates that the supplicant state as seen
     * by the framework is not initialized yet. We are
     * in this state right after establishing a control
     * channel connection before any supplicant events
     * or after we have lost the control channel
     * connection to the supplicant
     */
    class UninitializedState extends State {
        @Override
         public void enter() {
             if (DBG) Log.d(TAG, getName() + "\n");
         }
    }

    class InactiveState extends State {
        @Override
         public void enter() {
             if (DBG) Log.d(TAG, getName() + "\n");
         }
    }

    class DisconnectedState extends State {
        @Override
         public void enter() {
             if (DBG) Log.d(TAG, getName() + "\n");
         }
    }

    class ScanState extends State {
        @Override
         public void enter() {
             if (DBG) Log.d(TAG, getName() + "\n");
         }
    }

    class HandshakeState extends State {
        /**
         * The max number of the WPA supplicant loop iterations before we
         * decide that the loop should be terminated:
         */
        private static final int MAX_SUPPLICANT_LOOP_ITERATIONS = 4;
        private int mLoopDetectIndex;
        private int mLoopDetectCount;

        @Override
         public void enter() {
             if (DBG) Log.d(TAG, getName() + "\n");
             mLoopDetectIndex = 0;
             mLoopDetectCount = 0;
         }
        @Override
        public boolean processMessage(Message message) {
            if (DBG) Log.d(TAG, getName() + message.toString() + "\n");
            switch (message.what) {
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
                    SupplicantState state = stateChangeResult.state;
                    if (SupplicantState.isHandshakeState(state)) {
                        if (mLoopDetectIndex > state.ordinal()) {
                            mLoopDetectCount++;
                        }
                        if (mLoopDetectCount > MAX_SUPPLICANT_LOOP_ITERATIONS) {
                            Log.d(TAG, "Supplicant loop detected, disabling network " +
                                    stateChangeResult.networkId);
                            handleNetworkConnectionFailure(stateChangeResult.networkId,
                                    WifiConfiguration.NetworkSelectionStatus
                                            .DISABLED_AUTHENTICATION_FAILURE);
                        }
                        mLoopDetectIndex = state.ordinal();
                        sendSupplicantStateChangedBroadcast(state,
                                mAuthFailureInSupplicantBroadcast);
                    } else {
                        //Have the DefaultState handle the transition
                        return NOT_HANDLED;
                    }
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    class CompletedState extends State {
        @Override
         public void enter() {
             if (DBG) Log.d(TAG, getName() + "\n");
             /* Reset authentication failure count */
             if (mNetworksDisabledDuringConnect) {
                 mWifiConfigManager.enableAllNetworks();
                 mNetworksDisabledDuringConnect = false;
             }
        }
        @Override
        public boolean processMessage(Message message) {
            if (DBG) Log.d(TAG, getName() + message.toString() + "\n");
            switch(message.what) {
                case WifiMonitor.SUPPLICANT_STATE_CHANGE_EVENT:
                    StateChangeResult stateChangeResult = (StateChangeResult) message.obj;
                    SupplicantState state = stateChangeResult.state;
                    sendSupplicantStateChangedBroadcast(state, mAuthFailureInSupplicantBroadcast);
                    /* Ignore any connecting state in completed state. Group re-keying
                     * events and other auth events that do not affect connectivity are
                     * ignored
                     */
                    if (SupplicantState.isConnecting(state)) {
                        break;
                    }
                    transitionOnSupplicantStateChange(stateChangeResult);
                    break;
                case WifiStateMachine.CMD_RESET_SUPPLICANT_STATE:
                    sendSupplicantStateChangedBroadcast(SupplicantState.DISCONNECTED, false);
                    transitionTo(mUninitializedState);
                    break;
                default:
                    return NOT_HANDLED;
            }
            return HANDLED;
        }
    }

    //TODO: remove after getting rid of the state in supplicant
    class DormantState extends State {
        @Override
        public void enter() {
            if (DBG) Log.d(TAG, getName() + "\n");
        }
    }

    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        super.dump(fd, pw, args);
        pw.println("mAuthFailureInSupplicantBroadcast " + mAuthFailureInSupplicantBroadcast);
        pw.println("mNetworksDisabledDuringConnect " + mNetworksDisabledDuringConnect);
        pw.println();
    }
}
