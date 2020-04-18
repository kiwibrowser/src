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

import static com.android.server.wifi.util.ApConfigUtil.ERROR_GENERIC;
import static com.android.server.wifi.util.ApConfigUtil.ERROR_NO_CHANNEL;
import static com.android.server.wifi.util.ApConfigUtil.SUCCESS;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.INetworkManagementService;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.server.wifi.util.ApConfigUtil;

import java.util.ArrayList;
import java.util.Locale;

/**
 * Manage WiFi in AP mode.
 * The internal state machine runs under "WifiStateMachine" thread context.
 */
public class SoftApManager {
    private static final String TAG = "SoftApManager";

    private final INetworkManagementService mNmService;
    private final WifiNative mWifiNative;
    private final ArrayList<Integer> mAllowed2GChannels;

    private final String mCountryCode;

    private final String mInterfaceName;

    private final SoftApStateMachine mStateMachine;

    private final Listener mListener;

    /**
     * Listener for soft AP state changes.
     */
    public interface Listener {
        /**
         * Invoke when AP state changed.
         * @param state new AP state
         * @param failureReason reason when in failed state
         */
        void onStateChanged(int state, int failureReason);
    }

    public SoftApManager(Looper looper,
                         WifiNative wifiNative,
                         INetworkManagementService nmService,
                         String countryCode,
                         ArrayList<Integer> allowed2GChannels,
                         Listener listener) {
        mStateMachine = new SoftApStateMachine(looper);

        mNmService = nmService;
        mWifiNative = wifiNative;
        mCountryCode = countryCode;
        mAllowed2GChannels = allowed2GChannels;
        mListener = listener;

        mInterfaceName = mWifiNative.getInterfaceName();
    }

    /**
     * Start soft AP with given configuration.
     * @param config AP configuration
     */
    public void start(WifiConfiguration config) {
        mStateMachine.sendMessage(SoftApStateMachine.CMD_START, config);
    }

    /**
     * Stop soft AP.
     */
    public void stop() {
        mStateMachine.sendMessage(SoftApStateMachine.CMD_STOP);
    }

    /**
     * Update AP state.
     * @param state new AP state
     * @param reason Failure reason if the new AP state is in failure state
     */
    private void updateApState(int state, int reason) {
        if (mListener != null) {
            mListener.onStateChanged(state, reason);
        }
    }

    /**
     * Start a soft AP instance with the given configuration.
     * @param config AP configuration
     * @return integer result code
     */
    private int startSoftAp(WifiConfiguration config) {
        if (config == null) {
            Log.e(TAG, "Unable to start soft AP without configuration");
            return ERROR_GENERIC;
        }

        /* Make a copy of configuration for updating AP band and channel. */
        WifiConfiguration localConfig = new WifiConfiguration(config);

        int result = ApConfigUtil.updateApChannelConfig(
                mWifiNative, mCountryCode, mAllowed2GChannels, localConfig);
        if (result != SUCCESS) {
            Log.e(TAG, "Failed to update AP band and channel");
            return result;
        }

        /* Setup country code if it is provide. */
        if (mCountryCode != null) {
            /**
             * Country code is mandatory for 5GHz band, return an error if failed to set
             * country code when AP is configured for 5GHz band.
             */
            if (!mWifiNative.setCountryCodeHal(mCountryCode.toUpperCase(Locale.ROOT))
                    && config.apBand == WifiConfiguration.AP_BAND_5GHZ) {
                Log.e(TAG, "Failed to set country code, required for setting up "
                        + "soft ap in 5GHz");
                return ERROR_GENERIC;
            }
        }

        try {
            mNmService.startAccessPoint(localConfig, mInterfaceName);
        } catch (Exception e) {
            Log.e(TAG, "Exception in starting soft AP: " + e);
            return ERROR_GENERIC;
        }

        Log.d(TAG, "Soft AP is started");

        return SUCCESS;
    }

    /**
     * Teardown soft AP.
     */
    private void stopSoftAp() {
        try {
            mNmService.stopAccessPoint(mInterfaceName);
        } catch (Exception e) {
            Log.e(TAG, "Exception in stopping soft AP: " + e);
            return;
        }
        Log.d(TAG, "Soft AP is stopped");
    }

    private class SoftApStateMachine extends StateMachine {
        /* Commands for the state machine. */
        public static final int CMD_START = 0;
        public static final int CMD_STOP = 1;

        private final State mIdleState = new IdleState();
        private final State mStartedState = new StartedState();

        SoftApStateMachine(Looper looper) {
            super(TAG, looper);

            addState(mIdleState);
            addState(mStartedState, mIdleState);

            setInitialState(mIdleState);
            start();
        }

        private class IdleState extends State {
            @Override
            public boolean processMessage(Message message) {
                switch (message.what) {
                    case CMD_START:
                        updateApState(WifiManager.WIFI_AP_STATE_ENABLING, 0);
                        int result = startSoftAp((WifiConfiguration) message.obj);
                        if (result == SUCCESS) {
                            updateApState(WifiManager.WIFI_AP_STATE_ENABLED, 0);
                            transitionTo(mStartedState);
                        } else {
                            int reason = WifiManager.SAP_START_FAILURE_GENERAL;
                            if (result == ERROR_NO_CHANNEL) {
                                reason = WifiManager.SAP_START_FAILURE_NO_CHANNEL;
                            }
                            updateApState(WifiManager.WIFI_AP_STATE_FAILED, reason);
                        }
                        break;
                    default:
                        /* Ignore all other commands. */
                        break;
                }
                return HANDLED;
            }
        }

        private class StartedState extends State {
            @Override
            public boolean processMessage(Message message) {
                switch (message.what) {
                    case CMD_START:
                        /* Already started, ignore this command. */
                        break;
                    case CMD_STOP:
                        updateApState(WifiManager.WIFI_AP_STATE_DISABLING, 0);
                        stopSoftAp();
                        updateApState(WifiManager.WIFI_AP_STATE_DISABLED, 0);
                        transitionTo(mIdleState);
                        break;
                    default:
                        return NOT_HANDLED;
                }
                return HANDLED;
            }
        }

    }
}
