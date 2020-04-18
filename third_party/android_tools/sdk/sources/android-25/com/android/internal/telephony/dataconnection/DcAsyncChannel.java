/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.internal.telephony.dataconnection;

import com.android.internal.telephony.dataconnection.DataConnection.ConnectionParams;
import com.android.internal.telephony.dataconnection.DataConnection.DisconnectParams;
import com.android.internal.util.AsyncChannel;
import com.android.internal.util.Protocol;

import android.net.LinkProperties;
import android.net.NetworkCapabilities;
import android.net.ProxyInfo;
import android.os.Message;

/**
 * AsyncChannel to a DataConnection
 */
public class DcAsyncChannel extends AsyncChannel {
    private static final boolean DBG = false;
    private String mLogTag;

    private DataConnection mDc;
    private long mDcThreadId;

    public static final int BASE = Protocol.BASE_DATA_CONNECTION_AC;

    public static final int REQ_IS_INACTIVE = BASE + 0;
    public static final int RSP_IS_INACTIVE = BASE + 1;

    public static final int REQ_GET_CID = BASE + 2;
    public static final int RSP_GET_CID = BASE + 3;

    public static final int REQ_GET_APNSETTING = BASE + 4;
    public static final int RSP_GET_APNSETTING = BASE + 5;

    public static final int REQ_GET_LINK_PROPERTIES = BASE + 6;
    public static final int RSP_GET_LINK_PROPERTIES = BASE + 7;

    public static final int REQ_SET_LINK_PROPERTIES_HTTP_PROXY = BASE + 8;
    public static final int RSP_SET_LINK_PROPERTIES_HTTP_PROXY = BASE + 9;

    public static final int REQ_GET_NETWORK_CAPABILITIES = BASE + 10;
    public static final int RSP_GET_NETWORK_CAPABILITIES = BASE + 11;

    public static final int REQ_RESET = BASE + 12;
    public static final int RSP_RESET = BASE + 13;

    private static final int CMD_TO_STRING_COUNT = RSP_RESET - BASE + 1;
    private static String[] sCmdToString = new String[CMD_TO_STRING_COUNT];
    static {
        sCmdToString[REQ_IS_INACTIVE - BASE] = "REQ_IS_INACTIVE";
        sCmdToString[RSP_IS_INACTIVE - BASE] = "RSP_IS_INACTIVE";
        sCmdToString[REQ_GET_CID - BASE] = "REQ_GET_CID";
        sCmdToString[RSP_GET_CID - BASE] = "RSP_GET_CID";
        sCmdToString[REQ_GET_APNSETTING - BASE] = "REQ_GET_APNSETTING";
        sCmdToString[RSP_GET_APNSETTING - BASE] = "RSP_GET_APNSETTING";
        sCmdToString[REQ_GET_LINK_PROPERTIES - BASE] = "REQ_GET_LINK_PROPERTIES";
        sCmdToString[RSP_GET_LINK_PROPERTIES - BASE] = "RSP_GET_LINK_PROPERTIES";
        sCmdToString[REQ_SET_LINK_PROPERTIES_HTTP_PROXY - BASE] =
                "REQ_SET_LINK_PROPERTIES_HTTP_PROXY";
        sCmdToString[RSP_SET_LINK_PROPERTIES_HTTP_PROXY - BASE] =
                "RSP_SET_LINK_PROPERTIES_HTTP_PROXY";
        sCmdToString[REQ_GET_NETWORK_CAPABILITIES - BASE] = "REQ_GET_NETWORK_CAPABILITIES";
        sCmdToString[RSP_GET_NETWORK_CAPABILITIES - BASE] = "RSP_GET_NETWORK_CAPABILITIES";
        sCmdToString[REQ_RESET - BASE] = "REQ_RESET";
        sCmdToString[RSP_RESET - BASE] = "RSP_RESET";
    }

    // Convert cmd to string or null if unknown
    protected static String cmdToString(int cmd) {
        cmd -= BASE;
        if ((cmd >= 0) && (cmd < sCmdToString.length)) {
            return sCmdToString[cmd];
        } else {
            return AsyncChannel.cmdToString(cmd + BASE);
        }
    }

    /**
     * enum used to notify action taken or necessary to be
     * taken after the link property is changed.
     */
    public enum LinkPropertyChangeAction {
        NONE, CHANGED, RESET;

        public static LinkPropertyChangeAction fromInt(int value) {
            if (value == NONE.ordinal()) {
                return NONE;
            } else if (value == CHANGED.ordinal()) {
                return CHANGED;
            } else if (value == RESET.ordinal()) {
                return RESET;
            } else {
                throw new RuntimeException("LinkPropertyChangeAction.fromInt: bad value=" + value);
            }
        }
    }

    public DcAsyncChannel(DataConnection dc, String logTag) {
        mDc = dc;
        mDcThreadId = mDc.getHandler().getLooper().getThread().getId();
        mLogTag = logTag;
    }

    /**
     * Request if the state machine is in the inactive state.
     * Response {@link #rspIsInactive}
     */
    public void reqIsInactive() {
        sendMessage(REQ_IS_INACTIVE);
        if (DBG) log("reqIsInactive");
    }

    /**
     * Evaluate RSP_IS_INACTIVE.
     *
     * @return true if the state machine is in the inactive state.
     */
    public boolean rspIsInactive(Message response) {
        boolean retVal = response.arg1 == 1;
        if (DBG) log("rspIsInactive=" + retVal);
        return retVal;
    }

    /**
     * @return true if the state machine is in the inactive state
     * and can be used for a new connection.
     */
    public boolean isInactiveSync() {
        boolean value;
        if (isCallerOnDifferentThread()) {
            Message response = sendMessageSynchronously(REQ_IS_INACTIVE);
            if ((response != null) && (response.what == RSP_IS_INACTIVE)) {
                value = rspIsInactive(response);
            } else {
                log("rspIsInactive error response=" + response);
                value = false;
            }
        } else {
            value = mDc.getIsInactive();
        }
        return value;
    }

    /**
     * Request the Connection ID.
     * Response {@link #rspCid}
     */
    public void reqCid() {
        sendMessage(REQ_GET_CID);
        if (DBG) log("reqCid");
    }

    /**
     * Evaluate a RSP_GET_CID message and return the cid.
     *
     * @param response Message
     * @return connection id or -1 if an error
     */
    public int rspCid(Message response) {
        int retVal = response.arg1;
        if (DBG) log("rspCid=" + retVal);
        return retVal;
    }

    /**
     * @return connection id or -1 if an error
     */
    public int getCidSync() {
        int value;
        if (isCallerOnDifferentThread()) {
            Message response = sendMessageSynchronously(REQ_GET_CID);
            if ((response != null) && (response.what == RSP_GET_CID)) {
                value = rspCid(response);
            } else {
                log("rspCid error response=" + response);
                value = -1;
            }
        } else {
            value = mDc.getCid();
        }
        return value;
    }

    /**
     * Request the connections ApnSetting.
     * Response {@link #rspApnSetting}
     */
    public void reqApnSetting() {
        sendMessage(REQ_GET_APNSETTING);
        if (DBG) log("reqApnSetting");
    }

    /**
     * Evaluate a RSP_APN_SETTING message and return the ApnSetting.
     *
     * @param response Message
     * @return ApnSetting, maybe null
     */
    public ApnSetting rspApnSetting(Message response) {
        ApnSetting retVal = (ApnSetting) response.obj;
        if (DBG) log("rspApnSetting=" + retVal);
        return retVal;
    }

    /**
     * Get the connections ApnSetting.
     *
     * @return ApnSetting or null if an error
     */
    public ApnSetting getApnSettingSync() {
        ApnSetting value;
        if (isCallerOnDifferentThread()) {
            Message response = sendMessageSynchronously(REQ_GET_APNSETTING);
            if ((response != null) && (response.what == RSP_GET_APNSETTING)) {
                value = rspApnSetting(response);
            } else {
                log("getApnSetting error response=" + response);
                value = null;
            }
        } else {
            value = mDc.getApnSetting();
        }
        return value;
    }

    /**
     * Request the connections LinkProperties.
     * Response {@link #rspLinkProperties}
     */
    public void reqLinkProperties() {
        sendMessage(REQ_GET_LINK_PROPERTIES);
        if (DBG) log("reqLinkProperties");
    }

    /**
     * Evaluate RSP_GET_LINK_PROPERTIES
     *
     * @param response
     * @return LinkProperties, maybe null.
     */
    public LinkProperties rspLinkProperties(Message response) {
        LinkProperties retVal = (LinkProperties) response.obj;
        if (DBG) log("rspLinkProperties=" + retVal);
        return retVal;
    }

    /**
     * Get the connections LinkProperties.
     *
     * @return LinkProperties or null if an error
     */
    public LinkProperties getLinkPropertiesSync() {
        LinkProperties value;
        if (isCallerOnDifferentThread()) {
            Message response = sendMessageSynchronously(REQ_GET_LINK_PROPERTIES);
            if ((response != null) && (response.what == RSP_GET_LINK_PROPERTIES)) {
                value = rspLinkProperties(response);
            } else {
                log("getLinkProperties error response=" + response);
                value = null;
            }
        } else {
            value = mDc.getCopyLinkProperties();
        }
        return value;
    }

    /**
     * Request setting the connections LinkProperties.HttpProxy.
     * Response RSP_SET_LINK_PROPERTIES when complete.
     */
    public void reqSetLinkPropertiesHttpProxy(ProxyInfo proxy) {
        sendMessage(REQ_SET_LINK_PROPERTIES_HTTP_PROXY, proxy);
        if (DBG) log("reqSetLinkPropertiesHttpProxy proxy=" + proxy);
    }

    /**
     * Set the connections LinkProperties.HttpProxy
     */
    public void setLinkPropertiesHttpProxySync(ProxyInfo proxy) {
        if (isCallerOnDifferentThread()) {
            Message response =
                sendMessageSynchronously(REQ_SET_LINK_PROPERTIES_HTTP_PROXY, proxy);
            if ((response != null) && (response.what == RSP_SET_LINK_PROPERTIES_HTTP_PROXY)) {
                if (DBG) log("setLinkPropertiesHttpPoxy ok");
            } else {
                log("setLinkPropertiesHttpPoxy error response=" + response);
            }
        } else {
            mDc.setLinkPropertiesHttpProxy(proxy);
        }
    }

    /**
     * Request the connections NetworkCapabilities.
     * Response {@link #rspNetworkCapabilities}
     */
    public void reqNetworkCapabilities() {
        sendMessage(REQ_GET_NETWORK_CAPABILITIES);
        if (DBG) log("reqNetworkCapabilities");
    }

    /**
     * Evaluate RSP_GET_NETWORK_CAPABILITIES
     *
     * @param response
     * @return NetworkCapabilites, maybe null.
     */
    public NetworkCapabilities rspNetworkCapabilities(Message response) {
        NetworkCapabilities retVal = (NetworkCapabilities) response.obj;
        if (DBG) log("rspNetworkCapabilities=" + retVal);
        return retVal;
    }

    /**
     * Get the connections NetworkCapabilities.
     *
     * @return NetworkCapabilities or null if an error
     */
    public NetworkCapabilities getNetworkCapabilitiesSync() {
        NetworkCapabilities value;
        if (isCallerOnDifferentThread()) {
            Message response = sendMessageSynchronously(REQ_GET_NETWORK_CAPABILITIES);
            if ((response != null) && (response.what == RSP_GET_NETWORK_CAPABILITIES)) {
                value = rspNetworkCapabilities(response);
            } else {
                value = null;
            }
        } else {
            value = mDc.getCopyNetworkCapabilities();
        }
        return value;
    }

    /**
     * Response RSP_RESET when complete
     */
    public void reqReset() {
        sendMessage(REQ_RESET);
        if (DBG) log("reqReset");
    }

    /**
     * Bring up a connection to the apn and return an AsyncResult in onCompletedMsg.
     * Used for cellular networks that use Acesss Point Names (APN) such
     * as GSM networks.
     *
     * @param apnContext is the Access Point Name to bring up a connection to
     * @param profileId for the conneciton
     * @param onCompletedMsg is sent with its msg.obj as an AsyncResult object.
     *        With AsyncResult.userObj set to the original msg.obj,
     *        AsyncResult.result = FailCause and AsyncResult.exception = Exception().
     */
    public void bringUp(ApnContext apnContext, int profileId, int rilRadioTechnology,
                        Message onCompletedMsg, int connectionGeneration) {
        if (DBG) {
            log("bringUp: apnContext=" + apnContext + " onCompletedMsg=" + onCompletedMsg);
        }
        sendMessage(DataConnection.EVENT_CONNECT,
                new ConnectionParams(apnContext, profileId, rilRadioTechnology, onCompletedMsg,
                        connectionGeneration));
    }

    /**
     * Tear down the connection through the apn on the network.
     *
     * @param onCompletedMsg is sent with its msg.obj as an AsyncResult object.
     *        With AsyncResult.userObj set to the original msg.obj.
     */
    public void tearDown(ApnContext apnContext, String reason, Message onCompletedMsg) {
        if (DBG) {
            log("tearDown: apnContext=" + apnContext
                    + " reason=" + reason + " onCompletedMsg=" + onCompletedMsg);
        }
        sendMessage(DataConnection.EVENT_DISCONNECT,
                        new DisconnectParams(apnContext, reason, onCompletedMsg));
    }

    /**
     * Tear down the connection through the apn on the network.  Ignores refcount and
     * and always tears down.
     *
     * @param onCompletedMsg is sent with its msg.obj as an AsyncResult object.
     *        With AsyncResult.userObj set to the original msg.obj.
     */
    public void tearDownAll(String reason, Message onCompletedMsg) {
        if (DBG) log("tearDownAll: reason=" + reason + " onCompletedMsg=" + onCompletedMsg);
        sendMessage(DataConnection.EVENT_DISCONNECT_ALL,
                new DisconnectParams(null, reason, onCompletedMsg));
    }

    /**
     * @return connection id
     */
    public int getDataConnectionIdSync() {
        // Safe because this is owned by the caller.
        return mDc.getDataConnectionId();
    }

    @Override
    public String toString() {
        return mDc.getName();
    }

    private boolean isCallerOnDifferentThread() {
        long curThreadId = Thread.currentThread().getId();
        boolean value = mDcThreadId != curThreadId;
        if (DBG) log("isCallerOnDifferentThread: " + value);
        return value;
    }

    private void log(String s) {
        android.telephony.Rlog.d(mLogTag, "DataConnectionAc " + s);
    }

    public String[] getPcscfAddr() {
        return mDc.mPcscfAddr;
    }
}
