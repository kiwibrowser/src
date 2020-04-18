/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.internal.telephony.test;

import android.os.Looper;
import android.os.Message;
import android.os.Handler;
import android.telephony.PhoneNumberUtils;
import com.android.internal.telephony.ATParseEx;
import com.android.internal.telephony.DriverCall;
import java.util.List;
import java.util.ArrayList;

import android.telephony.Rlog;

class CallInfo {
    enum State {
        ACTIVE(0),
        HOLDING(1),
        DIALING(2),    // MO call only
        ALERTING(3),   // MO call only
        INCOMING(4),   // MT call only
        WAITING(5);    // MT call only

        State(int value) {mValue = value;}

        private final int mValue;
        public int value() {return mValue;}
    }

    boolean mIsMT;
    State mState;
    boolean mIsMpty;
    String mNumber;
    int mTOA;

    CallInfo (boolean isMT, State state, boolean isMpty, String number) {
        mIsMT = isMT;
        mState = state;
        mIsMpty = isMpty;
        mNumber = number;

        if (number.length() > 0 && number.charAt(0) == '+') {
            mTOA = PhoneNumberUtils.TOA_International;
        } else {
            mTOA = PhoneNumberUtils.TOA_Unknown;
        }
    }

    static CallInfo
    createOutgoingCall(String number) {
        return new CallInfo (false, State.DIALING, false, number);
    }

    static CallInfo
    createIncomingCall(String number) {
        return new CallInfo (true, State.INCOMING, false, number);
    }

    String
    toCLCCLine(int index) {
        return
            "+CLCC: "
            + index + "," + (mIsMT ? "1" : "0") +","
            + mState.value() + ",0," + (mIsMpty ? "1" : "0")
            + ",\"" + mNumber + "\"," + mTOA;
    }

    DriverCall
    toDriverCall(int index) {
        DriverCall ret;

        ret = new DriverCall();

        ret.index = index;
        ret.isMT = mIsMT;

        try {
            ret.state = DriverCall.stateFromCLCC(mState.value());
        } catch (ATParseEx ex) {
            throw new RuntimeException("should never happen", ex);
        }

        ret.isMpty = mIsMpty;
        ret.number = mNumber;
        ret.TOA = mTOA;
        ret.isVoice = true;
        ret.als = 0;

        return ret;
    }


    boolean
    isActiveOrHeld() {
        return mState == State.ACTIVE || mState == State.HOLDING;
    }

    boolean
    isConnecting() {
        return mState == State.DIALING || mState == State.ALERTING;
    }

    boolean
    isRinging() {
        return mState == State.INCOMING || mState == State.WAITING;
    }

}

class InvalidStateEx extends Exception {
    InvalidStateEx() {

    }
}


class SimulatedGsmCallState extends Handler {
    //***** Instance Variables

    CallInfo mCalls[] = new CallInfo[MAX_CALLS];

    private boolean mAutoProgressConnecting = true;
    private boolean mNextDialFailImmediately;


    //***** Event Constants

    static final int EVENT_PROGRESS_CALL_STATE = 1;

    //***** Constants

    static final int MAX_CALLS = 7;
    /** number of msec between dialing -> alerting and alerting->active */
    static final int CONNECTING_PAUSE_MSEC = 5 * 100;


    //***** Overridden from Handler

    public SimulatedGsmCallState(Looper looper) {
        super(looper);
    }

    @Override
    public void
    handleMessage(Message msg) {
        synchronized(this) { switch (msg.what) {
            // PLEASE REMEMBER
            // calls may have hung up by the time delayed events happen

            case EVENT_PROGRESS_CALL_STATE:
                progressConnectingCallState();
            break;
        }}
    }

    //***** Public Methods

    /**
     * Start the simulated phone ringing
     * true if succeeded, false if failed
     */
    public boolean
    triggerRing(String number) {
        synchronized (this) {
            int empty = -1;
            boolean isCallWaiting = false;

            // ensure there aren't already calls INCOMING or WAITING
            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo call = mCalls[i];

                if (call == null && empty < 0) {
                    empty = i;
                } else if (call != null
                    && (call.mState == CallInfo.State.INCOMING
                        || call.mState == CallInfo.State.WAITING)
                ) {
                    Rlog.w("ModelInterpreter",
                        "triggerRing failed; phone already ringing");
                    return false;
                } else if (call != null) {
                    isCallWaiting = true;
                }
            }

            if (empty < 0 ) {
                Rlog.w("ModelInterpreter", "triggerRing failed; all full");
                return false;
            }

            mCalls[empty] = CallInfo.createIncomingCall(
                PhoneNumberUtils.extractNetworkPortion(number));

            if (isCallWaiting) {
                mCalls[empty].mState = CallInfo.State.WAITING;
            }

        }
        return true;
    }

    /** If a call is DIALING or ALERTING, progress it to the next state */
    public void
    progressConnectingCallState() {
        synchronized (this)  {
            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo call = mCalls[i];

                if (call != null && call.mState == CallInfo.State.DIALING) {
                    call.mState = CallInfo.State.ALERTING;

                    if (mAutoProgressConnecting) {
                        sendMessageDelayed(
                                obtainMessage(EVENT_PROGRESS_CALL_STATE, call),
                                CONNECTING_PAUSE_MSEC);
                    }
                    break;
                } else if (call != null
                        && call.mState == CallInfo.State.ALERTING
                ) {
                    call.mState = CallInfo.State.ACTIVE;
                    break;
                }
            }
        }
    }

    /** If a call is DIALING or ALERTING, progress it all the way to ACTIVE */
    public void
    progressConnectingToActive() {
        synchronized (this)  {
            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo call = mCalls[i];

                if (call != null && (call.mState == CallInfo.State.DIALING
                    || call.mState == CallInfo.State.ALERTING)
                ) {
                    call.mState = CallInfo.State.ACTIVE;
                    break;
                }
            }
        }
    }

    /** automatically progress mobile originated calls to ACTIVE.
     *  default to true
     */
    public void
    setAutoProgressConnectingCall(boolean b) {
        mAutoProgressConnecting = b;
    }

    public void
    setNextDialFailImmediately(boolean b) {
        mNextDialFailImmediately = b;
    }

    /**
     * hangup ringing, dialing, or active calls
     * returns true if call was hung up, false if not
     */
    public boolean
    triggerHangupForeground() {
        synchronized (this) {
            boolean found;

            found = false;

            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo call = mCalls[i];

                if (call != null
                    && (call.mState == CallInfo.State.INCOMING
                        || call.mState == CallInfo.State.WAITING)
                ) {
                    mCalls[i] = null;
                    found = true;
                }
            }

            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo call = mCalls[i];

                if (call != null
                    && (call.mState == CallInfo.State.DIALING
                        || call.mState == CallInfo.State.ACTIVE
                        || call.mState == CallInfo.State.ALERTING)
                ) {
                    mCalls[i] = null;
                    found = true;
                }
            }
            return found;
        }
    }

    /**
     * hangup holding calls
     * returns true if call was hung up, false if not
     */
    public boolean
    triggerHangupBackground() {
        synchronized (this) {
            boolean found = false;

            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo call = mCalls[i];

                if (call != null && call.mState == CallInfo.State.HOLDING) {
                    mCalls[i] = null;
                    found = true;
                }
            }

            return found;
        }
    }

    /**
     * hangup all
     * returns true if call was hung up, false if not
     */
    public boolean
    triggerHangupAll() {
        synchronized(this) {
            boolean found = false;

            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo call = mCalls[i];

                if (mCalls[i] != null) {
                    found = true;
                }

                mCalls[i] = null;
            }

            return found;
        }
    }

    public boolean
    onAnswer() {
        synchronized (this) {
            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo call = mCalls[i];

                if (call != null
                    && (call.mState == CallInfo.State.INCOMING
                        || call.mState == CallInfo.State.WAITING)
                ) {
                    return switchActiveAndHeldOrWaiting();
                }
            }
        }

        return false;
    }

    public boolean
    onHangup() {
        boolean found = false;

        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo call = mCalls[i];

            if (call != null && call.mState != CallInfo.State.WAITING) {
                mCalls[i] = null;
                found = true;
            }
        }

        return found;
    }

    public boolean
    onChld(char c0, char c1) {
        boolean ret;
        int callIndex = 0;

        if (c1 != 0) {
            callIndex = c1 - '1';

            if (callIndex < 0 || callIndex >= mCalls.length) {
                return false;
            }
        }

        switch (c0) {
            case '0':
                ret = releaseHeldOrUDUB();
            break;
            case '1':
                if (c1 <= 0) {
                    ret = releaseActiveAcceptHeldOrWaiting();
                } else {
                    if (mCalls[callIndex] == null) {
                        ret = false;
                    } else {
                        mCalls[callIndex] = null;
                        ret = true;
                    }
                }
            break;
            case '2':
                if (c1 <= 0) {
                    ret = switchActiveAndHeldOrWaiting();
                } else {
                    ret = separateCall(callIndex);
                }
            break;
            case '3':
                ret = conference();
            break;
            case '4':
                ret = explicitCallTransfer();
            break;
            case '5':
                if (true) { //just so javac doesnt complain about break
                    //CCBS not impled
                    ret = false;
                }
            break;
            default:
                ret = false;

        }

        return ret;
    }

    public boolean
    releaseHeldOrUDUB() {
        boolean found = false;

        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null && c.isRinging()) {
                found = true;
                mCalls[i] = null;
                break;
            }
        }

        if (!found) {
            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo c = mCalls[i];

                if (c != null && c.mState == CallInfo.State.HOLDING) {
                    found = true;
                    mCalls[i] = null;
                    // don't stop...there may be more than one
                }
            }
        }

        return true;
    }


    public boolean
    releaseActiveAcceptHeldOrWaiting() {
        boolean foundHeld = false;
        boolean foundActive = false;

        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null && c.mState == CallInfo.State.ACTIVE) {
                mCalls[i] = null;
                foundActive = true;
            }
        }

        if (!foundActive) {
            // FIXME this may not actually be how most basebands react
            // CHLD=1 may not hang up dialing/alerting calls
            for (int i = 0 ; i < mCalls.length ; i++) {
                CallInfo c = mCalls[i];

                if (c != null
                        && (c.mState == CallInfo.State.DIALING
                            || c.mState == CallInfo.State.ALERTING)
                ) {
                    mCalls[i] = null;
                    foundActive = true;
                }
            }
        }

        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null && c.mState == CallInfo.State.HOLDING) {
                c.mState = CallInfo.State.ACTIVE;
                foundHeld = true;
            }
        }

        if (foundHeld) {
            return true;
        }

        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null && c.isRinging()) {
                c.mState = CallInfo.State.ACTIVE;
                return true;
            }
        }

        return true;
    }

    public boolean
    switchActiveAndHeldOrWaiting() {
        boolean hasHeld = false;

        // first, are there held calls?
        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null && c.mState == CallInfo.State.HOLDING) {
                hasHeld = true;
                break;
            }
        }

        // Now, switch
        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null) {
                if (c.mState == CallInfo.State.ACTIVE) {
                    c.mState = CallInfo.State.HOLDING;
                } else if (c.mState == CallInfo.State.HOLDING) {
                    c.mState = CallInfo.State.ACTIVE;
                } else if (!hasHeld && c.isRinging())  {
                    c.mState = CallInfo.State.ACTIVE;
                }
            }
        }

        return true;
    }


    public boolean
    separateCall(int index) {
        try {
            CallInfo c;

            c = mCalls[index];

            if (c == null || c.isConnecting() || countActiveLines() != 1) {
                return false;
            }

            c.mState = CallInfo.State.ACTIVE;
            c.mIsMpty = false;

            for (int i = 0 ; i < mCalls.length ; i++) {
                int countHeld=0, lastHeld=0;

                if (i != index) {
                    CallInfo cb = mCalls[i];

                    if (cb != null && cb.mState == CallInfo.State.ACTIVE) {
                        cb.mState = CallInfo.State.HOLDING;
                        countHeld++;
                        lastHeld = i;
                    }
                }

                if (countHeld == 1) {
                    // if there's only one left, clear the MPTY flag
                    mCalls[lastHeld].mIsMpty = false;
                }
            }

            return true;
        } catch (InvalidStateEx ex) {
            return false;
        }
    }



    public boolean
    conference() {
        int countCalls = 0;

        // if there's connecting calls, we can't do this yet
        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null) {
                countCalls++;

                if (c.isConnecting()) {
                    return false;
                }
            }
        }
        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null) {
                c.mState = CallInfo.State.ACTIVE;
                if (countCalls > 0) {
                    c.mIsMpty = true;
                }
            }
        }

        return true;
    }

    public boolean
    explicitCallTransfer() {
        int countCalls = 0;

        // if there's connecting calls, we can't do this yet
        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null) {
                countCalls++;

                if (c.isConnecting()) {
                    return false;
                }
            }
        }

        // disconnect the subscriber from both calls
        return triggerHangupAll();
    }

    public boolean
    onDial(String address) {
        CallInfo call;
        int freeSlot = -1;

        Rlog.d("GSM", "SC> dial '" + address + "'");

        if (mNextDialFailImmediately) {
            mNextDialFailImmediately = false;

            Rlog.d("GSM", "SC< dial fail (per request)");
            return false;
        }

        String phNum = PhoneNumberUtils.extractNetworkPortion(address);

        if (phNum.length() == 0) {
            Rlog.d("GSM", "SC< dial fail (invalid ph num)");
            return false;
        }

        // Ignore setting up GPRS
        if (phNum.startsWith("*99") && phNum.endsWith("#")) {
            Rlog.d("GSM", "SC< dial ignored (gprs)");
            return true;
        }

        // There can be at most 1 active "line" when we initiate
        // a new call
        try {
            if (countActiveLines() > 1) {
                Rlog.d("GSM", "SC< dial fail (invalid call state)");
                return false;
            }
        } catch (InvalidStateEx ex) {
            Rlog.d("GSM", "SC< dial fail (invalid call state)");
            return false;
        }

        for (int i = 0 ; i < mCalls.length ; i++) {
            if (freeSlot < 0 && mCalls[i] == null) {
                freeSlot = i;
            }

            if (mCalls[i] != null && !mCalls[i].isActiveOrHeld()) {
                // Can't make outgoing calls when there is a ringing or
                // connecting outgoing call
                Rlog.d("GSM", "SC< dial fail (invalid call state)");
                return false;
            } else if (mCalls[i] != null && mCalls[i].mState == CallInfo.State.ACTIVE) {
                // All active calls behome held
                mCalls[i].mState = CallInfo.State.HOLDING;
            }
        }

        if (freeSlot < 0) {
            Rlog.d("GSM", "SC< dial fail (invalid call state)");
            return false;
        }

        mCalls[freeSlot] = CallInfo.createOutgoingCall(phNum);

        if (mAutoProgressConnecting) {
            sendMessageDelayed(
                    obtainMessage(EVENT_PROGRESS_CALL_STATE, mCalls[freeSlot]),
                    CONNECTING_PAUSE_MSEC);
        }

        Rlog.d("GSM", "SC< dial (slot = " + freeSlot + ")");

        return true;
    }

    public List<DriverCall>
    getDriverCalls() {
        ArrayList<DriverCall> ret = new ArrayList<DriverCall>(mCalls.length);

        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null) {
                DriverCall dc;

                dc = c.toDriverCall(i + 1);
                ret.add(dc);
            }
        }

        Rlog.d("GSM", "SC< getDriverCalls " + ret);

        return ret;
    }

    public List<String>
    getClccLines() {
        ArrayList<String> ret = new ArrayList<String>(mCalls.length);

        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo c = mCalls[i];

            if (c != null) {
                ret.add((c.toCLCCLine(i + 1)));
            }
        }

        return ret;
    }

    private int
    countActiveLines() throws InvalidStateEx {
        boolean hasMpty = false;
        boolean hasHeld = false;
        boolean hasActive = false;
        boolean hasConnecting = false;
        boolean hasRinging = false;
        boolean mptyIsHeld = false;

        for (int i = 0 ; i < mCalls.length ; i++) {
            CallInfo call = mCalls[i];

            if (call != null) {
                if (!hasMpty && call.mIsMpty) {
                    mptyIsHeld = call.mState == CallInfo.State.HOLDING;
                } else if (call.mIsMpty && mptyIsHeld
                    && call.mState == CallInfo.State.ACTIVE
                ) {
                    Rlog.e("ModelInterpreter", "Invalid state");
                    throw new InvalidStateEx();
                } else if (!call.mIsMpty && hasMpty && mptyIsHeld
                    && call.mState == CallInfo.State.HOLDING
                ) {
                    Rlog.e("ModelInterpreter", "Invalid state");
                    throw new InvalidStateEx();
                }

                hasMpty |= call.mIsMpty;
                hasHeld |= call.mState == CallInfo.State.HOLDING;
                hasActive |= call.mState == CallInfo.State.ACTIVE;
                hasConnecting |= call.isConnecting();
                hasRinging |= call.isRinging();
            }
        }

        int ret = 0;

        if (hasHeld) ret++;
        if (hasActive) ret++;
        if (hasConnecting) ret++;
        if (hasRinging) ret++;

        return ret;
    }

}
