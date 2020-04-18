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

package com.android.server.wifi.nan;

import android.net.wifi.nan.ConfigRequest;
import android.net.wifi.nan.IWifiNanEventListener;
import android.net.wifi.nan.IWifiNanSessionListener;
import android.net.wifi.nan.PublishData;
import android.net.wifi.nan.PublishSettings;
import android.net.wifi.nan.SubscribeData;
import android.net.wifi.nan.SubscribeSettings;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.util.SparseArray;

import libcore.util.HexEncoding;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

public class WifiNanStateManager {
    private static final String TAG = "WifiNanStateManager";
    private static final boolean DBG = false;
    private static final boolean VDBG = false; // STOPSHIP if true

    private static WifiNanStateManager sNanStateManagerSingleton;

    private static final int MESSAGE_CONNECT = 0;
    private static final int MESSAGE_DISCONNECT = 1;
    private static final int MESSAGE_REQUEST_CONFIG = 4;
    private static final int MESSAGE_CREATE_SESSION = 5;
    private static final int MESSAGE_DESTROY_SESSION = 6;
    private static final int MESSAGE_PUBLISH = 7;
    private static final int MESSAGE_SUBSCRIBE = 8;
    private static final int MESSAGE_SEND_MESSAGE = 9;
    private static final int MESSAGE_STOP_SESSION = 10;
    private static final int MESSAGE_ON_CONFIG_COMPLETED = 11;
    private static final int MESSAGE_ON_CONFIG_FAILED = 12;
    private static final int MESSAGE_ON_NAN_DOWN = 13;
    private static final int MESSAGE_ON_INTERFACE_CHANGE = 14;
    private static final int MESSAGE_ON_CLUSTER_CHANGE = 15;
    private static final int MESSAGE_ON_PUBLISH_SUCCESS = 16;
    private static final int MESSAGE_ON_PUBLISH_FAIL = 17;
    private static final int MESSAGE_ON_PUBLISH_TERMINATED = 18;
    private static final int MESSAGE_ON_SUBSCRIBE_SUCCESS = 19;
    private static final int MESSAGE_ON_SUBSCRIBE_FAIL = 20;
    private static final int MESSAGE_ON_SUBSCRIBE_TERMINATED = 21;
    private static final int MESSAGE_ON_MESSAGE_SEND_SUCCESS = 22;
    private static final int MESSAGE_ON_MESSAGE_SEND_FAIL = 23;
    private static final int MESSAGE_ON_UNKNOWN_TRANSACTION = 24;
    private static final int MESSAGE_ON_MATCH = 25;
    private static final int MESSAGE_ON_MESSAGE_RECEIVED = 26;
    private static final int MESSAGE_ON_CAPABILITIES_UPDATED = 27;

    private static final String MESSAGE_BUNDLE_KEY_SESSION_ID = "session_id";
    private static final String MESSAGE_BUNDLE_KEY_EVENTS = "events";
    private static final String MESSAGE_BUNDLE_KEY_PUBLISH_DATA = "publish_data";
    private static final String MESSAGE_BUNDLE_KEY_PUBLISH_SETTINGS = "publish_settings";
    private static final String MESSAGE_BUNDLE_KEY_SUBSCRIBE_DATA = "subscribe_data";
    private static final String MESSAGE_BUNDLE_KEY_SUBSCRIBE_SETTINGS = "subscribe_settings";
    private static final String MESSAGE_BUNDLE_KEY_MESSAGE = "message";
    private static final String MESSAGE_BUNDLE_KEY_MESSAGE_PEER_ID = "message_peer_id";
    private static final String MESSAGE_BUNDLE_KEY_MESSAGE_ID = "message_id";
    private static final String MESSAGE_BUNDLE_KEY_RESPONSE_TYPE = "response_type";
    private static final String MESSAGE_BUNDLE_KEY_SSI_LENGTH = "ssi_length";
    private static final String MESSAGE_BUNDLE_KEY_SSI_DATA = "ssi_data";
    private static final String MESSAGE_BUNDLE_KEY_FILTER_LENGTH = "filter_length";
    private static final String MESSAGE_BUNDLE_KEY_FILTER_DATA = "filter_data";
    private static final String MESSAGE_BUNDLE_KEY_MAC_ADDRESS = "mac_address";
    private static final String MESSAGE_BUNDLE_KEY_MESSAGE_DATA = "message_data";
    private static final String MESSAGE_BUNDLE_KEY_MESSAGE_LENGTH = "message_length";

    private WifiNanNative.Capabilities mCapabilities;

    private WifiNanStateHandler mHandler;

    // no synchronization necessary: only access through Handler
    private final SparseArray<WifiNanClientState> mClients = new SparseArray<>();
    private final SparseArray<TransactionInfoBase> mPendingResponses = new SparseArray<>();
    private short mNextTransactionId = 1;

    private WifiNanStateManager() {
        // EMPTY: singleton pattern
    }

    public static WifiNanStateManager getInstance() {
        if (sNanStateManagerSingleton == null) {
            sNanStateManagerSingleton = new WifiNanStateManager();
        }

        return sNanStateManagerSingleton;
    }

    public void start(Looper looper) {
        Log.i(TAG, "start()");

        mHandler = new WifiNanStateHandler(looper);
    }

    public void connect(int uid, IWifiNanEventListener listener, int events) {
        Message msg = mHandler.obtainMessage(MESSAGE_CONNECT);
        msg.arg1 = uid;
        msg.arg2 = events;
        msg.obj = listener;
        mHandler.sendMessage(msg);
    }

    public void disconnect(int uid) {
        Message msg = mHandler.obtainMessage(MESSAGE_DISCONNECT);
        msg.arg1 = uid;
        mHandler.sendMessage(msg);
    }

    public void requestConfig(int uid, ConfigRequest configRequest) {
        Message msg = mHandler.obtainMessage(MESSAGE_REQUEST_CONFIG);
        msg.arg1 = uid;
        msg.obj = configRequest;
        mHandler.sendMessage(msg);
    }

    public void stopSession(int uid, int sessionId) {
        Message msg = mHandler.obtainMessage(MESSAGE_STOP_SESSION);
        msg.arg1 = uid;
        msg.arg2 = sessionId;
        mHandler.sendMessage(msg);
    }

    public void destroySession(int uid, int sessionId) {
        Message msg = mHandler.obtainMessage(MESSAGE_DESTROY_SESSION);
        msg.arg1 = uid;
        msg.arg2 = sessionId;
        mHandler.sendMessage(msg);
    }

    public void createSession(int uid, int sessionId, IWifiNanSessionListener listener,
            int events) {
        Bundle data = new Bundle();
        data.putInt(MESSAGE_BUNDLE_KEY_EVENTS, events);

        Message msg = mHandler.obtainMessage(MESSAGE_CREATE_SESSION);
        msg.setData(data);
        msg.arg1 = uid;
        msg.arg2 = sessionId;
        msg.obj = listener;
        mHandler.sendMessage(msg);
    }

    public void publish(int uid, int sessionId, PublishData publishData,
            PublishSettings publishSettings) {
        Bundle data = new Bundle();
        data.putParcelable(MESSAGE_BUNDLE_KEY_PUBLISH_DATA, publishData);
        data.putParcelable(MESSAGE_BUNDLE_KEY_PUBLISH_SETTINGS, publishSettings);

        Message msg = mHandler.obtainMessage(MESSAGE_PUBLISH);
        msg.setData(data);
        msg.arg1 = uid;
        msg.arg2 = sessionId;
        mHandler.sendMessage(msg);
    }

    public void subscribe(int uid, int sessionId, SubscribeData subscribeData,
            SubscribeSettings subscribeSettings) {
        Bundle data = new Bundle();
        data.putParcelable(MESSAGE_BUNDLE_KEY_SUBSCRIBE_DATA, subscribeData);
        data.putParcelable(MESSAGE_BUNDLE_KEY_SUBSCRIBE_SETTINGS, subscribeSettings);

        Message msg = mHandler.obtainMessage(MESSAGE_SUBSCRIBE);
        msg.setData(data);
        msg.arg1 = uid;
        msg.arg2 = sessionId;
        mHandler.sendMessage(msg);
    }

    public void sendMessage(int uid, int sessionId, int peerId, byte[] message, int messageLength,
            int messageId) {
        Bundle data = new Bundle();
        data.putInt(MESSAGE_BUNDLE_KEY_SESSION_ID, sessionId);
        data.putInt(MESSAGE_BUNDLE_KEY_MESSAGE_PEER_ID, peerId);
        data.putByteArray(MESSAGE_BUNDLE_KEY_MESSAGE, message);
        data.putInt(MESSAGE_BUNDLE_KEY_MESSAGE_ID, messageId);

        Message msg = mHandler.obtainMessage(MESSAGE_SEND_MESSAGE);
        msg.arg1 = uid;
        msg.arg2 = messageLength;
        msg.setData(data);
        mHandler.sendMessage(msg);
    }

    public void onCapabilitiesUpdate(short transactionId, WifiNanNative.Capabilities capabilities) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_CAPABILITIES_UPDATED);
        msg.arg1 = transactionId;
        msg.obj = capabilities;
        mHandler.sendMessage(msg);
    }

    public void onConfigCompleted(short transactionId) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_CONFIG_COMPLETED);
        msg.arg1 = transactionId;
        mHandler.sendMessage(msg);
    }

    public void onConfigFailed(short transactionId, int reason) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_CONFIG_FAILED);
        msg.arg1 = transactionId;
        msg.arg2 = reason;
        mHandler.sendMessage(msg);
    }

    public void onPublishSuccess(short transactionId, int publishId) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_PUBLISH_SUCCESS);
        msg.arg1 = transactionId;
        msg.arg2 = publishId;
        mHandler.sendMessage(msg);
    }

    public void onPublishFail(short transactionId, int status) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_PUBLISH_FAIL);
        msg.arg1 = transactionId;
        msg.arg2 = status;
        mHandler.sendMessage(msg);
    }

    public void onMessageSendSuccess(short transactionId) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_MESSAGE_SEND_SUCCESS);
        msg.arg1 = transactionId;
        mHandler.sendMessage(msg);
    }

    public void onMessageSendFail(short transactionId, int status) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_MESSAGE_SEND_FAIL);
        msg.arg1 = transactionId;
        msg.arg2 = status;
        mHandler.sendMessage(msg);
    }

    public void onSubscribeSuccess(short transactionId, int subscribeId) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_SUBSCRIBE_SUCCESS);
        msg.arg1 = transactionId;
        msg.arg2 = subscribeId;
        mHandler.sendMessage(msg);
    }

    public void onSubscribeFail(short transactionId, int status) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_SUBSCRIBE_FAIL);
        msg.arg1 = transactionId;
        msg.arg2 = status;
        mHandler.sendMessage(msg);
    }

    public void onUnknownTransaction(int responseType, short transactionId, int status) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_UNKNOWN_TRANSACTION);
        Bundle data = new Bundle();
        data.putInt(MESSAGE_BUNDLE_KEY_RESPONSE_TYPE, responseType);
        msg.setData(data);
        msg.arg1 = transactionId;
        msg.arg2 = status;
        mHandler.sendMessage(msg);
    }

    public void onInterfaceAddressChange(byte[] mac) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_INTERFACE_CHANGE);
        msg.obj = mac;
        mHandler.sendMessage(msg);
    }

    public void onClusterChange(int flag, byte[] clusterId) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_CLUSTER_CHANGE);
        msg.arg1 = flag;
        msg.obj = clusterId;
        mHandler.sendMessage(msg);
    }

    public void onMatch(int pubSubId, int requestorInstanceId, byte[] peerMac,
            byte[] serviceSpecificInfo, int serviceSpecificInfoLength, byte[] matchFilter,
            int matchFilterLength) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_MATCH);
        msg.arg1 = pubSubId;
        msg.arg2 = requestorInstanceId;
        Bundle data = new Bundle();
        data.putByteArray(MESSAGE_BUNDLE_KEY_MAC_ADDRESS, peerMac);
        data.putByteArray(MESSAGE_BUNDLE_KEY_SSI_DATA, serviceSpecificInfo);
        data.putInt(MESSAGE_BUNDLE_KEY_SSI_LENGTH, serviceSpecificInfoLength);
        data.putByteArray(MESSAGE_BUNDLE_KEY_FILTER_DATA, matchFilter);
        data.putInt(MESSAGE_BUNDLE_KEY_FILTER_LENGTH, matchFilterLength);
        msg.setData(data);
        mHandler.sendMessage(msg);
    }

    public void onPublishTerminated(int publishId, int status) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_PUBLISH_TERMINATED);
        msg.arg1 = publishId;
        msg.arg2 = status;
        mHandler.sendMessage(msg);
    }

    public void onSubscribeTerminated(int subscribeId, int status) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_SUBSCRIBE_TERMINATED);
        msg.arg1 = subscribeId;
        msg.arg2 = status;
        mHandler.sendMessage(msg);
    }

    public void onMessageReceived(int pubSubId, int requestorInstanceId, byte[] peerMac,
            byte[] message, int messageLength) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_MESSAGE_RECEIVED);
        msg.arg1 = pubSubId;
        msg.arg2 = requestorInstanceId;
        Bundle data = new Bundle();
        data.putByteArray(MESSAGE_BUNDLE_KEY_MAC_ADDRESS, peerMac);
        data.putByteArray(MESSAGE_BUNDLE_KEY_MESSAGE_DATA, message);
        data.putInt(MESSAGE_BUNDLE_KEY_MESSAGE_LENGTH, messageLength);
        msg.setData(data);
        mHandler.sendMessage(msg);
    }

    public void onNanDown(int reason) {
        Message msg = mHandler.obtainMessage(MESSAGE_ON_NAN_DOWN);
        msg.arg1 = reason;
        mHandler.sendMessage(msg);
    }

    private class WifiNanStateHandler extends Handler {
        WifiNanStateHandler(android.os.Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            if (DBG) {
                Log.d(TAG, "Message: " + msg.what);
            }
            switch (msg.what) {
                case MESSAGE_CONNECT: {
                    if (VDBG) {
                        Log.d(TAG, "NAN connection request received");
                    }
                    connectLocal(msg.arg1, (IWifiNanEventListener) msg.obj, msg.arg2);
                    break;
                }
                case MESSAGE_DISCONNECT: {
                    if (VDBG) {
                        Log.d(TAG, "NAN disconnection request received");
                    }
                    disconnectLocal(msg.arg1);
                    break;
                }
                case MESSAGE_REQUEST_CONFIG: {
                    if (VDBG) {
                        Log.d(TAG, "NAN configuration request received");
                    }
                    requestConfigLocal(msg.arg1, (ConfigRequest) msg.obj);
                    break;
                }
                case MESSAGE_CREATE_SESSION: {
                    if (VDBG) {
                        Log.d(TAG, "Create session");
                    }
                    int events = msg.getData().getInt(MESSAGE_BUNDLE_KEY_EVENTS);
                    createSessionLocal(msg.arg1, msg.arg2, (IWifiNanSessionListener) msg.obj,
                            events);
                    break;
                }
                case MESSAGE_DESTROY_SESSION: {
                    if (VDBG) {
                        Log.d(TAG, "Destroy session");
                    }
                    destroySessionLocal(msg.arg1, msg.arg2);
                    break;
                }
                case MESSAGE_PUBLISH: {
                    Bundle data = msg.getData();
                    PublishData publishData = (PublishData) data
                            .getParcelable(MESSAGE_BUNDLE_KEY_PUBLISH_DATA);
                    PublishSettings publishSettings = (PublishSettings) data
                            .getParcelable(MESSAGE_BUNDLE_KEY_PUBLISH_SETTINGS);
                    if (VDBG) {
                        Log.d(TAG,
                                "Publish: data='" + publishData + "', settings=" + publishSettings);
                    }

                    publishLocal(msg.arg1, msg.arg2, publishData, publishSettings);
                    break;
                }
                case MESSAGE_SUBSCRIBE: {
                    Bundle data = msg.getData();
                    SubscribeData subscribeData = (SubscribeData) data
                            .getParcelable(MESSAGE_BUNDLE_KEY_SUBSCRIBE_DATA);
                    SubscribeSettings subscribeSettings = (SubscribeSettings) data
                            .getParcelable(MESSAGE_BUNDLE_KEY_SUBSCRIBE_SETTINGS);
                    if (VDBG) {
                        Log.d(TAG, "Subscribe: data='" + subscribeData + "', settings="
                                + subscribeSettings);
                    }

                    subscribeLocal(msg.arg1, msg.arg2, subscribeData, subscribeSettings);
                    break;
                }
                case MESSAGE_SEND_MESSAGE: {
                    Bundle data = msg.getData();
                    int sessionId = msg.getData().getInt(MESSAGE_BUNDLE_KEY_SESSION_ID);
                    int peerId = data.getInt(MESSAGE_BUNDLE_KEY_MESSAGE_PEER_ID);
                    byte[] message = data.getByteArray(MESSAGE_BUNDLE_KEY_MESSAGE);
                    int messageId = data.getInt(MESSAGE_BUNDLE_KEY_MESSAGE_ID);

                    if (VDBG) {
                        Log.d(TAG, "Send Message: message='" + message + "' (ID=" + messageId
                                + ") to peerId=" + peerId);
                    }

                    sendFollowonMessageLocal(msg.arg1, sessionId, peerId, message, msg.arg2,
                            messageId);
                    break;
                }
                case MESSAGE_STOP_SESSION: {
                    if (VDBG) {
                        Log.d(TAG, "Stop session");
                    }
                    stopSessionLocal(msg.arg1, msg.arg2);
                    break;
                }
                case MESSAGE_ON_CAPABILITIES_UPDATED:
                    onCapabilitiesUpdatedLocal((short) msg.arg1,
                            (WifiNanNative.Capabilities) msg.obj);
                    break;
                case MESSAGE_ON_CONFIG_COMPLETED:
                    onConfigCompletedLocal((short) msg.arg1);
                    break;
                case MESSAGE_ON_CONFIG_FAILED:
                    onConfigFailedLocal((short) msg.arg1, msg.arg2);
                    break;
                case MESSAGE_ON_NAN_DOWN:
                    onNanDownLocal(msg.arg1);
                    break;
                case MESSAGE_ON_INTERFACE_CHANGE:
                    onInterfaceAddressChangeLocal((byte[]) msg.obj);
                    break;
                case MESSAGE_ON_CLUSTER_CHANGE:
                    onClusterChangeLocal(msg.arg1, (byte[]) msg.obj);
                    break;
                case MESSAGE_ON_PUBLISH_SUCCESS:
                    onPublishSuccessLocal((short) msg.arg1, msg.arg2);
                    break;
                case MESSAGE_ON_PUBLISH_FAIL:
                    onPublishFailLocal((short) msg.arg1, msg.arg2);
                    break;
                case MESSAGE_ON_PUBLISH_TERMINATED:
                    onPublishTerminatedLocal(msg.arg1, msg.arg2);
                    break;
                case MESSAGE_ON_SUBSCRIBE_SUCCESS:
                    onSubscribeSuccessLocal((short) msg.arg1, msg.arg2);
                    break;
                case MESSAGE_ON_SUBSCRIBE_FAIL:
                    onSubscribeFailLocal((short) msg.arg1, msg.arg2);
                    break;
                case MESSAGE_ON_SUBSCRIBE_TERMINATED:
                    onSubscribeTerminatedLocal(msg.arg1, msg.arg2);
                    break;
                case MESSAGE_ON_MESSAGE_SEND_SUCCESS:
                    onMessageSendSuccessLocal((short) msg.arg1);
                    break;
                case MESSAGE_ON_MESSAGE_SEND_FAIL:
                    onMessageSendFailLocal((short) msg.arg1, msg.arg2);
                    break;
                case MESSAGE_ON_UNKNOWN_TRANSACTION:
                    onUnknownTransactionLocal(
                            msg.getData().getInt(MESSAGE_BUNDLE_KEY_RESPONSE_TYPE),
                            (short) msg.arg1, msg.arg2);
                    break;
                case MESSAGE_ON_MATCH: {
                    int pubSubId = msg.arg1;
                    int requestorInstanceId = msg.arg2;
                    byte[] peerMac = msg.getData().getByteArray(MESSAGE_BUNDLE_KEY_MAC_ADDRESS);
                    byte[] serviceSpecificInfo = msg.getData()
                            .getByteArray(MESSAGE_BUNDLE_KEY_SSI_DATA);
                    int serviceSpecificInfoLength = msg.getData()
                            .getInt(MESSAGE_BUNDLE_KEY_SSI_LENGTH);
                    byte[] matchFilter = msg.getData().getByteArray(MESSAGE_BUNDLE_KEY_FILTER_DATA);
                    int matchFilterLength = msg.getData().getInt(MESSAGE_BUNDLE_KEY_FILTER_LENGTH);
                    onMatchLocal(pubSubId, requestorInstanceId, peerMac, serviceSpecificInfo,
                            serviceSpecificInfoLength, matchFilter, matchFilterLength);
                    break;
                }
                case MESSAGE_ON_MESSAGE_RECEIVED: {
                    int pubSubId = msg.arg1;
                    int requestorInstanceId = msg.arg2;
                    byte[] peerMac = msg.getData().getByteArray(MESSAGE_BUNDLE_KEY_MAC_ADDRESS);
                    byte[] message = msg.getData().getByteArray(MESSAGE_BUNDLE_KEY_MESSAGE_DATA);
                    int messageLength = msg.getData().getInt(MESSAGE_BUNDLE_KEY_MESSAGE_LENGTH);
                    onMessageReceivedLocal(pubSubId, requestorInstanceId, peerMac, message,
                            messageLength);
                    break;
                }
                default:
                    Log.e(TAG, "Unknown message code: " + msg.what);
            }
        }
    }

    /*
     * Transaction management classes & operations
     */

    // non-synchronized (should be ok as long as only used from NanStateManager,
    // NanClientState, and NanSessionState)
    /* package */ short createNextTransactionId() {
        return mNextTransactionId++;
    }

    private static class TransactionInfoBase {
        short mTransactionId;
    }

    private static class TransactionInfoSession extends TransactionInfoBase {
        public WifiNanClientState mClient;
        public WifiNanSessionState mSession;
    }

    private static class TransactionInfoMessage extends TransactionInfoSession {
        public int mMessageId;
    }

    private static class TransactionInfoConfig extends TransactionInfoBase {
        public ConfigRequest mConfig;
    }

    private void allocateAndRegisterTransactionId(TransactionInfoBase info) {
        info.mTransactionId = createNextTransactionId();

        mPendingResponses.put(info.mTransactionId, info);
    }

    private void fillInTransactionInfoSession(TransactionInfoSession info, int uid,
            int sessionId) {
        WifiNanClientState client = mClients.get(uid);
        if (client == null) {
            throw new IllegalStateException(
                    "getAndRegisterTransactionId: no client exists for uid=" + uid);
        }
        info.mClient = client;

        WifiNanSessionState session = info.mClient.getSession(sessionId);
        if (session == null) {
            throw new IllegalStateException(
                    "getAndRegisterSessionTransactionId: no session exists for uid=" + uid
                            + ", sessionId=" + sessionId);
        }
        info.mSession = session;
    }

    private TransactionInfoBase createTransactionInfo() {
        TransactionInfoBase info = new TransactionInfoBase();
        allocateAndRegisterTransactionId(info);
        return info;
    }

    private TransactionInfoSession createTransactionInfoSession(int uid, int sessionId) {
        TransactionInfoSession info = new TransactionInfoSession();
        fillInTransactionInfoSession(info, uid, sessionId);
        allocateAndRegisterTransactionId(info);
        return info;
    }

    private TransactionInfoMessage createTransactionInfoMessage(int uid, int sessionId,
            int messageId) {
        TransactionInfoMessage info = new TransactionInfoMessage();
        fillInTransactionInfoSession(info, uid, sessionId);
        info.mMessageId = messageId;
        allocateAndRegisterTransactionId(info);
        return info;
    }

    private TransactionInfoConfig createTransactionInfoConfig(ConfigRequest configRequest) {
        TransactionInfoConfig info = new TransactionInfoConfig();
        info.mConfig = configRequest;
        allocateAndRegisterTransactionId(info);
        return info;
    }

    private TransactionInfoBase getAndRemovePendingResponseTransactionInfo(short transactionId) {
        TransactionInfoBase transInfo = mPendingResponses.get(transactionId);
        if (transInfo != null) {
            mPendingResponses.remove(transactionId);
        }

        return transInfo;
    }

    private WifiNanSessionState getNanSessionStateForPubSubId(int pubSubId) {
        for (int i = 0; i < mClients.size(); ++i) {
            WifiNanSessionState session = mClients.valueAt(i)
                    .getNanSessionStateForPubSubId(pubSubId);
            if (session != null) {
                return session;
            }
        }

        return null;
    }

    /*
     * Actions (calls from API to service)
     */
    private void connectLocal(int uid, IWifiNanEventListener listener, int events) {
        if (VDBG) {
            Log.v(TAG, "connect(): uid=" + uid + ", listener=" + listener + ", events=" + events);
        }

        if (mClients.get(uid) != null) {
            Log.e(TAG, "connect: entry already exists for uid=" + uid);
            return;
        }

        WifiNanClientState client = new WifiNanClientState(uid, listener, events);
        mClients.put(uid, client);
    }

    private void disconnectLocal(int uid) {
        if (VDBG) {
            Log.v(TAG, "disconnect(): uid=" + uid);
        }

        WifiNanClientState client = mClients.get(uid);
        mClients.delete(uid);

        if (client == null) {
            Log.e(TAG, "disconnect: no entry for uid=" + uid);
            return;
        }

        List<Integer> toRemove = new ArrayList<>();
        for (int i = 0; i < mPendingResponses.size(); ++i) {
            TransactionInfoBase info = mPendingResponses.valueAt(i);
            if (!(info instanceof TransactionInfoSession)) {
                continue;
            }
            if (((TransactionInfoSession) info).mClient.getUid() == uid) {
                toRemove.add(i);
            }
        }
        for (Integer id : toRemove) {
            mPendingResponses.removeAt(id);
        }

        client.destroy();

        if (mClients.size() == 0) {
            WifiNanNative.getInstance().disable(createTransactionInfo().mTransactionId);
            return;
        }

        ConfigRequest merged = mergeConfigRequests();

        WifiNanNative.getInstance()
                .enableAndConfigure(createTransactionInfoConfig(merged).mTransactionId, merged);
    }

    private void requestConfigLocal(int uid, ConfigRequest configRequest) {
        if (VDBG) {
            Log.v(TAG, "requestConfig(): uid=" + uid + ", configRequest=" + configRequest);
        }

        WifiNanClientState client = mClients.get(uid);
        if (client == null) {
            Log.e(TAG, "requestConfig: no client exists for uid=" + uid);
            return;
        }

        client.setConfigRequest(configRequest);

        ConfigRequest merged = mergeConfigRequests();

        WifiNanNative.getInstance()
                .enableAndConfigure(createTransactionInfoConfig(merged).mTransactionId, merged);
    }

    private void createSessionLocal(int uid, int sessionId, IWifiNanSessionListener listener,
            int events) {
        if (VDBG) {
            Log.v(TAG, "createSession(): uid=" + uid + ", sessionId=" + sessionId + ", listener="
                    + listener + ", events=" + events);
        }

        WifiNanClientState client = mClients.get(uid);
        if (client == null) {
            Log.e(TAG, "createSession: no client exists for uid=" + uid);
            return;
        }

        client.createSession(sessionId, listener, events);
    }

    private void destroySessionLocal(int uid, int sessionId) {
        if (VDBG) {
            Log.v(TAG, "destroySession(): uid=" + uid + ", sessionId=" + sessionId);
        }

        WifiNanClientState client = mClients.get(uid);
        if (client == null) {
            Log.e(TAG, "destroySession: no client exists for uid=" + uid);
            return;
        }

        List<Integer> toRemove = new ArrayList<>();
        for (int i = 0; i < mPendingResponses.size(); ++i) {
            TransactionInfoBase info = mPendingResponses.valueAt(i);
            if (!(info instanceof TransactionInfoSession)) {
                continue;
            }
            TransactionInfoSession infoSession = (TransactionInfoSession) info;
            if (infoSession.mClient.getUid() == uid
                    && infoSession.mSession.getSessionId() == sessionId) {
                toRemove.add(i);
            }
        }
        for (Integer id : toRemove) {
            mPendingResponses.removeAt(id);
        }

        client.destroySession(sessionId);
    }

    private void publishLocal(int uid, int sessionId, PublishData publishData,
            PublishSettings publishSettings) {
        if (VDBG) {
            Log.v(TAG, "publish(): uid=" + uid + ", sessionId=" + sessionId + ", data="
                    + publishData + ", settings=" + publishSettings);
        }

        TransactionInfoSession info = createTransactionInfoSession(uid, sessionId);

        info.mSession.publish(info.mTransactionId, publishData, publishSettings);
    }

    private void subscribeLocal(int uid, int sessionId, SubscribeData subscribeData,
            SubscribeSettings subscribeSettings) {
        if (VDBG) {
            Log.v(TAG, "subscribe(): uid=" + uid + ", sessionId=" + sessionId + ", data="
                    + subscribeData + ", settings=" + subscribeSettings);
        }

        TransactionInfoSession info = createTransactionInfoSession(uid, sessionId);

        info.mSession.subscribe(info.mTransactionId, subscribeData, subscribeSettings);
    }

    private void sendFollowonMessageLocal(int uid, int sessionId, int peerId, byte[] message,
            int messageLength, int messageId) {
        if (VDBG) {
            Log.v(TAG, "sendMessage(): uid=" + uid + ", sessionId=" + sessionId + ", peerId="
                    + peerId + ", messageLength=" + messageLength + ", messageId=" + messageId);
        }

        TransactionInfoMessage info = createTransactionInfoMessage(uid, sessionId, messageId);

        info.mSession.sendMessage(info.mTransactionId, peerId, message, messageLength, messageId);
    }

    private void stopSessionLocal(int uid, int sessionId) {
        if (VDBG) {
            Log.v(TAG, "stopSession(): uid=" + uid + ", sessionId=" + sessionId);
        }

        TransactionInfoSession info = createTransactionInfoSession(uid, sessionId);

        info.mSession.stop(info.mTransactionId);
    }

    /*
     * Callbacks (calls from HAL/Native to service)
     */

    private void onCapabilitiesUpdatedLocal(short transactionId,
            WifiNanNative.Capabilities capabilities) {
        if (VDBG) {
            Log.v(TAG, "onCapabilitiesUpdatedLocal: transactionId=" + transactionId
                    + ", capabilites=" + capabilities);
        }

        mCapabilities = capabilities;
    }

    private void onConfigCompletedLocal(short transactionId) {
        if (VDBG) {
            Log.v(TAG, "onConfigCompleted: transactionId=" + transactionId);
        }

        TransactionInfoBase info = getAndRemovePendingResponseTransactionInfo(transactionId);
        if (info == null) {
            Log.e(TAG, "onConfigCompleted: no transaction info for transactionId=" + transactionId);
            return;
        }
        if (!(info instanceof TransactionInfoConfig)) {
            Log.e(TAG, "onConfigCompleted: invalid info structure stored for transactionId="
                    + transactionId);
            return;
        }
        TransactionInfoConfig infoConfig = (TransactionInfoConfig) info;

        if (DBG) {
            Log.d(TAG, "onConfigCompleted: request=" + infoConfig.mConfig);
        }

        for (int i = 0; i < mClients.size(); ++i) {
            WifiNanClientState client = mClients.valueAt(i);
            client.onConfigCompleted(infoConfig.mConfig);
        }
    }

    private void onConfigFailedLocal(short transactionId, int reason) {
        if (VDBG) {
            Log.v(TAG, "onEnableFailed: transactionId=" + transactionId + ", reason=" + reason);
        }

        TransactionInfoBase info = getAndRemovePendingResponseTransactionInfo(transactionId);
        if (info == null) {
            Log.e(TAG, "onConfigFailed: no transaction info for transactionId=" + transactionId);
            return;
        }
        if (!(info instanceof TransactionInfoConfig)) {
            Log.e(TAG, "onConfigCompleted: invalid info structure stored for transactionId="
                    + transactionId);
            return;
        }
        TransactionInfoConfig infoConfig = (TransactionInfoConfig) info;

        if (DBG) {
            Log.d(TAG, "onConfigFailed: request=" + infoConfig.mConfig);
        }

        for (int i = 0; i < mClients.size(); ++i) {
            WifiNanClientState client = mClients.valueAt(i);
            client.onConfigFailed(infoConfig.mConfig, reason);
        }
    }

    private void onNanDownLocal(int reason) {
        if (VDBG) {
            Log.v(TAG, "onNanDown: reason=" + reason);
        }

        int interested = 0;
        for (int i = 0; i < mClients.size(); ++i) {
            WifiNanClientState client = mClients.valueAt(i);
            interested += client.onNanDown(reason);
        }

        if (interested == 0) {
            Log.e(TAG, "onNanDown: event received but no listeners registered for this event "
                    + "- should be disabled from fw!");
        }
    }

    private void onInterfaceAddressChangeLocal(byte[] mac) {
        if (VDBG) {
            Log.v(TAG, "onInterfaceAddressChange: mac=" + String.valueOf(HexEncoding.encode(mac)));
        }

        int interested = 0;
        for (int i = 0; i < mClients.size(); ++i) {
            WifiNanClientState client = mClients.valueAt(i);
            interested += client.onInterfaceAddressChange(mac);
        }

        if (interested == 0) {
            Log.e(TAG, "onInterfaceAddressChange: event received but no listeners registered "
                    + "for this event - should be disabled from fw!");
        }
    }

    private void onClusterChangeLocal(int flag, byte[] clusterId) {
        if (VDBG) {
            Log.v(TAG, "onClusterChange: flag=" + flag + ", clusterId="
                    + String.valueOf(HexEncoding.encode(clusterId)));
        }

        int interested = 0;
        for (int i = 0; i < mClients.size(); ++i) {
            WifiNanClientState client = mClients.valueAt(i);
            interested += client.onClusterChange(flag, clusterId);
        }

        if (interested == 0) {
            Log.e(TAG, "onClusterChange: event received but no listeners registered for this "
                    + "event - should be disabled from fw!");
        }
    }

    private void onPublishSuccessLocal(short transactionId, int publishId) {
        if (VDBG) {
            Log.v(TAG, "onPublishSuccess: transactionId=" + transactionId + ", publishId="
                    + publishId);
        }

        TransactionInfoBase info = getAndRemovePendingResponseTransactionInfo(transactionId);
        if (info == null) {
            Log.e(TAG, "onPublishSuccess(): no info registered for transactionId=" + transactionId);
            return;
        }
        if (!(info instanceof TransactionInfoSession)) {
            Log.e(TAG, "onPublishSuccess: invalid info structure stored for transactionId="
                    + transactionId);
            return;
        }
        TransactionInfoSession infoSession = (TransactionInfoSession) info;

        infoSession.mSession.onPublishSuccess(publishId);
    }

    private void onPublishFailLocal(short transactionId, int status) {
        if (VDBG) {
            Log.v(TAG, "onPublishFail: transactionId=" + transactionId + ", status=" + status);
        }

        TransactionInfoBase info = getAndRemovePendingResponseTransactionInfo(transactionId);
        if (info == null) {
            Log.e(TAG, "onPublishFail(): no info registered for transactionId=" + transactionId);
            return;
        }
        if (!(info instanceof TransactionInfoSession)) {
            Log.e(TAG, "onPublishFail: invalid info structure stored for transactionId="
                    + transactionId);
            return;
        }
        TransactionInfoSession infoSession = (TransactionInfoSession) info;

        infoSession.mSession.onPublishFail(status);
    }

    private void onPublishTerminatedLocal(int publishId, int status) {
        if (VDBG) {
            Log.v(TAG, "onPublishTerminated: publishId=" + publishId + ", status=" + status);
        }

        WifiNanSessionState session = getNanSessionStateForPubSubId(publishId);
        if (session == null) {
            Log.e(TAG, "onPublishTerminated: no session found for publishId=" + publishId);
            return;
        }

        session.onPublishTerminated(status);
    }

    private void onSubscribeSuccessLocal(short transactionId, int subscribeId) {
        if (VDBG) {
            Log.v(TAG, "onSubscribeSuccess: transactionId=" + transactionId + ", subscribeId="
                    + subscribeId);
        }

        TransactionInfoBase info = getAndRemovePendingResponseTransactionInfo(transactionId);
        if (info == null) {
            Log.e(TAG,
                    "onSubscribeSuccess(): no info registered for transactionId=" + transactionId);
            return;
        }
        if (!(info instanceof TransactionInfoSession)) {
            Log.e(TAG, "onSubscribeSuccess: invalid info structure stored for transactionId="
                    + transactionId);
            return;
        }
        TransactionInfoSession infoSession = (TransactionInfoSession) info;

        infoSession.mSession.onSubscribeSuccess(subscribeId);
    }

    private void onSubscribeFailLocal(short transactionId, int status) {
        if (VDBG) {
            Log.v(TAG, "onSubscribeFail: transactionId=" + transactionId + ", status=" + status);
        }

        TransactionInfoBase info = getAndRemovePendingResponseTransactionInfo(transactionId);
        if (info == null) {
            Log.e(TAG, "onSubscribeFail(): no info registered for transactionId=" + transactionId);
            return;
        }
        if (!(info instanceof TransactionInfoSession)) {
            Log.e(TAG, "onSubscribeFail: invalid info structure stored for transactionId="
                    + transactionId);
            return;
        }
        TransactionInfoSession infoSession = (TransactionInfoSession) info;

        infoSession.mSession.onSubscribeFail(status);
    }

    private void onSubscribeTerminatedLocal(int subscribeId, int status) {
        if (VDBG) {
            Log.v(TAG, "onPublishTerminated: subscribeId=" + subscribeId + ", status=" + status);
        }

        WifiNanSessionState session = getNanSessionStateForPubSubId(subscribeId);
        if (session == null) {
            Log.e(TAG, "onSubscribeTerminated: no session found for subscribeId=" + subscribeId);
            return;
        }

        session.onSubscribeTerminated(status);
    }

    private void onMessageSendSuccessLocal(short transactionId) {
        if (VDBG) {
            Log.v(TAG, "onMessageSendSuccess: transactionId=" + transactionId);
        }

        TransactionInfoBase info = getAndRemovePendingResponseTransactionInfo(transactionId);
        if (info == null) {
            Log.e(TAG, "onMessageSendSuccess(): no info registered for transactionId="
                    + transactionId);
            return;
        }
        if (!(info instanceof TransactionInfoMessage)) {
            Log.e(TAG, "onMessageSendSuccess: invalid info structure stored for transactionId="
                    + transactionId);
            return;
        }
        TransactionInfoMessage infoMessage = (TransactionInfoMessage) info;

        infoMessage.mSession.onMessageSendSuccess(infoMessage.mMessageId);
    }

    private void onMessageSendFailLocal(short transactionId, int status) {
        if (VDBG) {
            Log.v(TAG, "onMessageSendFail: transactionId=" + transactionId + ", status=" + status);
        }

        TransactionInfoBase info = getAndRemovePendingResponseTransactionInfo(transactionId);
        if (info == null) {
            Log.e(TAG,
                    "onMessageSendFail(): no info registered for transactionId=" + transactionId);
            return;
        }
        if (!(info instanceof TransactionInfoMessage)) {
            Log.e(TAG, "onMessageSendFail: invalid info structure stored for transactionId="
                    + transactionId);
            return;
        }
        TransactionInfoMessage infoMessage = (TransactionInfoMessage) info;

        infoMessage.mSession.onMessageSendFail(infoMessage.mMessageId, status);
    }

    private void onUnknownTransactionLocal(int responseType, short transactionId, int status) {
        Log.e(TAG, "onUnknownTransaction: responseType=" + responseType + ", transactionId="
                + transactionId + ", status=" + status);

        TransactionInfoBase info = getAndRemovePendingResponseTransactionInfo(transactionId);
        if (info == null) {
            Log.e(TAG, "onUnknownTransaction(): no info registered for transactionId="
                    + transactionId);
        }
    }

    private void onMatchLocal(int pubSubId, int requestorInstanceId, byte[] peerMac,
            byte[] serviceSpecificInfo, int serviceSpecificInfoLength, byte[] matchFilter,
            int matchFilterLength) {
        if (VDBG) {
            Log.v(TAG, "onMatch: pubSubId=" + pubSubId + ", requestorInstanceId="
                    + requestorInstanceId + ", peerMac="
                    + String.valueOf(HexEncoding.encode(peerMac)) + ", serviceSpecificInfoLength="
                    + serviceSpecificInfoLength + ", serviceSpecificInfo=" + serviceSpecificInfo
                    + ", matchFilterLength=" + matchFilterLength + ", matchFilter=" + matchFilter);
        }

        WifiNanSessionState session = getNanSessionStateForPubSubId(pubSubId);
        if (session == null) {
            Log.e(TAG, "onMatch: no session found for pubSubId=" + pubSubId);
            return;
        }

        session.onMatch(requestorInstanceId, peerMac, serviceSpecificInfo,
                serviceSpecificInfoLength, matchFilter, matchFilterLength);
    }

    private void onMessageReceivedLocal(int pubSubId, int requestorInstanceId, byte[] peerMac,
            byte[] message, int messageLength) {
        if (VDBG) {
            Log.v(TAG,
                    "onMessageReceived: pubSubId=" + pubSubId + ", requestorInstanceId="
                            + requestorInstanceId + ", peerMac="
                            + String.valueOf(HexEncoding.encode(peerMac)) + ", messageLength="
                            + messageLength);
        }

        WifiNanSessionState session = getNanSessionStateForPubSubId(pubSubId);
        if (session == null) {
            Log.e(TAG, "onMessageReceived: no session found for pubSubId=" + pubSubId);
            return;
        }

        session.onMessageReceived(requestorInstanceId, peerMac, message, messageLength);
    }

    private ConfigRequest mergeConfigRequests() {
        if (VDBG) {
            Log.v(TAG, "mergeConfigRequests(): mClients=[" + mClients + "]");
        }

        if (mClients.size() == 0) {
            Log.e(TAG, "mergeConfigRequests: invalid state - called with 0 clients registered!");
            return null;
        }

        if (mClients.size() == 1) {
            return mClients.valueAt(0).getConfigRequest();
        }

        // TODO: continue working on merge algorithm:
        // - if any request 5g: enable
        // - maximal master preference
        // - cluster range covering all requests: assume that [0,max] is a
        // non-request
        boolean support5gBand = false;
        int masterPreference = 0;
        boolean clusterIdValid = false;
        int clusterLow = 0;
        int clusterHigh = ConfigRequest.CLUSTER_ID_MAX;
        for (int i = 0; i < mClients.size(); ++i) {
            ConfigRequest cr = mClients.valueAt(i).getConfigRequest();

            if (cr.mSupport5gBand) {
                support5gBand = true;
            }

            masterPreference = Math.max(masterPreference, cr.mMasterPreference);

            if (cr.mClusterLow != 0 || cr.mClusterHigh != ConfigRequest.CLUSTER_ID_MAX) {
                if (!clusterIdValid) {
                    clusterLow = cr.mClusterLow;
                    clusterHigh = cr.mClusterHigh;
                } else {
                    clusterLow = Math.min(clusterLow, cr.mClusterLow);
                    clusterHigh = Math.max(clusterHigh, cr.mClusterHigh);
                }
                clusterIdValid = true;
            }
        }
        ConfigRequest.Builder builder = new ConfigRequest.Builder();
        builder.setSupport5gBand(support5gBand).setMasterPreference(masterPreference)
                .setClusterLow(clusterLow).setClusterHigh(clusterHigh);

        return builder.build();
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("NanStateManager:");
        pw.println("  mClients: [" + mClients + "]");
        pw.println("  mPendingResponses: [" + mPendingResponses + "]");
        pw.println("  mCapabilities: [" + mCapabilities + "]");
        pw.println("  mNextTransactionId: " + mNextTransactionId);
        for (int i = 0; i < mClients.size(); ++i) {
            mClients.valueAt(i).dump(fd, pw, args);
        }
    }
}
