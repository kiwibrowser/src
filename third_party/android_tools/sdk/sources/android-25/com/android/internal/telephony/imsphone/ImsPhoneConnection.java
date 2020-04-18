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

import android.content.Context;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.PowerManager;
import android.os.Registrant;
import android.os.SystemClock;
import android.telecom.VideoProfile;
import android.telephony.CarrierConfigManager;
import android.telephony.DisconnectCause;
import android.telephony.PhoneNumberUtils;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.text.TextUtils;

import com.android.ims.ImsException;
import com.android.ims.ImsStreamMediaProfile;
import com.android.ims.internal.ImsVideoCallProviderWrapper;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.UUSInfo;

import com.android.ims.ImsCall;
import com.android.ims.ImsCallProfile;

import java.util.Objects;

/**
 * {@hide}
 */
public class ImsPhoneConnection extends Connection implements
        ImsVideoCallProviderWrapper.ImsVideoProviderWrapperCallback {

    private static final String LOG_TAG = "ImsPhoneConnection";
    private static final boolean DBG = true;

    //***** Instance Variables

    private ImsPhoneCallTracker mOwner;
    private ImsPhoneCall mParent;
    private ImsCall mImsCall;
    private Bundle mExtras = new Bundle();

    private boolean mDisconnected;

    /*
    int mIndex;          // index in ImsPhoneCallTracker.connections[], -1 if unassigned
                        // The GSM index is 1 + this
    */

    /*
     * These time/timespan values are based on System.currentTimeMillis(),
     * i.e., "wall clock" time.
     */
    private long mDisconnectTime;

    private UUSInfo mUusInfo;
    private Handler mHandler;

    private PowerManager.WakeLock mPartialWakeLock;

    // The cached connect time of the connection when it turns into a conference.
    private long mConferenceConnectTime = 0;

    // The cached delay to be used between DTMF tones fetched from carrier config.
    private int mDtmfToneDelay = 0;

    private boolean mIsEmergency = false;

    /**
     * Used to indicate that video state changes detected by
     * {@link #updateMediaCapabilities(ImsCall)} should be ignored.  When a video state change from
     * unpaused to paused occurs, we set this flag and then update the existing video state when
     * new {@link #onReceiveSessionModifyResponse(int, VideoProfile, VideoProfile)} callbacks come
     * in.  When the video un-pauses we continue receiving the video state updates.
     */
    private boolean mShouldIgnoreVideoStateChanges = false;

    /**
     * Used to indicate whether the wifi state is based on
     * {@link com.android.ims.ImsConnectionStateListener#
     *      onFeatureCapabilityChanged(int, int[], int[])} callbacks, or values received via the
     * {@link ImsCallProfile#EXTRA_CALL_RAT_TYPE} extra.  Util we receive a value via the extras,
     * we will use the wifi state based on the {@code onFeatureCapabilityChanged}.  Once a value
     * is received via the extras, we will prefer those values going forward.
     */
    private boolean mIsWifiStateFromExtras = false;

    //***** Event Constants
    private static final int EVENT_DTMF_DONE = 1;
    private static final int EVENT_PAUSE_DONE = 2;
    private static final int EVENT_NEXT_POST_DIAL = 3;
    private static final int EVENT_WAKE_LOCK_TIMEOUT = 4;
    private static final int EVENT_DTMF_DELAY_DONE = 5;

    //***** Constants
    private static final int PAUSE_DELAY_MILLIS = 3 * 1000;
    private static final int WAKE_LOCK_TIMEOUT_MILLIS = 60*1000;

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

    /** This is probably an MT call */
    public ImsPhoneConnection(Phone phone, ImsCall imsCall, ImsPhoneCallTracker ct,
           ImsPhoneCall parent, boolean isUnknown) {
        super(PhoneConstants.PHONE_TYPE_IMS);
        createWakeLock(phone.getContext());
        acquireWakeLock();

        mOwner = ct;
        mHandler = new MyHandler(mOwner.getLooper());
        mImsCall = imsCall;

        if ((imsCall != null) && (imsCall.getCallProfile() != null)) {
            mAddress = imsCall.getCallProfile().getCallExtra(ImsCallProfile.EXTRA_OI);
            mCnapName = imsCall.getCallProfile().getCallExtra(ImsCallProfile.EXTRA_CNA);
            mNumberPresentation = ImsCallProfile.OIRToPresentation(
                    imsCall.getCallProfile().getCallExtraInt(ImsCallProfile.EXTRA_OIR));
            mCnapNamePresentation = ImsCallProfile.OIRToPresentation(
                    imsCall.getCallProfile().getCallExtraInt(ImsCallProfile.EXTRA_CNAP));
            updateMediaCapabilities(imsCall);
        } else {
            mNumberPresentation = PhoneConstants.PRESENTATION_UNKNOWN;
            mCnapNamePresentation = PhoneConstants.PRESENTATION_UNKNOWN;
        }

        mIsIncoming = !isUnknown;
        mCreateTime = System.currentTimeMillis();
        mUusInfo = null;

        updateWifiState();

        // Ensure any extras set on the ImsCallProfile at the start of the call are cached locally
        // in the ImsPhoneConnection.  This isn't going to inform any listeners (since the original
        // connection is not likely to be associated with a TelephonyConnection yet).
        updateExtras(imsCall);

        mParent = parent;
        mParent.attach(this,
                (mIsIncoming? ImsPhoneCall.State.INCOMING: ImsPhoneCall.State.DIALING));

        fetchDtmfToneDelay(phone);
    }

    /** This is an MO call, created when dialing */
    public ImsPhoneConnection(Phone phone, String dialString, ImsPhoneCallTracker ct,
            ImsPhoneCall parent, boolean isEmergency) {
        super(PhoneConstants.PHONE_TYPE_IMS);
        createWakeLock(phone.getContext());
        acquireWakeLock();

        mOwner = ct;
        mHandler = new MyHandler(mOwner.getLooper());

        mDialString = dialString;

        mAddress = PhoneNumberUtils.extractNetworkPortionAlt(dialString);
        mPostDialString = PhoneNumberUtils.extractPostDialPortion(dialString);

        //mIndex = -1;

        mIsIncoming = false;
        mCnapName = null;
        mCnapNamePresentation = PhoneConstants.PRESENTATION_ALLOWED;
        mNumberPresentation = PhoneConstants.PRESENTATION_ALLOWED;
        mCreateTime = System.currentTimeMillis();

        mParent = parent;
        parent.attachFake(this, ImsPhoneCall.State.DIALING);

        mIsEmergency = isEmergency;

        fetchDtmfToneDelay(phone);
    }

    public void dispose() {
    }

    static boolean
    equalsHandlesNulls (Object a, Object b) {
        return (a == null) ? (b == null) : a.equals (b);
    }

    private static int applyLocalCallCapabilities(ImsCallProfile localProfile, int capabilities) {
        Rlog.w(LOG_TAG, "applyLocalCallCapabilities - localProfile = "+localProfile);
        capabilities = removeCapability(capabilities,
                Connection.Capability.SUPPORTS_VT_LOCAL_BIDIRECTIONAL);

        switch (localProfile.mCallType) {
            case ImsCallProfile.CALL_TYPE_VT:
                // Fall-through
            case ImsCallProfile.CALL_TYPE_VIDEO_N_VOICE:
                capabilities = addCapability(capabilities,
                        Connection.Capability.SUPPORTS_VT_LOCAL_BIDIRECTIONAL);
                break;
        }
        return capabilities;
    }

    private static int applyRemoteCallCapabilities(ImsCallProfile remoteProfile, int capabilities) {
        Rlog.w(LOG_TAG, "applyRemoteCallCapabilities - remoteProfile = "+remoteProfile);
        capabilities = removeCapability(capabilities,
                Connection.Capability.SUPPORTS_VT_REMOTE_BIDIRECTIONAL);

        switch (remoteProfile.mCallType) {
            case ImsCallProfile.CALL_TYPE_VT:
                // fall-through
            case ImsCallProfile.CALL_TYPE_VIDEO_N_VOICE:
                capabilities = addCapability(capabilities,
                        Connection.Capability.SUPPORTS_VT_REMOTE_BIDIRECTIONAL);
                break;
        }
        return capabilities;
    }

    @Override
    public String getOrigDialString(){
        return mDialString;
    }

    @Override
    public ImsPhoneCall getCall() {
        return mParent;
    }

    @Override
    public long getDisconnectTime() {
        return mDisconnectTime;
    }

    @Override
    public long getHoldingStartTime() {
        return mHoldingStartTime;
    }

    @Override
    public long getHoldDurationMillis() {
        if (getState() != ImsPhoneCall.State.HOLDING) {
            // If not holding, return 0
            return 0;
        } else {
            return SystemClock.elapsedRealtime() - mHoldingStartTime;
        }
    }

    public void setDisconnectCause(int cause) {
        mCause = cause;
    }

    @Override
    public String getVendorDisconnectCause() {
      return null;
    }

    public ImsPhoneCallTracker getOwner () {
        return mOwner;
    }

    @Override
    public ImsPhoneCall.State getState() {
        if (mDisconnected) {
            return ImsPhoneCall.State.DISCONNECTED;
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
        throw new CallStateException ("not supported");
    }

    @Override
    public void proceedAfterWaitChar() {
        if (mPostDialState != PostDialState.WAIT) {
            Rlog.w(LOG_TAG, "ImsPhoneConnection.proceedAfterWaitChar(): Expected "
                    + "getPostDialState() to be WAIT but was " + mPostDialState);
            return;
        }

        setPostDialState(PostDialState.STARTED);

        processNextPostDialChar();
    }

    @Override
    public void proceedAfterWildChar(String str) {
        if (mPostDialState != PostDialState.WILD) {
            Rlog.w(LOG_TAG, "ImsPhoneConnection.proceedAfterWaitChar(): Expected "
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
            Rlog.d(LOG_TAG, "proceedAfterWildChar: new postDialString is " +
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
     */
    void
    onHangupLocal() {
        mCause = DisconnectCause.LOCAL;
    }

    /** Called when the connection has been disconnected */
    @Override
    public boolean onDisconnect(int cause) {
        Rlog.d(LOG_TAG, "onDisconnect: cause=" + cause);
        if (mCause != DisconnectCause.LOCAL || cause == DisconnectCause.INCOMING_REJECTED) {
            mCause = cause;
        }
        return onDisconnect();
    }

    public boolean onDisconnect() {
        boolean changed = false;

        if (!mDisconnected) {
            //mIndex = -1;

            mDisconnectTime = System.currentTimeMillis();
            mDuration = SystemClock.elapsedRealtime() - mConnectTimeReal;
            mDisconnected = true;

            mOwner.mPhone.notifyDisconnect(this);

            if (mParent != null) {
                changed = mParent.connectionDisconnected(this);
            } else {
                Rlog.d(LOG_TAG, "onDisconnect: no parent");
            }
            if (mImsCall != null) mImsCall.close();
            mImsCall = null;
        }
        releaseWakeLock();
        return changed;
    }

    /**
     * An incoming or outgoing call has connected
     */
    void
    onConnectedInOrOut() {
        mConnectTime = System.currentTimeMillis();
        mConnectTimeReal = SystemClock.elapsedRealtime();
        mDuration = 0;

        if (Phone.DEBUG_PHONE) {
            Rlog.d(LOG_TAG, "onConnectedInOrOut: connectTime=" + mConnectTime);
        }

        if (!mIsIncoming) {
            // outgoing calls only
            processNextPostDialChar();
        }
        releaseWakeLock();
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
            mOwner.sendDtmf(c, mHandler.obtainMessage(EVENT_DTMF_DONE));
        } else if (c == PhoneNumberUtils.PAUSE) {
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
                    PAUSE_DELAY_MILLIS);
        } else if (c == PhoneNumberUtils.WAIT) {
            setPostDialState(PostDialState.WAIT);
        } else if (c == PhoneNumberUtils.WILD) {
            setPostDialState(PostDialState.WILD);
        } else {
            return false;
        }

        return true;
    }

    @Override
    protected void finalize() {
        releaseWakeLock();
    }

    private void
    processNextPostDialChar() {
        char c = 0;
        Registrant postDialHandler;

        if (mPostDialState == PostDialState.CANCELLED) {
            //Rlog.d(LOG_TAG, "##### processNextPostDialChar: postDialState == CANCELLED, bail");
            return;
        }

        if (mPostDialString == null || mPostDialString.length() <= mNextPostDialChar) {
            setPostDialState(PostDialState.COMPLETE);

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
        postDialHandler = mOwner.mPhone.getPostDialHandler();

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

            //Rlog.v(LOG_TAG,
            //      "##### processNextPostDialChar: send msg to postDialHandler, arg1=" + c);
            notifyMessage.sendToTarget();
        }
    }

    /**
     * Set post dial state and acquire wake lock while switching to "started"
     * state, the wake lock will be released if state switches out of "started"
     * state or after WAKE_LOCK_TIMEOUT_MILLIS.
     * @param s new PostDialState
     */
    private void setPostDialState(PostDialState s) {
        if (mPostDialState != PostDialState.STARTED
                && s == PostDialState.STARTED) {
            acquireWakeLock();
            Message msg = mHandler.obtainMessage(EVENT_WAKE_LOCK_TIMEOUT);
            mHandler.sendMessageDelayed(msg, WAKE_LOCK_TIMEOUT_MILLIS);
        } else if (mPostDialState == PostDialState.STARTED
                && s != PostDialState.STARTED) {
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
        Rlog.d(LOG_TAG, "acquireWakeLock");
        mPartialWakeLock.acquire();
    }

    void
    releaseWakeLock() {
        synchronized(mPartialWakeLock) {
            if (mPartialWakeLock.isHeld()) {
                Rlog.d(LOG_TAG, "releaseWakeLock");
                mPartialWakeLock.release();
            }
        }
    }

    private void fetchDtmfToneDelay(Phone phone) {
        CarrierConfigManager configMgr = (CarrierConfigManager)
                phone.getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
        PersistableBundle b = configMgr.getConfigForSubId(phone.getSubId());
        if (b != null) {
            mDtmfToneDelay = b.getInt(CarrierConfigManager.KEY_IMS_DTMF_TONE_DELAY_INT);
        }
    }

    @Override
    public int getNumberPresentation() {
        return mNumberPresentation;
    }

    @Override
    public UUSInfo getUUSInfo() {
        return mUusInfo;
    }

    @Override
    public Connection getOrigConnection() {
        return null;
    }

    @Override
    public boolean isMultiparty() {
        return mImsCall != null && mImsCall.isMultiparty();
    }

    /**
     * Where {@link #isMultiparty()} is {@code true}, determines if this {@link ImsCall} is the
     * origin of the conference call (i.e. {@code #isConferenceHost()} is {@code true}), or if this
     * {@link ImsCall} is a member of a conference hosted on another device.
     *
     * @return {@code true} if this call is the origin of the conference call it is a member of,
     *      {@code false} otherwise.
     */
    @Override
    public boolean isConferenceHost() {
        if (mImsCall == null) {
            return false;
        }
        return mImsCall.isConferenceHost();
    }

    @Override
    public boolean isMemberOfPeerConference() {
        return !isConferenceHost();
    }

    public ImsCall getImsCall() {
        return mImsCall;
    }

    public void setImsCall(ImsCall imsCall) {
        mImsCall = imsCall;
    }

    public void changeParent(ImsPhoneCall parent) {
        mParent = parent;
    }

    /**
     * @return {@code true} if the {@link ImsPhoneConnection} or its media capabilities have been
     *     changed, and {@code false} otherwise.
     */
    public boolean update(ImsCall imsCall, ImsPhoneCall.State state) {
        if (state == ImsPhoneCall.State.ACTIVE) {
            // If the state of the call is active, but there is a pending request to the RIL to hold
            // the call, we will skip this update.  This is really a signalling delay or failure
            // from the RIL, but we will prevent it from going through as we will end up erroneously
            // making this call active when really it should be on hold.
            if (imsCall.isPendingHold()) {
                Rlog.w(LOG_TAG, "update : state is ACTIVE, but call is pending hold, skipping");
                return false;
            }

            if (mParent.getState().isRinging() || mParent.getState().isDialing()) {
                onConnectedInOrOut();
            }

            if (mParent.getState().isRinging() || mParent == mOwner.mBackgroundCall) {
                //mForegroundCall should be IDLE
                //when accepting WAITING call
                //before accept WAITING call,
                //the ACTIVE call should be held ahead
                mParent.detach(this);
                mParent = mOwner.mForegroundCall;
                mParent.attach(this);
            }
        } else if (state == ImsPhoneCall.State.HOLDING) {
            onStartedHolding();
        }

        boolean updateParent = mParent.update(this, imsCall, state);
        boolean updateWifiState = updateWifiState();
        boolean updateAddressDisplay = updateAddressDisplay(imsCall);
        boolean updateMediaCapabilities = updateMediaCapabilities(imsCall);
        boolean updateExtras = updateExtras(imsCall);

        return updateParent || updateWifiState || updateAddressDisplay || updateMediaCapabilities
                || updateExtras;
    }

    @Override
    public int getPreciseDisconnectCause() {
        return 0;
    }

    /**
     * Notifies this Connection of a request to disconnect a participant of the conference managed
     * by the connection.
     *
     * @param endpoint the {@link android.net.Uri} of the participant to disconnect.
     */
    @Override
    public void onDisconnectConferenceParticipant(Uri endpoint) {
        ImsCall imsCall = getImsCall();
        if (imsCall == null) {
            return;
        }
        try {
            imsCall.removeParticipants(new String[]{endpoint.toString()});
        } catch (ImsException e) {
            // No session in place -- no change
            Rlog.e(LOG_TAG, "onDisconnectConferenceParticipant: no session in place. "+
                    "Failed to disconnect endpoint = " + endpoint);
        }
    }

    /**
     * Sets the conference connect time.  Used when an {@code ImsConference} is created to out of
     * this phone connection.
     *
     * @param conferenceConnectTime The conference connect time.
     */
    public void setConferenceConnectTime(long conferenceConnectTime) {
        mConferenceConnectTime = conferenceConnectTime;
    }

    /**
     * @return The conference connect time.
     */
    public long getConferenceConnectTime() {
        return mConferenceConnectTime;
    }

    /**
     * Check for a change in the address display related fields for the {@link ImsCall}, and
     * update the {@link ImsPhoneConnection} with this information.
     *
     * @param imsCall The call to check for changes in address display fields.
     * @return Whether the address display fields have been changed.
     */
    public boolean updateAddressDisplay(ImsCall imsCall) {
        if (imsCall == null) {
            return false;
        }

        boolean changed = false;
        ImsCallProfile callProfile = imsCall.getCallProfile();
        if (callProfile != null) {
            String address = callProfile.getCallExtra(ImsCallProfile.EXTRA_OI);
            String name = callProfile.getCallExtra(ImsCallProfile.EXTRA_CNA);
            int nump = ImsCallProfile.OIRToPresentation(
                    callProfile.getCallExtraInt(ImsCallProfile.EXTRA_OIR));
            int namep = ImsCallProfile.OIRToPresentation(
                    callProfile.getCallExtraInt(ImsCallProfile.EXTRA_CNAP));
            if (Phone.DEBUG_PHONE) {
                Rlog.d(LOG_TAG, "address = " + Rlog.pii(LOG_TAG, address) + " name = " + name +
                        " nump = " + nump + " namep = " + namep);
            }
            if(equalsHandlesNulls(mAddress, address)) {
                mAddress = address;
                changed = true;
            }
            if (TextUtils.isEmpty(name)) {
                if (!TextUtils.isEmpty(mCnapName)) {
                    mCnapName = "";
                    changed = true;
                }
            } else if (!name.equals(mCnapName)) {
                mCnapName = name;
                changed = true;
            }
            if (mNumberPresentation != nump) {
                mNumberPresentation = nump;
                changed = true;
            }
            if (mCnapNamePresentation != namep) {
                mCnapNamePresentation = namep;
                changed = true;
            }
        }
        return changed;
    }

    /**
     * Check for a change in the video capabilities and audio quality for the {@link ImsCall}, and
     * update the {@link ImsPhoneConnection} with this information.
     *
     * @param imsCall The call to check for changes in media capabilities.
     * @return Whether the media capabilities have been changed.
     */
    public boolean updateMediaCapabilities(ImsCall imsCall) {
        if (imsCall == null) {
            return false;
        }

        boolean changed = false;

        try {
            // The actual call profile (negotiated between local and peer).
            ImsCallProfile negotiatedCallProfile = imsCall.getCallProfile();

            if (negotiatedCallProfile != null) {
                int oldVideoState = getVideoState();
                int newVideoState = ImsCallProfile
                        .getVideoStateFromImsCallProfile(negotiatedCallProfile);

                if (oldVideoState != newVideoState) {
                    // The video state has changed.  See also code in onReceiveSessionModifyResponse
                    // below.  When the video enters a paused state, subsequent changes to the video
                    // state will not be reported by the modem.  In onReceiveSessionModifyResponse
                    // we will be updating the current video state while paused to include any
                    // changes the modem reports via the video provider.  When the video enters an
                    // unpaused state, we will resume passing the video states from the modem as is.
                    if (VideoProfile.isPaused(oldVideoState) &&
                            !VideoProfile.isPaused(newVideoState)) {
                        // Video entered un-paused state; recognize updates from now on; we want to
                        // ensure that the new un-paused state is propagated to Telecom, so change
                        // this now.
                        mShouldIgnoreVideoStateChanges = false;
                    }

                    if (!mShouldIgnoreVideoStateChanges) {
                        setVideoState(newVideoState);
                        changed = true;
                    } else {
                        Rlog.d(LOG_TAG, "updateMediaCapabilities - ignoring video state change " +
                                "due to paused state.");
                    }

                    if (!VideoProfile.isPaused(oldVideoState) &&
                            VideoProfile.isPaused(newVideoState)) {
                        // Video entered pause state; ignore updates until un-paused.  We do this
                        // after setVideoState is called above to ensure Telecom is notified that
                        // the device has entered paused state.
                        mShouldIgnoreVideoStateChanges = true;
                    }
                }
            }

            // Check for a change in the capabilities for the call and update
            // {@link ImsPhoneConnection} with this information.
            int capabilities = getConnectionCapabilities();

            // Use carrier config to determine if downgrading directly to audio-only is supported.
            if (mOwner.isCarrierDowngradeOfVtCallSupported()) {
                capabilities = addCapability(capabilities,
                        Connection.Capability.SUPPORTS_DOWNGRADE_TO_VOICE_REMOTE |
                                Capability.SUPPORTS_DOWNGRADE_TO_VOICE_LOCAL);
            } else {
                capabilities = removeCapability(capabilities,
                        Connection.Capability.SUPPORTS_DOWNGRADE_TO_VOICE_REMOTE |
                                Capability.SUPPORTS_DOWNGRADE_TO_VOICE_LOCAL);
            }

            // Get the current local call capabilities which might be voice or video or both.
            ImsCallProfile localCallProfile = imsCall.getLocalCallProfile();
            Rlog.v(LOG_TAG, "update localCallProfile=" + localCallProfile);
            if (localCallProfile != null) {
                capabilities = applyLocalCallCapabilities(localCallProfile, capabilities);
            }

            // Get the current remote call capabilities which might be voice or video or both.
            ImsCallProfile remoteCallProfile = imsCall.getRemoteCallProfile();
            Rlog.v(LOG_TAG, "update remoteCallProfile=" + remoteCallProfile);
            if (remoteCallProfile != null) {
                capabilities = applyRemoteCallCapabilities(remoteCallProfile, capabilities);
            }
            if (getConnectionCapabilities() != capabilities) {
                setConnectionCapabilities(capabilities);
                changed = true;
            }

            int newAudioQuality =
                    getAudioQualityFromCallProfile(localCallProfile, remoteCallProfile);
            if (getAudioQuality() != newAudioQuality) {
                setAudioQuality(newAudioQuality);
                changed = true;
            }
        } catch (ImsException e) {
            // No session in place -- no change
        }

        return changed;
    }

    /**
     * Check for a change in the wifi state of the ImsPhoneCallTracker and update the
     * {@link ImsPhoneConnection} with this information.
     *
     * @return Whether the ImsPhoneCallTracker's usage of wifi has been changed.
     */
    public boolean updateWifiState() {
        // If we've received the wifi state via the ImsCallProfile.EXTRA_CALL_RAT_TYPE extra, we
        // will no longer use state updates which are based on the onFeatureCapabilityChanged
        // callback.
        if (mIsWifiStateFromExtras) {
            return false;
        }

        Rlog.d(LOG_TAG, "updateWifiState: " + mOwner.isVowifiEnabled());
        if (isWifi() != mOwner.isVowifiEnabled()) {
            setWifi(mOwner.isVowifiEnabled());
            return true;
        }
        return false;
    }

    /**
     * Updates the wifi state based on the {@link ImsCallProfile#EXTRA_CALL_RAT_TYPE}.
     * The call is considered to be a WIFI call if the extra value is
     * {@link ServiceState#RIL_RADIO_TECHNOLOGY_IWLAN}.
     *
     * @param extras The ImsCallProfile extras.
     */
    private void updateWifiStateFromExtras(Bundle extras) {
        if (extras.containsKey(ImsCallProfile.EXTRA_CALL_RAT_TYPE) ||
                extras.containsKey(ImsCallProfile.EXTRA_CALL_RAT_TYPE_ALT)) {

            // We've received the extra indicating the radio technology, so we will continue to
            // prefer the radio technology received via this extra going forward.
            mIsWifiStateFromExtras = true;

            ImsCall call = getImsCall();
            boolean isWifi = false;
            if (call != null) {
                isWifi = call.isWifiCall();
            }

            // Report any changes
            if (isWifi() != isWifi) {
                setWifi(isWifi);
            }
        }
    }

    /**
     * Check for a change in call extras of {@link ImsCall}, and
     * update the {@link ImsPhoneConnection} accordingly.
     *
     * @param imsCall The call to check for changes in extras.
     * @return Whether the extras fields have been changed.
     */
     boolean updateExtras(ImsCall imsCall) {
        if (imsCall == null) {
            return false;
        }

        final ImsCallProfile callProfile = imsCall.getCallProfile();
        final Bundle extras = callProfile != null ? callProfile.mCallExtras : null;
        if (extras == null && DBG) {
            Rlog.d(LOG_TAG, "Call profile extras are null.");
        }

        final boolean changed = !areBundlesEqual(extras, mExtras);
        if (changed) {
            updateWifiStateFromExtras(extras);

            mExtras.clear();
            mExtras.putAll(extras);
            setConnectionExtras(mExtras);
        }
        return changed;
    }

    private static boolean areBundlesEqual(Bundle extras, Bundle newExtras) {
        if (extras == null || newExtras == null) {
            return extras == newExtras;
        }

        if (extras.size() != newExtras.size()) {
            return false;
        }

        for(String key : extras.keySet()) {
            if (key != null) {
                final Object value = extras.get(key);
                final Object newValue = newExtras.get(key);
                if (!Objects.equals(value, newValue)) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Determines the {@link ImsPhoneConnection} audio quality based on the local and remote
     * {@link ImsCallProfile}. Indicate a HD audio call if the local stream profile
     * is AMR_WB, EVRC_WB, EVS_WB, EVS_SWB, EVS_FB and
     * there is no remote restrict cause.
     *
     * @param localCallProfile The local call profile.
     * @param remoteCallProfile The remote call profile.
     * @return The audio quality.
     */
    private int getAudioQualityFromCallProfile(
            ImsCallProfile localCallProfile, ImsCallProfile remoteCallProfile) {
        if (localCallProfile == null || remoteCallProfile == null
                || localCallProfile.mMediaProfile == null) {
            return AUDIO_QUALITY_STANDARD;
        }

        final boolean isEvsCodecHighDef = (localCallProfile.mMediaProfile.mAudioQuality
                        == ImsStreamMediaProfile.AUDIO_QUALITY_EVS_WB
                || localCallProfile.mMediaProfile.mAudioQuality
                        == ImsStreamMediaProfile.AUDIO_QUALITY_EVS_SWB
                || localCallProfile.mMediaProfile.mAudioQuality
                        == ImsStreamMediaProfile.AUDIO_QUALITY_EVS_FB);

        final boolean isHighDef = (localCallProfile.mMediaProfile.mAudioQuality
                        == ImsStreamMediaProfile.AUDIO_QUALITY_AMR_WB
                || localCallProfile.mMediaProfile.mAudioQuality
                        == ImsStreamMediaProfile.AUDIO_QUALITY_EVRC_WB
                || isEvsCodecHighDef)
                && remoteCallProfile.mRestrictCause == ImsCallProfile.CALL_RESTRICT_CAUSE_NONE;
        return isHighDef ? AUDIO_QUALITY_HIGH_DEFINITION : AUDIO_QUALITY_STANDARD;
    }

    /**
     * Provides a string representation of the {@link ImsPhoneConnection}.  Primarily intended for
     * use in log statements.
     *
     * @return String representation of call.
     */
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("[ImsPhoneConnection objId: ");
        sb.append(System.identityHashCode(this));
        sb.append(" telecomCallID: ");
        sb.append(getTelecomCallId());
        sb.append(" address: ");
        sb.append(Rlog.pii(LOG_TAG, getAddress()));
        sb.append(" ImsCall: ");
        if (mImsCall == null) {
            sb.append("null");
        } else {
            sb.append(mImsCall);
        }
        sb.append("]");
        return sb.toString();
    }

    /**
     * Indicates whether current phone connection is emergency or not
     * @return boolean: true if emergency, false otherwise
     */
    protected boolean isEmergency() {
        return mIsEmergency;
    }

    /**
     * Handles notifications from the {@link ImsVideoCallProviderWrapper} of session modification
     * responses received.
     *
     * @param status The status of the original request.
     * @param requestProfile The requested video profile.
     * @param responseProfile The response upon video profile.
     */
    @Override
    public void onReceiveSessionModifyResponse(int status, VideoProfile requestProfile,
            VideoProfile responseProfile) {
        if (status == android.telecom.Connection.VideoProvider.SESSION_MODIFY_REQUEST_SUCCESS &&
                mShouldIgnoreVideoStateChanges) {
            int currentVideoState = getVideoState();
            int newVideoState = responseProfile.getVideoState();

            // If the current video state is paused, the modem will not send us any changes to
            // the TX and RX bits of the video state.  Until the video is un-paused we will
            // "fake out" the video state by applying the changes that the modem reports via a
            // response.

            // First, find out whether there was a change to the TX or RX bits:
            int changedBits = currentVideoState ^ newVideoState;
            changedBits &= VideoProfile.STATE_BIDIRECTIONAL;
            if (changedBits == 0) {
                // No applicable change, bail out.
                return;
            }

            // Turn off any existing bits that changed.
            currentVideoState &= ~(changedBits & currentVideoState);
            // Turn on any new bits that turned on.
            currentVideoState |= changedBits & newVideoState;

            Rlog.d(LOG_TAG, "onReceiveSessionModifyResponse : received " +
                    VideoProfile.videoStateToString(requestProfile.getVideoState()) +
                    " / " +
                    VideoProfile.videoStateToString(responseProfile.getVideoState()) +
                    " while paused ; sending new videoState = " +
                    VideoProfile.videoStateToString(currentVideoState));
            setVideoState(currentVideoState);
        }
    }
}
