/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.internal.telephony;
import android.content.Context;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.PowerManager;
import android.os.Registrant;
import android.os.SystemClock;
import android.telephony.CarrierConfigManager;
import android.telephony.DisconnectCause;
import android.telephony.Rlog;
import android.telephony.PhoneNumberUtils;
import android.telephony.ServiceState;
import android.text.TextUtils;

import com.android.internal.telephony.cdma.CdmaCallWaitingNotification;
import com.android.internal.telephony.cdma.CdmaSubscriptionSourceManager;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppState;

/**
 * {@hide}
 */
public class GsmCdmaConnection extends Connection {
    private static final String LOG_TAG = "GsmCdmaConnection";
    private static final boolean DBG = true;
    private static final boolean VDBG = false;

    //***** Instance Variables

    GsmCdmaCallTracker mOwner;
    GsmCdmaCall mParent;

    boolean mDisconnected;

    int mIndex;          // index in GsmCdmaCallTracker.connections[], -1 if unassigned
                        // The GsmCdma index is 1 + this

    /*
     * These time/timespan values are based on System.currentTimeMillis(),
     * i.e., "wall clock" time.
     */
    long mDisconnectTime;

    UUSInfo mUusInfo;
    int mPreciseCause = 0;
    String mVendorCause;

    Connection mOrigConnection;

    Handler mHandler;

    private PowerManager.WakeLock mPartialWakeLock;

    private boolean mIsEmergencyCall = false;

    // The cached delay to be used between DTMF tones fetched from carrier config.
    private int mDtmfToneDelay = 0;

    //***** Event Constants
    static final int EVENT_DTMF_DONE = 1;
    static final int EVENT_PAUSE_DONE = 2;
    static final int EVENT_NEXT_POST_DIAL = 3;
    static final int EVENT_WAKE_LOCK_TIMEOUT = 4;
    static final int EVENT_DTMF_DELAY_DONE = 5;

    //***** Constants
    static final int PAUSE_DELAY_MILLIS_GSM = 3 * 1000;
    static final int PAUSE_DELAY_MILLIS_CDMA = 2 * 1000;
    static final int WAKE_LOCK_TIMEOUT_MILLIS = 60*1000;

    //***** Inner Classes

    class MyHandler extends Handler {
        MyHandler(Looper l) {super(l);}

        @Override
        public void
        handleMessage(Message msg) {

            switch (msg.what) {
                case EVENT_NEXT_POST_DIAL:
                case EVENT_DTMF_DELAY_DONE:
                case EVENT_PAUSE_DONE:
                    processNextPostDialChar();
                    break;
                case EVENT_WAKE_LOCK_TIMEOUT:
                    releaseWakeLock();
                    break;
                case EVENT_DTMF_DONE:
                    // We may need to add a delay specified by carrier between DTMF tones that are
                    // sent out.
                    mHandler.sendMessageDelayed(mHandler.obtainMessage(EVENT_DTMF_DELAY_DONE),
                            mDtmfToneDelay);
                    break;
            }
        }
    }

    //***** Constructors

    /** This is probably an MT call that we first saw in a CLCC response or a hand over. */
    public GsmCdmaConnection (GsmCdmaPhone phone, DriverCall dc, GsmCdmaCallTracker ct, int index) {
        super(phone.getPhoneType());
        createWakeLock(phone.getContext());
        acquireWakeLock();

        mOwner = ct;
        mHandler = new MyHandler(mOwner.getLooper());

        mAddress = dc.number;
        mIsEmergencyCall = PhoneNumberUtils.isLocalEmergencyNumber(phone.getContext(), mAddress);
        mIsIncoming = dc.isMT;
        mCreateTime = System.currentTimeMillis();
        mCnapName = dc.name;
        mCnapNamePresentation = dc.namePresentation;
        mNumberPresentation = dc.numberPresentation;
        mUusInfo = dc.uusInfo;

        mIndex = index;

        mParent = parentFromDCState(dc.state);
        mParent.attach(this, dc);

        fetchDtmfToneDelay(phone);
    }

    /** This is an MO call, created when dialing */
    public GsmCdmaConnection (GsmCdmaPhone phone, String dialString, GsmCdmaCallTracker ct,
                              GsmCdmaCall parent, boolean isEmergencyCall) {
        super(phone.getPhoneType());
        createWakeLock(phone.getContext());
        acquireWakeLock();

        mOwner = ct;
        mHandler = new MyHandler(mOwner.getLooper());

        if (isPhoneTypeGsm()) {
            mDialString = dialString;
        } else {
            Rlog.d(LOG_TAG, "[GsmCdmaConn] GsmCdmaConnection: dialString=" +
                    maskDialString(dialString));
            dialString = formatDialString(dialString);
            Rlog.d(LOG_TAG,
                    "[GsmCdmaConn] GsmCdmaConnection:formated dialString=" +
                            maskDialString(dialString));
        }

        mAddress = PhoneNumberUtils.extractNetworkPortionAlt(dialString);
        mIsEmergencyCall = isEmergencyCall;
        mPostDialString = PhoneNumberUtils.extractPostDialPortion(dialString);

        mIndex = -1;

        mIsIncoming = false;
        mCnapName = null;
        mCnapNamePresentation = PhoneConstants.PRESENTATION_ALLOWED;
        mNumberPresentation = PhoneConstants.PRESENTATION_ALLOWED;
        mCreateTime = System.currentTimeMillis();

        if (parent != null) {
            mParent = parent;
            if (isPhoneTypeGsm()) {
                parent.attachFake(this, GsmCdmaCall.State.DIALING);
            } else {
                //for the three way call case, not change parent state
                if (parent.mState == GsmCdmaCall.State.ACTIVE) {
                    parent.attachFake(this, GsmCdmaCall.State.ACTIVE);
                } else {
                    parent.attachFake(this, GsmCdmaCall.State.DIALING);
                }

            }
        }

        fetchDtmfToneDelay(phone);
    }

    //CDMA
    /** This is a Call waiting call*/
    public GsmCdmaConnection(Context context, CdmaCallWaitingNotification cw, GsmCdmaCallTracker ct,
                             GsmCdmaCall parent) {
        super(parent.getPhone().getPhoneType());
        createWakeLock(context);
        acquireWakeLock();

        mOwner = ct;
        mHandler = new MyHandler(mOwner.getLooper());
        mAddress = cw.number;
        mNumberPresentation = cw.numberPresentation;
        mCnapName = cw.name;
        mCnapNamePresentation = cw.namePresentation;
        mIndex = -1;
        mIsIncoming = true;
        mCreateTime = System.currentTimeMillis();
        mConnectTime = 0;
        mParent = parent;
        parent.attachFake(this, GsmCdmaCall.State.WAITING);
    }


    public void dispose() {
        clearPostDialListeners();
        if (mParent != null) {
            mParent.detach(this);
        }
        releaseAllWakeLocks();
    }

    static boolean
    equalsHandlesNulls (Object a, Object b) {
        return (a == null) ? (b == null) : a.equals (b);
    }

    //CDMA
    /**
     * format original dial string
     * 1) convert international dialing prefix "+" to
     *    string specified per region
     *
     * 2) handle corner cases for PAUSE/WAIT dialing:
     *
     *    If PAUSE/WAIT sequence at the end, ignore them.
     *
     *    If consecutive PAUSE/WAIT sequence in the middle of the string,
     *    and if there is any WAIT in PAUSE/WAIT sequence, treat them like WAIT.
     */
    public static String formatDialString(String phoneNumber) {
        /**
         * TODO(cleanup): This function should move to PhoneNumberUtils, and
         * tests should be added.
         */

        if (phoneNumber == null) {
            return null;
        }
        int length = phoneNumber.length();
        StringBuilder ret = new StringBuilder();
        char c;
        int currIndex = 0;

        while (currIndex < length) {
            c = phoneNumber.charAt(currIndex);
            if (isPause(c) || isWait(c)) {
                if (currIndex < length - 1) {
                    // if PW not at the end
                    int nextIndex = findNextPCharOrNonPOrNonWCharIndex(phoneNumber, currIndex);
                    // If there is non PW char following PW sequence
                    if (nextIndex < length) {
                        char pC = findPOrWCharToAppend(phoneNumber, currIndex, nextIndex);
                        ret.append(pC);
                        // If PW char sequence has more than 2 PW characters,
                        // skip to the last PW character since the sequence already be
                        // converted to WAIT character
                        if (nextIndex > (currIndex + 1)) {
                            currIndex = nextIndex - 1;
                        }
                    } else if (nextIndex == length) {
                        // It means PW characters at the end, ignore
                        currIndex = length - 1;
                    }
                }
            } else {
                ret.append(c);
            }
            currIndex++;
        }
        return PhoneNumberUtils.cdmaCheckAndProcessPlusCode(ret.toString());
    }

    /*package*/ boolean
    compareTo(DriverCall c) {
        // On mobile originated (MO) calls, the phone number may have changed
        // due to a SIM Toolkit call control modification.
        //
        // We assume we know when MO calls are created (since we created them)
        // and therefore don't need to compare the phone number anyway.
        if (! (mIsIncoming || c.isMT)) return true;

        // A new call appearing by SRVCC may have invalid number
        //  if IMS service is not tightly coupled with cellular modem stack.
        // Thus we prefer the preexisting handover connection instance.
        if (isPhoneTypeGsm() && mOrigConnection != null) return true;

        // ... but we can compare phone numbers on MT calls, and we have
        // no control over when they begin, so we might as well

        String cAddress = PhoneNumberUtils.stringFromStringAndTOA(c.number, c.TOA);
        return mIsIncoming == c.isMT && equalsHandlesNulls(mAddress, cAddress);
    }

    @Override
    public String getOrigDialString(){
        return mDialString;
    }

    @Override
    public GsmCdmaCall getCall() {
        return mParent;
    }

    @Override
    public long getDisconnectTime() {
        return mDisconnectTime;
    }

    @Override
    public long getHoldDurationMillis() {
        if (getState() != GsmCdmaCall.State.HOLDING) {
            // If not holding, return 0
            return 0;
        } else {
            return SystemClock.elapsedRealtime() - mHoldingStartTime;
        }
    }

    @Override
    public GsmCdmaCall.State getState() {
        if (mDisconnected) {
            return GsmCdmaCall.State.DISCONNECTED;
        } else {
            return super.getState();
        }
    }

    @Override
    public void hangup() throws CallStateException {
        if (!mDisconnected) {
            mOwner.hangup(this);
        } else {
            throw new CallStateException ("disconnected");
        }
    }

    @Override
    public void separate() throws CallStateException {
        if (!mDisconnected) {
            mOwner.separate(this);
        } else {
            throw new CallStateException ("disconnected");
        }
    }

    @Override
    public void proceedAfterWaitChar() {
        if (mPostDialState != PostDialState.WAIT) {
            Rlog.w(LOG_TAG, "GsmCdmaConnection.proceedAfterWaitChar(): Expected "
                    + "getPostDialState() to be WAIT but was " + mPostDialState);
            return;
        }

        setPostDialState(PostDialState.STARTED);

        processNextPostDialChar();
    }

    @Override
    public void proceedAfterWildChar(String str) {
        if (mPostDialState != PostDialState.WILD) {
            Rlog.w(LOG_TAG, "GsmCdmaConnection.proceedAfterWaitChar(): Expected "
                + "getPostDialState() to be WILD but was " + mPostDialState);
            return;
        }

        setPostDialState(PostDialState.STARTED);

        // make a new postDialString, with the wild char replacement string
        // at the beginning, followed by the remaining postDialString.

        StringBuilder buf = new StringBuilder(str);
        buf.append(mPostDialString.substring(mNextPostDialChar));
        mPostDialString = buf.toString();
        mNextPostDialChar = 0;
        if (Phone.DEBUG_PHONE) {
            log("proceedAfterWildChar: new postDialString is " +
                    mPostDialString);
        }

        processNextPostDialChar();
    }

    @Override
    public void cancelPostDial() {
        setPostDialState(PostDialState.CANCELLED);
    }

    /**
     * Called when this Connection is being hung up locally (eg, user pressed "end")
     * Note that at this point, the hangup request has been dispatched to the radio
     * but no response has yet been received so update() has not yet been called
     */
    void
    onHangupLocal() {
        mCause = DisconnectCause.LOCAL;
        mPreciseCause = 0;
        mVendorCause = null;
    }

    /**
     * Maps RIL call disconnect code to {@link DisconnectCause}.
     * @param causeCode RIL disconnect code
     * @return the corresponding value from {@link DisconnectCause}
     */
    int disconnectCauseFromCode(int causeCode) {
        /**
         * See 22.001 Annex F.4 for mapping of cause codes
         * to local tones
         */

        switch (causeCode) {
            case CallFailCause.USER_BUSY:
                return DisconnectCause.BUSY;

            case CallFailCause.NO_CIRCUIT_AVAIL:
            case CallFailCause.TEMPORARY_FAILURE:
            case CallFailCause.SWITCHING_CONGESTION:
            case CallFailCause.CHANNEL_NOT_AVAIL:
            case CallFailCause.QOS_NOT_AVAIL:
            case CallFailCause.BEARER_NOT_AVAIL:
                return DisconnectCause.CONGESTION;

            case CallFailCause.ACM_LIMIT_EXCEEDED:
                return DisconnectCause.LIMIT_EXCEEDED;

            case CallFailCause.CALL_BARRED:
                return DisconnectCause.CALL_BARRED;

            case CallFailCause.FDN_BLOCKED:
                return DisconnectCause.FDN_BLOCKED;

            case CallFailCause.UNOBTAINABLE_NUMBER:
                return DisconnectCause.UNOBTAINABLE_NUMBER;

            case CallFailCause.DIAL_MODIFIED_TO_USSD:
                return DisconnectCause.DIAL_MODIFIED_TO_USSD;

            case CallFailCause.DIAL_MODIFIED_TO_SS:
                return DisconnectCause.DIAL_MODIFIED_TO_SS;

            case CallFailCause.DIAL_MODIFIED_TO_DIAL:
                return DisconnectCause.DIAL_MODIFIED_TO_DIAL;

            case CallFailCause.CDMA_LOCKED_UNTIL_POWER_CYCLE:
                return DisconnectCause.CDMA_LOCKED_UNTIL_POWER_CYCLE;

            case CallFailCause.CDMA_DROP:
                return DisconnectCause.CDMA_DROP;

            case CallFailCause.CDMA_INTERCEPT:
                return DisconnectCause.CDMA_INTERCEPT;

            case CallFailCause.CDMA_REORDER:
                return DisconnectCause.CDMA_REORDER;

            case CallFailCause.CDMA_SO_REJECT:
                return DisconnectCause.CDMA_SO_REJECT;

            case CallFailCause.CDMA_RETRY_ORDER:
                return DisconnectCause.CDMA_RETRY_ORDER;

            case CallFailCause.CDMA_ACCESS_FAILURE:
                return DisconnectCause.CDMA_ACCESS_FAILURE;

            case CallFailCause.CDMA_PREEMPTED:
                return DisconnectCause.CDMA_PREEMPTED;

            case CallFailCause.CDMA_NOT_EMERGENCY:
                return DisconnectCause.CDMA_NOT_EMERGENCY;

            case CallFailCause.CDMA_ACCESS_BLOCKED:
                return DisconnectCause.CDMA_ACCESS_BLOCKED;

            case CallFailCause.ERROR_UNSPECIFIED:
            case CallFailCause.NORMAL_CLEARING:
            default:
                GsmCdmaPhone phone = mOwner.getPhone();
                int serviceState = phone.getServiceState().getState();
                UiccCardApplication cardApp = phone.getUiccCardApplication();
                AppState uiccAppState = (cardApp != null) ? cardApp.getState() :
                        AppState.APPSTATE_UNKNOWN;
                if (serviceState == ServiceState.STATE_POWER_OFF) {
                    return DisconnectCause.POWER_OFF;
                }
                if (!mIsEmergencyCall) {
                    // Only send OUT_OF_SERVICE if it is not an emergency call. We can still
                    // technically be in STATE_OUT_OF_SERVICE or STATE_EMERGENCY_ONLY during
                    // an emergency call and when it ends, we do not want to mistakenly generate
                    // an OUT_OF_SERVICE disconnect cause during normal call ending.
                    if ((serviceState == ServiceState.STATE_OUT_OF_SERVICE
                            || serviceState == ServiceState.STATE_EMERGENCY_ONLY)) {
                        return DisconnectCause.OUT_OF_SERVICE;
                    }
                    // If we are placing an emergency call and the SIM is currently PIN/PUK
                    // locked the AppState will always not be equal to APPSTATE_READY.
                    if (uiccAppState != AppState.APPSTATE_READY) {
                        if (isPhoneTypeGsm()) {
                            return DisconnectCause.ICC_ERROR;
                        } else { // CDMA
                            if (phone.mCdmaSubscriptionSource ==
                                    CdmaSubscriptionSourceManager.SUBSCRIPTION_FROM_RUIM) {
                                return DisconnectCause.ICC_ERROR;
                            }
                        }
                    }
                }
                if (isPhoneTypeGsm()) {
                    if (causeCode == CallFailCause.ERROR_UNSPECIFIED) {
                        if (phone.mSST.mRestrictedState.isCsRestricted()) {
                            return DisconnectCause.CS_RESTRICTED;
                        } else if (phone.mSST.mRestrictedState.isCsEmergencyRestricted()) {
                            return DisconnectCause.CS_RESTRICTED_EMERGENCY;
                        } else if (phone.mSST.mRestrictedState.isCsNormalRestricted()) {
                            return DisconnectCause.CS_RESTRICTED_NORMAL;
                        }
                    }
                }
                if (causeCode == CallFailCause.NORMAL_CLEARING) {
                    return DisconnectCause.NORMAL;
                }
                // If nothing else matches, report unknown call drop reason
                // to app, not NORMAL call end.
                return DisconnectCause.ERROR_UNSPECIFIED;
        }
    }

    /*package*/ void
    onRemoteDisconnect(int causeCode, String vendorCause) {
        this.mPreciseCause = causeCode;
        this.mVendorCause = vendorCause;
        onDisconnect(disconnectCauseFromCode(causeCode));
    }

    /**
     * Called when the radio indicates the connection has been disconnected.
     * @param cause call disconnect cause; values are defined in {@link DisconnectCause}
     */
    @Override
    public boolean onDisconnect(int cause) {
        boolean changed = false;

        mCause = cause;

        if (!mDisconnected) {
            doDisconnect();

            if (DBG) Rlog.d(LOG_TAG, "onDisconnect: cause=" + cause);

            mOwner.getPhone().notifyDisconnect(this);

            if (mParent != null) {
                changed = mParent.connectionDisconnected(this);
            }

            mOrigConnection = null;
        }
        clearPostDialListeners();
        releaseWakeLock();
        return changed;
    }

    //CDMA
    /** Called when the call waiting connection has been hung up */
    /*package*/ void
    onLocalDisconnect() {
        if (!mDisconnected) {
            doDisconnect();
            if (VDBG) Rlog.d(LOG_TAG, "onLoalDisconnect" );

            if (mParent != null) {
                mParent.detach(this);
            }
        }
        releaseWakeLock();
    }

    // Returns true if state has changed, false if nothing changed
    public boolean
    update (DriverCall dc) {
        GsmCdmaCall newParent;
        boolean changed = false;
        boolean wasConnectingInOrOut = isConnectingInOrOut();
        boolean wasHolding = (getState() == GsmCdmaCall.State.HOLDING);

        newParent = parentFromDCState(dc.state);

        if (Phone.DEBUG_PHONE) log("parent= " +mParent +", newParent= " + newParent);

        //Ignore dc.number and dc.name in case of a handover connection
        if (isPhoneTypeGsm() && mOrigConnection != null) {
            if (Phone.DEBUG_PHONE) log("update: mOrigConnection is not null");
        } else {
            log(" mNumberConverted " + mNumberConverted);
            if (!equalsHandlesNulls(mAddress, dc.number) && (!mNumberConverted
                    || !equalsHandlesNulls(mConvertedNumber, dc.number))) {
                if (Phone.DEBUG_PHONE) log("update: phone # changed!");
                mAddress = dc.number;
                changed = true;
            }
        }

        // A null cnapName should be the same as ""
        if (TextUtils.isEmpty(dc.name)) {
            if (!TextUtils.isEmpty(mCnapName)) {
                changed = true;
                mCnapName = "";
            }
        } else if (!dc.name.equals(mCnapName)) {
            changed = true;
            mCnapName = dc.name;
        }

        if (Phone.DEBUG_PHONE) log("--dssds----"+mCnapName);
        mCnapNamePresentation = dc.namePresentation;
        mNumberPresentation = dc.numberPresentation;

        if (newParent != mParent) {
            if (mParent != null) {
                mParent.detach(this);
            }
            newParent.attach(this, dc);
            mParent = newParent;
            changed = true;
        } else {
            boolean parentStateChange;
            parentStateChange = mParent.update (this, dc);
            changed = changed || parentStateChange;
        }

        /** Some state-transition events */

        if (Phone.DEBUG_PHONE) log(
                "update: parent=" + mParent +
                ", hasNewParent=" + (newParent != mParent) +
                ", wasConnectingInOrOut=" + wasConnectingInOrOut +
                ", wasHolding=" + wasHolding +
                ", isConnectingInOrOut=" + isConnectingInOrOut() +
                ", changed=" + changed);


        if (wasConnectingInOrOut && !isConnectingInOrOut()) {
            onConnectedInOrOut();
        }

        if (changed && !wasHolding && (getState() == GsmCdmaCall.State.HOLDING)) {
            // We've transitioned into HOLDING
            onStartedHolding();
        }

        return changed;
    }

    /**
     * Called when this Connection is in the foregroundCall
     * when a dial is initiated.
     * We know we're ACTIVE, and we know we're going to end up
     * HOLDING in the backgroundCall
     */
    void
    fakeHoldBeforeDial() {
        if (mParent != null) {
            mParent.detach(this);
        }

        mParent = mOwner.mBackgroundCall;
        mParent.attachFake(this, GsmCdmaCall.State.HOLDING);

        onStartedHolding();
    }

    /*package*/ int
    getGsmCdmaIndex() throws CallStateException {
        if (mIndex >= 0) {
            return mIndex + 1;
        } else {
            throw new CallStateException ("GsmCdma index not yet assigned");
        }
    }

    /**
     * An incoming or outgoing call has connected
     */
    void
    onConnectedInOrOut() {
        mConnectTime = System.currentTimeMillis();
        mConnectTimeReal = SystemClock.elapsedRealtime();
        mDuration = 0;

        // bug #678474: incoming call interpreted as missed call, even though
        // it sounds like the user has picked up the call.
        if (Phone.DEBUG_PHONE) {
            log("onConnectedInOrOut: connectTime=" + mConnectTime);
        }

        if (!mIsIncoming) {
            // outgoing calls only
            processNextPostDialChar();
        } else {
            // Only release wake lock for incoming calls, for outgoing calls the wake lock
            // will be released after any pause-dial is completed
            releaseWakeLock();
        }
    }

    private void
    doDisconnect() {
        mIndex = -1;
        mDisconnectTime = System.currentTimeMillis();
        mDuration = SystemClock.elapsedRealtime() - mConnectTimeReal;
        mDisconnected = true;
        clearPostDialListeners();
    }

    /*package*/ void
    onStartedHolding() {
        mHoldingStartTime = SystemClock.elapsedRealtime();
    }

    /**
     * Performs the appropriate action for a post-dial char, but does not
     * notify application. returns false if the character is invalid and
     * should be ignored
     */
    private boolean
    processPostDialChar(char c) {
        if (PhoneNumberUtils.is12Key(c)) {
            mOwner.mCi.sendDtmf(c, mHandler.obtainMessage(EVENT_DTMF_DONE));
        } else if (isPause(c)) {
            if (!isPhoneTypeGsm()) {
                setPostDialState(PostDialState.PAUSE);
            }
            // From TS 22.101:
            // It continues...
            // Upon the called party answering the UE shall send the DTMF digits
            // automatically to the network after a delay of 3 seconds( 20 ).
            // The digits shall be sent according to the procedures and timing
            // specified in 3GPP TS 24.008 [13]. The first occurrence of the
            // "DTMF Control Digits Separator" shall be used by the ME to
            // distinguish between the addressing digits (i.e. the phone number)
            // and the DTMF digits. Upon subsequent occurrences of the
            // separator,
            // the UE shall pause again for 3 seconds ( 20 ) before sending
            // any further DTMF digits.
            mHandler.sendMessageDelayed(mHandler.obtainMessage(EVENT_PAUSE_DONE),
                    isPhoneTypeGsm() ? PAUSE_DELAY_MILLIS_GSM: PAUSE_DELAY_MILLIS_CDMA);
        } else if (isWait(c)) {
            setPostDialState(PostDialState.WAIT);
        } else if (isWild(c)) {
            setPostDialState(PostDialState.WILD);
        } else {
            return false;
        }

        return true;
    }

    @Override
    public String
    getRemainingPostDialString() {
        String subStr = super.getRemainingPostDialString();
        if (!isPhoneTypeGsm() && !TextUtils.isEmpty(subStr)) {
            int wIndex = subStr.indexOf(PhoneNumberUtils.WAIT);
            int pIndex = subStr.indexOf(PhoneNumberUtils.PAUSE);

            if (wIndex > 0 && (wIndex < pIndex || pIndex <= 0)) {
                subStr = subStr.substring(0, wIndex);
            } else if (pIndex > 0) {
                subStr = subStr.substring(0, pIndex);
            }
        }
        return subStr;
    }

    //CDMA
    public void updateParent(GsmCdmaCall oldParent, GsmCdmaCall newParent){
        if (newParent != oldParent) {
            if (oldParent != null) {
                oldParent.detach(this);
            }
            newParent.attachFake(this, GsmCdmaCall.State.ACTIVE);
            mParent = newParent;
        }
    }

    @Override
    protected void finalize()
    {
        /**
         * It is understood that This finializer is not guaranteed
         * to be called and the release lock call is here just in
         * case there is some path that doesn't call onDisconnect
         * and or onConnectedInOrOut.
         */
        if (mPartialWakeLock.isHeld()) {
            Rlog.e(LOG_TAG, "[GsmCdmaConn] UNEXPECTED; mPartialWakeLock is held when finalizing.");
        }
        clearPostDialListeners();
        releaseWakeLock();
    }

    private void
    processNextPostDialChar() {
        char c = 0;
        Registrant postDialHandler;

        if (mPostDialState == PostDialState.CANCELLED) {
            releaseWakeLock();
            return;
        }

        if (mPostDialString == null ||
                mPostDialString.length() <= mNextPostDialChar) {
            setPostDialState(PostDialState.COMPLETE);

            // We were holding a wake lock until pause-dial was complete, so give it up now
            releaseWakeLock();

            // notifyMessage.arg1 is 0 on complete
            c = 0;
        } else {
            boolean isValid;

            setPostDialState(PostDialState.STARTED);

            c = mPostDialString.charAt(mNextPostDialChar++);

            isValid = processPostDialChar(c);

            if (!isValid) {
                // Will call processNextPostDialChar
                mHandler.obtainMessage(EVENT_NEXT_POST_DIAL).sendToTarget();
                // Don't notify application
                Rlog.e(LOG_TAG, "processNextPostDialChar: c=" + c + " isn't valid!");
                return;
            }
        }

        notifyPostDialListenersNextChar(c);

        // TODO: remove the following code since the handler no longer executes anything.
        postDialHandler = mOwner.getPhone().getPostDialHandler();

        Message notifyMessage;

        if (postDialHandler != null
                && (notifyMessage = postDialHandler.messageForRegistrant()) != null) {
            // The AsyncResult.result is the Connection object
            PostDialState state = mPostDialState;
            AsyncResult ar = AsyncResult.forMessage(notifyMessage);
            ar.result = this;
            ar.userObj = state;

            // arg1 is the character that was/is being processed
            notifyMessage.arg1 = c;

            //Rlog.v("GsmCdma", "##### processNextPostDialChar: send msg to postDialHandler, arg1=" + c);
            notifyMessage.sendToTarget();
        }
    }

    /** "connecting" means "has never been ACTIVE" for both incoming
     *  and outgoing calls
     */
    private boolean
    isConnectingInOrOut() {
        return mParent == null || mParent == mOwner.mRingingCall
            || mParent.mState == GsmCdmaCall.State.DIALING
            || mParent.mState == GsmCdmaCall.State.ALERTING;
    }

    private GsmCdmaCall
    parentFromDCState (DriverCall.State state) {
        switch (state) {
            case ACTIVE:
            case DIALING:
            case ALERTING:
                return mOwner.mForegroundCall;
            //break;

            case HOLDING:
                return mOwner.mBackgroundCall;
            //break;

            case INCOMING:
            case WAITING:
                return mOwner.mRingingCall;
            //break;

            default:
                throw new RuntimeException("illegal call state: " + state);
        }
    }

    /**
     * Set post dial state and acquire wake lock while switching to "started" or "pause"
     * state, the wake lock will be released if state switches out of "started" or "pause"
     * state or after WAKE_LOCK_TIMEOUT_MILLIS.
     * @param s new PostDialState
     */
    private void setPostDialState(PostDialState s) {
        if (s == PostDialState.STARTED ||
                s == PostDialState.PAUSE) {
            synchronized (mPartialWakeLock) {
                if (mPartialWakeLock.isHeld()) {
                    mHandler.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
                } else {
                    acquireWakeLock();
                }
                Message msg = mHandler.obtainMessage(EVENT_WAKE_LOCK_TIMEOUT);
                mHandler.sendMessageDelayed(msg, WAKE_LOCK_TIMEOUT_MILLIS);
            }
        } else {
            mHandler.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
            releaseWakeLock();
        }
        mPostDialState = s;
        notifyPostDialListeners();
    }

    private void
    createWakeLock(Context context) {
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mPartialWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, LOG_TAG);
    }

    private void
    acquireWakeLock() {
        log("acquireWakeLock");
        mPartialWakeLock.acquire();
    }

    private void
    releaseWakeLock() {
        synchronized(mPartialWakeLock) {
            if (mPartialWakeLock.isHeld()) {
                log("releaseWakeLock");
                mPartialWakeLock.release();
            }
        }
    }

    private void
    releaseAllWakeLocks() {
        synchronized(mPartialWakeLock) {
            while (mPartialWakeLock.isHeld()) {
                mPartialWakeLock.release();
            }
        }
    }

    private static boolean isPause(char c) {
        return c == PhoneNumberUtils.PAUSE;
    }

    private static boolean isWait(char c) {
        return c == PhoneNumberUtils.WAIT;
    }

    private static boolean isWild(char c) {
        return c == PhoneNumberUtils.WILD;
    }

    //CDMA
    // This function is to find the next PAUSE character index if
    // multiple pauses in a row. Otherwise it finds the next non PAUSE or
    // non WAIT character index.
    private static int
    findNextPCharOrNonPOrNonWCharIndex(String phoneNumber, int currIndex) {
        boolean wMatched = isWait(phoneNumber.charAt(currIndex));
        int index = currIndex + 1;
        int length = phoneNumber.length();
        while (index < length) {
            char cNext = phoneNumber.charAt(index);
            // if there is any W inside P/W sequence,mark it
            if (isWait(cNext)) {
                wMatched = true;
            }
            // if any characters other than P/W chars after P/W sequence
            // we break out the loop and append the correct
            if (!isWait(cNext) && !isPause(cNext)) {
                break;
            }
            index++;
        }

        // It means the PAUSE character(s) is in the middle of dial string
        // and it needs to be handled one by one.
        if ((index < length) && (index > (currIndex + 1))  &&
                ((wMatched == false) && isPause(phoneNumber.charAt(currIndex)))) {
            return (currIndex + 1);
        }
        return index;
    }

    //CDMA
    // This function returns either PAUSE or WAIT character to append.
    // It is based on the next non PAUSE/WAIT character in the phoneNumber and the
    // index for the current PAUSE/WAIT character
    private static char
    findPOrWCharToAppend(String phoneNumber, int currPwIndex, int nextNonPwCharIndex) {
        char c = phoneNumber.charAt(currPwIndex);
        char ret;

        // Append the PW char
        ret = (isPause(c)) ? PhoneNumberUtils.PAUSE : PhoneNumberUtils.WAIT;

        // If the nextNonPwCharIndex is greater than currPwIndex + 1,
        // it means the PW sequence contains not only P characters.
        // Since for the sequence that only contains P character,
        // the P character is handled one by one, the nextNonPwCharIndex
        // equals to currPwIndex + 1.
        // In this case, skip P, append W.
        if (nextNonPwCharIndex > (currPwIndex + 1)) {
            ret = PhoneNumberUtils.WAIT;
        }
        return ret;
    }

    private String maskDialString(String dialString) {
        if (VDBG) {
            return dialString;
        }

        return "<MASKED>";
    }

    private void fetchDtmfToneDelay(GsmCdmaPhone phone) {
        CarrierConfigManager configMgr = (CarrierConfigManager)
                phone.getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
        PersistableBundle b = configMgr.getConfigForSubId(phone.getSubId());
        if (b != null) {
            mDtmfToneDelay = b.getInt(phone.getDtmfToneDelayKey());
        }
    }

    private boolean isPhoneTypeGsm() {
        return mOwner.getPhone().getPhoneType() == PhoneConstants.PHONE_TYPE_GSM;
    }

    private void log(String msg) {
        Rlog.d(LOG_TAG, "[GsmCdmaConn] " + msg);
    }

    @Override
    public int getNumberPresentation() {
        return mNumberPresentation;
    }

    @Override
    public UUSInfo getUUSInfo() {
        return mUusInfo;
    }

    public int getPreciseDisconnectCause() {
        return mPreciseCause;
    }

    @Override
    public String getVendorDisconnectCause() {
        return mVendorCause;
    }

    @Override
    public void migrateFrom(Connection c) {
        if (c == null) return;

        super.migrateFrom(c);

        this.mUusInfo = c.getUUSInfo();

        this.setUserData(c.getUserData());
    }

    @Override
    public Connection getOrigConnection() {
        return mOrigConnection;
    }

    @Override
    public boolean isMultiparty() {
        if (mOrigConnection != null) {
            return mOrigConnection.isMultiparty();
        }

        return false;
    }
}
