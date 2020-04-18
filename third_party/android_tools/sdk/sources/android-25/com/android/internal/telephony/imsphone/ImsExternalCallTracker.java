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
 * limitations under the License
 */

package com.android.internal.telephony.imsphone;

import com.android.ims.ImsCallProfile;
import com.android.ims.ImsExternalCallState;
import com.android.ims.ImsExternalCallStateListener;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.Call;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;

import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.telecom.PhoneAccountHandle;
import android.telecom.VideoProfile;
import android.telephony.TelephonyManager;
import android.util.ArrayMap;
import android.util.Log;

import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Responsible for tracking external calls known to the system.
 */
public class ImsExternalCallTracker implements ImsPhoneCallTracker.PhoneStateListener {

    /**
     * Interface implemented by modules which are capable of notifying interested parties of new
     * unknown connections, and changes to call state.
     * This is used to break the dependency between {@link ImsExternalCallTracker} and
     * {@link ImsPhone}.
     *
     * @hide
     */
    public static interface ImsCallNotify {
        /**
         * Notifies that an unknown connection has been added.
         * @param c The new unknown connection.
         */
        void notifyUnknownConnection(Connection c);

        /**
         * Notifies of a change to call state.
         */
        void notifyPreciseCallStateChanged();
    }


    /**
     * Implements the {@link ImsExternalCallStateListener}, which is responsible for receiving
     * external call state updates from the IMS framework.
     */
    public class ExternalCallStateListener extends ImsExternalCallStateListener {
        @Override
        public void onImsExternalCallStateUpdate(List<ImsExternalCallState> externalCallState) {
            refreshExternalCallState(externalCallState);
        }
    }

    /**
     * Receives callbacks from {@link ImsExternalConnection}s when a call pull has been initiated.
     */
    public class ExternalConnectionListener implements ImsExternalConnection.Listener {
        @Override
        public void onPullExternalCall(ImsExternalConnection connection) {
            Log.d(TAG, "onPullExternalCall: connection = " + connection);
            if (mCallPuller == null) {
                Log.e(TAG, "onPullExternalCall : No call puller defined");
                return;
            }
            mCallPuller.pullExternalCall(connection.getAddress(), connection.getVideoState(),
                    connection.getCallId());
        }
    }

    public final static String TAG = "ImsExternalCallTracker";

    private static final int EVENT_VIDEO_CAPABILITIES_CHANGED = 1;

    /**
     * Extra key used when informing telecom of a new external call using the
     * {@link android.telecom.TelecomManager#addNewUnknownCall(PhoneAccountHandle, Bundle)} API.
     * Used to ensure that when Telecom requests the {@link android.telecom.ConnectionService} to
     * create the connection for the unknown call that we can determine which
     * {@link ImsExternalConnection} in {@link #mExternalConnections} is the one being requested.
     */
    public final static String EXTRA_IMS_EXTERNAL_CALL_ID =
            "android.telephony.ImsExternalCallTracker.extra.EXTERNAL_CALL_ID";

    /**
     * Contains a list of the external connections known by the ImsExternalCallTracker.  These are
     * connections which originated from a dialog event package and reside on another device.
     * Used in multi-endpoint (VoLTE for internet connected endpoints) scenarios.
     */
    private Map<Integer, ImsExternalConnection> mExternalConnections =
            new ArrayMap<>();

    /**
     * Tracks whether each external connection tracked in
     * {@link #mExternalConnections} can be pulled, as reported by the latest dialog event package
     * received from the network.  We need to know this because the pull state of a call can be
     * overridden based on the following factors:
     * 1) An external video call cannot be pulled if the current device does not have video
     *    capability.
     * 2) If the device has any active or held calls locally, no external calls may be pulled to
     *    the local device.
     */
    private Map<Integer, Boolean> mExternalCallPullableState = new ArrayMap<>();
    private final ImsPhone mPhone;
    private final ImsCallNotify mCallStateNotifier;
    private final ExternalCallStateListener mExternalCallStateListener;
    private final ExternalConnectionListener mExternalConnectionListener =
            new ExternalConnectionListener();
    private ImsPullCall mCallPuller;
    private boolean mIsVideoCapable;
    private boolean mHasActiveCalls;

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case EVENT_VIDEO_CAPABILITIES_CHANGED:
                    handleVideoCapabilitiesChanged((AsyncResult) msg.obj);
                    break;
                default:
                    break;
            }
        }
    };

    @VisibleForTesting
    public ImsExternalCallTracker(ImsPhone phone, ImsPullCall callPuller,
            ImsCallNotify callNotifier) {

        mPhone = phone;
        mCallStateNotifier = callNotifier;
        mExternalCallStateListener = new ExternalCallStateListener();
        mCallPuller = callPuller;
    }

    public ImsExternalCallTracker(ImsPhone phone) {
        mPhone = phone;
        mCallStateNotifier = new ImsCallNotify() {
            @Override
            public void notifyUnknownConnection(Connection c) {
                mPhone.notifyUnknownConnection(c);
            }

            @Override
            public void notifyPreciseCallStateChanged() {
                mPhone.notifyPreciseCallStateChanged();
            }
        };
        mExternalCallStateListener = new ExternalCallStateListener();
        registerForNotifications();
    }

    /**
     * Performs any cleanup required before the ImsExternalCallTracker is destroyed.
     */
    public void tearDown() {
        unregisterForNotifications();
    }

    /**
     * Sets the implementation of {@link ImsPullCall} which is responsible for pulling calls.
     *
     * @param callPuller The pull call implementation.
     */
    public void setCallPuller(ImsPullCall callPuller) {
       mCallPuller = callPuller;
    }

    public ExternalCallStateListener getExternalCallStateListener() {
        return mExternalCallStateListener;
    }

    /**
     * Handles changes to the phone state as notified by the {@link ImsPhoneCallTracker}.
     *
     * @param oldState The previous phone state.
     * @param newState The new phone state.
     */
    @Override
    public void onPhoneStateChanged(PhoneConstants.State oldState, PhoneConstants.State newState) {
        mHasActiveCalls = newState != PhoneConstants.State.IDLE;
        Log.i(TAG, "onPhoneStateChanged : hasActiveCalls = " + mHasActiveCalls);

        refreshCallPullState();
    }

    /**
     * Registers for video capability changes.
     */
    private void registerForNotifications() {
        if (mPhone != null) {
            Log.d(TAG, "Registering: " + mPhone);
            mPhone.getDefaultPhone().registerForVideoCapabilityChanged(mHandler,
                    EVENT_VIDEO_CAPABILITIES_CHANGED, null);
        }
    }

    /**
     * Unregisters for video capability changes.
     */
    private void unregisterForNotifications() {
        if (mPhone != null) {
            Log.d(TAG, "Unregistering: " + mPhone);
            mPhone.unregisterForVideoCapabilityChanged(mHandler);
        }
    }


    /**
     * Called when the IMS stack receives a new dialog event package.  Triggers the creation and
     * update of {@link ImsExternalConnection}s to represent the dialogs in the dialog event
     * package data.
     *
     * @param externalCallStates the {@link ImsExternalCallState} information for the dialog event
     *                           package.
     */
    public void refreshExternalCallState(List<ImsExternalCallState> externalCallStates) {
        Log.d(TAG, "refreshExternalCallState");

        // Check to see if any call Ids are no longer present in the external call state.  If they
        // are, the calls are terminated and should be removed.
        Iterator<Map.Entry<Integer, ImsExternalConnection>> connectionIterator =
                mExternalConnections.entrySet().iterator();
        boolean wasCallRemoved = false;
        while (connectionIterator.hasNext()) {
            Map.Entry<Integer, ImsExternalConnection> entry = connectionIterator.next();
            int callId = entry.getKey().intValue();

            if (!containsCallId(externalCallStates, callId)) {
                ImsExternalConnection externalConnection = entry.getValue();
                externalConnection.setTerminated();
                externalConnection.removeListener(mExternalConnectionListener);
                connectionIterator.remove();
                wasCallRemoved = true;
            }
        }
        // If one or more calls were removed, trigger a notification that will cause the
        // TelephonyConnection instancse to refresh their state with Telecom.
        if (wasCallRemoved) {
            mCallStateNotifier.notifyPreciseCallStateChanged();
        }

        // Check for new calls, and updates to existing ones.
        if (externalCallStates != null && !externalCallStates.isEmpty()) {
            for (ImsExternalCallState callState : externalCallStates) {
                if (!mExternalConnections.containsKey(callState.getCallId())) {
                    Log.d(TAG, "refreshExternalCallState: got = " + callState);
                    // If there is a new entry and it is already terminated, don't bother adding it to
                    // telecom.
                    if (callState.getCallState() != ImsExternalCallState.CALL_STATE_CONFIRMED) {
                        continue;
                    }
                    createExternalConnection(callState);
                } else {
                    updateExistingConnection(mExternalConnections.get(callState.getCallId()),
                            callState);
                }
            }
        }
    }

    /**
     * Finds an external connection given a call Id.
     *
     * @param callId The call Id.
     * @return The {@link Connection}, or {@code null} if no match found.
     */
    public Connection getConnectionById(int callId) {
        return mExternalConnections.get(callId);
    }

    /**
     * Given an {@link ImsExternalCallState} instance obtained from a dialog event package,
     * creates a new instance of {@link ImsExternalConnection} to represent the connection, and
     * initiates the addition of the new call to Telecom as an unknown call.
     *
     * @param state External call state from a dialog event package.
     */
    private void createExternalConnection(ImsExternalCallState state) {
        Log.i(TAG, "createExternalConnection : state = " + state);

        int videoState = ImsCallProfile.getVideoStateFromCallType(state.getCallType());

        boolean isCallPullPermitted = isCallPullPermitted(state.isCallPullable(), videoState);
        ImsExternalConnection connection = new ImsExternalConnection(mPhone,
                state.getCallId(), /* Dialog event package call id */
                state.getAddress() /* phone number */,
                isCallPullPermitted);
        connection.setVideoState(videoState);
        connection.addListener(mExternalConnectionListener);

        Log.d(TAG,
                "createExternalConnection - pullable state : externalCallId = "
                        + connection.getCallId()
                        + " ; isPullable = " + isCallPullPermitted
                        + " ; networkPullable = " + state.isCallPullable()
                        + " ; isVideo = " + VideoProfile.isVideo(videoState)
                        + " ; videoEnabled = " + mIsVideoCapable
                        + " ; hasActiveCalls = " + mHasActiveCalls);

        // Add to list of tracked connections.
        mExternalConnections.put(connection.getCallId(), connection);
        mExternalCallPullableState.put(connection.getCallId(), state.isCallPullable());

        // Note: The notification of unknown connection is ultimately handled by
        // PstnIncomingCallNotifier#addNewUnknownCall.  That method will ensure that an extra is set
        // containing the ImsExternalConnection#mCallId so that we have a means of reconciling which
        // unknown call was added.
        mCallStateNotifier.notifyUnknownConnection(connection);
    }

    /**
     * Given an existing {@link ImsExternalConnection}, applies any changes found found in a
     * {@link ImsExternalCallState} instance received from a dialog event package to the connection.
     *
     * @param connection The connection to apply changes to.
     * @param state The new dialog state for the connection.
     */
    private void updateExistingConnection(ImsExternalConnection connection,
            ImsExternalCallState state) {

        Log.i(TAG, "updateExistingConnection : state = " + state);
        Call.State existingState = connection.getState();
        Call.State newState = state.getCallState() == ImsExternalCallState.CALL_STATE_CONFIRMED ?
                Call.State.ACTIVE : Call.State.DISCONNECTED;

        if (existingState != newState) {
            if (newState == Call.State.ACTIVE) {
                connection.setActive();
            } else {
                connection.setTerminated();
                connection.removeListener(mExternalConnectionListener);
                mExternalConnections.remove(connection.getCallId());
                mExternalCallPullableState.remove(connection.getCallId());
                mCallStateNotifier.notifyPreciseCallStateChanged();
            }
        }

        int newVideoState = ImsCallProfile.getVideoStateFromCallType(state.getCallType());
        if (newVideoState != connection.getVideoState()) {
            connection.setVideoState(newVideoState);
        }

        mExternalCallPullableState.put(state.getCallId(), state.isCallPullable());
        boolean isCallPullPermitted = isCallPullPermitted(state.isCallPullable(), newVideoState);
        Log.d(TAG,
                "updateExistingConnection - pullable state : externalCallId = " + connection
                        .getCallId()
                        + " ; isPullable = " + isCallPullPermitted
                        + " ; networkPullable = " + state.isCallPullable()
                        + " ; isVideo = "
                        + VideoProfile.isVideo(connection.getVideoState())
                        + " ; videoEnabled = " + mIsVideoCapable
                        + " ; hasActiveCalls = " + mHasActiveCalls);

        connection.setIsPullable(isCallPullPermitted);
    }

    /**
     * Update whether the external calls known can be pulled.  Combines the last known network
     * pullable state with local device conditions to determine if each call can be pulled.
     */
    private void refreshCallPullState() {
        Log.d(TAG, "refreshCallPullState");

        for (ImsExternalConnection imsExternalConnection : mExternalConnections.values()) {
            boolean isNetworkPullable =
                    mExternalCallPullableState.get(imsExternalConnection.getCallId())
                            .booleanValue();
            boolean isCallPullPermitted =
                    isCallPullPermitted(isNetworkPullable, imsExternalConnection.getVideoState());
            Log.d(TAG,
                    "refreshCallPullState : externalCallId = " + imsExternalConnection.getCallId()
                            + " ; isPullable = " + isCallPullPermitted
                            + " ; networkPullable = " + isNetworkPullable
                            + " ; isVideo = "
                            + VideoProfile.isVideo(imsExternalConnection.getVideoState())
                            + " ; videoEnabled = " + mIsVideoCapable
                            + " ; hasActiveCalls = " + mHasActiveCalls);
            imsExternalConnection.setIsPullable(isCallPullPermitted);
        }
    }

    /**
     * Determines if a list of call states obtained from a dialog event package contacts an existing
     * call Id.
     *
     * @param externalCallStates The dialog event package state information.
     * @param callId The call Id.
     * @return {@code true} if the state information contains the call Id, {@code false} otherwise.
     */
    private boolean containsCallId(List<ImsExternalCallState> externalCallStates, int callId) {
        if (externalCallStates == null) {
            return false;
        }

        for (ImsExternalCallState state : externalCallStates) {
            if (state.getCallId() == callId) {
                return true;
            }
        }

        return false;
    }

    /**
     * Handles a change to the video capabilities reported by
     * {@link Phone#notifyForVideoCapabilityChanged(boolean)}.
     *
     * @param ar The AsyncResult containing the new video capability of the device.
     */
    private void handleVideoCapabilitiesChanged(AsyncResult ar) {
        mIsVideoCapable = (Boolean) ar.result;
        Log.i(TAG, "handleVideoCapabilitiesChanged : isVideoCapable = " + mIsVideoCapable);

        // Refresh pullable state if video capability changed.
        refreshCallPullState();
    }

    /**
     * Determines whether an external call can be pulled based on the pullability state enforced
     * by the network, as well as local device rules.
     *
     * @param isNetworkPullable {@code true} if the network indicates the call can be pulled,
     *      {@code false} otherwise.
     * @param videoState the VideoState of the external call.
     * @return {@code true} if the external call can be pulled, {@code false} otherwise.
     */
    private boolean isCallPullPermitted(boolean isNetworkPullable, int videoState) {
        if (VideoProfile.isVideo(videoState) && !mIsVideoCapable) {
            // If the external call is a video call and the local device does not have video
            // capability at this time, it cannot be pulled.
            return false;
        }

        if (mHasActiveCalls) {
            // If there are active calls on the local device, the call cannot be pulled.
            return false;
        }

        return isNetworkPullable;
    }
}
