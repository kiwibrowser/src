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
import android.media.AudioManager;
import android.net.rtp.AudioGroup;
import android.net.sip.SipAudioCall;
import android.net.sip.SipErrorCode;
import android.net.sip.SipException;
import android.net.sip.SipManager;
import android.net.sip.SipProfile;
import android.net.sip.SipSession;
import android.os.AsyncResult;
import android.os.Message;
import android.telephony.DisconnectCause;
import android.telephony.PhoneNumberUtils;
import android.telephony.ServiceState;
import android.text.TextUtils;
import android.telephony.Rlog;

import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneNotifier;

import java.text.ParseException;
import java.util.List;
import java.util.regex.Pattern;

/**
 * {@hide}
 */
public class SipPhone extends SipPhoneBase {
    private static final String LOG_TAG = "SipPhone";
    private static final boolean DBG = true;
    private static final boolean VDBG = false; // STOPSHIP if true
    private static final int TIMEOUT_MAKE_CALL = 15; // in seconds
    private static final int TIMEOUT_ANSWER_CALL = 8; // in seconds
    private static final int TIMEOUT_HOLD_CALL = 15; // in seconds
    // Minimum time needed between hold/unhold requests.
    private static final long TIMEOUT_HOLD_PROCESSING = 1000; // ms

    // A call that is ringing or (call) waiting
    private SipCall mRingingCall = new SipCall();
    private SipCall mForegroundCall = new SipCall();
    private SipCall mBackgroundCall = new SipCall();

    private SipManager mSipManager;
    private SipProfile mProfile;

    private long mTimeOfLastValidHoldRequest = System.currentTimeMillis();

    SipPhone (Context context, PhoneNotifier notifier, SipProfile profile) {
        super("SIP:" + profile.getUriString(), context, notifier);

        if (DBG) log("new SipPhone: " + hidePii(profile.getUriString()));
        mRingingCall = new SipCall();
        mForegroundCall = new SipCall();
        mBackgroundCall = new SipCall();
        mProfile = profile;
        mSipManager = SipManager.newInstance(context);
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) return true;
        if (!(o instanceof SipPhone)) return false;
        SipPhone that = (SipPhone) o;
        return mProfile.getUriString().equals(that.mProfile.getUriString());
    }

    public String getSipUri() {
        return mProfile.getUriString();
    }

    public boolean equals(SipPhone phone) {
        return getSipUri().equals(phone.getSipUri());
    }

    public Connection takeIncomingCall(Object incomingCall) {
        // FIXME: Is synchronizing on the class necessary, should we use a mLockObj?
        // Also there are many things not synchronized, of course
        // this may be true of GsmCdmaPhone too!!!
        synchronized (SipPhone.class) {
            if (!(incomingCall instanceof SipAudioCall)) {
                if (DBG) log("takeIncomingCall: ret=null, not a SipAudioCall");
                return null;
            }
            if (mRingingCall.getState().isAlive()) {
                if (DBG) log("takeIncomingCall: ret=null, ringingCall not alive");
                return null;
            }

            // FIXME: is it true that we cannot take any incoming call if
            // both foreground and background are active
            if (mForegroundCall.getState().isAlive()
                    && mBackgroundCall.getState().isAlive()) {
                if (DBG) {
                    log("takeIncomingCall: ret=null," + " foreground and background both alive");
                }
                return null;
            }

            try {
                SipAudioCall sipAudioCall = (SipAudioCall) incomingCall;
                if (DBG) log("takeIncomingCall: taking call from: "
                        + sipAudioCall.getPeerProfile().getUriString());
                String localUri = sipAudioCall.getLocalProfile().getUriString();
                if (localUri.equals(mProfile.getUriString())) {
                    boolean makeCallWait = mForegroundCall.getState().isAlive();
                    SipConnection connection = mRingingCall.initIncomingCall(sipAudioCall,
                            makeCallWait);
                    if (sipAudioCall.getState() != SipSession.State.INCOMING_CALL) {
                        // Peer cancelled the call!
                        if (DBG) log("    takeIncomingCall: call cancelled !!");
                        mRingingCall.reset();
                        connection = null;
                    }
                    return connection;
                }
            } catch (Exception e) {
                // Peer may cancel the call at any time during the time we hook
                // up ringingCall with sipAudioCall. Clean up ringingCall when
                // that happens.
                if (DBG) log("    takeIncomingCall: exception e=" + e);
                mRingingCall.reset();
            }
            if (DBG) log("takeIncomingCall: NOT taking !!");
            return null;
        }
    }

    @Override
    public void acceptCall(int videoState) throws CallStateException {
        synchronized (SipPhone.class) {
            if ((mRingingCall.getState() == Call.State.INCOMING) ||
                    (mRingingCall.getState() == Call.State.WAITING)) {
                if (DBG) log("acceptCall: accepting");
                // Always unmute when answering a new call
                mRingingCall.setMute(false);
                mRingingCall.acceptCall();
            } else {
                if (DBG) {
                    log("acceptCall:" +
                        " throw CallStateException(\"phone not ringing\")");
                }
                throw new CallStateException("phone not ringing");
            }
        }
    }

    @Override
    public void rejectCall() throws CallStateException {
        synchronized (SipPhone.class) {
            if (mRingingCall.getState().isRinging()) {
                if (DBG) log("rejectCall: rejecting");
                mRingingCall.rejectCall();
            } else {
                if (DBG) {
                    log("rejectCall:" +
                        " throw CallStateException(\"phone not ringing\")");
                }
                throw new CallStateException("phone not ringing");
            }
        }
    }

    @Override
    public Connection dial(String dialString, int videoState) throws CallStateException {
        synchronized (SipPhone.class) {
            return dialInternal(dialString, videoState);
        }
    }

    private Connection dialInternal(String dialString, int videoState)
            throws CallStateException {
        if (DBG) log("dialInternal: dialString=" + hidePii(dialString));
        clearDisconnected();

        if (!canDial()) {
            throw new CallStateException("dialInternal: cannot dial in current state");
        }
        if (mForegroundCall.getState() == SipCall.State.ACTIVE) {
            switchHoldingAndActive();
        }
        if (mForegroundCall.getState() != SipCall.State.IDLE) {
            //we should have failed in !canDial() above before we get here
            throw new CallStateException("cannot dial in current state");
        }

        mForegroundCall.setMute(false);
        try {
            Connection c = mForegroundCall.dial(dialString);
            return c;
        } catch (SipException e) {
            loge("dialInternal: ", e);
            throw new CallStateException("dial error: " + e);
        }
    }

    @Override
    public void switchHoldingAndActive() throws CallStateException {
        // Wait for at least TIMEOUT_HOLD_PROCESSING ms to occur before sending hold/unhold requests
        // to prevent spamming the SipAudioCall state machine and putting it into an invalid state.
        if (!isHoldTimeoutExpired()) {
            if (DBG) log("switchHoldingAndActive: Disregarded! Under " + TIMEOUT_HOLD_PROCESSING +
                    " ms...");
            return;
        }
        if (DBG) log("switchHoldingAndActive: switch fg and bg");
        synchronized (SipPhone.class) {
            mForegroundCall.switchWith(mBackgroundCall);
            if (mBackgroundCall.getState().isAlive()) mBackgroundCall.hold();
            if (mForegroundCall.getState().isAlive()) mForegroundCall.unhold();
        }
    }

    @Override
    public boolean canConference() {
        if (DBG) log("canConference: ret=true");
        return true;
    }

    @Override
    public void conference() throws CallStateException {
        synchronized (SipPhone.class) {
            if ((mForegroundCall.getState() != SipCall.State.ACTIVE)
                    || (mForegroundCall.getState() != SipCall.State.ACTIVE)) {
                throw new CallStateException("wrong state to merge calls: fg="
                        + mForegroundCall.getState() + ", bg="
                        + mBackgroundCall.getState());
            }
            if (DBG) log("conference: merge fg & bg");
            mForegroundCall.merge(mBackgroundCall);
        }
    }

    public void conference(Call that) throws CallStateException {
        synchronized (SipPhone.class) {
            if (!(that instanceof SipCall)) {
                throw new CallStateException("expect " + SipCall.class
                        + ", cannot merge with " + that.getClass());
            }
            mForegroundCall.merge((SipCall) that);
        }
    }

    @Override
    public boolean canTransfer() {
        return false;
    }

    @Override
    public void explicitCallTransfer() {
        //mCT.explicitCallTransfer();
    }

    @Override
    public void clearDisconnected() {
        synchronized (SipPhone.class) {
            mRingingCall.clearDisconnected();
            mForegroundCall.clearDisconnected();
            mBackgroundCall.clearDisconnected();

            updatePhoneState();
            notifyPreciseCallStateChanged();
        }
    }

    @Override
    public void sendDtmf(char c) {
        if (!PhoneNumberUtils.is12Key(c)) {
            loge("sendDtmf called with invalid character '" + c + "'");
        } else if (mForegroundCall.getState().isAlive()) {
            synchronized (SipPhone.class) {
                mForegroundCall.sendDtmf(c);
            }
        }
    }

    @Override
    public void startDtmf(char c) {
        if (!PhoneNumberUtils.is12Key(c)) {
            loge("startDtmf called with invalid character '" + c + "'");
        } else {
            sendDtmf(c);
        }
    }

    @Override
    public void stopDtmf() {
        // no op
    }

    public void sendBurstDtmf(String dtmfString) {
        loge("sendBurstDtmf() is a CDMA method");
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
        // FIXME: what to reply?
        AsyncResult.forMessage(onComplete, null, null);
        onComplete.sendToTarget();
    }

    @Override
    public void setCallWaiting(boolean enable, Message onComplete) {
        // FIXME: what to reply?
        loge("call waiting not supported");
    }

    @Override
    public void setEchoSuppressionEnabled() {
        // Echo suppression may not be available on every device. So, check
        // whether it is supported
        synchronized (SipPhone.class) {
            AudioManager audioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
            String echoSuppression = audioManager.getParameters("ec_supported");
            if (echoSuppression.contains("off")) {
                mForegroundCall.setAudioGroupMode();
            }
        }
    }

    @Override
    public void setMute(boolean muted) {
        synchronized (SipPhone.class) {
            mForegroundCall.setMute(muted);
        }
    }

    @Override
    public boolean getMute() {
        return (mForegroundCall.getState().isAlive()
                ? mForegroundCall.getMute()
                : mBackgroundCall.getMute());
    }

    @Override
    public Call getForegroundCall() {
        return mForegroundCall;
    }

    @Override
    public Call getBackgroundCall() {
        return mBackgroundCall;
    }

    @Override
    public Call getRingingCall() {
        return mRingingCall;
    }

    @Override
    public ServiceState getServiceState() {
        // FIXME: we may need to provide this when data connectivity is lost
        // or when server is down
        return super.getServiceState();
    }

    private String getUriString(SipProfile p) {
        // SipProfile.getUriString() may contain "SIP:" and port
        return p.getUserName() + "@" + getSipDomain(p);
    }

    private String getSipDomain(SipProfile p) {
        String domain = p.getSipDomain();
        // TODO: move this to SipProfile
        if (domain.endsWith(":5060")) {
            return domain.substring(0, domain.length() - 5);
        } else {
            return domain;
        }
    }

    private static Call.State getCallStateFrom(SipAudioCall sipAudioCall) {
        if (sipAudioCall.isOnHold()) return Call.State.HOLDING;
        int sessionState = sipAudioCall.getState();
        switch (sessionState) {
            case SipSession.State.READY_TO_CALL:            return Call.State.IDLE;
            case SipSession.State.INCOMING_CALL:
            case SipSession.State.INCOMING_CALL_ANSWERING:  return Call.State.INCOMING;
            case SipSession.State.OUTGOING_CALL:            return Call.State.DIALING;
            case SipSession.State.OUTGOING_CALL_RING_BACK:  return Call.State.ALERTING;
            case SipSession.State.OUTGOING_CALL_CANCELING:  return Call.State.DISCONNECTING;
            case SipSession.State.IN_CALL:                  return Call.State.ACTIVE;
            default:
                slog("illegal connection state: " + sessionState);
                return Call.State.DISCONNECTED;
        }
    }

    private synchronized boolean isHoldTimeoutExpired() {
        long currTime = System.currentTimeMillis();
        if ((currTime - mTimeOfLastValidHoldRequest) > TIMEOUT_HOLD_PROCESSING) {
            mTimeOfLastValidHoldRequest = currTime;
            return true;
        }
        return false;
    }

    private void log(String s) {
        Rlog.d(LOG_TAG, s);
    }

    private static void slog(String s) {
        Rlog.d(LOG_TAG, s);
    }

    private void loge(String s) {
        Rlog.e(LOG_TAG, s);
    }

    private void loge(String s, Exception e) {
        Rlog.e(LOG_TAG, s, e);
    }

    private class SipCall extends SipCallBase {
        private static final String SC_TAG = "SipCall";
        private static final boolean SC_DBG = true;
        private static final boolean SC_VDBG = false; // STOPSHIP if true

        void reset() {
            if (SC_DBG) log("reset");
            mConnections.clear();
            setState(Call.State.IDLE);
        }

        void switchWith(SipCall that) {
            if (SC_DBG) log("switchWith");
            synchronized (SipPhone.class) {
                SipCall tmp = new SipCall();
                tmp.takeOver(this);
                this.takeOver(that);
                that.takeOver(tmp);
            }
        }

        private void takeOver(SipCall that) {
            if (SC_DBG) log("takeOver");
            mConnections = that.mConnections;
            mState = that.mState;
            for (Connection c : mConnections) {
                ((SipConnection) c).changeOwner(this);
            }
        }

        @Override
        public Phone getPhone() {
            return SipPhone.this;
        }

        @Override
        public List<Connection> getConnections() {
            if (SC_VDBG) log("getConnections");
            synchronized (SipPhone.class) {
                // FIXME should return Collections.unmodifiableList();
                return mConnections;
            }
        }

        Connection dial(String originalNumber) throws SipException {
            if (SC_DBG) log("dial: num=" + (SC_VDBG ? originalNumber : "xxx"));
            // TODO: Should this be synchronized?
            String calleeSipUri = originalNumber;
            if (!calleeSipUri.contains("@")) {
                String replaceStr = Pattern.quote(mProfile.getUserName() + "@");
                calleeSipUri = mProfile.getUriString().replaceFirst(replaceStr,
                        calleeSipUri + "@");
            }
            try {
                SipProfile callee =
                        new SipProfile.Builder(calleeSipUri).build();
                SipConnection c = new SipConnection(this, callee,
                        originalNumber);
                c.dial();
                mConnections.add(c);
                setState(Call.State.DIALING);
                return c;
            } catch (ParseException e) {
                throw new SipException("dial", e);
            }
        }

        @Override
        public void hangup() throws CallStateException {
            synchronized (SipPhone.class) {
                if (mState.isAlive()) {
                    if (SC_DBG) log("hangup: call " + getState()
                            + ": " + this + " on phone " + getPhone());
                    setState(State.DISCONNECTING);
                    CallStateException excp = null;
                    for (Connection c : mConnections) {
                        try {
                            c.hangup();
                        } catch (CallStateException e) {
                            excp = e;
                        }
                    }
                    if (excp != null) throw excp;
                } else {
                    if (SC_DBG) log("hangup: dead call " + getState()
                            + ": " + this + " on phone " + getPhone());
                }
            }
        }

        SipConnection initIncomingCall(SipAudioCall sipAudioCall, boolean makeCallWait) {
            SipProfile callee = sipAudioCall.getPeerProfile();
            SipConnection c = new SipConnection(this, callee);
            mConnections.add(c);

            Call.State newState = makeCallWait ? State.WAITING : State.INCOMING;
            c.initIncomingCall(sipAudioCall, newState);

            setState(newState);
            notifyNewRingingConnectionP(c);
            return c;
        }

        void rejectCall() throws CallStateException {
            if (SC_DBG) log("rejectCall:");
            hangup();
        }

        void acceptCall() throws CallStateException {
            if (SC_DBG) log("acceptCall: accepting");
            if (this != mRingingCall) {
                throw new CallStateException("acceptCall() in a non-ringing call");
            }
            if (mConnections.size() != 1) {
                throw new CallStateException("acceptCall() in a conf call");
            }
            ((SipConnection) mConnections.get(0)).acceptCall();
        }

        private boolean isSpeakerOn() {
            Boolean ret = ((AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE))
                    .isSpeakerphoneOn();
            if (SC_VDBG) log("isSpeakerOn: ret=" + ret);
            return ret;
        }

        void setAudioGroupMode() {
            AudioGroup audioGroup = getAudioGroup();
            if (audioGroup == null) {
                if (SC_DBG) log("setAudioGroupMode: audioGroup == null ignore");
                return;
            }
            int mode = audioGroup.getMode();
            if (mState == State.HOLDING) {
                audioGroup.setMode(AudioGroup.MODE_ON_HOLD);
            } else if (getMute()) {
                audioGroup.setMode(AudioGroup.MODE_MUTED);
            } else if (isSpeakerOn()) {
                audioGroup.setMode(AudioGroup.MODE_ECHO_SUPPRESSION);
            } else {
                audioGroup.setMode(AudioGroup.MODE_NORMAL);
            }
            if (SC_DBG) log(String.format(
                    "setAudioGroupMode change: %d --> %d", mode,
                    audioGroup.getMode()));
        }

        void hold() throws CallStateException {
            if (SC_DBG) log("hold:");
            setState(State.HOLDING);
            for (Connection c : mConnections) ((SipConnection) c).hold();
            setAudioGroupMode();
        }

        void unhold() throws CallStateException {
            if (SC_DBG) log("unhold:");
            setState(State.ACTIVE);
            AudioGroup audioGroup = new AudioGroup();
            for (Connection c : mConnections) {
                ((SipConnection) c).unhold(audioGroup);
            }
            setAudioGroupMode();
        }

        void setMute(boolean muted) {
            if (SC_DBG) log("setMute: muted=" + muted);
            for (Connection c : mConnections) {
                ((SipConnection) c).setMute(muted);
            }
        }

        boolean getMute() {
            boolean ret = mConnections.isEmpty()
                    ? false
                    : ((SipConnection) mConnections.get(0)).getMute();
            if (SC_DBG) log("getMute: ret=" + ret);
            return ret;
        }

        void merge(SipCall that) throws CallStateException {
            if (SC_DBG) log("merge:");
            AudioGroup audioGroup = getAudioGroup();

            // copy to an array to avoid concurrent modification as connections
            // in that.connections will be removed in add(SipConnection).
            Connection[] cc = that.mConnections.toArray(
                    new Connection[that.mConnections.size()]);
            for (Connection c : cc) {
                SipConnection conn = (SipConnection) c;
                add(conn);
                if (conn.getState() == Call.State.HOLDING) {
                    conn.unhold(audioGroup);
                }
            }
            that.setState(Call.State.IDLE);
        }

        private void add(SipConnection conn) {
            if (SC_DBG) log("add:");
            SipCall call = conn.getCall();
            if (call == this) return;
            if (call != null) call.mConnections.remove(conn);

            mConnections.add(conn);
            conn.changeOwner(this);
        }

        void sendDtmf(char c) {
            if (SC_DBG) log("sendDtmf: c=" + c);
            AudioGroup audioGroup = getAudioGroup();
            if (audioGroup == null) {
                if (SC_DBG) log("sendDtmf: audioGroup == null, ignore c=" + c);
                return;
            }
            audioGroup.sendDtmf(convertDtmf(c));
        }

        private int convertDtmf(char c) {
            int code = c - '0';
            if ((code < 0) || (code > 9)) {
                switch (c) {
                    case '*': return 10;
                    case '#': return 11;
                    case 'A': return 12;
                    case 'B': return 13;
                    case 'C': return 14;
                    case 'D': return 15;
                    default:
                        throw new IllegalArgumentException(
                                "invalid DTMF char: " + (int) c);
                }
            }
            return code;
        }

        @Override
        protected void setState(State newState) {
            if (mState != newState) {
                if (SC_DBG) log("setState: cur state" + mState
                        + " --> " + newState + ": " + this + ": on phone "
                        + getPhone() + " " + mConnections.size());

                if (newState == Call.State.ALERTING) {
                    mState = newState; // need in ALERTING to enable ringback
                    startRingbackTone();
                } else if (mState == Call.State.ALERTING) {
                    stopRingbackTone();
                }
                mState = newState;
                updatePhoneState();
                notifyPreciseCallStateChanged();
            }
        }

        void onConnectionStateChanged(SipConnection conn) {
            // this can be called back when a conf call is formed
            if (SC_DBG) log("onConnectionStateChanged: conn=" + conn);
            if (mState != State.ACTIVE) {
                setState(conn.getState());
            }
        }

        void onConnectionEnded(SipConnection conn) {
            // set state to DISCONNECTED only when all conns are disconnected
            if (SC_DBG) log("onConnectionEnded: conn=" + conn);
            if (mState != State.DISCONNECTED) {
                boolean allConnectionsDisconnected = true;
                if (SC_DBG) log("---check connections: "
                        + mConnections.size());
                for (Connection c : mConnections) {
                    if (SC_DBG) log("   state=" + c.getState() + ": "
                            + c);
                    if (c.getState() != State.DISCONNECTED) {
                        allConnectionsDisconnected = false;
                        break;
                    }
                }
                if (allConnectionsDisconnected) setState(State.DISCONNECTED);
            }
            notifyDisconnectP(conn);
        }

        private AudioGroup getAudioGroup() {
            if (mConnections.isEmpty()) return null;
            return ((SipConnection) mConnections.get(0)).getAudioGroup();
        }

        private void log(String s) {
            Rlog.d(SC_TAG, s);
        }
    }

    private class SipConnection extends SipConnectionBase {
        private static final String SCN_TAG = "SipConnection";
        private static final boolean SCN_DBG = true;

        private SipCall mOwner;
        private SipAudioCall mSipAudioCall;
        private Call.State mState = Call.State.IDLE;
        private SipProfile mPeer;
        private boolean mIncoming = false;
        private String mOriginalNumber; // may be a PSTN number

        private SipAudioCallAdapter mAdapter = new SipAudioCallAdapter() {
            @Override
            protected void onCallEnded(int cause) {
                if (getDisconnectCause() != DisconnectCause.LOCAL) {
                    setDisconnectCause(cause);
                }
                synchronized (SipPhone.class) {
                    setState(Call.State.DISCONNECTED);
                    SipAudioCall sipAudioCall = mSipAudioCall;
                    // FIXME: This goes null and is synchronized, but many uses aren't sync'd
                    mSipAudioCall = null;
                    String sessionState = (sipAudioCall == null)
                            ? ""
                            : (sipAudioCall.getState() + ", ");
                    if (SCN_DBG) log("[SipAudioCallAdapter] onCallEnded: "
                            + hidePii(mPeer.getUriString()) + ": " + sessionState
                            + "cause: " + getDisconnectCause() + ", on phone "
                            + getPhone());
                    if (sipAudioCall != null) {
                        sipAudioCall.setListener(null);
                        sipAudioCall.close();
                    }
                    mOwner.onConnectionEnded(SipConnection.this);
                }
            }

            @Override
            public void onCallEstablished(SipAudioCall call) {
                onChanged(call);
                // Race onChanged synchronized this isn't
                if (mState == Call.State.ACTIVE) call.startAudio();
            }

            @Override
            public void onCallHeld(SipAudioCall call) {
                onChanged(call);
                // Race onChanged synchronized this isn't
                if (mState == Call.State.HOLDING) call.startAudio();
            }

            @Override
            public void onChanged(SipAudioCall call) {
                synchronized (SipPhone.class) {
                    Call.State newState = getCallStateFrom(call);
                    if (mState == newState) return;
                    if (newState == Call.State.INCOMING) {
                        setState(mOwner.getState()); // INCOMING or WAITING
                    } else {
                        if (mOwner == mRingingCall) {
                            if (mRingingCall.getState() == Call.State.WAITING) {
                                try {
                                    switchHoldingAndActive();
                                } catch (CallStateException e) {
                                    // disconnect the call.
                                    onCallEnded(DisconnectCause.LOCAL);
                                    return;
                                }
                            }
                            mForegroundCall.switchWith(mRingingCall);
                        }
                        setState(newState);
                    }
                    mOwner.onConnectionStateChanged(SipConnection.this);
                    if (SCN_DBG) log("onChanged: "
                            + mPeer.getUriString() + ": " + mState
                            + " on phone " + getPhone());
                }
            }

            @Override
            protected void onError(int cause) {
                if (SCN_DBG) log("onError: " + cause);
                onCallEnded(cause);
            }
        };

        public SipConnection(SipCall owner, SipProfile callee,
                String originalNumber) {
            super(originalNumber);
            mOwner = owner;
            mPeer = callee;
            mOriginalNumber = originalNumber;
        }

        public SipConnection(SipCall owner, SipProfile callee) {
            this(owner, callee, getUriString(callee));
        }

        @Override
        public String getCnapName() {
            String displayName = mPeer.getDisplayName();
            return TextUtils.isEmpty(displayName) ? null
                                                  : displayName;
        }

        @Override
        public int getNumberPresentation() {
            return PhoneConstants.PRESENTATION_ALLOWED;
        }

        void initIncomingCall(SipAudioCall sipAudioCall, Call.State newState) {
            setState(newState);
            mSipAudioCall = sipAudioCall;
            sipAudioCall.setListener(mAdapter); // call back to set state
            mIncoming = true;
        }

        void acceptCall() throws CallStateException {
            try {
                mSipAudioCall.answerCall(TIMEOUT_ANSWER_CALL);
            } catch (SipException e) {
                throw new CallStateException("acceptCall(): " + e);
            }
        }

        void changeOwner(SipCall owner) {
            mOwner = owner;
        }

        AudioGroup getAudioGroup() {
            if (mSipAudioCall == null) return null;
            return mSipAudioCall.getAudioGroup();
        }

        void dial() throws SipException {
            setState(Call.State.DIALING);
            mSipAudioCall = mSipManager.makeAudioCall(mProfile, mPeer, null,
                    TIMEOUT_MAKE_CALL);
            mSipAudioCall.setListener(mAdapter);
        }

        void hold() throws CallStateException {
            setState(Call.State.HOLDING);
            try {
                mSipAudioCall.holdCall(TIMEOUT_HOLD_CALL);
            } catch (SipException e) {
                throw new CallStateException("hold(): " + e);
            }
        }

        void unhold(AudioGroup audioGroup) throws CallStateException {
            mSipAudioCall.setAudioGroup(audioGroup);
            setState(Call.State.ACTIVE);
            try {
                mSipAudioCall.continueCall(TIMEOUT_HOLD_CALL);
            } catch (SipException e) {
                throw new CallStateException("unhold(): " + e);
            }
        }

        void setMute(boolean muted) {
            if ((mSipAudioCall != null) && (muted != mSipAudioCall.isMuted())) {
                if (SCN_DBG) log("setState: prev muted=" + !muted + " new muted=" + muted);
                mSipAudioCall.toggleMute();
            }
        }

        boolean getMute() {
            return (mSipAudioCall == null) ? false
                                           : mSipAudioCall.isMuted();
        }

        @Override
        protected void setState(Call.State state) {
            if (state == mState) return;
            super.setState(state);
            mState = state;
        }

        @Override
        public Call.State getState() {
            return mState;
        }

        @Override
        public boolean isIncoming() {
            return mIncoming;
        }

        @Override
        public String getAddress() {
            // Phone app uses this to query caller ID. Return the original dial
            // number (which may be a PSTN number) instead of the peer's SIP
            // URI.
            return mOriginalNumber;
        }

        @Override
        public SipCall getCall() {
            return mOwner;
        }

        @Override
        protected Phone getPhone() {
            return mOwner.getPhone();
        }

        @Override
        public void hangup() throws CallStateException {
            synchronized (SipPhone.class) {
                if (SCN_DBG) log("hangup: conn=" + mPeer.getUriString()
                        + ": " + mState + ": on phone "
                        + getPhone().getPhoneName());
                if (!mState.isAlive()) return;
                try {
                    SipAudioCall sipAudioCall = mSipAudioCall;
                    if (sipAudioCall != null) {
                        sipAudioCall.setListener(null);
                        sipAudioCall.endCall();
                    }
                } catch (SipException e) {
                    throw new CallStateException("hangup(): " + e);
                } finally {
                    mAdapter.onCallEnded(((mState == Call.State.INCOMING)
                            || (mState == Call.State.WAITING))
                            ? DisconnectCause.INCOMING_REJECTED
                            : DisconnectCause.LOCAL);
                }
            }
        }

        @Override
        public void separate() throws CallStateException {
            synchronized (SipPhone.class) {
                SipCall call = (getPhone() == SipPhone.this)
                        ? (SipCall) getBackgroundCall()
                        : (SipCall) getForegroundCall();
                if (call.getState() != Call.State.IDLE) {
                    throw new CallStateException(
                            "cannot put conn back to a call in non-idle state: "
                            + call.getState());
                }
                if (SCN_DBG) log("separate: conn="
                        + mPeer.getUriString() + " from " + mOwner + " back to "
                        + call);

                // separate the AudioGroup and connection from the original call
                Phone originalPhone = getPhone();
                AudioGroup audioGroup = call.getAudioGroup(); // may be null
                call.add(this);
                mSipAudioCall.setAudioGroup(audioGroup);

                // put the original call to bg; and the separated call becomes
                // fg if it was in bg
                originalPhone.switchHoldingAndActive();

                // start audio and notify the phone app of the state change
                call = (SipCall) getForegroundCall();
                mSipAudioCall.startAudio();
                call.onConnectionStateChanged(this);
            }
        }

        private void log(String s) {
            Rlog.d(SCN_TAG, s);
        }
    }

    private abstract class SipAudioCallAdapter extends SipAudioCall.Listener {
        private static final String SACA_TAG = "SipAudioCallAdapter";
        private static final boolean SACA_DBG = true;
        /** Call ended with cause defined in {@link DisconnectCause}. */
        protected abstract void onCallEnded(int cause);
        /** Call failed with cause defined in {@link DisconnectCause}. */
        protected abstract void onError(int cause);

        @Override
        public void onCallEnded(SipAudioCall call) {
            if (SACA_DBG) log("onCallEnded: call=" + call);
            onCallEnded(call.isInCall()
                    ? DisconnectCause.NORMAL
                    : DisconnectCause.INCOMING_MISSED);
        }

        @Override
        public void onCallBusy(SipAudioCall call) {
            if (SACA_DBG) log("onCallBusy: call=" + call);
            onCallEnded(DisconnectCause.BUSY);
        }

        @Override
        public void onError(SipAudioCall call, int errorCode,
                String errorMessage) {
            if (SACA_DBG) {
                log("onError: call=" + call + " code="+ SipErrorCode.toString(errorCode)
                    + ": " + errorMessage);
            }
            switch (errorCode) {
                case SipErrorCode.SERVER_UNREACHABLE:
                    onError(DisconnectCause.SERVER_UNREACHABLE);
                    break;
                case SipErrorCode.PEER_NOT_REACHABLE:
                    onError(DisconnectCause.NUMBER_UNREACHABLE);
                    break;
                case SipErrorCode.INVALID_REMOTE_URI:
                    onError(DisconnectCause.INVALID_NUMBER);
                    break;
                case SipErrorCode.TIME_OUT:
                case SipErrorCode.TRANSACTION_TERMINTED:
                    onError(DisconnectCause.TIMED_OUT);
                    break;
                case SipErrorCode.DATA_CONNECTION_LOST:
                    onError(DisconnectCause.LOST_SIGNAL);
                    break;
                case SipErrorCode.INVALID_CREDENTIALS:
                    onError(DisconnectCause.INVALID_CREDENTIALS);
                    break;
                case SipErrorCode.CROSS_DOMAIN_AUTHENTICATION:
                    onError(DisconnectCause.OUT_OF_NETWORK);
                    break;
                case SipErrorCode.SERVER_ERROR:
                    onError(DisconnectCause.SERVER_ERROR);
                    break;
                case SipErrorCode.SOCKET_ERROR:
                case SipErrorCode.CLIENT_ERROR:
                default:
                    onError(DisconnectCause.ERROR_UNSPECIFIED);
            }
        }

        private void log(String s) {
            Rlog.d(SACA_TAG, s);
        }
    }

    public static String hidePii(String s) {
        return VDBG ? s : "xxxxx";
    }
}
