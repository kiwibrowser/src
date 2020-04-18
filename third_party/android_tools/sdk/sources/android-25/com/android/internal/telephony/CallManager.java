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

package com.android.internal.telephony;

import com.android.internal.telephony.sip.SipPhone;

import android.content.Context;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.RegistrantList;
import android.os.Registrant;
import android.telecom.VideoProfile;
import android.telephony.PhoneNumberUtils;
import android.telephony.TelephonyManager;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.Rlog;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;



/**
 * @hide
 *
 * CallManager class provides an abstract layer for PhoneApp to access
 * and control calls. It implements Phone interface.
 *
 * CallManager provides call and connection control as well as
 * channel capability.
 *
 * There are three categories of APIs CallManager provided
 *
 *  1. Call control and operation, such as dial() and hangup()
 *  2. Channel capabilities, such as CanConference()
 *  3. Register notification
 *
 *
 */
public class CallManager {

    private static final String LOG_TAG ="CallManager";
    private static final boolean DBG = true;
    private static final boolean VDBG = false;

    private static final int EVENT_DISCONNECT = 100;
    private static final int EVENT_PRECISE_CALL_STATE_CHANGED = 101;
    private static final int EVENT_NEW_RINGING_CONNECTION = 102;
    private static final int EVENT_UNKNOWN_CONNECTION = 103;
    private static final int EVENT_INCOMING_RING = 104;
    private static final int EVENT_RINGBACK_TONE = 105;
    private static final int EVENT_IN_CALL_VOICE_PRIVACY_ON = 106;
    private static final int EVENT_IN_CALL_VOICE_PRIVACY_OFF = 107;
    private static final int EVENT_CALL_WAITING = 108;
    private static final int EVENT_DISPLAY_INFO = 109;
    private static final int EVENT_SIGNAL_INFO = 110;
    private static final int EVENT_CDMA_OTA_STATUS_CHANGE = 111;
    private static final int EVENT_RESEND_INCALL_MUTE = 112;
    private static final int EVENT_MMI_INITIATE = 113;
    private static final int EVENT_MMI_COMPLETE = 114;
    private static final int EVENT_ECM_TIMER_RESET = 115;
    private static final int EVENT_SUBSCRIPTION_INFO_READY = 116;
    private static final int EVENT_SUPP_SERVICE_FAILED = 117;
    private static final int EVENT_SERVICE_STATE_CHANGED = 118;
    private static final int EVENT_POST_DIAL_CHARACTER = 119;
    private static final int EVENT_ONHOLD_TONE = 120;
    // FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
    //private static final int EVENT_RADIO_OFF_OR_NOT_AVAILABLE = 121;
    private static final int EVENT_TTY_MODE_RECEIVED = 122;

    // Singleton instance
    private static final CallManager INSTANCE = new CallManager();

    // list of registered phones, which are Phone objs
    private final ArrayList<Phone> mPhones;

    // list of supported ringing calls
    private final ArrayList<Call> mRingingCalls;

    // list of supported background calls
    private final ArrayList<Call> mBackgroundCalls;

    // list of supported foreground calls
    private final ArrayList<Call> mForegroundCalls;

    // empty connection list
    private final ArrayList<Connection> mEmptyConnections = new ArrayList<Connection>();

    // mapping of phones to registered handler instances used for callbacks from RIL
    private final HashMap<Phone, CallManagerHandler> mHandlerMap = new HashMap<>();

    // default phone as the first phone registered, which is Phone obj
    private Phone mDefaultPhone;

    private boolean mSpeedUpAudioForMtCall = false;
    // FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
    //private boolean mIsEccDialing = false;

    private Object mRegistrantidentifier = new Object();

    // state registrants
    protected final RegistrantList mPreciseCallStateRegistrants
    = new RegistrantList();

    protected final RegistrantList mNewRingingConnectionRegistrants
    = new RegistrantList();

    protected final RegistrantList mIncomingRingRegistrants
    = new RegistrantList();

    protected final RegistrantList mDisconnectRegistrants
    = new RegistrantList();

    protected final RegistrantList mMmiRegistrants
    = new RegistrantList();

    protected final RegistrantList mUnknownConnectionRegistrants
    = new RegistrantList();

    protected final RegistrantList mRingbackToneRegistrants
    = new RegistrantList();

    protected final RegistrantList mOnHoldToneRegistrants
    = new RegistrantList();

    protected final RegistrantList mInCallVoicePrivacyOnRegistrants
    = new RegistrantList();

    protected final RegistrantList mInCallVoicePrivacyOffRegistrants
    = new RegistrantList();

    protected final RegistrantList mCallWaitingRegistrants
    = new RegistrantList();

    protected final RegistrantList mDisplayInfoRegistrants
    = new RegistrantList();

    protected final RegistrantList mSignalInfoRegistrants
    = new RegistrantList();

    protected final RegistrantList mCdmaOtaStatusChangeRegistrants
    = new RegistrantList();

    protected final RegistrantList mResendIncallMuteRegistrants
    = new RegistrantList();

    protected final RegistrantList mMmiInitiateRegistrants
    = new RegistrantList();

    protected final RegistrantList mMmiCompleteRegistrants
    = new RegistrantList();

    protected final RegistrantList mEcmTimerResetRegistrants
    = new RegistrantList();

    protected final RegistrantList mSubscriptionInfoReadyRegistrants
    = new RegistrantList();

    protected final RegistrantList mSuppServiceFailedRegistrants
    = new RegistrantList();

    protected final RegistrantList mServiceStateChangedRegistrants
    = new RegistrantList();

    protected final RegistrantList mPostDialCharacterRegistrants
    = new RegistrantList();

    protected final RegistrantList mTtyModeReceivedRegistrants
    = new RegistrantList();

    private CallManager() {
        mPhones = new ArrayList<Phone>();
        mRingingCalls = new ArrayList<Call>();
        mBackgroundCalls = new ArrayList<Call>();
        mForegroundCalls = new ArrayList<Call>();
        mDefaultPhone = null;
    }

    /**
     * get singleton instance of CallManager
     * @return CallManager
     */
    public static CallManager getInstance() {
        return INSTANCE;
    }

    /**
     * Returns all the registered phone objects.
     * @return all the registered phone objects.
     */
    public List<Phone> getAllPhones() {
        return Collections.unmodifiableList(mPhones);
    }

    /**
     * get Phone object corresponds to subId
     * @return Phone
     */
    private Phone getPhone(int subId) {
        Phone p = null;
        for (Phone phone : mPhones) {
            if (phone.getSubId() == subId &&
                    phone.getPhoneType() != PhoneConstants.PHONE_TYPE_IMS) {
                p = phone;
                break;
            }
        }
        return p;
    }

    /**
     * Get current coarse-grained voice call state.
     * If the Call Manager has an active call and call waiting occurs,
     * then the phone state is RINGING not OFFHOOK
     *
     */
    public PhoneConstants.State getState() {
        PhoneConstants.State s = PhoneConstants.State.IDLE;

        for (Phone phone : mPhones) {
            if (phone.getState() == PhoneConstants.State.RINGING) {
                s = PhoneConstants.State.RINGING;
            } else if (phone.getState() == PhoneConstants.State.OFFHOOK) {
                if (s == PhoneConstants.State.IDLE) s = PhoneConstants.State.OFFHOOK;
            }
        }
        return s;
    }

    /**
     * Get current coarse-grained voice call state on a subId.
     * If the Call Manager has an active call and call waiting occurs,
     * then the phone state is RINGING not OFFHOOK
     *
     */
    public PhoneConstants.State getState(int subId) {
        PhoneConstants.State s = PhoneConstants.State.IDLE;

        for (Phone phone : mPhones) {
            if (phone.getSubId() == subId) {
                if (phone.getState() == PhoneConstants.State.RINGING) {
                    s = PhoneConstants.State.RINGING;
                } else if (phone.getState() == PhoneConstants.State.OFFHOOK) {
                    if (s == PhoneConstants.State.IDLE) s = PhoneConstants.State.OFFHOOK;
                }
            }
        }
        return s;
    }

    /**
     * @return the service state of CallManager, which represents the
     * highest priority state of all the service states of phones
     *
     * The priority is defined as
     *
     * STATE_IN_SERIVCE > STATE_OUT_OF_SERIVCE > STATE_EMERGENCY > STATE_POWER_OFF
     *
     */

    public int getServiceState() {
        int resultState = ServiceState.STATE_OUT_OF_SERVICE;

        for (Phone phone : mPhones) {
            int serviceState = phone.getServiceState().getState();
            if (serviceState == ServiceState.STATE_IN_SERVICE) {
                // IN_SERVICE has the highest priority
                resultState = serviceState;
                break;
            } else if (serviceState == ServiceState.STATE_OUT_OF_SERVICE) {
                // OUT_OF_SERVICE replaces EMERGENCY_ONLY and POWER_OFF
                // Note: EMERGENCY_ONLY is not in use at this moment
                if ( resultState == ServiceState.STATE_EMERGENCY_ONLY ||
                        resultState == ServiceState.STATE_POWER_OFF) {
                    resultState = serviceState;
                }
            } else if (serviceState == ServiceState.STATE_EMERGENCY_ONLY) {
                if (resultState == ServiceState.STATE_POWER_OFF) {
                    resultState = serviceState;
                }
            }
        }
        return resultState;
    }

    /**
     * @return the Phone service state corresponds to subId
     */
    public int getServiceState(int subId) {
        int resultState = ServiceState.STATE_OUT_OF_SERVICE;

        for (Phone phone : mPhones) {
            if (phone.getSubId() == subId) {
                int serviceState = phone.getServiceState().getState();
                if (serviceState == ServiceState.STATE_IN_SERVICE) {
                    // IN_SERVICE has the highest priority
                    resultState = serviceState;
                    break;
                } else if (serviceState == ServiceState.STATE_OUT_OF_SERVICE) {
                    // OUT_OF_SERVICE replaces EMERGENCY_ONLY and POWER_OFF
                    // Note: EMERGENCY_ONLY is not in use at this moment
                    if ( resultState == ServiceState.STATE_EMERGENCY_ONLY ||
                            resultState == ServiceState.STATE_POWER_OFF) {
                        resultState = serviceState;
                    }
                } else if (serviceState == ServiceState.STATE_EMERGENCY_ONLY) {
                    if (resultState == ServiceState.STATE_POWER_OFF) {
                        resultState = serviceState;
                    }
                }
            }
        }
        return resultState;
    }

    /**
     * @return the phone associated with any call
     */
    public Phone getPhoneInCall() {
        Phone phone = null;
        if (!getFirstActiveRingingCall().isIdle()) {
            phone = getFirstActiveRingingCall().getPhone();
        } else if (!getActiveFgCall().isIdle()) {
            phone = getActiveFgCall().getPhone();
        } else {
            // If BG call is idle, we return default phone
            phone = getFirstActiveBgCall().getPhone();
        }
        return phone;
    }

    public Phone getPhoneInCall(int subId) {
        Phone phone = null;
        if (!getFirstActiveRingingCall(subId).isIdle()) {
            phone = getFirstActiveRingingCall(subId).getPhone();
        } else if (!getActiveFgCall(subId).isIdle()) {
            phone = getActiveFgCall(subId).getPhone();
        } else {
            // If BG call is idle, we return default phone
            phone = getFirstActiveBgCall(subId).getPhone();
        }
        return phone;
    }

    /**
     * Register phone to CallManager
     * @param phone to be registered
     * @return true if register successfully
     */
    public boolean registerPhone(Phone phone) {
        if (phone != null && !mPhones.contains(phone)) {

            if (DBG) {
                Rlog.d(LOG_TAG, "registerPhone(" +
                        phone.getPhoneName() + " " + phone + ")");
            }

            if (mPhones.isEmpty()) {
                mDefaultPhone = phone;
            }
            mPhones.add(phone);
            mRingingCalls.add(phone.getRingingCall());
            mBackgroundCalls.add(phone.getBackgroundCall());
            mForegroundCalls.add(phone.getForegroundCall());
            registerForPhoneStates(phone);
            return true;
        }
        return false;
    }

    /**
     * unregister phone from CallManager
     * @param phone to be unregistered
     */
    public void unregisterPhone(Phone phone) {
        if (phone != null && mPhones.contains(phone)) {

            if (DBG) {
                Rlog.d(LOG_TAG, "unregisterPhone(" +
                        phone.getPhoneName() + " " + phone + ")");
            }

            Phone imsPhone = phone.getImsPhone();
            if (imsPhone != null) {
                unregisterPhone(imsPhone);
            }

            mPhones.remove(phone);
            mRingingCalls.remove(phone.getRingingCall());
            mBackgroundCalls.remove(phone.getBackgroundCall());
            mForegroundCalls.remove(phone.getForegroundCall());
            unregisterForPhoneStates(phone);
            if (phone == mDefaultPhone) {
                if (mPhones.isEmpty()) {
                    mDefaultPhone = null;
                } else {
                    mDefaultPhone = mPhones.get(0);
                }
            }
        }
    }

    /**
     * return the default phone or null if no phone available
     */
    public Phone getDefaultPhone() {
        return mDefaultPhone;
    }

    /**
     * @return the phone associated with the foreground call
     */
    public Phone getFgPhone() {
        return getActiveFgCall().getPhone();
    }

    /**
     * @return the phone associated with the foreground call
     * of a particular subId
     */
    public Phone getFgPhone(int subId) {
        return getActiveFgCall(subId).getPhone();
    }

    /**
     * @return the phone associated with the background call
     */
    public Phone getBgPhone() {
        return getFirstActiveBgCall().getPhone();
    }

    /**
     * @return the phone associated with the background call
     * of a particular subId
     */
    public Phone getBgPhone(int subId) {
        return getFirstActiveBgCall(subId).getPhone();
    }

    /**
     * @return the phone associated with the ringing call
     */
    public Phone getRingingPhone() {
        return getFirstActiveRingingCall().getPhone();
    }

    /**
     * @return the phone associated with the ringing call
     * of a particular subId
     */
    public Phone getRingingPhone(int subId) {
        return getFirstActiveRingingCall(subId).getPhone();
    }

    /* FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
    public void setAudioMode() {
        Context context = getContext();
        if (context == null) return;
        AudioManager audioManager = (AudioManager)
                context.getSystemService(Context.AUDIO_SERVICE);

        if (!isServiceStateInService() && !mIsEccDialing) {
            if (audioManager.getMode() != AudioManager.MODE_NORMAL) {
                if (VDBG) Rlog.d(LOG_TAG, "abandonAudioFocus");
                // abandon audio focus after the mode has been set back to normal
                audioManager.abandonAudioFocusForCall();
                audioManager.setMode(AudioManager.MODE_NORMAL);
            }
            return;
        }

        // change the audio mode and request/abandon audio focus according to phone state,
        // but only on audio mode transitions
        switch (getState()) {
            case RINGING:
                int curAudioMode = audioManager.getMode();
                if (curAudioMode != AudioManager.MODE_RINGTONE) {
                    if (VDBG) Rlog.d(LOG_TAG, "requestAudioFocus on STREAM_RING");
                    audioManager.requestAudioFocusForCall(AudioManager.STREAM_RING,
                            AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
                    if(!mSpeedUpAudioForMtCall) {
                        audioManager.setMode(AudioManager.MODE_RINGTONE);
                    }
                }

                if (mSpeedUpAudioForMtCall && (curAudioMode != AudioManager.MODE_IN_CALL)) {
                    audioManager.setMode(AudioManager.MODE_IN_CALL);
                }
                break;
            case OFFHOOK:
                Phone offhookPhone = getFgPhone();
                if (getActiveFgCallState() == Call.State.IDLE) {
                    // There is no active Fg calls, the OFFHOOK state
                    // is set by the Bg call. So set the phone to bgPhone.
                    offhookPhone = getBgPhone();
                }

                int newAudioMode = AudioManager.MODE_IN_CALL;
                if (offhookPhone instanceof SipPhone) {
                    Rlog.d(LOG_TAG, "setAudioMode Set audio mode for SIP call!");
                    // enable IN_COMMUNICATION audio mode instead for sipPhone
                    newAudioMode = AudioManager.MODE_IN_COMMUNICATION;
                }
                int currMode = audioManager.getMode();
                if (currMode != newAudioMode || mSpeedUpAudioForMtCall) {
                    // request audio focus before setting the new mode
                    if (VDBG) Rlog.d(LOG_TAG, "requestAudioFocus on STREAM_VOICE_CALL");
                    audioManager.requestAudioFocusForCall(AudioManager.STREAM_VOICE_CALL,
                            AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
                    Rlog.d(LOG_TAG, "setAudioMode Setting audio mode from "
                            + currMode + " to " + newAudioMode);
                    audioManager.setMode(newAudioMode);
                }
                mSpeedUpAudioForMtCall = false;
                break;
            case IDLE:
                if (audioManager.getMode() != AudioManager.MODE_NORMAL) {
                    audioManager.setMode(AudioManager.MODE_NORMAL);
                    if (VDBG) Rlog.d(LOG_TAG, "abandonAudioFocus");
                    // abandon audio focus after the mode has been set back to normal
                    audioManager.abandonAudioFocusForCall();
                }
                mSpeedUpAudioForMtCall = false;
                break;
        }
        Rlog.d(LOG_TAG, "setAudioMode state = " + getState());
    }
    */

    private Context getContext() {
        Phone defaultPhone = getDefaultPhone();
        return ((defaultPhone == null) ? null : defaultPhone.getContext());
    }

    public Object getRegistrantIdentifier() {
        return mRegistrantidentifier;
    }

    private void registerForPhoneStates(Phone phone) {
        // We need to keep a mapping of handler to Phone for proper unregistration.
        // TODO: Clean up this solution as it is just a work around for each Phone instance
        // using the same Handler to register with the RIL. When time permits, we should consider
        // moving the handler (or the reference ot the handler) into the Phone object.
        // See b/17414427.
        CallManagerHandler handler = mHandlerMap.get(phone);
        if (handler != null) {
            Rlog.d(LOG_TAG, "This phone has already been registered.");
            return;
        }

        // New registration, create a new handler instance and register the phone.
        handler = new CallManagerHandler();
        mHandlerMap.put(phone, handler);

        // for common events supported by all phones
        // The mRegistrantIdentifier passed here, is to identify in the Phone
        // that the registrants are coming from the CallManager.
        phone.registerForPreciseCallStateChanged(handler, EVENT_PRECISE_CALL_STATE_CHANGED,
                mRegistrantidentifier);
        phone.registerForDisconnect(handler, EVENT_DISCONNECT,
                mRegistrantidentifier);
        phone.registerForNewRingingConnection(handler, EVENT_NEW_RINGING_CONNECTION,
                mRegistrantidentifier);
        phone.registerForUnknownConnection(handler, EVENT_UNKNOWN_CONNECTION,
                mRegistrantidentifier);
        phone.registerForIncomingRing(handler, EVENT_INCOMING_RING,
                mRegistrantidentifier);
        phone.registerForRingbackTone(handler, EVENT_RINGBACK_TONE,
                mRegistrantidentifier);
        phone.registerForInCallVoicePrivacyOn(handler, EVENT_IN_CALL_VOICE_PRIVACY_ON,
                mRegistrantidentifier);
        phone.registerForInCallVoicePrivacyOff(handler, EVENT_IN_CALL_VOICE_PRIVACY_OFF,
                mRegistrantidentifier);
        phone.registerForDisplayInfo(handler, EVENT_DISPLAY_INFO,
                mRegistrantidentifier);
        phone.registerForSignalInfo(handler, EVENT_SIGNAL_INFO,
                mRegistrantidentifier);
        phone.registerForResendIncallMute(handler, EVENT_RESEND_INCALL_MUTE,
                mRegistrantidentifier);
        phone.registerForMmiInitiate(handler, EVENT_MMI_INITIATE,
                mRegistrantidentifier);
        phone.registerForMmiComplete(handler, EVENT_MMI_COMPLETE,
                mRegistrantidentifier);
        phone.registerForSuppServiceFailed(handler, EVENT_SUPP_SERVICE_FAILED,
                mRegistrantidentifier);
        phone.registerForServiceStateChanged(handler, EVENT_SERVICE_STATE_CHANGED,
                mRegistrantidentifier);

        // FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
        //phone.registerForRadioOffOrNotAvailable(handler, EVENT_RADIO_OFF_OR_NOT_AVAILABLE, null);

        // for events supported only by GSM, CDMA and IMS phone
        phone.setOnPostDialCharacter(handler, EVENT_POST_DIAL_CHARACTER, null);

        // for events supported only by CDMA phone
        phone.registerForCdmaOtaStatusChange(handler, EVENT_CDMA_OTA_STATUS_CHANGE, null);
        phone.registerForSubscriptionInfoReady(handler, EVENT_SUBSCRIPTION_INFO_READY, null);
        phone.registerForCallWaiting(handler, EVENT_CALL_WAITING, null);
        phone.registerForEcmTimerReset(handler, EVENT_ECM_TIMER_RESET, null);

        // for events supported only by IMS phone
        phone.registerForOnHoldTone(handler, EVENT_ONHOLD_TONE, null);
        phone.registerForSuppServiceFailed(handler, EVENT_SUPP_SERVICE_FAILED, null);
        phone.registerForTtyModeReceived(handler, EVENT_TTY_MODE_RECEIVED, null);
    }

    private void unregisterForPhoneStates(Phone phone) {
        // Make sure that we clean up our map of handlers to Phones.
        CallManagerHandler handler = mHandlerMap.get(phone);
        if (handler == null) {
            Rlog.e(LOG_TAG, "Could not find Phone handler for unregistration");
            return;
        }
        mHandlerMap.remove(phone);

        //  for common events supported by all phones
        phone.unregisterForPreciseCallStateChanged(handler);
        phone.unregisterForDisconnect(handler);
        phone.unregisterForNewRingingConnection(handler);
        phone.unregisterForUnknownConnection(handler);
        phone.unregisterForIncomingRing(handler);
        phone.unregisterForRingbackTone(handler);
        phone.unregisterForInCallVoicePrivacyOn(handler);
        phone.unregisterForInCallVoicePrivacyOff(handler);
        phone.unregisterForDisplayInfo(handler);
        phone.unregisterForSignalInfo(handler);
        phone.unregisterForResendIncallMute(handler);
        phone.unregisterForMmiInitiate(handler);
        phone.unregisterForMmiComplete(handler);
        phone.unregisterForSuppServiceFailed(handler);
        phone.unregisterForServiceStateChanged(handler);
        phone.unregisterForTtyModeReceived(handler);
        // FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
        //phone.unregisterForRadioOffOrNotAvailable(handler);

        // for events supported only by GSM, CDMA and IMS phone
        phone.setOnPostDialCharacter(null, EVENT_POST_DIAL_CHARACTER, null);

        // for events supported only by CDMA phone
        phone.unregisterForCdmaOtaStatusChange(handler);
        phone.unregisterForSubscriptionInfoReady(handler);
        phone.unregisterForCallWaiting(handler);
        phone.unregisterForEcmTimerReset(handler);

        // for events supported only by IMS phone
        phone.unregisterForOnHoldTone(handler);
        phone.unregisterForSuppServiceFailed(handler);
    }

    /**
     * Answers a ringing or waiting call.
     *
     * Active call, if any, go on hold.
     * If active call can't be held, i.e., a background call of the same channel exists,
     * the active call will be hang up.
     *
     * Answering occurs asynchronously, and final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @exception CallStateException when call is not ringing or waiting
     */
    public void acceptCall(Call ringingCall) throws CallStateException {
        Phone ringingPhone = ringingCall.getPhone();

        if (VDBG) {
            Rlog.d(LOG_TAG, "acceptCall(" +ringingCall + " from " + ringingCall.getPhone() + ")");
            Rlog.d(LOG_TAG, toString());
        }

        if ( hasActiveFgCall() ) {
            Phone activePhone = getActiveFgCall().getPhone();
            boolean hasBgCall = ! (activePhone.getBackgroundCall().isIdle());
            boolean sameChannel = (activePhone == ringingPhone);

            if (VDBG) {
                Rlog.d(LOG_TAG, "hasBgCall: "+ hasBgCall + "sameChannel:" + sameChannel);
            }

            if (sameChannel && hasBgCall) {
                getActiveFgCall().hangup();
            } else if (!sameChannel && !hasBgCall) {
                activePhone.switchHoldingAndActive();
            } else if (!sameChannel && hasBgCall) {
                getActiveFgCall().hangup();
            }
        }

        // We only support the AUDIO_ONLY video state in this scenario.
        ringingPhone.acceptCall(VideoProfile.STATE_AUDIO_ONLY);

        if (VDBG) {
            Rlog.d(LOG_TAG, "End acceptCall(" +ringingCall + ")");
            Rlog.d(LOG_TAG, toString());
        }
    }

    /**
     * Reject (ignore) a ringing call. In GSM, this means UDUB
     * (User Determined User Busy). Reject occurs asynchronously,
     * and final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @exception CallStateException when no call is ringing or waiting
     */
    public void rejectCall(Call ringingCall) throws CallStateException {
        if (VDBG) {
            Rlog.d(LOG_TAG, "rejectCall(" +ringingCall + ")");
            Rlog.d(LOG_TAG, toString());
        }

        Phone ringingPhone = ringingCall.getPhone();

        ringingPhone.rejectCall();

        if (VDBG) {
            Rlog.d(LOG_TAG, "End rejectCall(" +ringingCall + ")");
            Rlog.d(LOG_TAG, toString());
        }
    }

    /**
     * Places active call on hold, and makes held call active.
     * Switch occurs asynchronously and may fail.
     *
     * There are 4 scenarios
     * 1. only active call but no held call, aka, hold
     * 2. no active call but only held call, aka, unhold
     * 3. both active and held calls from same phone, aka, swap
     * 4. active and held calls from different phones, aka, phone swap
     *
     * Final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @exception CallStateException if active call is ringing, waiting, or
     * dialing/alerting, or heldCall can't be active.
     * In these cases, this operation may not be performed.
     */
    public void switchHoldingAndActive(Call heldCall) throws CallStateException {
        Phone activePhone = null;
        Phone heldPhone = null;

        if (VDBG) {
            Rlog.d(LOG_TAG, "switchHoldingAndActive(" +heldCall + ")");
            Rlog.d(LOG_TAG, toString());
        }

        if (hasActiveFgCall()) {
            activePhone = getActiveFgCall().getPhone();
        }

        if (heldCall != null) {
            heldPhone = heldCall.getPhone();
        }

        if (activePhone != null) {
            activePhone.switchHoldingAndActive();
        }

        if (heldPhone != null && heldPhone != activePhone) {
            heldPhone.switchHoldingAndActive();
        }

        if (VDBG) {
            Rlog.d(LOG_TAG, "End switchHoldingAndActive(" +heldCall + ")");
            Rlog.d(LOG_TAG, toString());
        }
    }

    /**
     * Hangup foreground call and resume the specific background call
     *
     * Note: this is noop if there is no foreground call or the heldCall is null
     *
     * @param heldCall to become foreground
     * @throws CallStateException
     */
    public void hangupForegroundResumeBackground(Call heldCall) throws CallStateException {
        Phone foregroundPhone = null;
        Phone backgroundPhone = null;

        if (VDBG) {
            Rlog.d(LOG_TAG, "hangupForegroundResumeBackground(" +heldCall + ")");
            Rlog.d(LOG_TAG, toString());
        }

        if (hasActiveFgCall()) {
            foregroundPhone = getFgPhone();
            if (heldCall != null) {
                backgroundPhone = heldCall.getPhone();
                if (foregroundPhone == backgroundPhone) {
                    getActiveFgCall().hangup();
                } else {
                // the call to be hangup and resumed belongs to different phones
                    getActiveFgCall().hangup();
                    switchHoldingAndActive(heldCall);
                }
            }
        }

        if (VDBG) {
            Rlog.d(LOG_TAG, "End hangupForegroundResumeBackground(" +heldCall + ")");
            Rlog.d(LOG_TAG, toString());
        }
    }

    /**
     * Whether or not the phone can conference in the current phone
     * state--that is, one call holding and one call active.
     * @return true if the phone can conference; false otherwise.
     */
    public boolean canConference(Call heldCall) {
        Phone activePhone = null;
        Phone heldPhone = null;

        if (hasActiveFgCall()) {
            activePhone = getActiveFgCall().getPhone();
        }

        if (heldCall != null) {
            heldPhone = heldCall.getPhone();
        }

        return heldPhone.getClass().equals(activePhone.getClass());
    }

    /**
     * Whether or not the phone can conference in the current phone
     * state--that is, one call holding and one call active.
     * This method consider the phone object which is specific
     * to the provided subId.
     * @return true if the phone can conference; false otherwise.
     */
    public boolean canConference(Call heldCall, int subId) {
        Phone activePhone = null;
        Phone heldPhone = null;

        if (hasActiveFgCall(subId)) {
            activePhone = getActiveFgCall(subId).getPhone();
        }

        if (heldCall != null) {
            heldPhone = heldCall.getPhone();
        }

        return heldPhone.getClass().equals(activePhone.getClass());
    }

    /**
     * Conferences holding and active. Conference occurs asynchronously
     * and may fail. Final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @exception CallStateException if canConference() would return false.
     * In these cases, this operation may not be performed.
     */
    public void conference(Call heldCall) throws CallStateException {
        int subId  = heldCall.getPhone().getSubId();

        if (VDBG) {
            Rlog.d(LOG_TAG, "conference(" +heldCall + ")");
            Rlog.d(LOG_TAG, toString());
        }

        Phone fgPhone = getFgPhone(subId);
        if (fgPhone != null) {
            if (fgPhone instanceof SipPhone) {
                ((SipPhone) fgPhone).conference(heldCall);
            } else if (canConference(heldCall)) {
                fgPhone.conference();
            } else {
                throw(new CallStateException("Can't conference foreground and selected background call"));
            }
        } else {
            Rlog.d(LOG_TAG, "conference: fgPhone=null");
        }

        if (VDBG) {
            Rlog.d(LOG_TAG, "End conference(" +heldCall + ")");
            Rlog.d(LOG_TAG, toString());
        }

    }

    /**
     * Initiate a new voice connection. This happens asynchronously, so you
     * cannot assume the audio path is connected (or a call index has been
     * assigned) until PhoneStateChanged notification has occurred.
     *
     * @exception CallStateException if a new outgoing call is not currently
     * possible because no more call slots exist or a call exists that is
     * dialing, alerting, ringing, or waiting.  Other errors are
     * handled asynchronously.
     */
    public Connection dial(Phone phone, String dialString, int videoState)
            throws CallStateException {
        int subId = phone.getSubId();
        Connection result;

        if (VDBG) {
            Rlog.d(LOG_TAG, " dial(" + phone + ", "+ dialString + ")" +
                    " subId = " + subId);
            Rlog.d(LOG_TAG, toString());
        }

        if (!canDial(phone)) {
            /*
             * canDial function only checks whether the phone can make a new call.
             * InCall MMI commmands are basically supplementary services
             * within a call eg: call hold, call deflection, explicit call transfer etc.
             */
            String newDialString = PhoneNumberUtils.stripSeparators(dialString);
            if (phone.handleInCallMmiCommands(newDialString)) {
                return null;
            } else {
                throw new CallStateException("cannot dial in current state");
            }
        }

        if ( hasActiveFgCall(subId) ) {
            Phone activePhone = getActiveFgCall(subId).getPhone();
            boolean hasBgCall = !(activePhone.getBackgroundCall().isIdle());

            if (DBG) {
                Rlog.d(LOG_TAG, "hasBgCall: "+ hasBgCall + " sameChannel:" + (activePhone == phone));
            }

            // Manipulation between IMS phone and its owner
            // will be treated in GSM/CDMA phone.
            Phone imsPhone = phone.getImsPhone();
            if (activePhone != phone
                    && (imsPhone == null || imsPhone != activePhone)) {
                if (hasBgCall) {
                    Rlog.d(LOG_TAG, "Hangup");
                    getActiveFgCall(subId).hangup();
                } else {
                    Rlog.d(LOG_TAG, "Switch");
                    activePhone.switchHoldingAndActive();
                }
            }
        }

        // FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
        //mIsEccDialing = PhoneNumberUtils.isEmergencyNumber(dialString);

        result = phone.dial(dialString, videoState);

        if (VDBG) {
            Rlog.d(LOG_TAG, "End dial(" + phone + ", "+ dialString + ")");
            Rlog.d(LOG_TAG, toString());
        }

        return result;
    }

    /**
     * Initiate a new voice connection. This happens asynchronously, so you
     * cannot assume the audio path is connected (or a call index has been
     * assigned) until PhoneStateChanged notification has occurred.
     *
     * @exception CallStateException if a new outgoing call is not currently
     * possible because no more call slots exist or a call exists that is
     * dialing, alerting, ringing, or waiting.  Other errors are
     * handled asynchronously.
     */
    public Connection dial(Phone phone, String dialString, UUSInfo uusInfo, int videoState)
            throws CallStateException {
        return phone.dial(dialString, uusInfo, videoState, null);
    }

    /**
     * clear disconnect connection for each phone
     */
    public void clearDisconnected() {
        for(Phone phone : mPhones) {
            phone.clearDisconnected();
        }
    }

    /**
     * clear disconnect connection for a phone specific
     * to the provided subId
     */
    public void clearDisconnected(int subId) {
        for(Phone phone : mPhones) {
            if (phone.getSubId() == subId) {
                phone.clearDisconnected();
            }
        }
    }

    /**
     * Phone can make a call only if ALL of the following are true:
     *        - Phone is not powered off
     *        - There's no incoming or waiting call
     *        - The foreground call is ACTIVE or IDLE or DISCONNECTED.
     *          (We mainly need to make sure it *isn't* DIALING or ALERTING.)
     * @param phone
     * @return true if the phone can make a new call
     */
    private boolean canDial(Phone phone) {
        int serviceState = phone.getServiceState().getState();
        int subId = phone.getSubId();
        boolean hasRingingCall = hasActiveRingingCall();
        Call.State fgCallState = getActiveFgCallState(subId);

        boolean result = (serviceState != ServiceState.STATE_POWER_OFF
                && !hasRingingCall
                && ((fgCallState == Call.State.ACTIVE)
                    || (fgCallState == Call.State.IDLE)
                    || (fgCallState == Call.State.DISCONNECTED)
                    /*As per 3GPP TS 51.010-1 section 31.13.1.4
                    call should be alowed when the foreground
                    call is in ALERTING state*/
                    || (fgCallState == Call.State.ALERTING)));

        if (result == false) {
            Rlog.d(LOG_TAG, "canDial serviceState=" + serviceState
                            + " hasRingingCall=" + hasRingingCall
                            + " fgCallState=" + fgCallState);
        }
        return result;
    }

    /**
     * Whether or not the phone can do explicit call transfer in the current
     * phone state--that is, one call holding and one call active.
     * @return true if the phone can do explicit call transfer; false otherwise.
     */
    public boolean canTransfer(Call heldCall) {
        Phone activePhone = null;
        Phone heldPhone = null;

        if (hasActiveFgCall()) {
            activePhone = getActiveFgCall().getPhone();
        }

        if (heldCall != null) {
            heldPhone = heldCall.getPhone();
        }

        return (heldPhone == activePhone && activePhone.canTransfer());
    }

    /**
     * Whether or not the phone specific to subId can do explicit call transfer
     * in the current phone state--that is, one call holding and one call active.
     * @return true if the phone can do explicit call transfer; false otherwise.
     */
    public boolean canTransfer(Call heldCall, int subId) {
        Phone activePhone = null;
        Phone heldPhone = null;

        if (hasActiveFgCall(subId)) {
            activePhone = getActiveFgCall(subId).getPhone();
        }

        if (heldCall != null) {
            heldPhone = heldCall.getPhone();
        }

        return (heldPhone == activePhone && activePhone.canTransfer());
    }

    /**
     * Connects the held call and active call
     * Disconnects the subscriber from both calls
     *
     * Explicit Call Transfer occurs asynchronously
     * and may fail. Final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @exception CallStateException if canTransfer() would return false.
     * In these cases, this operation may not be performed.
     */
    public void explicitCallTransfer(Call heldCall) throws CallStateException {
        if (VDBG) {
            Rlog.d(LOG_TAG, " explicitCallTransfer(" + heldCall + ")");
            Rlog.d(LOG_TAG, toString());
        }

        if (canTransfer(heldCall)) {
            heldCall.getPhone().explicitCallTransfer();
        }

        if (VDBG) {
            Rlog.d(LOG_TAG, "End explicitCallTransfer(" + heldCall + ")");
            Rlog.d(LOG_TAG, toString());
        }

    }

    /**
     * Returns a list of MMI codes that are pending for a phone. (They have initiated
     * but have not yet completed).
     * Presently there is only ever one.
     *
     * Use <code>registerForMmiInitiate</code>
     * and <code>registerForMmiComplete</code> for change notification.
     * @return null if phone doesn't have or support mmi code
     */
    public List<? extends MmiCode> getPendingMmiCodes(Phone phone) {
        Rlog.e(LOG_TAG, "getPendingMmiCodes not implemented");
        return null;
    }

    /**
     * Sends user response to a USSD REQUEST message.  An MmiCode instance
     * representing this response is sent to handlers registered with
     * registerForMmiInitiate.
     *
     * @param ussdMessge    Message to send in the response.
     * @return false if phone doesn't support ussd service
     */
    public boolean sendUssdResponse(Phone phone, String ussdMessge) {
        Rlog.e(LOG_TAG, "sendUssdResponse not implemented");
        return false;
    }

    /**
     * Mutes or unmutes the microphone for the active call. The microphone
     * is automatically unmuted if a call is answered, dialed, or resumed
     * from a holding state.
     *
     * @param muted true to mute the microphone,
     * false to activate the microphone.
     */

    public void setMute(boolean muted) {
        if (VDBG) {
            Rlog.d(LOG_TAG, " setMute(" + muted + ")");
            Rlog.d(LOG_TAG, toString());
        }

        if (hasActiveFgCall()) {
            getActiveFgCall().getPhone().setMute(muted);
        }

        if (VDBG) {
            Rlog.d(LOG_TAG, "End setMute(" + muted + ")");
            Rlog.d(LOG_TAG, toString());
        }
    }

    /**
     * Gets current mute status. Use
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}
     * as a change notifcation, although presently phone state changed is not
     * fired when setMute() is called.
     *
     * @return true is muting, false is unmuting
     */
    public boolean getMute() {
        if (hasActiveFgCall()) {
            return getActiveFgCall().getPhone().getMute();
        } else if (hasActiveBgCall()) {
            return getFirstActiveBgCall().getPhone().getMute();
        }
        return false;
    }

    /**
     * Enables or disables echo suppression.
     */
    public void setEchoSuppressionEnabled() {
        if (VDBG) {
            Rlog.d(LOG_TAG, " setEchoSuppression()");
            Rlog.d(LOG_TAG, toString());
        }

        if (hasActiveFgCall()) {
            getActiveFgCall().getPhone().setEchoSuppressionEnabled();
        }

        if (VDBG) {
            Rlog.d(LOG_TAG, "End setEchoSuppression()");
            Rlog.d(LOG_TAG, toString());
        }
    }

    /**
     * Play a DTMF tone on the active call.
     *
     * @param c should be one of 0-9, '*' or '#'. Other values will be
     * silently ignored.
     * @return false if no active call or the active call doesn't support
     *         dtmf tone
     */
    public boolean sendDtmf(char c) {
        boolean result = false;

        if (VDBG) {
            Rlog.d(LOG_TAG, " sendDtmf(" + c + ")");
            Rlog.d(LOG_TAG, toString());
        }

        if (hasActiveFgCall()) {
            getActiveFgCall().getPhone().sendDtmf(c);
            result = true;
        }

        if (VDBG) {
            Rlog.d(LOG_TAG, "End sendDtmf(" + c + ")");
            Rlog.d(LOG_TAG, toString());
        }
        return result;
    }

    /**
     * Start to paly a DTMF tone on the active call.
     * or there is a playing DTMF tone.
     * @param c should be one of 0-9, '*' or '#'. Other values will be
     * silently ignored.
     *
     * @return false if no active call or the active call doesn't support
     *         dtmf tone
     */
    public boolean startDtmf(char c) {
        boolean result = false;

        if (VDBG) {
            Rlog.d(LOG_TAG, " startDtmf(" + c + ")");
            Rlog.d(LOG_TAG, toString());
        }

        if (hasActiveFgCall()) {
            getActiveFgCall().getPhone().startDtmf(c);
            result = true;
        }

        if (VDBG) {
            Rlog.d(LOG_TAG, "End startDtmf(" + c + ")");
            Rlog.d(LOG_TAG, toString());
        }

        return result;
    }

    /**
     * Stop the playing DTMF tone. Ignored if there is no playing DTMF
     * tone or no active call.
     */
    public void stopDtmf() {
        if (VDBG) {
            Rlog.d(LOG_TAG, " stopDtmf()" );
            Rlog.d(LOG_TAG, toString());
        }

        if (hasActiveFgCall()) getFgPhone().stopDtmf();

        if (VDBG) {
            Rlog.d(LOG_TAG, "End stopDtmf()");
            Rlog.d(LOG_TAG, toString());
        }
    }

    /**
     * send burst DTMF tone, it can send the string as single character or multiple character
     * ignore if there is no active call or not valid digits string.
     * Valid digit means only includes characters ISO-LATIN characters 0-9, *, #
     * The difference between sendDtmf and sendBurstDtmf is sendDtmf only sends one character,
     * this api can send single character and multiple character, also, this api has response
     * back to caller.
     *
     * @param dtmfString is string representing the dialing digit(s) in the active call
     * @param on the DTMF ON length in milliseconds, or 0 for default
     * @param off the DTMF OFF length in milliseconds, or 0 for default
     * @param onComplete is the callback message when the action is processed by BP
     *
     */
    public boolean sendBurstDtmf(String dtmfString, int on, int off, Message onComplete) {
        if (hasActiveFgCall()) {
            getActiveFgCall().getPhone().sendBurstDtmf(dtmfString, on, off, onComplete);
            return true;
        }
        return false;
    }

    /**
     * Notifies when a voice connection has disconnected, either due to local
     * or remote hangup or error.
     *
     *  Messages received from this will have the following members:<p>
     *  <ul><li>Message.obj will be an AsyncResult</li>
     *  <li>AsyncResult.userObj = obj</li>
     *  <li>AsyncResult.result = a Connection object that is
     *  no longer connected.</li></ul>
     */
    public void registerForDisconnect(Handler h, int what, Object obj) {
        mDisconnectRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for voice disconnection notification.
     * Extraneous calls are tolerated silently
     */
    public void unregisterForDisconnect(Handler h){
        mDisconnectRegistrants.remove(h);
    }

    /**
     * Register for getting notifications for change in the Call State {@link Call.State}
     * This is called PreciseCallState because the call state is more precise than what
     * can be obtained using the {@link PhoneStateListener}
     *
     * Resulting events will have an AsyncResult in <code>Message.obj</code>.
     * AsyncResult.userData will be set to the obj argument here.
     * The <em>h</em> parameter is held only by a weak reference.
     */
    public void registerForPreciseCallStateChanged(Handler h, int what, Object obj){
        mPreciseCallStateRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for voice call state change notifications.
     * Extraneous calls are tolerated silently.
     */
    public void unregisterForPreciseCallStateChanged(Handler h){
        mPreciseCallStateRegistrants.remove(h);
    }

    /**
     * Notifies when a previously untracked non-ringing/waiting connection has appeared.
     * This is likely due to some other entity (eg, SIM card application) initiating a call.
     */
    public void registerForUnknownConnection(Handler h, int what, Object obj){
        mUnknownConnectionRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for unknown connection notifications.
     */
    public void unregisterForUnknownConnection(Handler h){
        mUnknownConnectionRegistrants.remove(h);
    }


    /**
     * Notifies when a new ringing or waiting connection has appeared.<p>
     *
     *  Messages received from this:
     *  Message.obj will be an AsyncResult
     *  AsyncResult.userObj = obj
     *  AsyncResult.result = a Connection. <p>
     *  Please check Connection.isRinging() to make sure the Connection
     *  has not dropped since this message was posted.
     *  If Connection.isRinging() is true, then
     *   Connection.getCall() == Phone.getRingingCall()
     */
    public void registerForNewRingingConnection(Handler h, int what, Object obj){
        mNewRingingConnectionRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for new ringing connection notification.
     * Extraneous calls are tolerated silently
     */

    public void unregisterForNewRingingConnection(Handler h){
        mNewRingingConnectionRegistrants.remove(h);
    }

    /**
     * Notifies when an incoming call rings.<p>
     *
     *  Messages received from this:
     *  Message.obj will be an AsyncResult
     *  AsyncResult.userObj = obj
     *  AsyncResult.result = a Connection. <p>
     */
    public void registerForIncomingRing(Handler h, int what, Object obj){
        mIncomingRingRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for ring notification.
     * Extraneous calls are tolerated silently
     */

    public void unregisterForIncomingRing(Handler h){
        mIncomingRingRegistrants.remove(h);
    }

    /**
     * Notifies when out-band ringback tone is needed.<p>
     *
     *  Messages received from this:
     *  Message.obj will be an AsyncResult
     *  AsyncResult.userObj = obj
     *  AsyncResult.result = boolean, true to start play ringback tone
     *                       and false to stop. <p>
     */
    public void registerForRingbackTone(Handler h, int what, Object obj){
        mRingbackToneRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for ringback tone notification.
     */

    public void unregisterForRingbackTone(Handler h){
        mRingbackToneRegistrants.remove(h);
    }

    /**
     * Notifies when out-band on-hold tone is needed.<p>
     *
     *  Messages received from this:
     *  Message.obj will be an AsyncResult
     *  AsyncResult.userObj = obj
     *  AsyncResult.result = boolean, true to start play on-hold tone
     *                       and false to stop. <p>
     */
    public void registerForOnHoldTone(Handler h, int what, Object obj){
        mOnHoldToneRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for on-hold tone notification.
     */

    public void unregisterForOnHoldTone(Handler h){
        mOnHoldToneRegistrants.remove(h);
    }

    /**
     * Registers the handler to reset the uplink mute state to get
     * uplink audio.
     */
    public void registerForResendIncallMute(Handler h, int what, Object obj){
        mResendIncallMuteRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for resend incall mute notifications.
     */
    public void unregisterForResendIncallMute(Handler h){
        mResendIncallMuteRegistrants.remove(h);
    }

    /**
     * Register for notifications of initiation of a new MMI code request.
     * MMI codes for GSM are discussed in 3GPP TS 22.030.<p>
     *
     * Example: If Phone.dial is called with "*#31#", then the app will
     * be notified here.<p>
     *
     * The returned <code>Message.obj</code> will contain an AsyncResult.
     *
     * <code>obj.result</code> will be an "MmiCode" object.
     */
    public void registerForMmiInitiate(Handler h, int what, Object obj){
        mMmiInitiateRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for new MMI initiate notification.
     * Extraneous calls are tolerated silently
     */
    public void unregisterForMmiInitiate(Handler h){
        mMmiInitiateRegistrants.remove(h);
    }

    /**
     * Register for notifications that an MMI request has completed
     * its network activity and is in its final state. This may mean a state
     * of COMPLETE, FAILED, or CANCELLED.
     *
     * <code>Message.obj</code> will contain an AsyncResult.
     * <code>obj.result</code> will be an "MmiCode" object
     */
    public void registerForMmiComplete(Handler h, int what, Object obj){
        mMmiCompleteRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for MMI complete notification.
     * Extraneous calls are tolerated silently
     */
    public void unregisterForMmiComplete(Handler h){
        mMmiCompleteRegistrants.remove(h);
    }

    /**
     * Registration point for Ecm timer reset
     * @param h handler to notify
     * @param what user-defined message code
     * @param obj placed in Message.obj
     */
    public void registerForEcmTimerReset(Handler h, int what, Object obj){
        mEcmTimerResetRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregister for notification for Ecm timer reset
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForEcmTimerReset(Handler h){
        mEcmTimerResetRegistrants.remove(h);
    }

    /**
     * Register for ServiceState changed.
     * Message.obj will contain an AsyncResult.
     * AsyncResult.result will be a ServiceState instance
     */
    public void registerForServiceStateChanged(Handler h, int what, Object obj){
        mServiceStateChangedRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for ServiceStateChange notification.
     * Extraneous calls are tolerated silently
     */
    public void unregisterForServiceStateChanged(Handler h){
        mServiceStateChangedRegistrants.remove(h);
    }

    /**
     * Register for notifications when a supplementary service attempt fails.
     * Message.obj will contain an AsyncResult.
     *
     * @param h Handler that receives the notification message.
     * @param what User-defined message code.
     * @param obj User object.
     */
    public void registerForSuppServiceFailed(Handler h, int what, Object obj){
        mSuppServiceFailedRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregister for notifications when a supplementary service attempt fails.
     * Extraneous calls are tolerated silently
     *
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForSuppServiceFailed(Handler h){
        mSuppServiceFailedRegistrants.remove(h);
    }

    /**
     * Register for notifications when a sInCall VoicePrivacy is enabled
     *
     * @param h Handler that receives the notification message.
     * @param what User-defined message code.
     * @param obj User object.
     */
    public void registerForInCallVoicePrivacyOn(Handler h, int what, Object obj){
        mInCallVoicePrivacyOnRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregister for notifications when a sInCall VoicePrivacy is enabled
     *
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForInCallVoicePrivacyOn(Handler h){
        mInCallVoicePrivacyOnRegistrants.remove(h);
    }

    /**
     * Register for notifications when a sInCall VoicePrivacy is disabled
     *
     * @param h Handler that receives the notification message.
     * @param what User-defined message code.
     * @param obj User object.
     */
    public void registerForInCallVoicePrivacyOff(Handler h, int what, Object obj){
        mInCallVoicePrivacyOffRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregister for notifications when a sInCall VoicePrivacy is disabled
     *
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForInCallVoicePrivacyOff(Handler h){
        mInCallVoicePrivacyOffRegistrants.remove(h);
    }

    /**
     * Register for notifications when CDMA call waiting comes
     *
     * @param h Handler that receives the notification message.
     * @param what User-defined message code.
     * @param obj User object.
     */
    public void registerForCallWaiting(Handler h, int what, Object obj){
        mCallWaitingRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregister for notifications when CDMA Call waiting comes
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForCallWaiting(Handler h){
        mCallWaitingRegistrants.remove(h);
    }


    /**
     * Register for signal information notifications from the network.
     * Message.obj will contain an AsyncResult.
     * AsyncResult.result will be a SuppServiceNotification instance.
     *
     * @param h Handler that receives the notification message.
     * @param what User-defined message code.
     * @param obj User object.
     */

    public void registerForSignalInfo(Handler h, int what, Object obj){
        mSignalInfoRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for signal information notifications.
     * Extraneous calls are tolerated silently
     *
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForSignalInfo(Handler h){
        mSignalInfoRegistrants.remove(h);
    }

    /**
     * Register for display information notifications from the network.
     * Message.obj will contain an AsyncResult.
     * AsyncResult.result will be a SuppServiceNotification instance.
     *
     * @param h Handler that receives the notification message.
     * @param what User-defined message code.
     * @param obj User object.
     */
    public void registerForDisplayInfo(Handler h, int what, Object obj){
        mDisplayInfoRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for display information notifications.
     * Extraneous calls are tolerated silently
     *
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForDisplayInfo(Handler h) {
        mDisplayInfoRegistrants.remove(h);
    }

    /**
     * Register for notifications when CDMA OTA Provision status change
     *
     * @param h Handler that receives the notification message.
     * @param what User-defined message code.
     * @param obj User object.
     */
    public void registerForCdmaOtaStatusChange(Handler h, int what, Object obj){
        mCdmaOtaStatusChangeRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregister for notifications when CDMA OTA Provision status change
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForCdmaOtaStatusChange(Handler h){
        mCdmaOtaStatusChangeRegistrants.remove(h);
    }

    /**
     * Registration point for subscription info ready
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForSubscriptionInfoReady(Handler h, int what, Object obj){
        mSubscriptionInfoReadyRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregister for notifications for subscription info
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForSubscriptionInfoReady(Handler h){
        mSubscriptionInfoReadyRegistrants.remove(h);
    }

    /**
     * Sets an event to be fired when the telephony system processes
     * a post-dial character on an outgoing call.<p>
     *
     * Messages of type <code>what</code> will be sent to <code>h</code>.
     * The <code>obj</code> field of these Message's will be instances of
     * <code>AsyncResult</code>. <code>Message.obj.result</code> will be
     * a Connection object.<p>
     *
     * Message.arg1 will be the post dial character being processed,
     * or 0 ('\0') if end of string.<p>
     *
     * If Connection.getPostDialState() == WAIT,
     * the application must call
     * {@link com.android.internal.telephony.Connection#proceedAfterWaitChar()
     * Connection.proceedAfterWaitChar()} or
     * {@link com.android.internal.telephony.Connection#cancelPostDial()
     * Connection.cancelPostDial()}
     * for the telephony system to continue playing the post-dial
     * DTMF sequence.<p>
     *
     * If Connection.getPostDialState() == WILD,
     * the application must call
     * {@link com.android.internal.telephony.Connection#proceedAfterWildChar
     * Connection.proceedAfterWildChar()}
     * or
     * {@link com.android.internal.telephony.Connection#cancelPostDial()
     * Connection.cancelPostDial()}
     * for the telephony system to continue playing the
     * post-dial DTMF sequence.<p>
     *
     */
    public void registerForPostDialCharacter(Handler h, int what, Object obj){
        mPostDialCharacterRegistrants.addUnique(h, what, obj);
    }

    public void unregisterForPostDialCharacter(Handler h){
        mPostDialCharacterRegistrants.remove(h);
    }

    /**
     * Register for TTY mode change notifications from the network.
     * Message.obj will contain an AsyncResult.
     * AsyncResult.result will be an Integer containing new mode.
     *
     * @param h Handler that receives the notification message.
     * @param what User-defined message code.
     * @param obj User object.
     */
    public void registerForTtyModeReceived(Handler h, int what, Object obj){
        mTtyModeReceivedRegistrants.addUnique(h, what, obj);
    }

    /**
     * Unregisters for TTY mode change notifications.
     * Extraneous calls are tolerated silently
     *
     * @param h Handler to be removed from the registrant list.
     */
    public void unregisterForTtyModeReceived(Handler h) {
        mTtyModeReceivedRegistrants.remove(h);
    }

    /* APIs to access foregroudCalls, backgroudCalls, and ringingCalls
     * 1. APIs to access list of calls
     * 2. APIs to check if any active call, which has connection other than
     * disconnected ones, pleaser refer to Call.isIdle()
     * 3. APIs to return first active call
     * 4. APIs to return the connections of first active call
     * 5. APIs to return other property of first active call
     */

    /**
     * @return list of all ringing calls
     */
    public List<Call> getRingingCalls() {
        return Collections.unmodifiableList(mRingingCalls);
    }

    /**
     * @return list of all foreground calls
     */
    public List<Call> getForegroundCalls() {
        return Collections.unmodifiableList(mForegroundCalls);
    }

    /**
     * @return list of all background calls
     */
    public List<Call> getBackgroundCalls() {
        return Collections.unmodifiableList(mBackgroundCalls);
    }

    /**
     * Return true if there is at least one active foreground call
     */
    public boolean hasActiveFgCall() {
        return (getFirstActiveCall(mForegroundCalls) != null);
    }

    /**
     * Return true if there is at least one active foreground call
     * on a particular subId or an active sip call
     */
    public boolean hasActiveFgCall(int subId) {
        return (getFirstActiveCall(mForegroundCalls, subId) != null);
    }

    /**
     * Return true if there is at least one active background call
     */
    public boolean hasActiveBgCall() {
        // TODO since hasActiveBgCall may get called often
        // better to cache it to improve performance
        return (getFirstActiveCall(mBackgroundCalls) != null);
    }

    /**
     * Return true if there is at least one active background call
     * on a particular subId or an active sip call
     */
    public boolean hasActiveBgCall(int subId) {
        // TODO since hasActiveBgCall may get called often
        // better to cache it to improve performance
        return (getFirstActiveCall(mBackgroundCalls, subId) != null);
    }

    /**
     * Return true if there is at least one active ringing call
     *
     */
    public boolean hasActiveRingingCall() {
        return (getFirstActiveCall(mRingingCalls) != null);
    }

    /**
     * Return true if there is at least one active ringing call
     */
    public boolean hasActiveRingingCall(int subId) {
        return (getFirstActiveCall(mRingingCalls, subId) != null);
    }

    /**
     * return the active foreground call from foreground calls
     *
     * Active call means the call is NOT in Call.State.IDLE
     *
     * 1. If there is active foreground call, return it
     * 2. If there is no active foreground call, return the
     *    foreground call associated with default phone, which state is IDLE.
     * 3. If there is no phone registered at all, return null.
     *
     */
    public Call getActiveFgCall() {
        Call call = getFirstNonIdleCall(mForegroundCalls);
        if (call == null) {
            call = (mDefaultPhone == null)
                    ? null
                    : mDefaultPhone.getForegroundCall();
        }
        return call;
    }

    public Call getActiveFgCall(int subId) {
        Call call = getFirstNonIdleCall(mForegroundCalls, subId);
        if (call == null) {
            Phone phone = getPhone(subId);
            call = (phone == null)
                    ? null
                    : phone.getForegroundCall();
        }
        return call;
    }

    // Returns the first call that is not in IDLE state. If both active calls
    // and disconnecting/disconnected calls exist, return the first active call.
    private Call getFirstNonIdleCall(List<Call> calls) {
        Call result = null;
        for (Call call : calls) {
            if (!call.isIdle()) {
                return call;
            } else if (call.getState() != Call.State.IDLE) {
                if (result == null) result = call;
            }
        }
        return result;
    }

    // Returns the first call that is not in IDLE state. If both active calls
    // and disconnecting/disconnected calls exist, return the first active call.
    private Call getFirstNonIdleCall(List<Call> calls, int subId) {
        Call result = null;
        for (Call call : calls) {
            if ((call.getPhone().getSubId() == subId) ||
                    (call.getPhone() instanceof SipPhone)) {
                if (!call.isIdle()) {
                    return call;
                } else if (call.getState() != Call.State.IDLE) {
                    if (result == null) result = call;
                }
            }
        }
        return result;
    }

    /**
     * return one active background call from background calls
     *
     * Active call means the call is NOT idle defined by Call.isIdle()
     *
     * 1. If there is only one active background call, return it
     * 2. If there is more than one active background call, return the first one
     * 3. If there is no active background call, return the background call
     *    associated with default phone, which state is IDLE.
     * 4. If there is no background call at all, return null.
     *
     * Complete background calls list can be get by getBackgroundCalls()
     */
    public Call getFirstActiveBgCall() {
        Call call = getFirstNonIdleCall(mBackgroundCalls);
        if (call == null) {
            call = (mDefaultPhone == null)
                    ? null
                    : mDefaultPhone.getBackgroundCall();
        }
        return call;
    }

    /**
     * return one active background call from background calls of the
     * requested subId.
     *
     * Active call means the call is NOT idle defined by Call.isIdle()
     *
     * 1. If there is only one active background call on given sub or
     *    on SIP Phone, return it
     * 2. If there is more than one active background call, return the background call
     *    associated with the active sub.
     * 3. If there is no background call at all, return null.
     *
     * Complete background calls list can be get by getBackgroundCalls()
     */
    public Call getFirstActiveBgCall(int subId) {
        Phone phone = getPhone(subId);
        if (hasMoreThanOneHoldingCall(subId)) {
            return phone.getBackgroundCall();
        } else {
            Call call = getFirstNonIdleCall(mBackgroundCalls, subId);
            if (call == null) {
                call = (phone == null)
                        ? null
                        : phone.getBackgroundCall();
            }
            return call;
        }
    }

    /**
     * return one active ringing call from ringing calls
     *
     * Active call means the call is NOT idle defined by Call.isIdle()
     *
     * 1. If there is only one active ringing call, return it
     * 2. If there is more than one active ringing call, return the first one
     * 3. If there is no active ringing call, return the ringing call
     *    associated with default phone, which state is IDLE.
     * 4. If there is no ringing call at all, return null.
     *
     * Complete ringing calls list can be get by getRingingCalls()
     */
    public Call getFirstActiveRingingCall() {
        Call call = getFirstNonIdleCall(mRingingCalls);
        if (call == null) {
            call = (mDefaultPhone == null)
                    ? null
                    : mDefaultPhone.getRingingCall();
        }
        return call;
    }

    public Call getFirstActiveRingingCall(int subId) {
        Phone phone = getPhone(subId);
        Call call = getFirstNonIdleCall(mRingingCalls, subId);
        if (call == null) {
            call = (phone == null)
                    ? null
                    : phone.getRingingCall();
        }
        return call;
    }

    /**
     * @return the state of active foreground call
     * return IDLE if there is no active foreground call
     */
    public Call.State getActiveFgCallState() {
        Call fgCall = getActiveFgCall();

        if (fgCall != null) {
            return fgCall.getState();
        }

        return Call.State.IDLE;
    }

    public Call.State getActiveFgCallState(int subId) {
        Call fgCall = getActiveFgCall(subId);

        if (fgCall != null) {
            return fgCall.getState();
        }

        return Call.State.IDLE;
    }

    /**
     * @return the connections of active foreground call
     * return empty list if there is no active foreground call
     */
    public List<Connection> getFgCallConnections() {
        Call fgCall = getActiveFgCall();
        if ( fgCall != null) {
            return fgCall.getConnections();
        }
        return mEmptyConnections;
    }

    /**
     * @return the connections of active foreground call
     * return empty list if there is no active foreground call
     */
    public List<Connection> getFgCallConnections(int subId) {
        Call fgCall = getActiveFgCall(subId);
        if ( fgCall != null) {
            return fgCall.getConnections();
        }
        return mEmptyConnections;
    }

    /**
     * @return the connections of active background call
     * return empty list if there is no active background call
     */
    public List<Connection> getBgCallConnections() {
        Call bgCall = getFirstActiveBgCall();
        if ( bgCall != null) {
            return bgCall.getConnections();
        }
        return mEmptyConnections;
    }

    /**
     * @return the connections of active background call
     * return empty list if there is no active background call
     */
    public List<Connection> getBgCallConnections(int subId) {
        Call bgCall = getFirstActiveBgCall(subId);
        if ( bgCall != null) {
            return bgCall.getConnections();
        }
        return mEmptyConnections;
    }

    /**
     * @return the latest connection of active foreground call
     * return null if there is no active foreground call
     */
    public Connection getFgCallLatestConnection() {
        Call fgCall = getActiveFgCall();
        if ( fgCall != null) {
            return fgCall.getLatestConnection();
        }
        return null;
    }

    /**
     * @return the latest connection of active foreground call
     * return null if there is no active foreground call
     */
    public Connection getFgCallLatestConnection(int subId) {
        Call fgCall = getActiveFgCall(subId);
        if ( fgCall != null) {
            return fgCall.getLatestConnection();
        }
        return null;
    }

    /**
     * @return true if there is at least one Foreground call in disconnected state
     */
    public boolean hasDisconnectedFgCall() {
        return (getFirstCallOfState(mForegroundCalls, Call.State.DISCONNECTED) != null);
    }

    /**
     * @return true if there is at least one Foreground call in disconnected state
     */
    public boolean hasDisconnectedFgCall(int subId) {
        return (getFirstCallOfState(mForegroundCalls, Call.State.DISCONNECTED,
                subId) != null);
    }

    /**
     * @return true if there is at least one background call in disconnected state
     */
    public boolean hasDisconnectedBgCall() {
        return (getFirstCallOfState(mBackgroundCalls, Call.State.DISCONNECTED) != null);
    }

    /**
     * @return true if there is at least one background call in disconnected state
     */
    public boolean hasDisconnectedBgCall(int subId) {
        return (getFirstCallOfState(mBackgroundCalls, Call.State.DISCONNECTED,
                subId) != null);
    }


    /**
     * @return the first active call from a call list
     */
    private  Call getFirstActiveCall(ArrayList<Call> calls) {
        for (Call call : calls) {
            if (!call.isIdle()) {
                return call;
            }
        }
        return null;
    }

    /**
     * @return the first active call from a call list
     */
    private  Call getFirstActiveCall(ArrayList<Call> calls, int subId) {
        for (Call call : calls) {
            if ((!call.isIdle()) && ((call.getPhone().getSubId() == subId) ||
                    (call.getPhone() instanceof SipPhone))) {
                return call;
            }
        }
        return null;
    }

    /**
     * @return the first call in a the Call.state from a call list
     */
    private Call getFirstCallOfState(ArrayList<Call> calls, Call.State state) {
        for (Call call : calls) {
            if (call.getState() == state) {
                return call;
            }
        }
        return null;
    }

    /**
     * @return the first call in a the Call.state from a call list
     */
    private Call getFirstCallOfState(ArrayList<Call> calls, Call.State state,
            int subId) {
        for (Call call : calls) {
            if ((call.getState() == state) ||
                ((call.getPhone().getSubId() == subId) ||
                (call.getPhone() instanceof SipPhone))) {
                return call;
            }
        }
        return null;
    }

    private boolean hasMoreThanOneRingingCall() {
        int count = 0;
        for (Call call : mRingingCalls) {
            if (call.getState().isRinging()) {
                if (++count > 1) return true;
            }
        }
        return false;
    }

    /**
     * @return true if more than one active ringing call exists on
     * the active subId.
     * This checks for the active calls on provided
     * subId and also active calls on SIP Phone.
     *
     */
    private boolean hasMoreThanOneRingingCall(int subId) {
        int count = 0;
        for (Call call : mRingingCalls) {
            if ((call.getState().isRinging()) &&
                ((call.getPhone().getSubId() == subId) ||
                (call.getPhone() instanceof SipPhone))) {
                if (++count > 1) return true;
            }
        }
        return false;
    }

    /**
     * @return true if more than one active background call exists on
     * the provided subId.
     * This checks for the background calls on provided
     * subId and also background calls on SIP Phone.
     *
     */
    private boolean hasMoreThanOneHoldingCall(int subId) {
        int count = 0;
        for (Call call : mBackgroundCalls) {
            if ((call.getState() == Call.State.HOLDING) &&
                ((call.getPhone().getSubId() == subId) ||
                (call.getPhone() instanceof SipPhone))) {
                if (++count > 1) return true;
            }
        }
        return false;
    }

    /* FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
    private boolean isServiceStateInService() {
        boolean bInService = false;

        for (Phone phone : mPhones) {
            bInService = (phone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE);
            if (bInService) {
                break;
            }
        }

        if (VDBG) Rlog.d(LOG_TAG, "[isServiceStateInService] bInService = " + bInService);
        return bInService;
    }
    */

    private class CallManagerHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {

            switch (msg.what) {
                case EVENT_DISCONNECT:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_DISCONNECT)");
                    mDisconnectRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    // FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
                    //mIsEccDialing = false;
                    break;
                case EVENT_PRECISE_CALL_STATE_CHANGED:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_PRECISE_CALL_STATE_CHANGED)");
                    mPreciseCallStateRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_NEW_RINGING_CONNECTION:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_NEW_RINGING_CONNECTION)");
                    Connection c = (Connection) ((AsyncResult) msg.obj).result;
                    int subId = c.getCall().getPhone().getSubId();
                    if (getActiveFgCallState(subId).isDialing() || hasMoreThanOneRingingCall()) {
                        try {
                            Rlog.d(LOG_TAG, "silently drop incoming call: " + c.getCall());
                            c.getCall().hangup();
                        } catch (CallStateException e) {
                            Rlog.w(LOG_TAG, "new ringing connection", e);
                        }
                    } else {
                        mNewRingingConnectionRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    }
                    break;
                case EVENT_UNKNOWN_CONNECTION:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_UNKNOWN_CONNECTION)");
                    mUnknownConnectionRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_INCOMING_RING:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_INCOMING_RING)");
                    // The event may come from RIL who's not aware of an ongoing fg call
                    if (!hasActiveFgCall()) {
                        mIncomingRingRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    }
                    break;
                case EVENT_RINGBACK_TONE:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_RINGBACK_TONE)");
                    mRingbackToneRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_IN_CALL_VOICE_PRIVACY_ON:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_IN_CALL_VOICE_PRIVACY_ON)");
                    mInCallVoicePrivacyOnRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_IN_CALL_VOICE_PRIVACY_OFF:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_IN_CALL_VOICE_PRIVACY_OFF)");
                    mInCallVoicePrivacyOffRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_CALL_WAITING:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_CALL_WAITING)");
                    mCallWaitingRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_DISPLAY_INFO:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_DISPLAY_INFO)");
                    mDisplayInfoRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_SIGNAL_INFO:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_SIGNAL_INFO)");
                    mSignalInfoRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_CDMA_OTA_STATUS_CHANGE:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_CDMA_OTA_STATUS_CHANGE)");
                    mCdmaOtaStatusChangeRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_RESEND_INCALL_MUTE:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_RESEND_INCALL_MUTE)");
                    mResendIncallMuteRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_MMI_INITIATE:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_MMI_INITIATE)");
                    mMmiInitiateRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_MMI_COMPLETE:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_MMI_COMPLETE)");
                    mMmiCompleteRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_ECM_TIMER_RESET:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_ECM_TIMER_RESET)");
                    mEcmTimerResetRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_SUBSCRIPTION_INFO_READY:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_SUBSCRIPTION_INFO_READY)");
                    mSubscriptionInfoReadyRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_SUPP_SERVICE_FAILED:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_SUPP_SERVICE_FAILED)");
                    mSuppServiceFailedRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_SERVICE_STATE_CHANGED:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_SERVICE_STATE_CHANGED)");
                    mServiceStateChangedRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    // FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
                    //setAudioMode();
                    break;
                case EVENT_POST_DIAL_CHARACTER:
                    // we need send the character that is being processed in msg.arg1
                    // so can't use notifyRegistrants()
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_POST_DIAL_CHARACTER)");
                    for(int i=0; i < mPostDialCharacterRegistrants.size(); i++) {
                        Message notifyMsg;
                        notifyMsg = ((Registrant)mPostDialCharacterRegistrants.get(i)).messageForRegistrant();
                        notifyMsg.obj = msg.obj;
                        notifyMsg.arg1 = msg.arg1;
                        notifyMsg.sendToTarget();
                    }
                    break;
                case EVENT_ONHOLD_TONE:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_ONHOLD_TONE)");
                    mOnHoldToneRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                case EVENT_TTY_MODE_RECEIVED:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_TTY_MODE_RECEIVED)");
                    mTtyModeReceivedRegistrants.notifyRegistrants((AsyncResult) msg.obj);
                    break;
                /* FIXME Taken from klp-sprout-dev but setAudioMode was removed in L.
                case EVENT_RADIO_OFF_OR_NOT_AVAILABLE:
                    if (VDBG) Rlog.d(LOG_TAG, " handleMessage (EVENT_RADIO_OFF_OR_NOT_AVAILABLE)");
                    setAudioMode();
                    break;
                */
            }
        }
    };

    @Override
    public String toString() {
        Call call;
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < TelephonyManager.getDefault().getPhoneCount(); i++) {
            b.append("CallManager {");
            b.append("\nstate = " + getState(i));
            call = getActiveFgCall(i);
            if (call != null) {
                b.append("\n- Foreground: " + getActiveFgCallState(i));
                b.append(" from " + call.getPhone());
                b.append("\n  Conn: ").append(getFgCallConnections(i));
            }
            call = getFirstActiveBgCall(i);
            if (call != null) {
                b.append("\n- Background: " + call.getState());
                b.append(" from " + call.getPhone());
                b.append("\n  Conn: ").append(getBgCallConnections(i));
            }
            call = getFirstActiveRingingCall(i);
            if (call != null) {
                b.append("\n- Ringing: " +call.getState());
                b.append(" from " + call.getPhone());
            }
        }

        for (Phone phone : getAllPhones()) {
            if (phone != null) {
                b.append("\nPhone: " + phone + ", name = " + phone.getPhoneName()
                        + ", state = " + phone.getState());
                call = phone.getForegroundCall();
                if (call != null) {
                    b.append("\n- Foreground: ").append(call);
                }
                call = phone.getBackgroundCall();
                if (call != null) {
                    b.append(" Background: ").append(call);
                }
                call = phone.getRingingCall();
                if (call != null) {
                    b.append(" Ringing: ").append(call);
                }
            }
        }
        b.append("\n}");
        return b.toString();
    }
}
