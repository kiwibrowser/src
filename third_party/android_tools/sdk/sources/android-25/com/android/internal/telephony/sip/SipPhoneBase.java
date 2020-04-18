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

package com.android.internal.telephony.sip;

import android.content.Context;
import android.net.LinkProperties;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.RegistrantList;
import android.os.SystemProperties;
import android.telephony.CellLocation;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.telephony.Rlog;

import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.dataconnection.DataConnection;
import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.IccPhoneBookInterfaceManager;
import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.OperatorInfo;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneNotifier;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.UUSInfo;
import com.android.internal.telephony.uicc.IccFileHandler;

import java.util.ArrayList;
import java.util.List;

abstract class SipPhoneBase extends Phone {
    private static final String LOG_TAG = "SipPhoneBase";

    private RegistrantList mRingbackRegistrants = new RegistrantList();
    private PhoneConstants.State mState = PhoneConstants.State.IDLE;

    public SipPhoneBase(String name, Context context, PhoneNotifier notifier) {
        super(name, notifier, context, new SipCommandInterface(context), false);
    }

    @Override
    public abstract Call getForegroundCall();

    @Override
    public abstract Call getBackgroundCall();

    @Override
    public abstract Call getRingingCall();

    @Override
    public Connection dial(String dialString, UUSInfo uusInfo, int videoState, Bundle intentExtras)
            throws CallStateException {
        // ignore UUSInfo
        return dial(dialString, videoState);
    }

    void migrateFrom(SipPhoneBase from) {
        super.migrateFrom(from);
        migrate(mRingbackRegistrants, from.mRingbackRegistrants);
    }

    @Override
    public void registerForRingbackTone(Handler h, int what, Object obj) {
        mRingbackRegistrants.addUnique(h, what, obj);
    }

    @Override
    public void unregisterForRingbackTone(Handler h) {
        mRingbackRegistrants.remove(h);
    }

    @Override
    public void startRingbackTone() {
        AsyncResult result = new AsyncResult(null, Boolean.TRUE, null);
        mRingbackRegistrants.notifyRegistrants(result);
    }

    @Override
    public void stopRingbackTone() {
        AsyncResult result = new AsyncResult(null, Boolean.FALSE, null);
        mRingbackRegistrants.notifyRegistrants(result);
    }

    @Override
    public ServiceState getServiceState() {
        // FIXME: we may need to provide this when data connectivity is lost
        // or when server is down
        ServiceState s = new ServiceState();
        s.setVoiceRegState(ServiceState.STATE_IN_SERVICE);
        return s;
    }

    @Override
    public CellLocation getCellLocation() {
        return null;
    }

    @Override
    public PhoneConstants.State getState() {
        return mState;
    }

    @Override
    public int getPhoneType() {
        return PhoneConstants.PHONE_TYPE_SIP;
    }

    @Override
    public SignalStrength getSignalStrength() {
        return new SignalStrength();
    }

    @Override
    public boolean getMessageWaitingIndicator() {
        return false;
    }

    @Override
    public boolean getCallForwardingIndicator() {
        return false;
    }

    @Override
    public List<? extends MmiCode> getPendingMmiCodes() {
        return new ArrayList<MmiCode>(0);
    }

    @Override
    public PhoneConstants.DataState getDataConnectionState() {
        return PhoneConstants.DataState.DISCONNECTED;
    }

    @Override
    public PhoneConstants.DataState getDataConnectionState(String apnType) {
        return PhoneConstants.DataState.DISCONNECTED;
    }

    @Override
    public DataActivityState getDataActivityState() {
        return DataActivityState.NONE;
    }

    /**
     * SIP phones do not have a subscription id, so do not notify of specific phone state changes.
     */
    /* package */ void notifyPhoneStateChanged() {
        // Do nothing.
    }

    /**
     * Notify registrants of a change in the call state. This notifies changes in
     * {@link com.android.internal.telephony.Call.State}. Use this when changes
     * in the precise call state are needed, else use notifyPhoneStateChanged.
     */
    /* package */ void notifyPreciseCallStateChanged() {
        /* we'd love it if this was package-scoped*/
        super.notifyPreciseCallStateChangedP();
    }

    void notifyNewRingingConnection(Connection c) {
        super.notifyNewRingingConnectionP(c);
    }

    void notifyDisconnect(Connection cn) {
        mDisconnectRegistrants.notifyResult(cn);
    }

    void notifyUnknownConnection() {
        mUnknownConnectionRegistrants.notifyResult(this);
    }

    void notifySuppServiceFailed(SuppService code) {
        mSuppServiceFailedRegistrants.notifyResult(code);
    }

    void notifyServiceStateChanged(ServiceState ss) {
        super.notifyServiceStateChangedP(ss);
    }

    @Override
    public void notifyCallForwardingIndicator() {
        mNotifier.notifyCallForwardingChanged(this);
    }

    public boolean canDial() {
        int serviceState = getServiceState().getState();
        Rlog.v(LOG_TAG, "canDial(): serviceState = " + serviceState);
        if (serviceState == ServiceState.STATE_POWER_OFF) return false;

        String disableCall = SystemProperties.get(
                TelephonyProperties.PROPERTY_DISABLE_CALL, "false");
        Rlog.v(LOG_TAG, "canDial(): disableCall = " + disableCall);
        if (disableCall.equals("true")) return false;

        Rlog.v(LOG_TAG, "canDial(): ringingCall: " + getRingingCall().getState());
        Rlog.v(LOG_TAG, "canDial(): foregndCall: " + getForegroundCall().getState());
        Rlog.v(LOG_TAG, "canDial(): backgndCall: " + getBackgroundCall().getState());
        return !getRingingCall().isRinging()
                && (!getForegroundCall().getState().isAlive()
                    || !getBackgroundCall().getState().isAlive());
    }

    @Override
    public boolean handleInCallMmiCommands(String dialString) {
        return false;
    }

    boolean isInCall() {
        Call.State foregroundCallState = getForegroundCall().getState();
        Call.State backgroundCallState = getBackgroundCall().getState();
        Call.State ringingCallState = getRingingCall().getState();

       return (foregroundCallState.isAlive() || backgroundCallState.isAlive()
            || ringingCallState.isAlive());
    }

    @Override
    public boolean handlePinMmi(String dialString) {
        return false;
    }

    @Override
    public void sendUssdResponse(String ussdMessge) {
    }

    @Override
    public void registerForSuppServiceNotification(
            Handler h, int what, Object obj) {
    }

    @Override
    public void unregisterForSuppServiceNotification(Handler h) {
    }

    @Override
    public void setRadioPower(boolean power) {
    }

    @Override
    public String getVoiceMailNumber() {
        return null;
    }

    @Override
    public String getVoiceMailAlphaTag() {
        return null;
    }

    @Override
    public String getDeviceId() {
        return null;
    }

    @Override
    public String getDeviceSvn() {
        return null;
    }

    @Override
    public String getImei() {
        return null;
    }

    @Override
    public String getEsn() {
        Rlog.e(LOG_TAG, "[SipPhone] getEsn() is a CDMA method");
        return "0";
    }

    @Override
    public String getMeid() {
        Rlog.e(LOG_TAG, "[SipPhone] getMeid() is a CDMA method");
        return "0";
    }

    @Override
    public String getSubscriberId() {
        return null;
    }

    @Override
    public String getGroupIdLevel1() {
        return null;
    }

    @Override
    public String getGroupIdLevel2() {
        return null;
    }

    @Override
    public String getIccSerialNumber() {
        return null;
    }

    @Override
    public String getLine1Number() {
        return null;
    }

    @Override
    public String getLine1AlphaTag() {
        return null;
    }

    @Override
    public boolean setLine1Number(String alphaTag, String number, Message onComplete) {
        // FIXME: what to reply for SIP?
        return false;
    }

    @Override
    public void setVoiceMailNumber(String alphaTag, String voiceMailNumber,
            Message onComplete) {
        // FIXME: what to reply for SIP?
        AsyncResult.forMessage(onComplete, null, null);
        onComplete.sendToTarget();
    }

    @Override
    public void getCallForwardingOption(int commandInterfaceCFReason, Message onComplete) {
    }

    @Override
    public void setCallForwardingOption(int commandInterfaceCFAction,
            int commandInterfaceCFReason, String dialingNumber,
            int timerSeconds, Message onComplete) {
    }

    @Override
    public void getOutgoingCallerIdDisplay(Message onComplete) {
        // FIXME: what to reply?
        AsyncResult.forMessage(onComplete, null, null);
        onComplete.sendToTarget();
    }

    @Override
    public void setOutgoingCallerIdDisplay(int commandInterfaceCLIRMode,
                                           Message onComplete) {
        // FIXME: what's this for SIP?
        AsyncResult.forMessage(onComplete, null, null);
        onComplete.sendToTarget();
    }

    @Override
    public void getCallWaiting(Message onComplete) {
        AsyncResult.forMessage(onComplete, null, null);
        onComplete.sendToTarget();
    }

    @Override
    public void setCallWaiting(boolean enable, Message onComplete) {
        Rlog.e(LOG_TAG, "call waiting not supported");
    }

    @Override
    public boolean getIccRecordsLoaded() {
        return false;
    }

    @Override
    public IccCard getIccCard() {
        return null;
    }

    @Override
    public void getAvailableNetworks(Message response) {
    }

    @Override
    public void setNetworkSelectionModeAutomatic(Message response) {
    }

    @Override
    public void selectNetworkManually(OperatorInfo network, boolean persistSelection,
            Message response) {
    }

    @Override
    public void getNeighboringCids(Message response) {
    }

    @Override
    public void setOnPostDialCharacter(Handler h, int what, Object obj) {
    }

    @Override
    public void getDataCallList(Message response) {
    }

    public List<DataConnection> getCurrentDataConnectionList () {
        return null;
    }

    @Override
    public void updateServiceLocation() {
    }

    @Override
    public void enableLocationUpdates() {
    }

    @Override
    public void disableLocationUpdates() {
    }

    @Override
    public boolean getDataRoamingEnabled() {
        return false;
    }

    @Override
    public void setDataRoamingEnabled(boolean enable) {
    }

    @Override
    public boolean getDataEnabled() {
        return false;
    }

    @Override
    public void setDataEnabled(boolean enable) {
    }

    public boolean enableDataConnectivity() {
        return false;
    }

    public boolean disableDataConnectivity() {
        return false;
    }

    @Override
    public boolean isDataConnectivityPossible() {
        return false;
    }

    public void saveClirSetting(int commandInterfaceCLIRMode) {
    }

    @Override
    public IccPhoneBookInterfaceManager getIccPhoneBookInterfaceManager(){
        return null;
    }

    @Override
    public IccFileHandler getIccFileHandler(){
        return null;
    }

    @Override
    public void activateCellBroadcastSms(int activate, Message response) {
        Rlog.e(LOG_TAG, "Error! This functionality is not implemented for SIP.");
    }

    @Override
    public void getCellBroadcastSmsConfig(Message response) {
        Rlog.e(LOG_TAG, "Error! This functionality is not implemented for SIP.");
    }

    @Override
    public void setCellBroadcastSmsConfig(int[] configValuesArray, Message response){
        Rlog.e(LOG_TAG, "Error! This functionality is not implemented for SIP.");
    }

    //@Override
    @Override
    public boolean needsOtaServiceProvisioning() {
        // FIXME: what's this for SIP?
        return false;
    }

    //@Override
    @Override
    public LinkProperties getLinkProperties(String apnType) {
        // FIXME: what's this for SIP?
        return null;
    }

    /**
     * Determines if video calling is enabled.  Always {@code false} for SIP.
     *
     * @return {@code false} since SIP does not support video calling.
     */
    @Override
    public boolean isVideoEnabled() {
        return false;
    }

    void updatePhoneState() {
        PhoneConstants.State oldState = mState;

        if (getRingingCall().isRinging()) {
            mState = PhoneConstants.State.RINGING;
        } else if (getForegroundCall().isIdle()
                && getBackgroundCall().isIdle()) {
            mState = PhoneConstants.State.IDLE;
        } else {
            mState = PhoneConstants.State.OFFHOOK;
        }

        if (mState != oldState) {
            Rlog.d(LOG_TAG, " ^^^ new phone state: " + mState);
            notifyPhoneStateChanged();
        }
    }

    @Override
    protected void onUpdateIccAvailability() {
    }

    @Override
    public void sendEmergencyCallStateChange(boolean callActive) {
    }

    @Override
    public void setBroadcastEmergencyCallStateChanges(boolean broadcast) {
    }
}
