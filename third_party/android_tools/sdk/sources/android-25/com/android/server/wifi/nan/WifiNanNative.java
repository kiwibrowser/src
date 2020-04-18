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
import android.net.wifi.nan.PublishData;
import android.net.wifi.nan.PublishSettings;
import android.net.wifi.nan.SubscribeData;
import android.net.wifi.nan.SubscribeSettings;
import android.net.wifi.nan.WifiNanSessionListener;
import android.util.Log;

import com.android.server.wifi.WifiNative;

import libcore.util.HexEncoding;

/**
 * Native calls to access the Wi-Fi NAN HAL.
 *
 * Relies on WifiNative to perform the actual HAL registration.
 *
 * {@hide}
 */
public class WifiNanNative {
    private static final String TAG = "WifiNanNative";
    private static final boolean DBG = false;
    private static final boolean VDBG = false; // STOPSHIP if true

    private static final int WIFI_SUCCESS = 0;

    private static boolean sNanNativeInit = false;

    private static WifiNanNative sWifiNanNativeSingleton;

    private static native int registerNanNatives();

    public static WifiNanNative getInstance() {
        // dummy reference - used to make sure that WifiNative is loaded before
        // us since it is the one to load the shared library and starts its
        // initialization.
        WifiNative dummy = WifiNative.getWlanNativeInterface();
        if (dummy == null) {
            Log.w(TAG, "can't get access to WifiNative");
            return null;
        }

        if (sWifiNanNativeSingleton == null) {
            sWifiNanNativeSingleton = new WifiNanNative();
            registerNanNatives();
        }

        return sWifiNanNativeSingleton;
    }

    public static class Capabilities {
        public int maxConcurrentNanClusters;
        public int maxPublishes;
        public int maxSubscribes;
        public int maxServiceNameLen;
        public int maxMatchFilterLen;
        public int maxTotalMatchFilterLen;
        public int maxServiceSpecificInfoLen;
        public int maxVsaDataLen;
        public int maxMeshDataLen;
        public int maxNdiInterfaces;
        public int maxNdpSessions;
        public int maxAppInfoLen;

        @Override
        public String toString() {
            return "Capabilities [maxConcurrentNanClusters=" + maxConcurrentNanClusters
                    + ", maxPublishes=" + maxPublishes + ", maxSubscribes=" + maxSubscribes
                    + ", maxServiceNameLen=" + maxServiceNameLen + ", maxMatchFilterLen="
                    + maxMatchFilterLen + ", maxTotalMatchFilterLen=" + maxTotalMatchFilterLen
                    + ", maxServiceSpecificInfoLen=" + maxServiceSpecificInfoLen
                    + ", maxVsaDataLen=" + maxVsaDataLen + ", maxMeshDataLen=" + maxMeshDataLen
                    + ", maxNdiInterfaces=" + maxNdiInterfaces + ", maxNdpSessions="
                    + maxNdpSessions + ", maxAppInfoLen=" + maxAppInfoLen + "]";
        }
    }

    /* package */ static native int initNanHandlersNative(Object cls, int iface);

    private static native int getCapabilitiesNative(short transactionId, Object cls, int iface);

    private boolean isNanInit(boolean tryToInit) {
        if (!tryToInit || sNanNativeInit) {
            return sNanNativeInit;
        }

        if (DBG) Log.d(TAG, "isNanInit: trying to init");
        synchronized (WifiNative.sLock) {
            boolean halStarted = WifiNative.getWlanNativeInterface().isHalStarted();
            if (!halStarted) {
                halStarted = WifiNative.getWlanNativeInterface().startHal();
            }
            if (halStarted) {
                int ret = initNanHandlersNative(WifiNative.class, WifiNative.sWlan0Index);
                if (DBG) Log.d(TAG, "initNanHandlersNative: res=" + ret);
                sNanNativeInit = ret == WIFI_SUCCESS;

                if (sNanNativeInit) {
                    ret = getCapabilitiesNative(
                            WifiNanStateManager.getInstance().createNextTransactionId(),
                            WifiNative.class,
                            WifiNative.sWlan0Index);
                    if (DBG) Log.d(TAG, "getCapabilitiesNative: res=" + ret);
                }

                return sNanNativeInit;
            } else {
                Log.w(TAG, "isNanInit: HAL not initialized");
                return false;
            }
        }
    }

    private WifiNanNative() {
        // do nothing
    }

    private static native int enableAndConfigureNative(short transactionId, Object cls, int iface,
            ConfigRequest configRequest);

    public void enableAndConfigure(short transactionId, ConfigRequest configRequest) {
        boolean success;

        if (VDBG) Log.d(TAG, "enableAndConfigure: configRequest=" + configRequest);
        if (isNanInit(true)) {
            int ret;
            synchronized (WifiNative.sLock) {
                ret = enableAndConfigureNative(transactionId, WifiNative.class,
                        WifiNative.sWlan0Index, configRequest);
            }
            if (DBG) Log.d(TAG, "enableAndConfigureNative: ret=" + ret);
            success = ret == WIFI_SUCCESS;
        } else {
            Log.w(TAG, "enableAndConfigure: NanInit fails");
            success = false;
        }


        // TODO: do something on !success - send failure message back
    }

    private static native int disableNative(short transactionId, Object cls, int iface);

    public void disable(short transactionId) {
        boolean success;

        if (VDBG) Log.d(TAG, "disableNan");
        if (isNanInit(true)) {
            int ret;
            synchronized (WifiNative.sLock) {
                ret = disableNative(transactionId, WifiNative.class, WifiNative.sWlan0Index);
            }
            if (DBG) Log.d(TAG, "disableNative: ret=" + ret);
            success = ret == WIFI_SUCCESS;
        } else {
            Log.w(TAG, "disable: cannot initialize NAN");
            success = false;
        }

        // TODO: do something on !success - send failure message back
    }

    private static native int publishNative(short transactionId, int publishId, Object cls,
            int iface,
            PublishData publishData, PublishSettings publishSettings);

    public void publish(short transactionId, int publishId, PublishData publishData,
            PublishSettings publishSettings) {
        boolean success;

        if (VDBG) {
            Log.d(TAG, "publish: transactionId=" + transactionId + ",data='" + publishData
                    + "', settings=" + publishSettings);
        }

        if (isNanInit(true)) {
            int ret;
            synchronized (WifiNative.sLock) {
                ret = publishNative(transactionId, publishId, WifiNative.class,
                        WifiNative.sWlan0Index, publishData, publishSettings);
            }
            if (DBG) Log.d(TAG, "publishNative: ret=" + ret);
            success = ret == WIFI_SUCCESS;
        } else {
            Log.w(TAG, "publish: cannot initialize NAN");
            success = false;
        }

        // TODO: do something on !success - send failure message back
    }

    private static native int subscribeNative(short transactionId, int subscribeId, Object cls,
            int iface, SubscribeData subscribeData, SubscribeSettings subscribeSettings);

    public void subscribe(short transactionId, int subscribeId, SubscribeData subscribeData,
            SubscribeSettings subscribeSettings) {
        boolean success;

        if (VDBG) {
            Log.d(TAG, "subscribe: transactionId=" + transactionId + ", data='" + subscribeData
                    + "', settings=" + subscribeSettings);
        }

        if (isNanInit(true)) {
            int ret;
            synchronized (WifiNative.sLock) {
                ret = subscribeNative(transactionId, subscribeId, WifiNative.class,
                        WifiNative.sWlan0Index, subscribeData, subscribeSettings);
            }
            if (DBG) Log.d(TAG, "subscribeNative: ret=" + ret);
            success = ret == WIFI_SUCCESS;
        } else {
            Log.w(TAG, "subscribe: cannot initialize NAN");
            success = false;
        }

        // TODO: do something on !success - send failure message back
    }

    private static native int sendMessageNative(short transactionId, Object cls, int iface,
            int pubSubId, int requestorInstanceId, byte[] dest, byte[] message, int messageLength);

    public void sendMessage(short transactionId, int pubSubId, int requestorInstanceId, byte[] dest,
            byte[] message, int messageLength) {
        boolean success;

        if (VDBG) {
            Log.d(TAG,
                    "sendMessage: transactionId=" + transactionId + ", pubSubId=" + pubSubId
                            + ", requestorInstanceId=" + requestorInstanceId + ", dest="
                            + String.valueOf(HexEncoding.encode(dest)) + ", messageLength="
                            + messageLength);
        }

        if (isNanInit(true)) {
            int ret;
            synchronized (WifiNative.sLock) {
                ret = sendMessageNative(transactionId, WifiNative.class, WifiNative.sWlan0Index,
                        pubSubId, requestorInstanceId, dest, message, messageLength);
            }
            if (DBG) Log.d(TAG, "sendMessageNative: ret=" + ret);
            success = ret == WIFI_SUCCESS;
        } else {
            Log.w(TAG, "sendMessage: cannot initialize NAN");
            success = false;
        }

        // TODO: do something on !success - send failure message back
    }

    private static native int stopPublishNative(short transactionId, Object cls, int iface,
            int pubSubId);

    public void stopPublish(short transactionId, int pubSubId) {
        boolean success;

        if (VDBG) {
            Log.d(TAG, "stopPublish: transactionId=" + transactionId + ", pubSubId=" + pubSubId);
        }

        if (isNanInit(true)) {
            int ret;
            synchronized (WifiNative.sLock) {
                ret = stopPublishNative(transactionId, WifiNative.class, WifiNative.sWlan0Index,
                        pubSubId);
            }
            if (DBG) Log.d(TAG, "stopPublishNative: ret=" + ret);
            success = ret == WIFI_SUCCESS;
        } else {
            Log.w(TAG, "stopPublish: cannot initialize NAN");
            success = false;
        }

        // TODO: do something on !success - send failure message back
    }

    private static native int stopSubscribeNative(short transactionId, Object cls, int iface,
            int pubSubId);

    public void stopSubscribe(short transactionId, int pubSubId) {
        boolean success;

        if (VDBG) {
            Log.d(TAG, "stopSubscribe: transactionId=" + transactionId + ", pubSubId=" + pubSubId);
        }

        if (isNanInit(true)) {
            int ret;
            synchronized (WifiNative.sLock) {
                ret = stopSubscribeNative(transactionId, WifiNative.class, WifiNative.sWlan0Index,
                        pubSubId);
            }
            if (DBG) Log.d(TAG, "stopSubscribeNative: ret=" + ret);
            success = ret == WIFI_SUCCESS;
        } else {
            Log.w(TAG, "stopSubscribe: cannot initialize NAN");
            success = false;
        }

        // TODO: do something on !success - send failure message back
    }

    // EVENTS

    // NanResponseType for API responses: will add values as needed
    public static final int NAN_RESPONSE_ENABLED = 0;
    public static final int NAN_RESPONSE_PUBLISH = 2;
    public static final int NAN_RESPONSE_PUBLISH_CANCEL = 3;
    public static final int NAN_RESPONSE_TRANSMIT_FOLLOWUP = 4;
    public static final int NAN_RESPONSE_SUBSCRIBE = 5;
    public static final int NAN_RESPONSE_SUBSCRIBE_CANCEL = 6;
    public static final int NAN_RESPONSE_GET_CAPABILITIES = 12;

    // direct copy from wifi_nan.h: need to keep in sync
    public static final int NAN_STATUS_SUCCESS = 0;
    public static final int NAN_STATUS_TIMEOUT = 1;
    public static final int NAN_STATUS_DE_FAILURE = 2;
    public static final int NAN_STATUS_INVALID_MSG_VERSION = 3;
    public static final int NAN_STATUS_INVALID_MSG_LEN = 4;
    public static final int NAN_STATUS_INVALID_MSG_ID = 5;
    public static final int NAN_STATUS_INVALID_HANDLE = 6;
    public static final int NAN_STATUS_NO_SPACE_AVAILABLE = 7;
    public static final int NAN_STATUS_INVALID_PUBLISH_TYPE = 8;
    public static final int NAN_STATUS_INVALID_TX_TYPE = 9;
    public static final int NAN_STATUS_INVALID_MATCH_ALGORITHM = 10;
    public static final int NAN_STATUS_DISABLE_IN_PROGRESS = 11;
    public static final int NAN_STATUS_INVALID_TLV_LEN = 12;
    public static final int NAN_STATUS_INVALID_TLV_TYPE = 13;
    public static final int NAN_STATUS_MISSING_TLV_TYPE = 14;
    public static final int NAN_STATUS_INVALID_TOTAL_TLVS_LEN = 15;
    public static final int NAN_STATUS_INVALID_MATCH_HANDLE = 16;
    public static final int NAN_STATUS_INVALID_TLV_VALUE = 17;
    public static final int NAN_STATUS_INVALID_TX_PRIORITY = 18;
    public static final int NAN_STATUS_INVALID_CONNECTION_MAP = 19;

    // NAN Configuration Response codes
    public static final int NAN_STATUS_INVALID_RSSI_CLOSE_VALUE = 4096;
    public static final int NAN_STATUS_INVALID_RSSI_MIDDLE_VALUE = 4097;
    public static final int NAN_STATUS_INVALID_HOP_COUNT_LIMIT = 4098;
    public static final int NAN_STATUS_INVALID_MASTER_PREFERENCE_VALUE = 4099;
    public static final int NAN_STATUS_INVALID_LOW_CLUSTER_ID_VALUE = 4100;
    public static final int NAN_STATUS_INVALID_HIGH_CLUSTER_ID_VALUE = 4101;
    public static final int NAN_STATUS_INVALID_BACKGROUND_SCAN_PERIOD = 4102;
    public static final int NAN_STATUS_INVALID_RSSI_PROXIMITY_VALUE = 4103;
    public static final int NAN_STATUS_INVALID_SCAN_CHANNEL = 4104;
    public static final int NAN_STATUS_INVALID_POST_NAN_CONNECTIVITY_CAPABILITIES_BITMAP = 4105;
    public static final int NAN_STATUS_INVALID_FA_MAP_NUMCHAN_VALUE = 4106;
    public static final int NAN_STATUS_INVALID_FA_MAP_DURATION_VALUE = 4107;
    public static final int NAN_STATUS_INVALID_FA_MAP_CLASS_VALUE = 4108;
    public static final int NAN_STATUS_INVALID_FA_MAP_CHANNEL_VALUE = 4109;
    public static final int NAN_STATUS_INVALID_FA_MAP_AVAILABILITY_INTERVAL_BITMAP_VALUE = 4110;
    public static final int NAN_STATUS_INVALID_FA_MAP_MAP_ID = 4111;
    public static final int NAN_STATUS_INVALID_POST_NAN_DISCOVERY_CONN_TYPE_VALUE = 4112;
    public static final int NAN_STATUS_INVALID_POST_NAN_DISCOVERY_DEVICE_ROLE_VALUE = 4113;
    public static final int NAN_STATUS_INVALID_POST_NAN_DISCOVERY_DURATION_VALUE = 4114;
    public static final int NAN_STATUS_INVALID_POST_NAN_DISCOVERY_BITMAP_VALUE = 4115;
    public static final int NAN_STATUS_MISSING_FUTHER_AVAILABILITY_MAP = 4116;
    public static final int NAN_STATUS_INVALID_BAND_CONFIG_FLAGS = 4117;

    // publish/subscribe termination reasons
    public static final int NAN_TERMINATED_REASON_INVALID = 8192;
    public static final int NAN_TERMINATED_REASON_TIMEOUT = 8193;
    public static final int NAN_TERMINATED_REASON_USER_REQUEST = 8194;
    public static final int NAN_TERMINATED_REASON_FAILURE = 8195;
    public static final int NAN_TERMINATED_REASON_COUNT_REACHED = 8196;
    public static final int NAN_TERMINATED_REASON_DE_SHUTDOWN = 8197;
    public static final int NAN_TERMINATED_REASON_DISABLE_IN_PROGRESS = 8198;
    public static final int NAN_TERMINATED_REASON_POST_DISC_ATTR_EXPIRED = 8199;
    public static final int NAN_TERMINATED_REASON_POST_DISC_LEN_EXCEEDED = 8200;
    public static final int NAN_TERMINATED_REASON_FURTHER_AVAIL_MAP_EMPTY = 8201;

    private static int translateHalStatusToPublicStatus(int halStatus) {
        switch (halStatus) {
            case NAN_STATUS_NO_SPACE_AVAILABLE:
                return WifiNanSessionListener.FAIL_REASON_NO_RESOURCES;

            case NAN_STATUS_TIMEOUT:
            case NAN_STATUS_DE_FAILURE:
            case NAN_STATUS_DISABLE_IN_PROGRESS:
                return WifiNanSessionListener.FAIL_REASON_OTHER;

            case NAN_STATUS_INVALID_MSG_VERSION:
            case NAN_STATUS_INVALID_MSG_LEN:
            case NAN_STATUS_INVALID_MSG_ID:
            case NAN_STATUS_INVALID_HANDLE:
            case NAN_STATUS_INVALID_PUBLISH_TYPE:
            case NAN_STATUS_INVALID_TX_TYPE:
            case NAN_STATUS_INVALID_MATCH_ALGORITHM:
            case NAN_STATUS_INVALID_TLV_LEN:
            case NAN_STATUS_INVALID_TLV_TYPE:
            case NAN_STATUS_MISSING_TLV_TYPE:
            case NAN_STATUS_INVALID_TOTAL_TLVS_LEN:
            case NAN_STATUS_INVALID_MATCH_HANDLE:
            case NAN_STATUS_INVALID_TLV_VALUE:
            case NAN_STATUS_INVALID_TX_PRIORITY:
            case NAN_STATUS_INVALID_CONNECTION_MAP:
            case NAN_STATUS_INVALID_RSSI_CLOSE_VALUE:
            case NAN_STATUS_INVALID_RSSI_MIDDLE_VALUE:
            case NAN_STATUS_INVALID_HOP_COUNT_LIMIT:
            case NAN_STATUS_INVALID_MASTER_PREFERENCE_VALUE:
            case NAN_STATUS_INVALID_LOW_CLUSTER_ID_VALUE:
            case NAN_STATUS_INVALID_HIGH_CLUSTER_ID_VALUE:
            case NAN_STATUS_INVALID_BACKGROUND_SCAN_PERIOD:
            case NAN_STATUS_INVALID_RSSI_PROXIMITY_VALUE:
            case NAN_STATUS_INVALID_SCAN_CHANNEL:
            case NAN_STATUS_INVALID_POST_NAN_CONNECTIVITY_CAPABILITIES_BITMAP:
            case NAN_STATUS_INVALID_FA_MAP_NUMCHAN_VALUE:
            case NAN_STATUS_INVALID_FA_MAP_DURATION_VALUE:
            case NAN_STATUS_INVALID_FA_MAP_CLASS_VALUE:
            case NAN_STATUS_INVALID_FA_MAP_CHANNEL_VALUE:
            case NAN_STATUS_INVALID_FA_MAP_AVAILABILITY_INTERVAL_BITMAP_VALUE:
            case NAN_STATUS_INVALID_FA_MAP_MAP_ID:
            case NAN_STATUS_INVALID_POST_NAN_DISCOVERY_CONN_TYPE_VALUE:
            case NAN_STATUS_INVALID_POST_NAN_DISCOVERY_DEVICE_ROLE_VALUE:
            case NAN_STATUS_INVALID_POST_NAN_DISCOVERY_DURATION_VALUE:
            case NAN_STATUS_INVALID_POST_NAN_DISCOVERY_BITMAP_VALUE:
            case NAN_STATUS_MISSING_FUTHER_AVAILABILITY_MAP:
            case NAN_STATUS_INVALID_BAND_CONFIG_FLAGS:
                return WifiNanSessionListener.FAIL_REASON_INVALID_ARGS;

                // publish/subscribe termination reasons
            case NAN_TERMINATED_REASON_TIMEOUT:
            case NAN_TERMINATED_REASON_USER_REQUEST:
            case NAN_TERMINATED_REASON_COUNT_REACHED:
                return WifiNanSessionListener.TERMINATE_REASON_DONE;

            case NAN_TERMINATED_REASON_INVALID:
            case NAN_TERMINATED_REASON_FAILURE:
            case NAN_TERMINATED_REASON_DE_SHUTDOWN:
            case NAN_TERMINATED_REASON_DISABLE_IN_PROGRESS:
            case NAN_TERMINATED_REASON_POST_DISC_ATTR_EXPIRED:
            case NAN_TERMINATED_REASON_POST_DISC_LEN_EXCEEDED:
            case NAN_TERMINATED_REASON_FURTHER_AVAIL_MAP_EMPTY:
                return WifiNanSessionListener.TERMINATE_REASON_FAIL;
        }

        return WifiNanSessionListener.FAIL_REASON_OTHER;
    }

    // callback from native
    private static void onNanNotifyResponse(short transactionId, int responseType, int status,
            int value) {
        if (VDBG) {
            Log.v(TAG,
                    "onNanNotifyResponse: transactionId=" + transactionId + ", responseType="
                    + responseType + ", status=" + status + ", value=" + value);
        }

        switch (responseType) {
            case NAN_RESPONSE_ENABLED:
                if (status == NAN_STATUS_SUCCESS) {
                    WifiNanStateManager.getInstance().onConfigCompleted(transactionId);
                } else {
                    WifiNanStateManager.getInstance().onConfigFailed(transactionId,
                            translateHalStatusToPublicStatus(status));
                }
                break;
            case NAN_RESPONSE_PUBLISH_CANCEL:
                if (status != NAN_STATUS_SUCCESS) {
                    Log.e(TAG, "onNanNotifyResponse: NAN_RESPONSE_PUBLISH_CANCEL error - status="
                            + status + ", value=" + value);
                }
                break;
            case NAN_RESPONSE_TRANSMIT_FOLLOWUP:
                if (status == NAN_STATUS_SUCCESS) {
                    WifiNanStateManager.getInstance().onMessageSendSuccess(transactionId);
                } else {
                    WifiNanStateManager.getInstance().onMessageSendFail(transactionId,
                            translateHalStatusToPublicStatus(status));
                }
                break;
            case NAN_RESPONSE_SUBSCRIBE_CANCEL:
                if (status != NAN_STATUS_SUCCESS) {
                    Log.e(TAG, "onNanNotifyResponse: NAN_RESPONSE_PUBLISH_CANCEL error - status="
                            + status + ", value=" + value);
                }
                break;
            default:
                WifiNanStateManager.getInstance().onUnknownTransaction(responseType, transactionId,
                        translateHalStatusToPublicStatus(status));
                break;
        }
    }

    private static void onNanNotifyResponsePublishSubscribe(short transactionId, int responseType,
            int status, int value, int pubSubId) {
        if (VDBG) {
            Log.v(TAG,
                    "onNanNotifyResponsePublishSubscribe: transactionId=" + transactionId
                            + ", responseType=" + responseType + ", status=" + status + ", value="
                            + value + ", pubSubId=" + pubSubId);
        }

        switch (responseType) {
            case NAN_RESPONSE_PUBLISH:
                if (status == NAN_STATUS_SUCCESS) {
                    WifiNanStateManager.getInstance().onPublishSuccess(transactionId, pubSubId);
                } else {
                    WifiNanStateManager.getInstance().onPublishFail(transactionId,
                            translateHalStatusToPublicStatus(status));
                }
                break;
            case NAN_RESPONSE_SUBSCRIBE:
                if (status == NAN_STATUS_SUCCESS) {
                    WifiNanStateManager.getInstance().onSubscribeSuccess(transactionId, pubSubId);
                } else {
                    WifiNanStateManager.getInstance().onSubscribeFail(transactionId,
                            translateHalStatusToPublicStatus(status));
                }
                break;
            default:
                WifiNanStateManager.getInstance().onUnknownTransaction(responseType, transactionId,
                        translateHalStatusToPublicStatus(status));
                break;
        }
    }

    private static void onNanNotifyResponseCapabilities(short transactionId, int status, int value,
            Capabilities capabilities) {
        if (VDBG) {
            Log.v(TAG, "onNanNotifyResponsePublishSubscribe: transactionId=" + transactionId
                    + ", status=" + status + ", value=" + value + ", capabilities=" + capabilities);
        }

        if (status == NAN_STATUS_SUCCESS) {
            WifiNanStateManager.getInstance().onCapabilitiesUpdate(transactionId, capabilities);
        } else {
            Log.e(TAG,
                    "onNanNotifyResponseCapabilities: error status=" + status + ", value=" + value);
        }
    }

    public static final int NAN_EVENT_ID_DISC_MAC_ADDR = 0;
    public static final int NAN_EVENT_ID_STARTED_CLUSTER = 1;
    public static final int NAN_EVENT_ID_JOINED_CLUSTER = 2;

    // callback from native
    private static void onDiscoveryEngineEvent(int eventType, byte[] mac) {
        if (VDBG) {
            Log.v(TAG, "onDiscoveryEngineEvent: eventType=" + eventType + ", mac="
                    + String.valueOf(HexEncoding.encode(mac)));
        }

        if (eventType == NAN_EVENT_ID_DISC_MAC_ADDR) {
            WifiNanStateManager.getInstance().onInterfaceAddressChange(mac);
        } else if (eventType == NAN_EVENT_ID_STARTED_CLUSTER) {
            WifiNanStateManager.getInstance()
                    .onClusterChange(WifiNanClientState.CLUSTER_CHANGE_EVENT_STARTED, mac);
        } else if (eventType == NAN_EVENT_ID_JOINED_CLUSTER) {
            WifiNanStateManager.getInstance()
                    .onClusterChange(WifiNanClientState.CLUSTER_CHANGE_EVENT_JOINED, mac);
        } else {
            Log.w(TAG, "onDiscoveryEngineEvent: invalid eventType=" + eventType);
        }
    }

    // callback from native
    private static void onMatchEvent(int pubSubId, int requestorInstanceId, byte[] mac,
            byte[] serviceSpecificInfo, int serviceSpecificInfoLength, byte[] matchFilter,
            int matchFilterLength) {
        if (VDBG) {
            Log.v(TAG, "onMatchEvent: pubSubId=" + pubSubId + ", requestorInstanceId="
                    + requestorInstanceId + ", mac=" + String.valueOf(HexEncoding.encode(mac))
                    + ", serviceSpecificInfo=" + serviceSpecificInfo + ", matchFilterLength="
                    + matchFilterLength + ", matchFilter=" + matchFilter);
        }

        WifiNanStateManager.getInstance().onMatch(pubSubId, requestorInstanceId, mac,
                serviceSpecificInfo, serviceSpecificInfoLength, matchFilter, matchFilterLength);
    }

    // callback from native
    private static void onPublishTerminated(int publishId, int status) {
        if (VDBG) Log.v(TAG, "onPublishTerminated: publishId=" + publishId + ", status=" + status);

        WifiNanStateManager.getInstance().onPublishTerminated(publishId,
                translateHalStatusToPublicStatus(status));
    }

    // callback from native
    private static void onSubscribeTerminated(int subscribeId, int status) {
        if (VDBG) {
            Log.v(TAG, "onSubscribeTerminated: subscribeId=" + subscribeId + ", status=" + status);
        }

        WifiNanStateManager.getInstance().onSubscribeTerminated(subscribeId,
                translateHalStatusToPublicStatus(status));
    }

    // callback from native
    private static void onFollowupEvent(int pubSubId, int requestorInstanceId, byte[] mac,
            byte[] message, int messageLength) {
        if (VDBG) {
            Log.v(TAG, "onFollowupEvent: pubSubId=" + pubSubId + ", requestorInstanceId="
                    + requestorInstanceId + ", mac=" + String.valueOf(HexEncoding.encode(mac))
                    + ", messageLength=" + messageLength);
        }

        WifiNanStateManager.getInstance().onMessageReceived(pubSubId, requestorInstanceId, mac,
                message, messageLength);
    }

    // callback from native
    private static void onDisabledEvent(int status) {
        if (VDBG) Log.v(TAG, "onDisabledEvent: status=" + status);

        WifiNanStateManager.getInstance().onNanDown(translateHalStatusToPublicStatus(status));
    }
}
