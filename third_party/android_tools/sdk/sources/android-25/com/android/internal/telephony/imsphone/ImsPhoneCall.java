/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.internal.telephony.imsphone;

import android.telecom.ConferenceParticipant;
import android.telephony.Rlog;
import android.telephony.DisconnectCause;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.ims.ImsCall;
import com.android.ims.ImsException;
import com.android.ims.ImsStreamMediaProfile;

import java.util.List;

/**
 * {@hide}
 */
public class ImsPhoneCall extends Call {
    private static final String LOG_TAG = "ImsPhoneCall";

    // This flag is meant to be used as a debugging tool to quickly see all logs
    // regardless of the actual log level set on this component.
    private static final boolean FORCE_DEBUG = false; /* STOPSHIP if true */
    private static final boolean DBG = FORCE_DEBUG || Rlog.isLoggable(LOG_TAG, Log.DEBUG);
    private static final boolean VDBG = FORCE_DEBUG || Rlog.isLoggable(LOG_TAG, Log.VERBOSE);

    /*************************** Instance Variables **************************/
    public static final String CONTEXT_UNKNOWN = "UK";
    public static final String CONTEXT_RINGING = "RG";
    public static final String CONTEXT_FOREGROUND = "FG";
    public static final String CONTEXT_BACKGROUND = "BG";
    public static final String CONTEXT_HANDOVER = "HO";

    /*package*/ ImsPhoneCallTracker mOwner;

    private boolean mRingbackTonePlayed = false;

    // Determines what type of ImsPhoneCall this is.  ImsPhoneCallTracker uses instances of
    // ImsPhoneCall to for fg, bg, etc calls.  This is used as a convenience for logging so that it
    // can be made clear whether a call being logged is the foreground, background, etc.
    private final String mCallContext;

    /****************************** Constructors *****************************/
    /*package*/
    ImsPhoneCall() {
        mCallContext = CONTEXT_UNKNOWN;
    }

    public ImsPhoneCall(ImsPhoneCallTracker owner, String context) {
        mOwner = owner;
        mCallContext = context;
    }

    public void dispose() {
        try {
            mOwner.hangup(this);
        } catch (CallStateException ex) {
            //Rlog.e(LOG_TAG, "dispose: unexpected error on hangup", ex);
            //while disposing, ignore the exception and clean the connections
        } finally {
            for(int i = 0, s = mConnections.size(); i < s; i++) {
                ImsPhoneConnection c = (ImsPhoneConnection) mConnections.get(i);
                c.onDisconnect(DisconnectCause.LOST_SIGNAL);
            }
        }
    }

    /************************** Overridden from Call *************************/

    @Override
    public List<Connection>
    getConnections() {
        return mConnections;
    }

    @Override
    public Phone
    getPhone() {
        return mOwner.mPhone;
    }

    @Override
    public boolean
    isMultiparty() {
        ImsCall imsCall = getImsCall();
        if (imsCall == null) {
            return false;
        }

        return imsCall.isMultiparty();
    }

    /** Please note: if this is the foreground call and a
     *  background call exists, the background call will be resumed.
     */
    @Override
    public void
    hangup() throws CallStateException {
        mOwner.hangup(this);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("[ImsPhoneCall ");
        sb.append(mCallContext);
        sb.append(" state: ");
        sb.append(mState.toString());
        sb.append(" ");
        if (mConnections.size() > 1) {
            sb.append(" ERROR_MULTIPLE ");
        }
        for (Connection conn : mConnections) {
            sb.append(conn);
            sb.append(" ");
        }

        sb.append("]");
        return sb.toString();
    }

    @Override
    public List<ConferenceParticipant> getConferenceParticipants() {
         ImsCall call = getImsCall();
         if (call == null) {
             return null;
         }
         return call.getConferenceParticipants();
    }

    //***** Called from ImsPhoneConnection

    public void attach(Connection conn) {
        if (VDBG) {
            Rlog.v(LOG_TAG, "attach : " + mCallContext + " conn = " + conn);
        }
        clearDisconnected();
        mConnections.add(conn);

        mOwner.logState();
    }

    public void attach(Connection conn, State state) {
        if (VDBG) {
            Rlog.v(LOG_TAG, "attach : " + mCallContext + " state = " +
                    state.toString());
        }
        this.attach(conn);
        mState = state;
    }

    public void attachFake(Connection conn, State state) {
        attach(conn, state);
    }

    /**
     * Called by ImsPhoneConnection when it has disconnected
     */
    public boolean connectionDisconnected(ImsPhoneConnection conn) {
        if (mState != State.DISCONNECTED) {
            /* If only disconnected connections remain, we are disconnected*/

            boolean hasOnlyDisconnectedConnections = true;

            for (int i = 0, s = mConnections.size()  ; i < s; i ++) {
                if (mConnections.get(i).getState() != State.DISCONNECTED) {
                    hasOnlyDisconnectedConnections = false;
                    break;
                }
            }

            if (hasOnlyDisconnectedConnections) {
                mState = State.DISCONNECTED;
                if (VDBG) {
                    Rlog.v(LOG_TAG, "connectionDisconnected : " + mCallContext + " state = " +
                            mState);
                }
                return true;
            }
        }

        return false;
    }

    public void detach(ImsPhoneConnection conn) {
        if (VDBG) {
            Rlog.v(LOG_TAG, "detach : " + mCallContext + " conn = " + conn);
        }
        mConnections.remove(conn);
        clearDisconnected();

        mOwner.logState();
    }

    /**
     * @return true if there's no space in this call for additional
     * connections to be added via "conference"
     */
    /*package*/ boolean
    isFull() {
        return mConnections.size() == ImsPhoneCallTracker.MAX_CONNECTIONS_PER_CALL;
    }

    //***** Called from ImsPhoneCallTracker
    /**
     * Called when this Call is being hung up locally (eg, user pressed "end")
     */
    void
    onHangupLocal() {
        for (int i = 0, s = mConnections.size(); i < s; i++) {
            ImsPhoneConnection cn = (ImsPhoneConnection)mConnections.get(i);
            cn.onHangupLocal();
        }
        mState = State.DISCONNECTING;
        if (VDBG) {
            Rlog.v(LOG_TAG, "onHangupLocal : " + mCallContext + " state = " + mState);
        }
    }

    /*package*/ ImsPhoneConnection
    getFirstConnection() {
        if (mConnections.size() == 0) return null;

        return (ImsPhoneConnection) mConnections.get(0);
    }

    /*package*/ void
    setMute(boolean mute) {
        ImsCall imsCall = getFirstConnection() == null ?
                null : getFirstConnection().getImsCall();
        if (imsCall != null) {
            try {
                imsCall.setMute(mute);
            } catch (ImsException e) {
                Rlog.e(LOG_TAG, "setMute failed : " + e.getMessage());
            }
        }
    }

    /* package */ void
    merge(ImsPhoneCall that, State state) {
        // This call is the conference host and the "that" call is the one being merged in.
        // Set the connect time for the conference; this will have been determined when the
        // conference was initially created.
        ImsPhoneConnection imsPhoneConnection = getFirstConnection();
        if (imsPhoneConnection != null) {
            long conferenceConnectTime = imsPhoneConnection.getConferenceConnectTime();
            if (conferenceConnectTime > 0) {
                imsPhoneConnection.setConnectTime(conferenceConnectTime);
            } else {
                if (DBG) {
                    Rlog.d(LOG_TAG, "merge: conference connect time is 0");
                }
            }
        }
        if (DBG) {
            Rlog.d(LOG_TAG, "merge(" + mCallContext + "): " + that + "state = "
                    + state);
        }
    }

    /**
     * Retrieves the {@link ImsCall} for the current {@link ImsPhoneCall}.
     * <p>
     * Marked as {@code VisibleForTesting} so that the
     * {@link com.android.internal.telephony.TelephonyTester} class can inject a test conference
     * event package into a regular ongoing IMS call.
     *
     * @return The {@link ImsCall}.
     */
    @VisibleForTesting
    public ImsCall
    getImsCall() {
        return (getFirstConnection() == null) ? null : getFirstConnection().getImsCall();
    }

    /*package*/ static boolean isLocalTone(ImsCall imsCall) {
        if ((imsCall == null) || (imsCall.getCallProfile() == null)
                || (imsCall.getCallProfile().mMediaProfile == null)) {
            return false;
        }

        ImsStreamMediaProfile mediaProfile = imsCall.getCallProfile().mMediaProfile;

        return (mediaProfile.mAudioDirection == ImsStreamMediaProfile.DIRECTION_INACTIVE)
                ? true : false;
    }

    public boolean update (ImsPhoneConnection conn, ImsCall imsCall, State state) {
        boolean changed = false;
        State oldState = mState;

        //ImsCall.Listener.onCallProgressing can be invoked several times
        //and ringback tone mode can be changed during the call setup procedure
        if (state == State.ALERTING) {
            if (mRingbackTonePlayed && !isLocalTone(imsCall)) {
                mOwner.mPhone.stopRingbackTone();
                mRingbackTonePlayed = false;
            } else if (!mRingbackTonePlayed && isLocalTone(imsCall)) {
                mOwner.mPhone.startRingbackTone();
                mRingbackTonePlayed = true;
            }
        } else {
            if (mRingbackTonePlayed) {
                mOwner.mPhone.stopRingbackTone();
                mRingbackTonePlayed = false;
            }
        }

        if ((state != mState) && (state != State.DISCONNECTED)) {
            mState = state;
            changed = true;
        } else if (state == State.DISCONNECTED) {
            changed = true;
        }

        if (VDBG) {
            Rlog.v(LOG_TAG, "update : " + mCallContext + " state: " + oldState + " --> " + mState);
        }

        return changed;
    }

    /* package */ ImsPhoneConnection
    getHandoverConnection() {
        return (ImsPhoneConnection) getEarliestConnection();
    }

    public void switchWith(ImsPhoneCall that) {
        if (VDBG) {
            Rlog.v(LOG_TAG, "switchWith : switchCall = " + this + " withCall = " + that);
        }
        synchronized (ImsPhoneCall.class) {
            ImsPhoneCall tmp = new ImsPhoneCall();
            tmp.takeOver(this);
            this.takeOver(that);
            that.takeOver(tmp);
        }
        mOwner.logState();
    }

    private void takeOver(ImsPhoneCall that) {
        mConnections = that.mConnections;
        mState = that.mState;
        for (Connection c : mConnections) {
            ((ImsPhoneConnection) c).changeParent(this);
        }
    }
}
