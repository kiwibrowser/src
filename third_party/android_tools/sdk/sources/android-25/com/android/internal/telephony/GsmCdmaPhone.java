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

import android.app.ActivityManagerNative;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.database.SQLException;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.PowerManager;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.provider.Telephony;
import android.telecom.VideoProfile;
import android.telephony.CarrierConfigManager;
import android.telephony.CellLocation;
import android.telephony.PhoneNumberUtils;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import android.telephony.cdma.CdmaCellLocation;
import android.text.TextUtils;
import android.telephony.Rlog;
import android.util.Log;

import com.android.ims.ImsManager;
import static com.android.internal.telephony.CommandsInterface.CF_ACTION_DISABLE;
import static com.android.internal.telephony.CommandsInterface.CF_ACTION_ENABLE;
import static com.android.internal.telephony.CommandsInterface.CF_ACTION_ERASURE;
import static com.android.internal.telephony.CommandsInterface.CF_ACTION_REGISTRATION;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_ALL;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_ALL_CONDITIONAL;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_NO_REPLY;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_NOT_REACHABLE;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_BUSY;
import static com.android.internal.telephony.CommandsInterface.CF_REASON_UNCONDITIONAL;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_VOICE;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.cdma.CdmaMmiCode;
import com.android.internal.telephony.cdma.CdmaSubscriptionSourceManager;
import com.android.internal.telephony.cdma.EriManager;
import com.android.internal.telephony.dataconnection.DcTracker;
import com.android.internal.telephony.gsm.GsmMmiCode;
import com.android.internal.telephony.gsm.SuppServiceNotification;
import com.android.internal.telephony.test.SimulatedRadioControl;
import com.android.internal.telephony.uicc.IccCardProxy;
import com.android.internal.telephony.uicc.IccException;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IccVmNotSupportedException;
import com.android.internal.telephony.uicc.RuimRecords;
import com.android.internal.telephony.uicc.SIMRecords;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IsimRecords;
import com.android.internal.telephony.uicc.IsimUiccRecords;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


/**
 * {@hide}
 */
public class GsmCdmaPhone extends Phone {
    // NOTE that LOG_TAG here is "GsmCdma", which means that log messages
    // from this file will go into the radio log rather than the main
    // log.  (Use "adb logcat -b radio" to see them.)
    public static final String LOG_TAG = "GsmCdmaPhone";
    private static final boolean DBG = true;
    private static final boolean VDBG = false; /* STOPSHIP if true */

    //GSM
    // Key used to read/write voice mail number
    private static final String VM_NUMBER = "vm_number_key";
    // Key used to read/write the SIM IMSI used for storing the voice mail
    private static final String VM_SIM_IMSI = "vm_sim_imsi_key";
    /** List of Registrants to receive Supplementary Service Notifications. */
    private RegistrantList mSsnRegistrants = new RegistrantList();

    //CDMA
    // Default Emergency Callback Mode exit timer
    private static final int DEFAULT_ECM_EXIT_TIMER_VALUE = 300000;
    private static final String VM_NUMBER_CDMA = "vm_number_key_cdma";
    public static final int RESTART_ECM_TIMER = 0; // restart Ecm timer
    public static final int CANCEL_ECM_TIMER = 1; // cancel Ecm timer
    private CdmaSubscriptionSourceManager mCdmaSSM;
    public int mCdmaSubscriptionSource = CdmaSubscriptionSourceManager.SUBSCRIPTION_SOURCE_UNKNOWN;
    public EriManager mEriManager;
    private PowerManager.WakeLock mWakeLock;
    // mEriFileLoadedRegistrants are informed after the ERI text has been loaded
    private final RegistrantList mEriFileLoadedRegistrants = new RegistrantList();
    // mEcmExitRespRegistrant is informed after the phone has been exited
    //the emergency callback mode
    //keep track of if phone is in emergency callback mode
    private boolean mIsPhoneInEcmState;
    private Registrant mEcmExitRespRegistrant;
    private String mEsn;
    private String mMeid;
    // string to define how the carrier specifies its own ota sp number
    private String mCarrierOtaSpNumSchema;
    // A runnable which is used to automatically exit from Ecm after a period of time.
    private Runnable mExitEcmRunnable = new Runnable() {
        @Override
        public void run() {
            exitEmergencyCallbackMode();
        }
    };
    public static final String PROPERTY_CDMA_HOME_OPERATOR_NUMERIC =
            "ro.cdma.home.operator.numeric";

    //CDMALTE
    /** PHONE_TYPE_CDMA_LTE in addition to RuimRecords needs access to SIMRecords and
     * IsimUiccRecords
     */
    private SIMRecords mSimRecords;

    //Common
    // Instance Variables
    private IsimUiccRecords mIsimUiccRecords;
    public GsmCdmaCallTracker mCT;
    public ServiceStateTracker mSST;
    private ArrayList <MmiCode> mPendingMMIs = new ArrayList<MmiCode>();
    private IccPhoneBookInterfaceManager mIccPhoneBookIntManager;

    private int mPrecisePhoneType;

    // mEcmTimerResetRegistrants are informed after Ecm timer is canceled or re-started
    private final RegistrantList mEcmTimerResetRegistrants = new RegistrantList();

    private String mImei;
    private String mImeiSv;
    private String mVmNumber;

    // Create Cfu (Call forward unconditional) so that dialing number &
    // mOnComplete (Message object passed by client) can be packed &
    // given as a single Cfu object as user data to RIL.
    private static class Cfu {
        final String mSetCfNumber;
        final Message mOnComplete;

        Cfu(String cfNumber, Message onComplete) {
            mSetCfNumber = cfNumber;
            mOnComplete = onComplete;
        }
    }

    private IccSmsInterfaceManager mIccSmsInterfaceManager;
    private IccCardProxy mIccCardProxy;

    private boolean mResetModemOnRadioTechnologyChange = false;

    private int mRilVersion;
    private boolean mBroadcastEmergencyCallStateChanges = false;

    // Constructors

    public GsmCdmaPhone(Context context, CommandsInterface ci, PhoneNotifier notifier, int phoneId,
                        int precisePhoneType, TelephonyComponentFactory telephonyComponentFactory) {
        this(context, ci, notifier, false, phoneId, precisePhoneType, telephonyComponentFactory);
    }

    public GsmCdmaPhone(Context context, CommandsInterface ci, PhoneNotifier notifier,
                        boolean unitTestMode, int phoneId, int precisePhoneType,
                        TelephonyComponentFactory telephonyComponentFactory) {
        super(precisePhoneType == PhoneConstants.PHONE_TYPE_GSM ? "GSM" : "CDMA",
                notifier, context, ci, unitTestMode, phoneId, telephonyComponentFactory);

        // phone type needs to be set before other initialization as other objects rely on it
        mPrecisePhoneType = precisePhoneType;
        initOnce(ci);
        initRatSpecific(precisePhoneType);
        mSST = mTelephonyComponentFactory.makeServiceStateTracker(this, this.mCi);
        // DcTracker uses SST so needs to be created after it is instantiated
        mDcTracker = mTelephonyComponentFactory.makeDcTracker(this);
        mSST.registerForNetworkAttached(this, EVENT_REGISTERED_TO_NETWORK, null);
        logd("GsmCdmaPhone: constructor: sub = " + mPhoneId);
    }

    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Rlog.d(LOG_TAG, "mBroadcastReceiver: action " + intent.getAction());
            if (intent.getAction().equals(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED)) {
                sendMessage(obtainMessage(EVENT_CARRIER_CONFIG_CHANGED));
            }
        }
    };

    private void initOnce(CommandsInterface ci) {
        if (ci instanceof SimulatedRadioControl) {
            mSimulatedRadioControl = (SimulatedRadioControl) ci;
        }

        mCT = mTelephonyComponentFactory.makeGsmCdmaCallTracker(this);
        mIccPhoneBookIntManager = mTelephonyComponentFactory.makeIccPhoneBookInterfaceManager(this);
        PowerManager pm
                = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, LOG_TAG);
        mIccSmsInterfaceManager = mTelephonyComponentFactory.makeIccSmsInterfaceManager(this);
        mIccCardProxy = mTelephonyComponentFactory.makeIccCardProxy(mContext, mCi, mPhoneId);

        mCi.registerForAvailable(this, EVENT_RADIO_AVAILABLE, null);
        mCi.registerForOffOrNotAvailable(this, EVENT_RADIO_OFF_OR_NOT_AVAILABLE, null);
        mCi.registerForOn(this, EVENT_RADIO_ON, null);
        mCi.setOnSuppServiceNotification(this, EVENT_SSN, null);

        //GSM
        mCi.setOnUSSD(this, EVENT_USSD, null);
        mCi.setOnSs(this, EVENT_SS, null);

        //CDMA
        mCdmaSSM = mTelephonyComponentFactory.getCdmaSubscriptionSourceManagerInstance(mContext,
                mCi, this, EVENT_CDMA_SUBSCRIPTION_SOURCE_CHANGED, null);
        mEriManager = mTelephonyComponentFactory.makeEriManager(this, mContext,
                EriManager.ERI_FROM_XML);
        mCi.setEmergencyCallbackMode(this, EVENT_EMERGENCY_CALLBACK_MODE_ENTER, null);
        mCi.registerForExitEmergencyCallbackMode(this, EVENT_EXIT_EMERGENCY_CALLBACK_RESPONSE,
                null);
        // get the string that specifies the carrier OTA Sp number
        mCarrierOtaSpNumSchema = TelephonyManager.from(mContext).getOtaSpNumberSchemaForPhone(
                getPhoneId(), "");

        mResetModemOnRadioTechnologyChange = SystemProperties.getBoolean(
                TelephonyProperties.PROPERTY_RESET_ON_RADIO_TECH_CHANGE, false);

        mCi.registerForRilConnected(this, EVENT_RIL_CONNECTED, null);
        mCi.registerForVoiceRadioTechChanged(this, EVENT_VOICE_RADIO_TECH_CHANGED, null);
        mContext.registerReceiver(mBroadcastReceiver, new IntentFilter(
                CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED));
    }

    private void initRatSpecific(int precisePhoneType) {
        mPendingMMIs.clear();
        mIccPhoneBookIntManager.updateIccRecords(null);
        mEsn = null;
        mMeid = null;

        mPrecisePhoneType = precisePhoneType;

        TelephonyManager tm = TelephonyManager.from(mContext);
        if (isPhoneTypeGsm()) {
            mCi.setPhoneType(PhoneConstants.PHONE_TYPE_GSM);
            tm.setPhoneType(getPhoneId(), PhoneConstants.PHONE_TYPE_GSM);
            mIccCardProxy.setVoiceRadioTech(ServiceState.RIL_RADIO_TECHNOLOGY_UMTS);
        } else {
            mCdmaSubscriptionSource = CdmaSubscriptionSourceManager.SUBSCRIPTION_SOURCE_UNKNOWN;
            // This is needed to handle phone process crashes
            String inEcm = SystemProperties.get(TelephonyProperties.PROPERTY_INECM_MODE, "false");
            mIsPhoneInEcmState = inEcm.equals("true");
            if (mIsPhoneInEcmState) {
                // Send a message which will invoke handleExitEmergencyCallbackMode
                mCi.exitEmergencyCallbackMode(
                        obtainMessage(EVENT_EXIT_EMERGENCY_CALLBACK_RESPONSE));
            }

            mCi.setPhoneType(PhoneConstants.PHONE_TYPE_CDMA);
            tm.setPhoneType(getPhoneId(), PhoneConstants.PHONE_TYPE_CDMA);
            mIccCardProxy.setVoiceRadioTech(ServiceState.RIL_RADIO_TECHNOLOGY_1xRTT);
            // Sets operator properties by retrieving from build-time system property
            String operatorAlpha = SystemProperties.get("ro.cdma.home.operator.alpha");
            String operatorNumeric = SystemProperties.get(PROPERTY_CDMA_HOME_OPERATOR_NUMERIC);
            logd("init: operatorAlpha='" + operatorAlpha
                    + "' operatorNumeric='" + operatorNumeric + "'");
            if (mUiccController.getUiccCardApplication(mPhoneId, UiccController.APP_FAM_3GPP) ==
                    null || isPhoneTypeCdmaLte()) {
                if (!TextUtils.isEmpty(operatorAlpha)) {
                    logd("init: set 'gsm.sim.operator.alpha' to operator='" + operatorAlpha + "'");
                    tm.setSimOperatorNameForPhone(mPhoneId, operatorAlpha);
                }
                if (!TextUtils.isEmpty(operatorNumeric)) {
                    logd("init: set 'gsm.sim.operator.numeric' to operator='" + operatorNumeric +
                            "'");
                    logd("update icc_operator_numeric=" + operatorNumeric);
                    tm.setSimOperatorNumericForPhone(mPhoneId, operatorNumeric);

                    SubscriptionController.getInstance().setMccMnc(operatorNumeric, getSubId());
                    // Sets iso country property by retrieving from build-time system property
                    setIsoCountryProperty(operatorNumeric);
                    // Updates MCC MNC device configuration information
                    logd("update mccmnc=" + operatorNumeric);
                    MccTable.updateMccMncConfiguration(mContext, operatorNumeric, false);
                }
            }

            // Sets current entry in the telephony carrier table
            updateCurrentCarrierInProvider(operatorNumeric);
        }
    }

    //CDMA
    /**
     * Sets PROPERTY_ICC_OPERATOR_ISO_COUNTRY property
     *
     */
    private void setIsoCountryProperty(String operatorNumeric) {
        TelephonyManager tm = TelephonyManager.from(mContext);
        if (TextUtils.isEmpty(operatorNumeric)) {
            logd("setIsoCountryProperty: clear 'gsm.sim.operator.iso-country'");
            tm.setSimCountryIsoForPhone(mPhoneId, "");
        } else {
            String iso = "";
            try {
                iso = MccTable.countryCodeForMcc(Integer.parseInt(
                        operatorNumeric.substring(0,3)));
            } catch (NumberFormatException ex) {
                Rlog.e(LOG_TAG, "setIsoCountryProperty: countryCodeForMcc error", ex);
            } catch (StringIndexOutOfBoundsException ex) {
                Rlog.e(LOG_TAG, "setIsoCountryProperty: countryCodeForMcc error", ex);
            }

            logd("setIsoCountryProperty: set 'gsm.sim.operator.iso-country' to iso=" + iso);
            tm.setSimCountryIsoForPhone(mPhoneId, iso);
        }
    }

    public boolean isPhoneTypeGsm() {
        return mPrecisePhoneType == PhoneConstants.PHONE_TYPE_GSM;
    }

    public boolean isPhoneTypeCdma() {
        return mPrecisePhoneType == PhoneConstants.PHONE_TYPE_CDMA;
    }

    public boolean isPhoneTypeCdmaLte() {
        return mPrecisePhoneType == PhoneConstants.PHONE_TYPE_CDMA_LTE;
    }

    private void switchPhoneType(int precisePhoneType) {
        removeCallbacks(mExitEcmRunnable);

        initRatSpecific(precisePhoneType);

        mSST.updatePhoneType();
        setPhoneName(precisePhoneType == PhoneConstants.PHONE_TYPE_GSM ? "GSM" : "CDMA");
        onUpdateIccAvailability();
        mCT.updatePhoneType();

        CommandsInterface.RadioState radioState = mCi.getRadioState();
        if (radioState.isAvailable()) {
            handleRadioAvailable();
            if (radioState.isOn()) {
                handleRadioOn();
            }
        }
        if (!radioState.isAvailable() || !radioState.isOn()) {
            handleRadioOffOrNotAvailable();
        }
    }

    @Override
    protected void finalize() {
        if(DBG) logd("GsmCdmaPhone finalized");
        if (mWakeLock.isHeld()) {
            Rlog.e(LOG_TAG, "UNEXPECTED; mWakeLock is held when finalizing.");
            mWakeLock.release();
        }
    }

    @Override
    public ServiceState getServiceState() {
        if (mSST == null || mSST.mSS.getState() != ServiceState.STATE_IN_SERVICE) {
            if (mImsPhone != null) {
                return ServiceState.mergeServiceStates(
                        (mSST == null) ? new ServiceState() : mSST.mSS,
                        mImsPhone.getServiceState());
            }
        }

        if (mSST != null) {
            return mSST.mSS;
        } else {
            // avoid potential NPE in EmergencyCallHelper during Phone switch
            return new ServiceState();
        }
    }

    @Override
    public CellLocation getCellLocation() {
        if (isPhoneTypeGsm()) {
            return mSST.getCellLocation();
        } else {
            CdmaCellLocation loc = (CdmaCellLocation)mSST.mCellLoc;

            int mode = Settings.Secure.getInt(getContext().getContentResolver(),
                    Settings.Secure.LOCATION_MODE, Settings.Secure.LOCATION_MODE_OFF);
            if (mode == Settings.Secure.LOCATION_MODE_OFF) {
                // clear lat/long values for location privacy
                CdmaCellLocation privateLoc = new CdmaCellLocation();
                privateLoc.setCellLocationData(loc.getBaseStationId(),
                        CdmaCellLocation.INVALID_LAT_LONG,
                        CdmaCellLocation.INVALID_LAT_LONG,
                        loc.getSystemId(), loc.getNetworkId());
                loc = privateLoc;
            }
            return loc;
        }
    }

    @Override
    public PhoneConstants.State getState() {
        if (mImsPhone != null) {
            PhoneConstants.State imsState = mImsPhone.getState();
            if (imsState != PhoneConstants.State.IDLE) {
                return imsState;
            }
        }

        return mCT.mState;
    }

    @Override
    public int getPhoneType() {
        if (mPrecisePhoneType == PhoneConstants.PHONE_TYPE_GSM) {
            return PhoneConstants.PHONE_TYPE_GSM;
        } else {
            return PhoneConstants.PHONE_TYPE_CDMA;
        }
    }

    @Override
    public ServiceStateTracker getServiceStateTracker() {
        return mSST;
    }

    @Override
    public CallTracker getCallTracker() {
        return mCT;
    }

    @Override
    public void updateVoiceMail() {
        if (isPhoneTypeGsm()) {
            int countVoiceMessages = 0;
            IccRecords r = mIccRecords.get();
            if (r != null) {
                // get voice mail count from SIM
                countVoiceMessages = r.getVoiceMessageCount();
            }
            int countVoiceMessagesStored = getStoredVoiceMessageCount();
            if (countVoiceMessages == -1 && countVoiceMessagesStored != 0) {
                countVoiceMessages = countVoiceMessagesStored;
            }
            logd("updateVoiceMail countVoiceMessages = " + countVoiceMessages
                    + " subId " + getSubId());
            setVoiceMessageCount(countVoiceMessages);
        } else {
            setVoiceMessageCount(getStoredVoiceMessageCount());
        }
    }

    @Override
    public List<? extends MmiCode>
    getPendingMmiCodes() {
        return mPendingMMIs;
    }

    @Override
    public PhoneConstants.DataState getDataConnectionState(String apnType) {
        PhoneConstants.DataState ret = PhoneConstants.DataState.DISCONNECTED;

        if (mSST == null) {
            // Radio Technology Change is ongoning, dispose() and removeReferences() have
            // already been called

            ret = PhoneConstants.DataState.DISCONNECTED;
        } else if (mSST.getCurrentDataConnectionState() != ServiceState.STATE_IN_SERVICE
                && (isPhoneTypeCdma() ||
                (isPhoneTypeGsm() && !apnType.equals(PhoneConstants.APN_TYPE_EMERGENCY)))) {
            // If we're out of service, open TCP sockets may still work
            // but no data will flow

            // Emergency APN is available even in Out Of Service
            // Pass the actual State of EPDN

            ret = PhoneConstants.DataState.DISCONNECTED;
        } else { /* mSST.gprsState == ServiceState.STATE_IN_SERVICE */
            switch (mDcTracker.getState(apnType)) {
                case RETRYING:
                case FAILED:
                case IDLE:
                    ret = PhoneConstants.DataState.DISCONNECTED;
                break;

                case CONNECTED:
                case DISCONNECTING:
                    if ( mCT.mState != PhoneConstants.State.IDLE
                            && !mSST.isConcurrentVoiceAndDataAllowed()) {
                        ret = PhoneConstants.DataState.SUSPENDED;
                    } else {
                        ret = PhoneConstants.DataState.CONNECTED;
                    }
                break;

                case CONNECTING:
                case SCANNING:
                    ret = PhoneConstants.DataState.CONNECTING;
                break;
            }
        }

        logd("getDataConnectionState apnType=" + apnType + " ret=" + ret);
        return ret;
    }

    @Override
    public DataActivityState getDataActivityState() {
        DataActivityState ret = DataActivityState.NONE;

        if (mSST.getCurrentDataConnectionState() == ServiceState.STATE_IN_SERVICE) {
            switch (mDcTracker.getActivity()) {
                case DATAIN:
                    ret = DataActivityState.DATAIN;
                break;

                case DATAOUT:
                    ret = DataActivityState.DATAOUT;
                break;

                case DATAINANDOUT:
                    ret = DataActivityState.DATAINANDOUT;
                break;

                case DORMANT:
                    ret = DataActivityState.DORMANT;
                break;

                default:
                    ret = DataActivityState.NONE;
                break;
            }
        }

        return ret;
    }

    /**
     * Notify any interested party of a Phone state change
     * {@link com.android.internal.telephony.PhoneConstants.State}
     */
    public void notifyPhoneStateChanged() {
        mNotifier.notifyPhoneState(this);
    }

    /**
     * Notify registrants of a change in the call state. This notifies changes in
     * {@link com.android.internal.telephony.Call.State}. Use this when changes
     * in the precise call state are needed, else use notifyPhoneStateChanged.
     */
    public void notifyPreciseCallStateChanged() {
        /* we'd love it if this was package-scoped*/
        super.notifyPreciseCallStateChangedP();
    }

    public void notifyNewRingingConnection(Connection c) {
        super.notifyNewRingingConnectionP(c);
    }

    public void notifyDisconnect(Connection cn) {
        mDisconnectRegistrants.notifyResult(cn);

        mNotifier.notifyDisconnectCause(cn.getDisconnectCause(), cn.getPreciseDisconnectCause());
    }

    public void notifyUnknownConnection(Connection cn) {
        super.notifyUnknownConnectionP(cn);
    }

    @Override
    public boolean isInEmergencyCall() {
        if (isPhoneTypeGsm()) {
            return false;
        } else {
            return mCT.isInEmergencyCall();
        }
    }

    @Override
    protected void setIsInEmergencyCall() {
        if (!isPhoneTypeGsm()) {
            mCT.setIsInEmergencyCall();
        }
    }

    @Override
    public boolean isInEcm() {
        if (isPhoneTypeGsm()) {
            return false;
        } else {
            return mIsPhoneInEcmState;
        }
    }

    //CDMA
    private void sendEmergencyCallbackModeChange(){
        //Send an Intent
        Intent intent = new Intent(TelephonyIntents.ACTION_EMERGENCY_CALLBACK_MODE_CHANGED);
        intent.putExtra(PhoneConstants.PHONE_IN_ECM_STATE, mIsPhoneInEcmState);
        SubscriptionManager.putPhoneIdAndSubIdExtra(intent, getPhoneId());
        ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);
        if (DBG) logd("sendEmergencyCallbackModeChange");
    }

    @Override
    public void sendEmergencyCallStateChange(boolean callActive) {
        if (mBroadcastEmergencyCallStateChanges) {
            Intent intent = new Intent(TelephonyIntents.ACTION_EMERGENCY_CALL_STATE_CHANGED);
            intent.putExtra(PhoneConstants.PHONE_IN_EMERGENCY_CALL, callActive);
            SubscriptionManager.putPhoneIdAndSubIdExtra(intent, getPhoneId());
            ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);
            if (DBG) Rlog.d(LOG_TAG, "sendEmergencyCallStateChange: callActive " + callActive);
        }
    }

    @Override
    public void setBroadcastEmergencyCallStateChanges(boolean broadcast) {
        mBroadcastEmergencyCallStateChanges = broadcast;
    }

    public void notifySuppServiceFailed(SuppService code) {
        mSuppServiceFailedRegistrants.notifyResult(code);
    }

    public void notifyServiceStateChanged(ServiceState ss) {
        super.notifyServiceStateChangedP(ss);
    }

    public void notifyLocationChanged() {
        mNotifier.notifyCellLocation(this);
    }

    @Override
    public void notifyCallForwardingIndicator() {
        mNotifier.notifyCallForwardingChanged(this);
    }

    // override for allowing access from other classes of this package
    /**
     * {@inheritDoc}
     */
    @Override
    public void setSystemProperty(String property, String value) {
        if (getUnitTestMode()) {
            return;
        }
        if (isPhoneTypeGsm() || isPhoneTypeCdmaLte()) {
            TelephonyManager.setTelephonyProperty(mPhoneId, property, value);
        } else {
            super.setSystemProperty(property, value);
        }
    }

    @Override
    public void registerForSuppServiceNotification(
            Handler h, int what, Object obj) {
        mSsnRegistrants.addUnique(h, what, obj);
        if (mSsnRegistrants.size() == 1) mCi.setSuppServiceNotifications(true, null);
    }

    @Override
    public void unregisterForSuppServiceNotification(Handler h) {
        mSsnRegistrants.remove(h);
        if (mSsnRegistrants.size() == 0) mCi.setSuppServiceNotifications(false, null);
    }

    @Override
    public void registerForSimRecordsLoaded(Handler h, int what, Object obj) {
        mSimRecordsLoadedRegistrants.addUnique(h, what, obj);
    }

    @Override
    public void unregisterForSimRecordsLoaded(Handler h) {
        mSimRecordsLoadedRegistrants.remove(h);
    }

    @Override
    public void acceptCall(int videoState) throws CallStateException {
        Phone imsPhone = mImsPhone;
        if ( imsPhone != null && imsPhone.getRingingCall().isRinging() ) {
            imsPhone.acceptCall(videoState);
        } else {
            mCT.acceptCall();
        }
    }

    @Override
    public void rejectCall() throws CallStateException {
        mCT.rejectCall();
    }

    @Override
    public void switchHoldingAndActive() throws CallStateException {
        mCT.switchWaitingOrHoldingAndActive();
    }

    @Override
    public String getIccSerialNumber() {
        IccRecords r = mIccRecords.get();
        if (!isPhoneTypeGsm() && r == null) {
            // to get ICCID form SIMRecords because it is on MF.
            r = mUiccController.getIccRecords(mPhoneId, UiccController.APP_FAM_3GPP);
        }
        return (r != null) ? r.getIccId() : null;
    }

    @Override
    public String getFullIccSerialNumber() {
        IccRecords r = mIccRecords.get();
        if (!isPhoneTypeGsm() && r == null) {
            // to get ICCID form SIMRecords because it is on MF.
            r = mUiccController.getIccRecords(mPhoneId, UiccController.APP_FAM_3GPP);
        }
        return (r != null) ? r.getFullIccId() : null;
    }

    @Override
    public boolean canConference() {
        if (mImsPhone != null && mImsPhone.canConference()) {
            return true;
        }
        if (isPhoneTypeGsm()) {
            return mCT.canConference();
        } else {
            loge("canConference: not possible in CDMA");
            return false;
        }
    }

    @Override
    public void conference() {
        if (mImsPhone != null && mImsPhone.canConference()) {
            logd("conference() - delegated to IMS phone");
            try {
                mImsPhone.conference();
            } catch (CallStateException e) {
                loge(e.toString());
            }
            return;
        }
        if (isPhoneTypeGsm()) {
            mCT.conference();
        } else {
            // three way calls in CDMA will be handled by feature codes
            loge("conference: not possible in CDMA");
        }
    }

    @Override
    public void enableEnhancedVoicePrivacy(boolean enable, Message onComplete) {
        if (isPhoneTypeGsm()) {
            loge("enableEnhancedVoicePrivacy: not expected on GSM");
        } else {
            mCi.setPreferredVoicePrivacy(enable, onComplete);
        }
    }

    @Override
    public void getEnhancedVoicePrivacy(Message onComplete) {
        if (isPhoneTypeGsm()) {
            loge("getEnhancedVoicePrivacy: not expected on GSM");
        } else {
            mCi.getPreferredVoicePrivacy(onComplete);
        }
    }

    @Override
    public void clearDisconnected() {
        mCT.clearDisconnected();
    }

    @Override
    public boolean canTransfer() {
        if (isPhoneTypeGsm()) {
            return mCT.canTransfer();
        } else {
            loge("canTransfer: not possible in CDMA");
            return false;
        }
    }

    @Override
    public void explicitCallTransfer() {
        if (isPhoneTypeGsm()) {
            mCT.explicitCallTransfer();
        } else {
            loge("explicitCallTransfer: not possible in CDMA");
        }
    }

    @Override
    public GsmCdmaCall getForegroundCall() {
        return mCT.mForegroundCall;
    }

    @Override
    public GsmCdmaCall getBackgroundCall() {
        return mCT.mBackgroundCall;
    }

    @Override
    public Call getRingingCall() {
        Phone imsPhone = mImsPhone;
        // It returns the ringing call of ImsPhone if the ringing call of GSMPhone isn't ringing.
        // In CallManager.registerPhone(), it always registers ringing call of ImsPhone, because
        // the ringing call of GSMPhone isn't ringing. Consequently, it can't answer GSM call
        // successfully by invoking TelephonyManager.answerRingingCall() since the implementation
        // in PhoneInterfaceManager.answerRingingCallInternal() could not get the correct ringing
        // call from CallManager. So we check the ringing call state of imsPhone first as
        // accpetCall() does.
        if ( imsPhone != null && imsPhone.getRingingCall().isRinging()) {
            return imsPhone.getRingingCall();
        }
        return mCT.mRingingCall;
    }

    private boolean handleCallDeflectionIncallSupplementaryService(
            String dialString) {
        if (dialString.length() > 1) {
            return false;
        }

        if (getRingingCall().getState() != GsmCdmaCall.State.IDLE) {
            if (DBG) logd("MmiCode 0: rejectCall");
            try {
                mCT.rejectCall();
            } catch (CallStateException e) {
                if (DBG) Rlog.d(LOG_TAG,
                        "reject failed", e);
                notifySuppServiceFailed(Phone.SuppService.REJECT);
            }
        } else if (getBackgroundCall().getState() != GsmCdmaCall.State.IDLE) {
            if (DBG) logd("MmiCode 0: hangupWaitingOrBackground");
            mCT.hangupWaitingOrBackground();
        }

        return true;
    }

    //GSM
    private boolean handleCallWaitingIncallSupplementaryService(String dialString) {
        int len = dialString.length();

        if (len > 2) {
            return false;
        }

        GsmCdmaCall call = getForegroundCall();

        try {
            if (len > 1) {
                char ch = dialString.charAt(1);
                int callIndex = ch - '0';

                if (callIndex >= 1 && callIndex <= GsmCdmaCallTracker.MAX_CONNECTIONS_GSM) {
                    if (DBG) logd("MmiCode 1: hangupConnectionByIndex " + callIndex);
                    mCT.hangupConnectionByIndex(call, callIndex);
                }
            } else {
                if (call.getState() != GsmCdmaCall.State.IDLE) {
                    if (DBG) logd("MmiCode 1: hangup foreground");
                    //mCT.hangupForegroundResumeBackground();
                    mCT.hangup(call);
                } else {
                    if (DBG) logd("MmiCode 1: switchWaitingOrHoldingAndActive");
                    mCT.switchWaitingOrHoldingAndActive();
                }
            }
        } catch (CallStateException e) {
            if (DBG) Rlog.d(LOG_TAG,
                    "hangup failed", e);
            notifySuppServiceFailed(Phone.SuppService.HANGUP);
        }

        return true;
    }

    private boolean handleCallHoldIncallSupplementaryService(String dialString) {
        int len = dialString.length();

        if (len > 2) {
            return false;
        }

        GsmCdmaCall call = getForegroundCall();

        if (len > 1) {
            try {
                char ch = dialString.charAt(1);
                int callIndex = ch - '0';
                GsmCdmaConnection conn = mCT.getConnectionByIndex(call, callIndex);

                // GsmCdma index starts at 1, up to 5 connections in a call,
                if (conn != null && callIndex >= 1 && callIndex <= GsmCdmaCallTracker.MAX_CONNECTIONS_GSM) {
                    if (DBG) logd("MmiCode 2: separate call " + callIndex);
                    mCT.separate(conn);
                } else {
                    if (DBG) logd("separate: invalid call index " + callIndex);
                    notifySuppServiceFailed(Phone.SuppService.SEPARATE);
                }
            } catch (CallStateException e) {
                if (DBG) Rlog.d(LOG_TAG, "separate failed", e);
                notifySuppServiceFailed(Phone.SuppService.SEPARATE);
            }
        } else {
            try {
                if (getRingingCall().getState() != GsmCdmaCall.State.IDLE) {
                    if (DBG) logd("MmiCode 2: accept ringing call");
                    mCT.acceptCall();
                } else {
                    if (DBG) logd("MmiCode 2: switchWaitingOrHoldingAndActive");
                    mCT.switchWaitingOrHoldingAndActive();
                }
            } catch (CallStateException e) {
                if (DBG) Rlog.d(LOG_TAG, "switch failed", e);
                notifySuppServiceFailed(Phone.SuppService.SWITCH);
            }
        }

        return true;
    }

    private boolean handleMultipartyIncallSupplementaryService(String dialString) {
        if (dialString.length() > 1) {
            return false;
        }

        if (DBG) logd("MmiCode 3: merge calls");
        conference();
        return true;
    }

    private boolean handleEctIncallSupplementaryService(String dialString) {

        int len = dialString.length();

        if (len != 1) {
            return false;
        }

        if (DBG) logd("MmiCode 4: explicit call transfer");
        explicitCallTransfer();
        return true;
    }

    private boolean handleCcbsIncallSupplementaryService(String dialString) {
        if (dialString.length() > 1) {
            return false;
        }

        Rlog.i(LOG_TAG, "MmiCode 5: CCBS not supported!");
        // Treat it as an "unknown" service.
        notifySuppServiceFailed(Phone.SuppService.UNKNOWN);
        return true;
    }

    @Override
    public boolean handleInCallMmiCommands(String dialString) throws CallStateException {
        if (!isPhoneTypeGsm()) {
            loge("method handleInCallMmiCommands is NOT supported in CDMA!");
            return false;
        }

        Phone imsPhone = mImsPhone;
        if (imsPhone != null
                && imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE) {
            return imsPhone.handleInCallMmiCommands(dialString);
        }

        if (!isInCall()) {
            return false;
        }

        if (TextUtils.isEmpty(dialString)) {
            return false;
        }

        boolean result = false;
        char ch = dialString.charAt(0);
        switch (ch) {
            case '0':
                result = handleCallDeflectionIncallSupplementaryService(dialString);
                break;
            case '1':
                result = handleCallWaitingIncallSupplementaryService(dialString);
                break;
            case '2':
                result = handleCallHoldIncallSupplementaryService(dialString);
                break;
            case '3':
                result = handleMultipartyIncallSupplementaryService(dialString);
                break;
            case '4':
                result = handleEctIncallSupplementaryService(dialString);
                break;
            case '5':
                result = handleCcbsIncallSupplementaryService(dialString);
                break;
            default:
                break;
        }

        return result;
    }

    public boolean isInCall() {
        GsmCdmaCall.State foregroundCallState = getForegroundCall().getState();
        GsmCdmaCall.State backgroundCallState = getBackgroundCall().getState();
        GsmCdmaCall.State ringingCallState = getRingingCall().getState();

       return (foregroundCallState.isAlive() ||
                backgroundCallState.isAlive() ||
                ringingCallState.isAlive());
    }

    @Override
    public Connection dial(String dialString, int videoState) throws CallStateException {
        return dial(dialString, null, videoState, null);
    }

    @Override
    public Connection dial(String dialString, UUSInfo uusInfo, int videoState, Bundle intentExtras)
            throws CallStateException {
        if (!isPhoneTypeGsm() && uusInfo != null) {
            throw new CallStateException("Sending UUS information NOT supported in CDMA!");
        }

        boolean isEmergency = PhoneNumberUtils.isEmergencyNumber(dialString);
        Phone imsPhone = mImsPhone;

        CarrierConfigManager configManager =
                (CarrierConfigManager) mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        boolean alwaysTryImsForEmergencyCarrierConfig = configManager.getConfigForSubId(getSubId())
                .getBoolean(CarrierConfigManager.KEY_CARRIER_USE_IMS_FIRST_FOR_EMERGENCY_BOOL);

        boolean imsUseEnabled = isImsUseEnabled()
                 && imsPhone != null
                 && (imsPhone.isVolteEnabled() || imsPhone.isWifiCallingEnabled() ||
                 (imsPhone.isVideoEnabled() && VideoProfile.isVideo(videoState)))
                 && (imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE);

        boolean useImsForEmergency = imsPhone != null
                && isEmergency
                && alwaysTryImsForEmergencyCarrierConfig
                && ImsManager.isNonTtyOrTtyOnVolteEnabled(mContext)
                && (imsPhone.getServiceState().getState() != ServiceState.STATE_POWER_OFF);

        String dialPart = PhoneNumberUtils.extractNetworkPortionAlt(PhoneNumberUtils.
                stripSeparators(dialString));
        boolean isUt = (dialPart.startsWith("*") || dialPart.startsWith("#"))
                && dialPart.endsWith("#");

        boolean useImsForUt = imsPhone != null && imsPhone.isUtEnabled();

        if (DBG) {
            logd("imsUseEnabled=" + imsUseEnabled
                    + ", useImsForEmergency=" + useImsForEmergency
                    + ", useImsForUt=" + useImsForUt
                    + ", isUt=" + isUt
                    + ", imsPhone=" + imsPhone
                    + ", imsPhone.isVolteEnabled()="
                    + ((imsPhone != null) ? imsPhone.isVolteEnabled() : "N/A")
                    + ", imsPhone.isVowifiEnabled()="
                    + ((imsPhone != null) ? imsPhone.isWifiCallingEnabled() : "N/A")
                    + ", imsPhone.isVideoEnabled()="
                    + ((imsPhone != null) ? imsPhone.isVideoEnabled() : "N/A")
                    + ", imsPhone.getServiceState().getState()="
                    + ((imsPhone != null) ? imsPhone.getServiceState().getState() : "N/A"));
        }

        Phone.checkWfcWifiOnlyModeBeforeDial(mImsPhone, mContext);

        if ((imsUseEnabled && (!isUt || useImsForUt)) || useImsForEmergency) {
            try {
                if (DBG) logd("Trying IMS PS call");
                return imsPhone.dial(dialString, uusInfo, videoState, intentExtras);
            } catch (CallStateException e) {
                if (DBG) logd("IMS PS call exception " + e +
                        "imsUseEnabled =" + imsUseEnabled + ", imsPhone =" + imsPhone);
                if (!Phone.CS_FALLBACK.equals(e.getMessage())) {
                    CallStateException ce = new CallStateException(e.getMessage());
                    ce.setStackTrace(e.getStackTrace());
                    throw ce;
                }
            }
        }

        if (mSST != null && mSST.mSS.getState() == ServiceState.STATE_OUT_OF_SERVICE
                && mSST.mSS.getDataRegState() != ServiceState.STATE_IN_SERVICE && !isEmergency) {
            throw new CallStateException("cannot dial in current state");
        }
        if (DBG) logd("Trying (non-IMS) CS call");

        if (isPhoneTypeGsm()) {
            return dialInternal(dialString, null, VideoProfile.STATE_AUDIO_ONLY, intentExtras);
        } else {
            return dialInternal(dialString, null, videoState, intentExtras);
        }
    }

    @Override
    protected Connection dialInternal(String dialString, UUSInfo uusInfo, int videoState,
                                      Bundle intentExtras)
            throws CallStateException {

        // Need to make sure dialString gets parsed properly
        String newDialString = PhoneNumberUtils.stripSeparators(dialString);

        if (isPhoneTypeGsm()) {
            // handle in-call MMI first if applicable
            if (handleInCallMmiCommands(newDialString)) {
                return null;
            }

            // Only look at the Network portion for mmi
            String networkPortion = PhoneNumberUtils.extractNetworkPortionAlt(newDialString);
            GsmMmiCode mmi =
                    GsmMmiCode.newFromDialString(networkPortion, this, mUiccApplication.get());
            if (DBG) logd("dialing w/ mmi '" + mmi + "'...");

            if (mmi == null) {
                return mCT.dial(newDialString, uusInfo, intentExtras);
            } else if (mmi.isTemporaryModeCLIR()) {
                return mCT.dial(mmi.mDialingNumber, mmi.getCLIRMode(), uusInfo, intentExtras);
            } else {
                mPendingMMIs.add(mmi);
                mMmiRegistrants.notifyRegistrants(new AsyncResult(null, mmi, null));
                try {
                    mmi.processCode();
                } catch (CallStateException e) {
                    //do nothing
                }

                // FIXME should this return null or something else?
                return null;
            }
        } else {
            return mCT.dial(newDialString);
        }
    }

    @Override
    public boolean handlePinMmi(String dialString) {
        MmiCode mmi;
        if (isPhoneTypeGsm()) {
            mmi = GsmMmiCode.newFromDialString(dialString, this, mUiccApplication.get());
        } else {
            mmi = CdmaMmiCode.newFromDialString(dialString, this, mUiccApplication.get());
        }

        if (mmi != null && mmi.isPinPukCommand()) {
            mPendingMMIs.add(mmi);
            mMmiRegistrants.notifyRegistrants(new AsyncResult(null, mmi, null));
            try {
                mmi.processCode();
            } catch (CallStateException e) {
                //do nothing
            }
            return true;
        }

        loge("Mmi is null or unrecognized!");
        return false;
    }

    @Override
    public void sendUssdResponse(String ussdMessge) {
        if (isPhoneTypeGsm()) {
            GsmMmiCode mmi = GsmMmiCode.newFromUssdUserInput(ussdMessge, this, mUiccApplication.get());
            mPendingMMIs.add(mmi);
            mMmiRegistrants.notifyRegistrants(new AsyncResult(null, mmi, null));
            mmi.sendUssd(ussdMessge);
        } else {
            loge("sendUssdResponse: not possible in CDMA");
        }
    }

    @Override
    public void sendDtmf(char c) {
        if (!PhoneNumberUtils.is12Key(c)) {
            loge("sendDtmf called with invalid character '" + c + "'");
        } else {
            if (mCT.mState ==  PhoneConstants.State.OFFHOOK) {
                mCi.sendDtmf(c, null);
            }
        }
    }

    @Override
    public void startDtmf(char c) {
        if (!PhoneNumberUtils.is12Key(c)) {
            loge("startDtmf called with invalid character '" + c + "'");
        } else {
            mCi.startDtmf(c, null);
        }
    }

    @Override
    public void stopDtmf() {
        mCi.stopDtmf(null);
    }

    @Override
    public void sendBurstDtmf(String dtmfString, int on, int off, Message onComplete) {
        if (isPhoneTypeGsm()) {
            loge("[GsmCdmaPhone] sendBurstDtmf() is a CDMA method");
        } else {
            boolean check = true;
            for (int itr = 0;itr < dtmfString.length(); itr++) {
                if (!PhoneNumberUtils.is12Key(dtmfString.charAt(itr))) {
                    Rlog.e(LOG_TAG,
                            "sendDtmf called with invalid character '" + dtmfString.charAt(itr)+ "'");
                    check = false;
                    break;
                }
            }
            if (mCT.mState == PhoneConstants.State.OFFHOOK && check) {
                mCi.sendBurstDtmf(dtmfString, on, off, onComplete);
            }
        }
    }

    @Override
    public void setRadioPower(boolean power) {
        mSST.setRadioPower(power);
    }

    private void storeVoiceMailNumber(String number) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
        SharedPreferences.Editor editor = sp.edit();
        if (isPhoneTypeGsm()) {
            editor.putString(VM_NUMBER + getPhoneId(), number);
            editor.apply();
            setVmSimImsi(getSubscriberId());
        } else {
            editor.putString(VM_NUMBER_CDMA + getPhoneId(), number);
            editor.apply();
        }
    }

    @Override
    public String getVoiceMailNumber() {
        String number = null;
        if (isPhoneTypeGsm()) {
            // Read from the SIM. If its null, try reading from the shared preference area.
            IccRecords r = mIccRecords.get();
            number = (r != null) ? r.getVoiceMailNumber() : "";
            if (TextUtils.isEmpty(number)) {
                SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
                number = sp.getString(VM_NUMBER + getPhoneId(), null);
            }
        } else {
            SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
            number = sp.getString(VM_NUMBER_CDMA + getPhoneId(), null);
        }

        if (TextUtils.isEmpty(number)) {
            String[] listArray = getContext().getResources()
                .getStringArray(com.android.internal.R.array.config_default_vm_number);
            if (listArray != null && listArray.length > 0) {
                for (int i=0; i<listArray.length; i++) {
                    if (!TextUtils.isEmpty(listArray[i])) {
                        String[] defaultVMNumberArray = listArray[i].split(";");
                        if (defaultVMNumberArray != null && defaultVMNumberArray.length > 0) {
                            if (defaultVMNumberArray.length == 1) {
                                number = defaultVMNumberArray[0];
                            } else if (defaultVMNumberArray.length == 2 &&
                                    !TextUtils.isEmpty(defaultVMNumberArray[1]) &&
                                    isMatchGid(defaultVMNumberArray[1])) {
                                number = defaultVMNumberArray[0];
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (!isPhoneTypeGsm() && TextUtils.isEmpty(number)) {
            // Read platform settings for dynamic voicemail number
            if (getContext().getResources().getBoolean(com.android.internal
                    .R.bool.config_telephony_use_own_number_for_voicemail)) {
                number = getLine1Number();
            } else {
                number = "*86";
            }
        }

        return number;
    }

    private String getVmSimImsi() {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
        return sp.getString(VM_SIM_IMSI + getPhoneId(), null);
    }

    private void setVmSimImsi(String imsi) {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
        SharedPreferences.Editor editor = sp.edit();
        editor.putString(VM_SIM_IMSI + getPhoneId(), imsi);
        editor.apply();
    }

    @Override
    public String getVoiceMailAlphaTag() {
        String ret = "";

        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();

            ret = (r != null) ? r.getVoiceMailAlphaTag() : "";
        }

        if (ret == null || ret.length() == 0) {
            return mContext.getText(
                com.android.internal.R.string.defaultVoiceMailAlphaTag).toString();
        }

        return ret;
    }

    @Override
    public String getDeviceId() {
        if (isPhoneTypeGsm()) {
            return mImei;
        } else {
            CarrierConfigManager configManager = (CarrierConfigManager)
                    mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
            boolean force_imei = configManager.getConfigForSubId(getSubId())
                    .getBoolean(CarrierConfigManager.KEY_FORCE_IMEI_BOOL);
            if (force_imei) return mImei;

            String id = getMeid();
            if ((id == null) || id.matches("^0*$")) {
                loge("getDeviceId(): MEID is not initialized use ESN");
                id = getEsn();
            }
            return id;
        }
    }

    @Override
    public String getDeviceSvn() {
        if (isPhoneTypeGsm() || isPhoneTypeCdmaLte()) {
            return mImeiSv;
        } else {
            loge("getDeviceSvn(): return 0");
            return "0";
        }
    }

    @Override
    public IsimRecords getIsimRecords() {
        return mIsimUiccRecords;
    }

    @Override
    public String getImei() {
        return mImei;
    }

    @Override
    public String getEsn() {
        if (isPhoneTypeGsm()) {
            loge("[GsmCdmaPhone] getEsn() is a CDMA method");
            return "0";
        } else {
            return mEsn;
        }
    }

    @Override
    public String getMeid() {
        if (isPhoneTypeGsm()) {
            loge("[GsmCdmaPhone] getMeid() is a CDMA method");
            return "0";
        } else {
            return mMeid;
        }
    }

    @Override
    public String getNai() {
        IccRecords r = mUiccController.getIccRecords(mPhoneId, UiccController.APP_FAM_3GPP2);
        if (Log.isLoggable(LOG_TAG, Log.VERBOSE)) {
            Rlog.v(LOG_TAG, "IccRecords is " + r);
        }
        return (r != null) ? r.getNAI() : null;
    }

    @Override
    public String getSubscriberId() {
        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();
            return (r != null) ? r.getIMSI() : null;
        } else if (isPhoneTypeCdma()) {
            return mSST.getImsi();
        } else { //isPhoneTypeCdmaLte()
            return (mSimRecords != null) ? mSimRecords.getIMSI() : "";
        }
    }

    @Override
    public String getGroupIdLevel1() {
        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();
            return (r != null) ? r.getGid1() : null;
        } else if (isPhoneTypeCdma()) {
            loge("GID1 is not available in CDMA");
            return null;
        } else { //isPhoneTypeCdmaLte()
            return (mSimRecords != null) ? mSimRecords.getGid1() : "";
        }
    }

    @Override
    public String getGroupIdLevel2() {
        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();
            return (r != null) ? r.getGid2() : null;
        } else if (isPhoneTypeCdma()) {
            loge("GID2 is not available in CDMA");
            return null;
        } else { //isPhoneTypeCdmaLte()
            return (mSimRecords != null) ? mSimRecords.getGid2() : "";
        }
    }

    @Override
    public String getLine1Number() {
        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();
            return (r != null) ? r.getMsisdnNumber() : null;
        } else {
            return mSST.getMdnNumber();
        }
    }

    @Override
    public String getCdmaPrlVersion() {
        return mSST.getPrlVersion();
    }

    @Override
    public String getCdmaMin() {
        return mSST.getCdmaMin();
    }

    @Override
    public boolean isMinInfoReady() {
        return mSST.isMinInfoReady();
    }

    @Override
    public String getMsisdn() {
        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();
            return (r != null) ? r.getMsisdnNumber() : null;
        } else if (isPhoneTypeCdmaLte()) {
            return (mSimRecords != null) ? mSimRecords.getMsisdnNumber() : null;
        } else {
            loge("getMsisdn: not expected on CDMA");
            return null;
        }
    }

    @Override
    public String getLine1AlphaTag() {
        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();
            return (r != null) ? r.getMsisdnAlphaTag() : null;
        } else {
            loge("getLine1AlphaTag: not possible in CDMA");
            return null;
        }
    }

    @Override
    public boolean setLine1Number(String alphaTag, String number, Message onComplete) {
        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();
            if (r != null) {
                r.setMsisdnNumber(alphaTag, number, onComplete);
                return true;
            } else {
                return false;
            }
        } else {
            loge("setLine1Number: not possible in CDMA");
            return false;
        }
    }

    @Override
    public void setVoiceMailNumber(String alphaTag, String voiceMailNumber, Message onComplete) {
        Message resp;
        mVmNumber = voiceMailNumber;
        resp = obtainMessage(EVENT_SET_VM_NUMBER_DONE, 0, 0, onComplete);
        IccRecords r = mIccRecords.get();
        if (r != null) {
            r.setVoiceMailNumber(alphaTag, mVmNumber, resp);
        }
    }

    private boolean isValidCommandInterfaceCFReason (int commandInterfaceCFReason) {
        switch (commandInterfaceCFReason) {
            case CF_REASON_UNCONDITIONAL:
            case CF_REASON_BUSY:
            case CF_REASON_NO_REPLY:
            case CF_REASON_NOT_REACHABLE:
            case CF_REASON_ALL:
            case CF_REASON_ALL_CONDITIONAL:
                return true;
            default:
                return false;
        }
    }

    @Override
    public String getSystemProperty(String property, String defValue) {
        if (isPhoneTypeGsm() || isPhoneTypeCdmaLte()) {
            if (getUnitTestMode()) {
                return null;
            }
            return TelephonyManager.getTelephonyProperty(mPhoneId, property, defValue);
        } else {
            return super.getSystemProperty(property, defValue);
        }
    }

    private boolean isValidCommandInterfaceCFAction (int commandInterfaceCFAction) {
        switch (commandInterfaceCFAction) {
            case CF_ACTION_DISABLE:
            case CF_ACTION_ENABLE:
            case CF_ACTION_REGISTRATION:
            case CF_ACTION_ERASURE:
                return true;
            default:
                return false;
        }
    }

    private boolean isCfEnable(int action) {
        return (action == CF_ACTION_ENABLE) || (action == CF_ACTION_REGISTRATION);
    }

    @Override
    public void getCallForwardingOption(int commandInterfaceCFReason, Message onComplete) {
        if (isPhoneTypeGsm()) {
            Phone imsPhone = mImsPhone;
            if ((imsPhone != null)
                    && ((imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE)
                    || imsPhone.isUtEnabled())) {
                imsPhone.getCallForwardingOption(commandInterfaceCFReason, onComplete);
                return;
            }

            if (isValidCommandInterfaceCFReason(commandInterfaceCFReason)) {
                if (DBG) logd("requesting call forwarding query.");
                Message resp;
                if (commandInterfaceCFReason == CF_REASON_UNCONDITIONAL) {
                    resp = obtainMessage(EVENT_GET_CALL_FORWARD_DONE, onComplete);
                } else {
                    resp = onComplete;
                }
                mCi.queryCallForwardStatus(commandInterfaceCFReason, 0, null, resp);
            }
        } else {
            loge("getCallForwardingOption: not possible in CDMA");
        }
    }

    @Override
    public void setCallForwardingOption(int commandInterfaceCFAction,
            int commandInterfaceCFReason,
            String dialingNumber,
            int timerSeconds,
            Message onComplete) {
        if (isPhoneTypeGsm()) {
            Phone imsPhone = mImsPhone;
            if ((imsPhone != null)
                    && ((imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE)
                    || imsPhone.isUtEnabled())) {
                imsPhone.setCallForwardingOption(commandInterfaceCFAction,
                        commandInterfaceCFReason, dialingNumber, timerSeconds, onComplete);
                return;
            }

            if ((isValidCommandInterfaceCFAction(commandInterfaceCFAction)) &&
                    (isValidCommandInterfaceCFReason(commandInterfaceCFReason))) {

                Message resp;
                if (commandInterfaceCFReason == CF_REASON_UNCONDITIONAL) {
                    Cfu cfu = new Cfu(dialingNumber, onComplete);
                    resp = obtainMessage(EVENT_SET_CALL_FORWARD_DONE,
                            isCfEnable(commandInterfaceCFAction) ? 1 : 0, 0, cfu);
                } else {
                    resp = onComplete;
                }
                mCi.setCallForward(commandInterfaceCFAction,
                        commandInterfaceCFReason,
                        CommandsInterface.SERVICE_CLASS_VOICE,
                        dialingNumber,
                        timerSeconds,
                        resp);
            }
        } else {
            loge("setCallForwardingOption: not possible in CDMA");
        }
    }

    @Override
    public void getOutgoingCallerIdDisplay(Message onComplete) {
        if (isPhoneTypeGsm()) {
            Phone imsPhone = mImsPhone;
            if ((imsPhone != null)
                    && (imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE)) {
                imsPhone.getOutgoingCallerIdDisplay(onComplete);
                return;
            }
            mCi.getCLIR(onComplete);
        } else {
            loge("getOutgoingCallerIdDisplay: not possible in CDMA");
        }
    }

    @Override
    public void setOutgoingCallerIdDisplay(int commandInterfaceCLIRMode, Message onComplete) {
        if (isPhoneTypeGsm()) {
            Phone imsPhone = mImsPhone;
            if ((imsPhone != null)
                    && (imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE)) {
                imsPhone.setOutgoingCallerIdDisplay(commandInterfaceCLIRMode, onComplete);
                return;
            }
            // Packing CLIR value in the message. This will be required for
            // SharedPreference caching, if the message comes back as part of
            // a success response.
            mCi.setCLIR(commandInterfaceCLIRMode,
                    obtainMessage(EVENT_SET_CLIR_COMPLETE, commandInterfaceCLIRMode, 0, onComplete));
        } else {
            loge("setOutgoingCallerIdDisplay: not possible in CDMA");
        }
    }

    @Override
    public void getCallWaiting(Message onComplete) {
        if (isPhoneTypeGsm()) {
            Phone imsPhone = mImsPhone;
            if ((imsPhone != null)
                    && ((imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE)
                    || imsPhone.isUtEnabled())) {
                imsPhone.getCallWaiting(onComplete);
                return;
            }

            //As per 3GPP TS 24.083, section 1.6 UE doesn't need to send service
            //class parameter in call waiting interrogation  to network
            mCi.queryCallWaiting(CommandsInterface.SERVICE_CLASS_NONE, onComplete);
        } else {
            mCi.queryCallWaiting(CommandsInterface.SERVICE_CLASS_VOICE, onComplete);
        }
    }

    @Override
    public void setCallWaiting(boolean enable, Message onComplete) {
        if (isPhoneTypeGsm()) {
            Phone imsPhone = mImsPhone;
            if ((imsPhone != null)
                    && ((imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE)
                    || imsPhone.isUtEnabled())) {
                imsPhone.setCallWaiting(enable, onComplete);
                return;
            }

            mCi.setCallWaiting(enable, CommandsInterface.SERVICE_CLASS_VOICE, onComplete);
        } else {
            loge("method setCallWaiting is NOT supported in CDMA!");
        }
    }

    @Override
    public void getAvailableNetworks(Message response) {
        if (isPhoneTypeGsm() || isPhoneTypeCdmaLte()) {
            mCi.getAvailableNetworks(response);
        } else {
            loge("getAvailableNetworks: not possible in CDMA");
        }
    }

    @Override
    public void getNeighboringCids(Message response) {
        if (isPhoneTypeGsm()) {
            mCi.getNeighboringCids(response);
        } else {
            /*
             * This is currently not implemented.  At least as of June
             * 2009, there is no neighbor cell information available for
             * CDMA because some party is resisting making this
             * information readily available.  Consequently, calling this
             * function can have no useful effect.  This situation may
             * (and hopefully will) change in the future.
             */
            if (response != null) {
                CommandException ce = new CommandException(
                        CommandException.Error.REQUEST_NOT_SUPPORTED);
                AsyncResult.forMessage(response).exception = ce;
                response.sendToTarget();
            }
        }
    }

    @Override
    public void setUiTTYMode(int uiTtyMode, Message onComplete) {
       if (mImsPhone != null) {
           mImsPhone.setUiTTYMode(uiTtyMode, onComplete);
       }
    }

    @Override
    public void setMute(boolean muted) {
        mCT.setMute(muted);
    }

    @Override
    public boolean getMute() {
        return mCT.getMute();
    }

    @Override
    public void getDataCallList(Message response) {
        mCi.getDataCallList(response);
    }

    @Override
    public void updateServiceLocation() {
        mSST.enableSingleLocationUpdate();
    }

    @Override
    public void enableLocationUpdates() {
        mSST.enableLocationUpdates();
    }

    @Override
    public void disableLocationUpdates() {
        mSST.disableLocationUpdates();
    }

    @Override
    public boolean getDataRoamingEnabled() {
        return mDcTracker.getDataOnRoamingEnabled();
    }

    @Override
    public void setDataRoamingEnabled(boolean enable) {
        mDcTracker.setDataOnRoamingEnabled(enable);
    }

    @Override
    public void registerForCdmaOtaStatusChange(Handler h, int what, Object obj) {
        mCi.registerForCdmaOtaProvision(h, what, obj);
    }

    @Override
    public void unregisterForCdmaOtaStatusChange(Handler h) {
        mCi.unregisterForCdmaOtaProvision(h);
    }

    @Override
    public void registerForSubscriptionInfoReady(Handler h, int what, Object obj) {
        mSST.registerForSubscriptionInfoReady(h, what, obj);
    }

    @Override
    public void unregisterForSubscriptionInfoReady(Handler h) {
        mSST.unregisterForSubscriptionInfoReady(h);
    }

    @Override
    public void setOnEcbModeExitResponse(Handler h, int what, Object obj) {
        mEcmExitRespRegistrant = new Registrant(h, what, obj);
    }

    @Override
    public void unsetOnEcbModeExitResponse(Handler h) {
        mEcmExitRespRegistrant.clear();
    }

    @Override
    public void registerForCallWaiting(Handler h, int what, Object obj) {
        mCT.registerForCallWaiting(h, what, obj);
    }

    @Override
    public void unregisterForCallWaiting(Handler h) {
        mCT.unregisterForCallWaiting(h);
    }

    @Override
    public boolean getDataEnabled() {
        return mDcTracker.getDataEnabled();
    }

    @Override
    public void setDataEnabled(boolean enable) {
        mDcTracker.setDataEnabled(enable);
    }

    /**
     * Removes the given MMI from the pending list and notifies
     * registrants that it is complete.
     * @param mmi MMI that is done
     */
    public void onMMIDone(MmiCode mmi) {

        /* Only notify complete if it's on the pending list.
         * Otherwise, it's already been handled (eg, previously canceled).
         * The exception is cancellation of an incoming USSD-REQUEST, which is
         * not on the list.
         */
        if (mPendingMMIs.remove(mmi) || (isPhoneTypeGsm() && (mmi.isUssdRequest() ||
                ((GsmMmiCode)mmi).isSsInfo()))) {
            mMmiCompleteRegistrants.notifyRegistrants(new AsyncResult(null, mmi, null));
        }
    }

    private void onNetworkInitiatedUssd(MmiCode mmi) {
        mMmiCompleteRegistrants.notifyRegistrants(
            new AsyncResult(null, mmi, null));
    }

    /** ussdMode is one of CommandsInterface.USSD_MODE_* */
    private void onIncomingUSSD (int ussdMode, String ussdMessage) {
        if (!isPhoneTypeGsm()) {
            loge("onIncomingUSSD: not expected on GSM");
        }
        boolean isUssdError;
        boolean isUssdRequest;
        boolean isUssdRelease;

        isUssdRequest
            = (ussdMode == CommandsInterface.USSD_MODE_REQUEST);

        isUssdError
            = (ussdMode != CommandsInterface.USSD_MODE_NOTIFY
                && ussdMode != CommandsInterface.USSD_MODE_REQUEST);

        isUssdRelease = (ussdMode == CommandsInterface.USSD_MODE_NW_RELEASE);


        // See comments in GsmMmiCode.java
        // USSD requests aren't finished until one
        // of these two events happen
        GsmMmiCode found = null;
        for (int i = 0, s = mPendingMMIs.size() ; i < s; i++) {
            if(((GsmMmiCode)mPendingMMIs.get(i)).isPendingUSSD()) {
                found = (GsmMmiCode)mPendingMMIs.get(i);
                break;
            }
        }

        if (found != null) {
            // Complete pending USSD

            if (isUssdRelease) {
                found.onUssdRelease();
            } else if (isUssdError) {
                found.onUssdFinishedError();
            } else {
                found.onUssdFinished(ussdMessage, isUssdRequest);
            }
        } else { // pending USSD not found
            // The network may initiate its own USSD request

            // ignore everything that isnt a Notify or a Request
            // also, discard if there is no message to present
            if (!isUssdError && ussdMessage != null) {
                GsmMmiCode mmi;
                mmi = GsmMmiCode.newNetworkInitiatedUssd(ussdMessage,
                                                   isUssdRequest,
                                                   GsmCdmaPhone.this,
                                                   mUiccApplication.get());
                onNetworkInitiatedUssd(mmi);
            }
        }
    }

    /**
     * Make sure the network knows our preferred setting.
     */
    private void syncClirSetting() {
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
        int clirSetting = sp.getInt(CLIR_KEY + getPhoneId(), -1);
        if (clirSetting >= 0) {
            mCi.setCLIR(clirSetting, null);
        }
    }

    private void handleRadioAvailable() {
        mCi.getBasebandVersion(obtainMessage(EVENT_GET_BASEBAND_VERSION_DONE));

        if (isPhoneTypeGsm()) {
            mCi.getIMEI(obtainMessage(EVENT_GET_IMEI_DONE));
            mCi.getIMEISV(obtainMessage(EVENT_GET_IMEISV_DONE));
        } else {
            mCi.getDeviceIdentity(obtainMessage(EVENT_GET_DEVICE_IDENTITY_DONE));
        }
        mCi.getRadioCapability(obtainMessage(EVENT_GET_RADIO_CAPABILITY));
        startLceAfterRadioIsAvailable();
    }

    private void handleRadioOn() {
        /* Proactively query voice radio technologies */
        mCi.getVoiceRadioTechnology(obtainMessage(EVENT_REQUEST_VOICE_RADIO_TECH_DONE));

        if (!isPhoneTypeGsm()) {
            mCdmaSubscriptionSource = mCdmaSSM.getCdmaSubscriptionSource();
        }

        // If this is on APM off, SIM may already be loaded. Send setPreferredNetworkType
        // request to RIL to preserve user setting across APM toggling
        setPreferredNetworkTypeIfSimLoaded();
    }

    private void handleRadioOffOrNotAvailable() {
        if (isPhoneTypeGsm()) {
            // Some MMI requests (eg USSD) are not completed
            // within the course of a CommandsInterface request
            // If the radio shuts off or resets while one of these
            // is pending, we need to clean up.

            for (int i = mPendingMMIs.size() - 1; i >= 0; i--) {
                if (((GsmMmiCode) mPendingMMIs.get(i)).isPendingUSSD()) {
                    ((GsmMmiCode) mPendingMMIs.get(i)).onUssdFinishedError();
                }
            }
        }
        Phone imsPhone = mImsPhone;
        if (imsPhone != null) {
            imsPhone.getServiceState().setStateOff();
        }
        mRadioOffOrNotAvailableRegistrants.notifyRegistrants();
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        Message onComplete;

        switch (msg.what) {
            case EVENT_RADIO_AVAILABLE: {
                handleRadioAvailable();
            }
            break;

            case EVENT_GET_DEVICE_IDENTITY_DONE:{
                ar = (AsyncResult)msg.obj;

                if (ar.exception != null) {
                    break;
                }
                String[] respId = (String[])ar.result;
                mImei = respId[0];
                mImeiSv = respId[1];
                mEsn  =  respId[2];
                mMeid =  respId[3];
            }
            break;

            case EVENT_EMERGENCY_CALLBACK_MODE_ENTER:{
                handleEnterEmergencyCallbackMode(msg);
            }
            break;

            case  EVENT_EXIT_EMERGENCY_CALLBACK_RESPONSE:{
                handleExitEmergencyCallbackMode(msg);
            }
            break;

            case EVENT_RUIM_RECORDS_LOADED:
                logd("Event EVENT_RUIM_RECORDS_LOADED Received");
                updateCurrentCarrierInProvider();
                break;

            case EVENT_RADIO_ON:
                logd("Event EVENT_RADIO_ON Received");
                handleRadioOn();
                break;

            case EVENT_RIL_CONNECTED:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null && ar.result != null) {
                    mRilVersion = (Integer) ar.result;
                } else {
                    logd("Unexpected exception on EVENT_RIL_CONNECTED");
                    mRilVersion = -1;
                }
                break;

            case EVENT_VOICE_RADIO_TECH_CHANGED:
            case EVENT_REQUEST_VOICE_RADIO_TECH_DONE:
                String what = (msg.what == EVENT_VOICE_RADIO_TECH_CHANGED) ?
                        "EVENT_VOICE_RADIO_TECH_CHANGED" : "EVENT_REQUEST_VOICE_RADIO_TECH_DONE";
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    if ((ar.result != null) && (((int[]) ar.result).length != 0)) {
                        int newVoiceTech = ((int[]) ar.result)[0];
                        logd(what + ": newVoiceTech=" + newVoiceTech);
                        phoneObjectUpdater(newVoiceTech);
                    } else {
                        loge(what + ": has no tech!");
                    }
                } else {
                    loge(what + ": exception=" + ar.exception);
                }
                break;

            case EVENT_UPDATE_PHONE_OBJECT:
                phoneObjectUpdater(msg.arg1);
                break;

            case EVENT_CARRIER_CONFIG_CHANGED:
                // Only check for the voice radio tech if it not going to be updated by the voice
                // registration changes.
                if (!mContext.getResources().getBoolean(com.android.internal.R.bool.
                        config_switch_phone_on_voice_reg_state_change)) {
                    mCi.getVoiceRadioTechnology(obtainMessage(EVENT_REQUEST_VOICE_RADIO_TECH_DONE));
                }
                // Force update IMS service
                ImsManager.updateImsServiceConfig(mContext, mPhoneId, true);

                // Update broadcastEmergencyCallStateChanges
                CarrierConfigManager configMgr = (CarrierConfigManager)
                        getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
                PersistableBundle b = configMgr.getConfigForSubId(getSubId());
                if (b != null) {
                    boolean broadcastEmergencyCallStateChanges = b.getBoolean(
                            CarrierConfigManager.KEY_BROADCAST_EMERGENCY_CALL_STATE_CHANGES_BOOL);
                    logd("broadcastEmergencyCallStateChanges = " +
                            broadcastEmergencyCallStateChanges);
                    setBroadcastEmergencyCallStateChanges(broadcastEmergencyCallStateChanges);
                } else {
                    loge("didn't get broadcastEmergencyCallStateChanges from carrier config");
                }

                // Changing the cdma roaming settings based carrier config.
                if (b != null) {
                    int config_cdma_roaming_mode = b.getInt(
                            CarrierConfigManager.KEY_CDMA_ROAMING_MODE_INT);
                    int current_cdma_roaming_mode =
                            Settings.Global.getInt(getContext().getContentResolver(),
                            Settings.Global.CDMA_ROAMING_MODE,
                            CarrierConfigManager.CDMA_ROAMING_MODE_RADIO_DEFAULT);
                    switch (config_cdma_roaming_mode) {
                        // Carrier's cdma_roaming_mode will overwrite the user's previous settings
                        // Keep the user's previous setting in global variable which will be used
                        // when carrier's setting is turn off.
                        case CarrierConfigManager.CDMA_ROAMING_MODE_HOME:
                        case CarrierConfigManager.CDMA_ROAMING_MODE_AFFILIATED:
                        case CarrierConfigManager.CDMA_ROAMING_MODE_ANY:
                            logd("cdma_roaming_mode is going to changed to "
                                    + config_cdma_roaming_mode);
                            setCdmaRoamingPreference(config_cdma_roaming_mode,
                                    obtainMessage(EVENT_SET_ROAMING_PREFERENCE_DONE));
                            break;

                        // When carrier's setting is turn off, change the cdma_roaming_mode to the
                        // previous user's setting
                        case CarrierConfigManager.CDMA_ROAMING_MODE_RADIO_DEFAULT:
                            if (current_cdma_roaming_mode != config_cdma_roaming_mode) {
                                logd("cdma_roaming_mode is going to changed to "
                                        + current_cdma_roaming_mode);
                                setCdmaRoamingPreference(current_cdma_roaming_mode,
                                        obtainMessage(EVENT_SET_ROAMING_PREFERENCE_DONE));
                            }

                        default:
                            loge("Invalid cdma_roaming_mode settings: "
                                    + config_cdma_roaming_mode);
                    }
                } else {
                    loge("didn't get the cdma_roaming_mode changes from the carrier config.");
                }

                // Load the ERI based on carrier config. Carrier might have their specific ERI.
                prepareEri();
                if (!isPhoneTypeGsm()) {
                    mSST.pollState();
                }

                break;

            case EVENT_SET_ROAMING_PREFERENCE_DONE:
                logd("cdma_roaming_mode change is done");
                break;

            case EVENT_CDMA_SUBSCRIPTION_SOURCE_CHANGED:
                logd("EVENT_CDMA_SUBSCRIPTION_SOURCE_CHANGED");
                mCdmaSubscriptionSource = mCdmaSSM.getCdmaSubscriptionSource();
                break;

            case EVENT_REGISTERED_TO_NETWORK:
                logd("Event EVENT_REGISTERED_TO_NETWORK Received");
                if (isPhoneTypeGsm()) {
                    syncClirSetting();
                }
                break;

            case EVENT_SIM_RECORDS_LOADED:
                if (isPhoneTypeGsm()) {
                    updateCurrentCarrierInProvider();

                    // Check if this is a different SIM than the previous one. If so unset the
                    // voice mail number.
                    String imsi = getVmSimImsi();
                    String imsiFromSIM = getSubscriberId();
                    if (imsi != null && imsiFromSIM != null && !imsiFromSIM.equals(imsi)) {
                        storeVoiceMailNumber(null);
                        setVmSimImsi(null);
                    }
                }

                mSimRecordsLoadedRegistrants.notifyRegistrants();
                break;

            case EVENT_GET_BASEBAND_VERSION_DONE:
                ar = (AsyncResult)msg.obj;

                if (ar.exception != null) {
                    break;
                }

                if (DBG) logd("Baseband version: " + ar.result);
                TelephonyManager.from(mContext).setBasebandVersionForPhone(getPhoneId(),
                        (String)ar.result);
            break;

            case EVENT_GET_IMEI_DONE:
                ar = (AsyncResult)msg.obj;

                if (ar.exception != null) {
                    break;
                }

                mImei = (String)ar.result;
            break;

            case EVENT_GET_IMEISV_DONE:
                ar = (AsyncResult)msg.obj;

                if (ar.exception != null) {
                    break;
                }

                mImeiSv = (String)ar.result;
            break;

            case EVENT_USSD:
                ar = (AsyncResult)msg.obj;

                String[] ussdResult = (String[]) ar.result;

                if (ussdResult.length > 1) {
                    try {
                        onIncomingUSSD(Integer.parseInt(ussdResult[0]), ussdResult[1]);
                    } catch (NumberFormatException e) {
                        Rlog.w(LOG_TAG, "error parsing USSD");
                    }
                }
            break;

            case EVENT_RADIO_OFF_OR_NOT_AVAILABLE: {
                logd("Event EVENT_RADIO_OFF_OR_NOT_AVAILABLE Received");
                handleRadioOffOrNotAvailable();
                break;
            }

            case EVENT_SSN:
                logd("Event EVENT_SSN Received");
                if (isPhoneTypeGsm()) {
                    ar = (AsyncResult) msg.obj;
                    SuppServiceNotification not = (SuppServiceNotification) ar.result;
                    mSsnRegistrants.notifyRegistrants(ar);
                }
                break;

            case EVENT_SET_CALL_FORWARD_DONE:
                ar = (AsyncResult)msg.obj;
                IccRecords r = mIccRecords.get();
                Cfu cfu = (Cfu) ar.userObj;
                if (ar.exception == null && r != null) {
                    setVoiceCallForwardingFlag(1, msg.arg1 == 1, cfu.mSetCfNumber);
                }
                if (cfu.mOnComplete != null) {
                    AsyncResult.forMessage(cfu.mOnComplete, ar.result, ar.exception);
                    cfu.mOnComplete.sendToTarget();
                }
                break;

            case EVENT_SET_VM_NUMBER_DONE:
                ar = (AsyncResult)msg.obj;
                if ((isPhoneTypeGsm() && IccVmNotSupportedException.class.isInstance(ar.exception)) ||
                        (!isPhoneTypeGsm() && IccException.class.isInstance(ar.exception))){
                    storeVoiceMailNumber(mVmNumber);
                    ar.exception = null;
                }
                onComplete = (Message) ar.userObj;
                if (onComplete != null) {
                    AsyncResult.forMessage(onComplete, ar.result, ar.exception);
                    onComplete.sendToTarget();
                }
                break;


            case EVENT_GET_CALL_FORWARD_DONE:
                ar = (AsyncResult)msg.obj;
                if (ar.exception == null) {
                    handleCfuQueryResult((CallForwardInfo[])ar.result);
                }
                onComplete = (Message) ar.userObj;
                if (onComplete != null) {
                    AsyncResult.forMessage(onComplete, ar.result, ar.exception);
                    onComplete.sendToTarget();
                }
                break;

            case EVENT_SET_NETWORK_AUTOMATIC:
                // Automatic network selection from EF_CSP SIM record
                ar = (AsyncResult) msg.obj;
                if (mSST.mSS.getIsManualSelection()) {
                    setNetworkSelectionModeAutomatic((Message) ar.result);
                    logd("SET_NETWORK_SELECTION_AUTOMATIC: set to automatic");
                } else {
                    // prevent duplicate request which will push current PLMN to low priority
                    logd("SET_NETWORK_SELECTION_AUTOMATIC: already automatic, ignore");
                }
                break;

            case EVENT_ICC_RECORD_EVENTS:
                ar = (AsyncResult)msg.obj;
                processIccRecordEvents((Integer)ar.result);
                break;

            case EVENT_SET_CLIR_COMPLETE:
                ar = (AsyncResult)msg.obj;
                if (ar.exception == null) {
                    saveClirSetting(msg.arg1);
                }
                onComplete = (Message) ar.userObj;
                if (onComplete != null) {
                    AsyncResult.forMessage(onComplete, ar.result, ar.exception);
                    onComplete.sendToTarget();
                }
                break;

            case EVENT_SS:
                ar = (AsyncResult)msg.obj;
                logd("Event EVENT_SS received");
                if (isPhoneTypeGsm()) {
                    // SS data is already being handled through MMI codes.
                    // So, this result if processed as MMI response would help
                    // in re-using the existing functionality.
                    GsmMmiCode mmi = new GsmMmiCode(this, mUiccApplication.get());
                    mmi.processSsData(ar);
                }
                break;

            case EVENT_GET_RADIO_CAPABILITY:
                ar = (AsyncResult) msg.obj;
                RadioCapability rc = (RadioCapability) ar.result;
                if (ar.exception != null) {
                    Rlog.d(LOG_TAG, "get phone radio capability fail, no need to change " +
                            "mRadioCapability");
                } else {
                    radioCapabilityUpdated(rc);
                }
                Rlog.d(LOG_TAG, "EVENT_GET_RADIO_CAPABILITY: phone rc: " + rc);
                break;

            default:
                super.handleMessage(msg);
        }
    }

    public UiccCardApplication getUiccCardApplication() {
        if (isPhoneTypeGsm()) {
            return mUiccController.getUiccCardApplication(mPhoneId, UiccController.APP_FAM_3GPP);
        } else {
            return mUiccController.getUiccCardApplication(mPhoneId, UiccController.APP_FAM_3GPP2);
        }
    }

    @Override
    protected void onUpdateIccAvailability() {
        if (mUiccController == null ) {
            return;
        }

        UiccCardApplication newUiccApplication = null;

        // Update mIsimUiccRecords
        if (isPhoneTypeGsm() || isPhoneTypeCdmaLte()) {
            newUiccApplication =
                    mUiccController.getUiccCardApplication(mPhoneId, UiccController.APP_FAM_IMS);
            IsimUiccRecords newIsimUiccRecords = null;

            if (newUiccApplication != null) {
                newIsimUiccRecords = (IsimUiccRecords) newUiccApplication.getIccRecords();
                if (DBG) logd("New ISIM application found");
            }
            mIsimUiccRecords = newIsimUiccRecords;
        }

        // Update mSimRecords
        if (mSimRecords != null) {
            mSimRecords.unregisterForRecordsLoaded(this);
        }
        if (isPhoneTypeCdmaLte()) {
            newUiccApplication = mUiccController.getUiccCardApplication(mPhoneId,
                    UiccController.APP_FAM_3GPP);
            SIMRecords newSimRecords = null;
            if (newUiccApplication != null) {
                newSimRecords = (SIMRecords) newUiccApplication.getIccRecords();
            }
            mSimRecords = newSimRecords;
            if (mSimRecords != null) {
                mSimRecords.registerForRecordsLoaded(this, EVENT_SIM_RECORDS_LOADED, null);
            }
        } else {
            mSimRecords = null;
        }

        // Update mIccRecords, mUiccApplication, mIccPhoneBookIntManager
        newUiccApplication = getUiccCardApplication();
        if (!isPhoneTypeGsm() && newUiccApplication == null) {
            logd("can't find 3GPP2 application; trying APP_FAM_3GPP");
            newUiccApplication = mUiccController.getUiccCardApplication(mPhoneId,
                    UiccController.APP_FAM_3GPP);
        }

        UiccCardApplication app = mUiccApplication.get();
        if (app != newUiccApplication) {
            if (app != null) {
                if (DBG) logd("Removing stale icc objects.");
                if (mIccRecords.get() != null) {
                    unregisterForIccRecordEvents();
                    mIccPhoneBookIntManager.updateIccRecords(null);
                }
                mIccRecords.set(null);
                mUiccApplication.set(null);
            }
            if (newUiccApplication != null) {
                if (DBG) {
                    logd("New Uicc application found. type = " + newUiccApplication.getType());
                }
                mUiccApplication.set(newUiccApplication);
                mIccRecords.set(newUiccApplication.getIccRecords());
                registerForIccRecordEvents();
                mIccPhoneBookIntManager.updateIccRecords(mIccRecords.get());
            }
        }
    }

    private void processIccRecordEvents(int eventCode) {
        switch (eventCode) {
            case IccRecords.EVENT_CFI:
                notifyCallForwardingIndicator();
                break;
        }
    }

    /**
     * Sets the "current" field in the telephony provider according to the SIM's operator
     *
     * @return true for success; false otherwise.
     */
    @Override
    public boolean updateCurrentCarrierInProvider() {
        if (isPhoneTypeGsm() || isPhoneTypeCdmaLte()) {
            long currentDds = SubscriptionManager.getDefaultDataSubscriptionId();
            String operatorNumeric = getOperatorNumeric();

            logd("updateCurrentCarrierInProvider: mSubId = " + getSubId()
                    + " currentDds = " + currentDds + " operatorNumeric = " + operatorNumeric);

            if (!TextUtils.isEmpty(operatorNumeric) && (getSubId() == currentDds)) {
                try {
                    Uri uri = Uri.withAppendedPath(Telephony.Carriers.CONTENT_URI, "current");
                    ContentValues map = new ContentValues();
                    map.put(Telephony.Carriers.NUMERIC, operatorNumeric);
                    mContext.getContentResolver().insert(uri, map);
                    return true;
                } catch (SQLException e) {
                    Rlog.e(LOG_TAG, "Can't store current operator", e);
                }
            }
            return false;
        } else {
            return true;
        }
    }

    //CDMA
    /**
     * Sets the "current" field in the telephony provider according to the
     * build-time operator numeric property
     *
     * @return true for success; false otherwise.
     */
    private boolean updateCurrentCarrierInProvider(String operatorNumeric) {
        if (isPhoneTypeCdma()
                || (isPhoneTypeCdmaLte() && mUiccController.getUiccCardApplication(mPhoneId,
                        UiccController.APP_FAM_3GPP) == null)) {
            logd("CDMAPhone: updateCurrentCarrierInProvider called");
            if (!TextUtils.isEmpty(operatorNumeric)) {
                try {
                    Uri uri = Uri.withAppendedPath(Telephony.Carriers.CONTENT_URI, "current");
                    ContentValues map = new ContentValues();
                    map.put(Telephony.Carriers.NUMERIC, operatorNumeric);
                    logd("updateCurrentCarrierInProvider from system: numeric=" + operatorNumeric);
                    getContext().getContentResolver().insert(uri, map);

                    // Updates MCC MNC device configuration information
                    logd("update mccmnc=" + operatorNumeric);
                    MccTable.updateMccMncConfiguration(mContext, operatorNumeric, false);

                    return true;
                } catch (SQLException e) {
                    Rlog.e(LOG_TAG, "Can't store current operator", e);
                }
            }
            return false;
        } else { // isPhoneTypeCdmaLte()
            if (DBG) logd("updateCurrentCarrierInProvider not updated X retVal=" + true);
            return true;
        }
    }

    private void handleCfuQueryResult(CallForwardInfo[] infos) {
        IccRecords r = mIccRecords.get();
        if (r != null) {
            if (infos == null || infos.length == 0) {
                // Assume the default is not active
                // Set unconditional CFF in SIM to false
                setVoiceCallForwardingFlag(1, false, null);
            } else {
                for (int i = 0, s = infos.length; i < s; i++) {
                    if ((infos[i].serviceClass & SERVICE_CLASS_VOICE) != 0) {
                        setVoiceCallForwardingFlag(1, (infos[i].status == 1),
                            infos[i].number);
                        // should only have the one
                        break;
                    }
                }
            }
        }
    }

    /**
     * Retrieves the IccPhoneBookInterfaceManager of the GsmCdmaPhone
     */
    @Override
    public IccPhoneBookInterfaceManager getIccPhoneBookInterfaceManager(){
        return mIccPhoneBookIntManager;
    }

    //CDMA
    public void registerForEriFileLoaded(Handler h, int what, Object obj) {
        Registrant r = new Registrant (h, what, obj);
        mEriFileLoadedRegistrants.add(r);
    }

    //CDMA
    public void unregisterForEriFileLoaded(Handler h) {
        mEriFileLoadedRegistrants.remove(h);
    }

    //CDMA
    public void prepareEri() {
        if (mEriManager == null) {
            Rlog.e(LOG_TAG, "PrepareEri: Trying to access stale objects");
            return;
        }
        mEriManager.loadEriFile();
        if(mEriManager.isEriFileLoaded()) {
            // when the ERI file is loaded
            logd("ERI read, notify registrants");
            mEriFileLoadedRegistrants.notifyRegistrants();
        }
    }

    //CDMA
    public boolean isEriFileLoaded() {
        return mEriManager.isEriFileLoaded();
    }


    /**
     * Activate or deactivate cell broadcast SMS.
     *
     * @param activate 0 = activate, 1 = deactivate
     * @param response Callback message is empty on completion
     */
    @Override
    public void activateCellBroadcastSms(int activate, Message response) {
        loge("[GsmCdmaPhone] activateCellBroadcastSms() is obsolete; use SmsManager");
        response.sendToTarget();
    }

    /**
     * Query the current configuration of cdma cell broadcast SMS.
     *
     * @param response Callback message is empty on completion
     */
    @Override
    public void getCellBroadcastSmsConfig(Message response) {
        loge("[GsmCdmaPhone] getCellBroadcastSmsConfig() is obsolete; use SmsManager");
        response.sendToTarget();
    }

    /**
     * Configure cdma cell broadcast SMS.
     *
     * @param response Callback message is empty on completion
     */
    @Override
    public void setCellBroadcastSmsConfig(int[] configValuesArray, Message response) {
        loge("[GsmCdmaPhone] setCellBroadcastSmsConfig() is obsolete; use SmsManager");
        response.sendToTarget();
    }

    /**
     * Returns true if OTA Service Provisioning needs to be performed.
     */
    @Override
    public boolean needsOtaServiceProvisioning() {
        if (isPhoneTypeGsm()) {
            return false;
        } else {
            return mSST.getOtasp() != ServiceStateTracker.OTASP_NOT_NEEDED;
        }
    }

    @Override
    public boolean isCspPlmnEnabled() {
        IccRecords r = mIccRecords.get();
        return (r != null) ? r.isCspPlmnEnabled() : false;
    }

    public boolean isManualNetSelAllowed() {

        int nwMode = Phone.PREFERRED_NT_MODE;
        int subId = getSubId();

        nwMode = android.provider.Settings.Global.getInt(mContext.getContentResolver(),
                    android.provider.Settings.Global.PREFERRED_NETWORK_MODE + subId, nwMode);

        logd("isManualNetSelAllowed in mode = " + nwMode);
        /*
         *  For multimode targets in global mode manual network
         *  selection is disallowed
         */
        if (isManualSelProhibitedInGlobalMode()
                && ((nwMode == Phone.NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA)
                        || (nwMode == Phone.NT_MODE_GLOBAL)) ){
            logd("Manual selection not supported in mode = " + nwMode);
            return false;
        } else {
            logd("Manual selection is supported in mode = " + nwMode);
        }

        /*
         *  Single mode phone with - GSM network modes/global mode
         *  LTE only for 3GPP
         *  LTE centric + 3GPP Legacy
         *  Note: the actual enabling/disabling manual selection for these
         *  cases will be controlled by csp
         */
        return true;
    }

    private boolean isManualSelProhibitedInGlobalMode() {
        boolean isProhibited = false;
        final String configString = getContext().getResources().getString(com.android.internal.
                R.string.prohibit_manual_network_selection_in_gobal_mode);

        if (!TextUtils.isEmpty(configString)) {
            String[] configArray = configString.split(";");

            if (configArray != null &&
                    ((configArray.length == 1 && configArray[0].equalsIgnoreCase("true")) ||
                        (configArray.length == 2 && !TextUtils.isEmpty(configArray[1]) &&
                            configArray[0].equalsIgnoreCase("true") &&
                            isMatchGid(configArray[1])))) {
                            isProhibited = true;
            }
        }
        logd("isManualNetSelAllowedInGlobal in current carrier is " + isProhibited);
        return isProhibited;
    }

    private void registerForIccRecordEvents() {
        IccRecords r = mIccRecords.get();
        if (r == null) {
            return;
        }
        if (isPhoneTypeGsm()) {
            r.registerForNetworkSelectionModeAutomatic(
                    this, EVENT_SET_NETWORK_AUTOMATIC, null);
            r.registerForRecordsEvents(this, EVENT_ICC_RECORD_EVENTS, null);
            r.registerForRecordsLoaded(this, EVENT_SIM_RECORDS_LOADED, null);
        } else {
            r.registerForRecordsLoaded(this, EVENT_RUIM_RECORDS_LOADED, null);
        }
    }

    private void unregisterForIccRecordEvents() {
        IccRecords r = mIccRecords.get();
        if (r == null) {
            return;
        }
        r.unregisterForNetworkSelectionModeAutomatic(this);
        r.unregisterForRecordsEvents(this);
        r.unregisterForRecordsLoaded(this);
    }

    @Override
    public void exitEmergencyCallbackMode() {
        if (isPhoneTypeGsm()) {
            if (mImsPhone != null) {
                mImsPhone.exitEmergencyCallbackMode();
            }
        } else {
            if (mWakeLock.isHeld()) {
                mWakeLock.release();
            }
            // Send a message which will invoke handleExitEmergencyCallbackMode
            mCi.exitEmergencyCallbackMode(obtainMessage(EVENT_EXIT_EMERGENCY_CALLBACK_RESPONSE));
        }
    }

    //CDMA
    private void handleEnterEmergencyCallbackMode(Message msg) {
        if (DBG) {
            Rlog.d(LOG_TAG, "handleEnterEmergencyCallbackMode,mIsPhoneInEcmState= "
                    + mIsPhoneInEcmState);
        }
        // if phone is not in Ecm mode, and it's changed to Ecm mode
        if (mIsPhoneInEcmState == false) {
            setSystemProperty(TelephonyProperties.PROPERTY_INECM_MODE, "true");
            mIsPhoneInEcmState = true;
            // notify change
            sendEmergencyCallbackModeChange();

            // Post this runnable so we will automatically exit
            // if no one invokes exitEmergencyCallbackMode() directly.
            long delayInMillis = SystemProperties.getLong(
                    TelephonyProperties.PROPERTY_ECM_EXIT_TIMER, DEFAULT_ECM_EXIT_TIMER_VALUE);
            postDelayed(mExitEcmRunnable, delayInMillis);
            // We don't want to go to sleep while in Ecm
            mWakeLock.acquire();
        }
    }

    //CDMA
    private void handleExitEmergencyCallbackMode(Message msg) {
        AsyncResult ar = (AsyncResult)msg.obj;
        if (DBG) {
            Rlog.d(LOG_TAG, "handleExitEmergencyCallbackMode,ar.exception , mIsPhoneInEcmState "
                    + ar.exception + mIsPhoneInEcmState);
        }
        // Remove pending exit Ecm runnable, if any
        removeCallbacks(mExitEcmRunnable);

        if (mEcmExitRespRegistrant != null) {
            mEcmExitRespRegistrant.notifyRegistrant(ar);
        }
        // if exiting ecm success
        if (ar.exception == null) {
            if (mIsPhoneInEcmState) {
                setSystemProperty(TelephonyProperties.PROPERTY_INECM_MODE, "false");
                mIsPhoneInEcmState = false;
            }

            // release wakeLock
            if (mWakeLock.isHeld()) {
                mWakeLock.release();
            }

            // send an Intent
            sendEmergencyCallbackModeChange();
            // Re-initiate data connection
            mDcTracker.setInternalDataEnabled(true);
            notifyEmergencyCallRegistrants(false);
        }
    }

    //CDMA
    public void notifyEmergencyCallRegistrants(boolean started) {
        mEmergencyCallToggledRegistrants.notifyResult(started ? 1 : 0);
    }

    //CDMA
    /**
     * Handle to cancel or restart Ecm timer in emergency call back mode
     * if action is CANCEL_ECM_TIMER, cancel Ecm timer and notify apps the timer is canceled;
     * otherwise, restart Ecm timer and notify apps the timer is restarted.
     */
    public void handleTimerInEmergencyCallbackMode(int action) {
        switch(action) {
            case CANCEL_ECM_TIMER:
                removeCallbacks(mExitEcmRunnable);
                mEcmTimerResetRegistrants.notifyResult(Boolean.TRUE);
                break;
            case RESTART_ECM_TIMER:
                long delayInMillis = SystemProperties.getLong(
                        TelephonyProperties.PROPERTY_ECM_EXIT_TIMER, DEFAULT_ECM_EXIT_TIMER_VALUE);
                postDelayed(mExitEcmRunnable, delayInMillis);
                mEcmTimerResetRegistrants.notifyResult(Boolean.FALSE);
                break;
            default:
                Rlog.e(LOG_TAG, "handleTimerInEmergencyCallbackMode, unsupported action " + action);
        }
    }

    //CDMA
    private static final String IS683A_FEATURE_CODE = "*228";
    private static final int IS683A_FEATURE_CODE_NUM_DIGITS = 4;
    private static final int IS683A_SYS_SEL_CODE_NUM_DIGITS = 2;
    private static final int IS683A_SYS_SEL_CODE_OFFSET = 4;

    private static final int IS683_CONST_800MHZ_A_BAND = 0;
    private static final int IS683_CONST_800MHZ_B_BAND = 1;
    private static final int IS683_CONST_1900MHZ_A_BLOCK = 2;
    private static final int IS683_CONST_1900MHZ_B_BLOCK = 3;
    private static final int IS683_CONST_1900MHZ_C_BLOCK = 4;
    private static final int IS683_CONST_1900MHZ_D_BLOCK = 5;
    private static final int IS683_CONST_1900MHZ_E_BLOCK = 6;
    private static final int IS683_CONST_1900MHZ_F_BLOCK = 7;
    private static final int INVALID_SYSTEM_SELECTION_CODE = -1;

    // Define the pattern/format for carrier specified OTASP number schema.
    // It separates by comma and/or whitespace.
    private static Pattern pOtaSpNumSchema = Pattern.compile("[,\\s]+");

    //CDMA
    private static boolean isIs683OtaSpDialStr(String dialStr) {
        int sysSelCodeInt;
        boolean isOtaspDialString = false;
        int dialStrLen = dialStr.length();

        if (dialStrLen == IS683A_FEATURE_CODE_NUM_DIGITS) {
            if (dialStr.equals(IS683A_FEATURE_CODE)) {
                isOtaspDialString = true;
            }
        } else {
            sysSelCodeInt = extractSelCodeFromOtaSpNum(dialStr);
            switch (sysSelCodeInt) {
                case IS683_CONST_800MHZ_A_BAND:
                case IS683_CONST_800MHZ_B_BAND:
                case IS683_CONST_1900MHZ_A_BLOCK:
                case IS683_CONST_1900MHZ_B_BLOCK:
                case IS683_CONST_1900MHZ_C_BLOCK:
                case IS683_CONST_1900MHZ_D_BLOCK:
                case IS683_CONST_1900MHZ_E_BLOCK:
                case IS683_CONST_1900MHZ_F_BLOCK:
                    isOtaspDialString = true;
                    break;
                default:
                    break;
            }
        }
        return isOtaspDialString;
    }

    //CDMA
    /**
     * This function extracts the system selection code from the dial string.
     */
    private static int extractSelCodeFromOtaSpNum(String dialStr) {
        int dialStrLen = dialStr.length();
        int sysSelCodeInt = INVALID_SYSTEM_SELECTION_CODE;

        if ((dialStr.regionMatches(0, IS683A_FEATURE_CODE,
                0, IS683A_FEATURE_CODE_NUM_DIGITS)) &&
                (dialStrLen >= (IS683A_FEATURE_CODE_NUM_DIGITS +
                        IS683A_SYS_SEL_CODE_NUM_DIGITS))) {
            // Since we checked the condition above, the system selection code
            // extracted from dialStr will not cause any exception
            sysSelCodeInt = Integer.parseInt (
                    dialStr.substring (IS683A_FEATURE_CODE_NUM_DIGITS,
                            IS683A_FEATURE_CODE_NUM_DIGITS + IS683A_SYS_SEL_CODE_NUM_DIGITS));
        }
        if (DBG) Rlog.d(LOG_TAG, "extractSelCodeFromOtaSpNum " + sysSelCodeInt);
        return sysSelCodeInt;
    }

    //CDMA
    /**
     * This function checks if the system selection code extracted from
     * the dial string "sysSelCodeInt' is the system selection code specified
     * in the carrier ota sp number schema "sch".
     */
    private static boolean checkOtaSpNumBasedOnSysSelCode(int sysSelCodeInt, String sch[]) {
        boolean isOtaSpNum = false;
        try {
            // Get how many number of system selection code ranges
            int selRc = Integer.parseInt(sch[1]);
            for (int i = 0; i < selRc; i++) {
                if (!TextUtils.isEmpty(sch[i+2]) && !TextUtils.isEmpty(sch[i+3])) {
                    int selMin = Integer.parseInt(sch[i+2]);
                    int selMax = Integer.parseInt(sch[i+3]);
                    // Check if the selection code extracted from the dial string falls
                    // within any of the range pairs specified in the schema.
                    if ((sysSelCodeInt >= selMin) && (sysSelCodeInt <= selMax)) {
                        isOtaSpNum = true;
                        break;
                    }
                }
            }
        } catch (NumberFormatException ex) {
            // If the carrier ota sp number schema is not correct, we still allow dial
            // and only log the error:
            Rlog.e(LOG_TAG, "checkOtaSpNumBasedOnSysSelCode, error", ex);
        }
        return isOtaSpNum;
    }

    //CDMA
    /**
     * The following function checks if a dial string is a carrier specified
     * OTASP number or not by checking against the OTASP number schema stored
     * in PROPERTY_OTASP_NUM_SCHEMA.
     *
     * Currently, there are 2 schemas for carriers to specify the OTASP number:
     * 1) Use system selection code:
     *    The schema is:
     *    SELC,the # of code pairs,min1,max1,min2,max2,...
     *    e.g "SELC,3,10,20,30,40,60,70" indicates that there are 3 pairs of
     *    selection codes, and they are {10,20}, {30,40} and {60,70} respectively.
     *
     * 2) Use feature code:
     *    The schema is:
     *    "FC,length of feature code,feature code".
     *     e.g "FC,2,*2" indicates that the length of the feature code is 2,
     *     and the code itself is "*2".
     */
    private boolean isCarrierOtaSpNum(String dialStr) {
        boolean isOtaSpNum = false;
        int sysSelCodeInt = extractSelCodeFromOtaSpNum(dialStr);
        if (sysSelCodeInt == INVALID_SYSTEM_SELECTION_CODE) {
            return isOtaSpNum;
        }
        // mCarrierOtaSpNumSchema is retrieved from PROPERTY_OTASP_NUM_SCHEMA:
        if (!TextUtils.isEmpty(mCarrierOtaSpNumSchema)) {
            Matcher m = pOtaSpNumSchema.matcher(mCarrierOtaSpNumSchema);
            if (DBG) {
                Rlog.d(LOG_TAG, "isCarrierOtaSpNum,schema" + mCarrierOtaSpNumSchema);
            }

            if (m.find()) {
                String sch[] = pOtaSpNumSchema.split(mCarrierOtaSpNumSchema);
                // If carrier uses system selection code mechanism
                if (!TextUtils.isEmpty(sch[0]) && sch[0].equals("SELC")) {
                    if (sysSelCodeInt!=INVALID_SYSTEM_SELECTION_CODE) {
                        isOtaSpNum=checkOtaSpNumBasedOnSysSelCode(sysSelCodeInt,sch);
                    } else {
                        if (DBG) {
                            Rlog.d(LOG_TAG, "isCarrierOtaSpNum,sysSelCodeInt is invalid");
                        }
                    }
                } else if (!TextUtils.isEmpty(sch[0]) && sch[0].equals("FC")) {
                    int fcLen =  Integer.parseInt(sch[1]);
                    String fc = sch[2];
                    if (dialStr.regionMatches(0,fc,0,fcLen)) {
                        isOtaSpNum = true;
                    } else {
                        if (DBG) Rlog.d(LOG_TAG, "isCarrierOtaSpNum,not otasp number");
                    }
                } else {
                    if (DBG) {
                        Rlog.d(LOG_TAG, "isCarrierOtaSpNum,ota schema not supported" + sch[0]);
                    }
                }
            } else {
                if (DBG) {
                    Rlog.d(LOG_TAG, "isCarrierOtaSpNum,ota schema pattern not right" +
                            mCarrierOtaSpNumSchema);
                }
            }
        } else {
            if (DBG) Rlog.d(LOG_TAG, "isCarrierOtaSpNum,ota schema pattern empty");
        }
        return isOtaSpNum;
    }

    /**
     * isOTASPNumber: checks a given number against the IS-683A OTASP dial string and carrier
     * OTASP dial string.
     *
     * @param dialStr the number to look up.
     * @return true if the number is in IS-683A OTASP dial string or carrier OTASP dial string
     */
    @Override
    public  boolean isOtaSpNumber(String dialStr) {
        if (isPhoneTypeGsm()) {
            return super.isOtaSpNumber(dialStr);
        } else {
            boolean isOtaSpNum = false;
            String dialableStr = PhoneNumberUtils.extractNetworkPortionAlt(dialStr);
            if (dialableStr != null) {
                isOtaSpNum = isIs683OtaSpDialStr(dialableStr);
                if (isOtaSpNum == false) {
                    isOtaSpNum = isCarrierOtaSpNum(dialableStr);
                }
            }
            if (DBG) Rlog.d(LOG_TAG, "isOtaSpNumber " + isOtaSpNum);
            return isOtaSpNum;
        }
    }

    @Override
    public int getCdmaEriIconIndex() {
        if (isPhoneTypeGsm()) {
            return super.getCdmaEriIconIndex();
        } else {
            return getServiceState().getCdmaEriIconIndex();
        }
    }

    /**
     * Returns the CDMA ERI icon mode,
     * 0 - ON
     * 1 - FLASHING
     */
    @Override
    public int getCdmaEriIconMode() {
        if (isPhoneTypeGsm()) {
            return super.getCdmaEriIconMode();
        } else {
            return getServiceState().getCdmaEriIconMode();
        }
    }

    /**
     * Returns the CDMA ERI text,
     */
    @Override
    public String getCdmaEriText() {
        if (isPhoneTypeGsm()) {
            return super.getCdmaEriText();
        } else {
            int roamInd = getServiceState().getCdmaRoamingIndicator();
            int defRoamInd = getServiceState().getCdmaDefaultRoamingIndicator();
            return mEriManager.getCdmaEriText(roamInd, defRoamInd);
        }
    }

    private void phoneObjectUpdater(int newVoiceRadioTech) {
        logd("phoneObjectUpdater: newVoiceRadioTech=" + newVoiceRadioTech);

        // Check for a voice over lte replacement
        if (ServiceState.isLte(newVoiceRadioTech)
                || (newVoiceRadioTech == ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN)) {
            CarrierConfigManager configMgr = (CarrierConfigManager)
                    getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
            PersistableBundle b = configMgr.getConfigForSubId(getSubId());
            if (b != null) {
                int volteReplacementRat =
                        b.getInt(CarrierConfigManager.KEY_VOLTE_REPLACEMENT_RAT_INT);
                logd("phoneObjectUpdater: volteReplacementRat=" + volteReplacementRat);
                if (volteReplacementRat != ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN) {
                    newVoiceRadioTech = volteReplacementRat;
                }
            } else {
                loge("phoneObjectUpdater: didn't get volteReplacementRat from carrier config");
            }
        }

        if(mRilVersion == 6 && getLteOnCdmaMode() == PhoneConstants.LTE_ON_CDMA_TRUE) {
            /*
             * On v6 RIL, when LTE_ON_CDMA is TRUE, always create CDMALTEPhone
             * irrespective of the voice radio tech reported.
             */
            if (getPhoneType() == PhoneConstants.PHONE_TYPE_CDMA) {
                logd("phoneObjectUpdater: LTE ON CDMA property is set. Use CDMA Phone" +
                        " newVoiceRadioTech=" + newVoiceRadioTech +
                        " mActivePhone=" + getPhoneName());
                return;
            } else {
                logd("phoneObjectUpdater: LTE ON CDMA property is set. Switch to CDMALTEPhone" +
                        " newVoiceRadioTech=" + newVoiceRadioTech +
                        " mActivePhone=" + getPhoneName());
                newVoiceRadioTech = ServiceState.RIL_RADIO_TECHNOLOGY_1xRTT;
            }
        } else {

            // If the device is shutting down, then there is no need to switch to the new phone
            // which might send unnecessary attach request to the modem.
            if (isShuttingDown()) {
                logd("Device is shutting down. No need to switch phone now.");
                return;
            }

            boolean matchCdma = ServiceState.isCdma(newVoiceRadioTech);
            boolean matchGsm = ServiceState.isGsm(newVoiceRadioTech);
            if ((matchCdma && getPhoneType() == PhoneConstants.PHONE_TYPE_CDMA) ||
                    (matchGsm && getPhoneType() == PhoneConstants.PHONE_TYPE_GSM)) {
                // Nothing changed. Keep phone as it is.
                logd("phoneObjectUpdater: No change ignore," +
                        " newVoiceRadioTech=" + newVoiceRadioTech +
                        " mActivePhone=" + getPhoneName());
                return;
            }
            if (!matchCdma && !matchGsm) {
                loge("phoneObjectUpdater: newVoiceRadioTech=" + newVoiceRadioTech +
                        " doesn't match either CDMA or GSM - error! No phone change");
                return;
            }
        }

        if (newVoiceRadioTech == ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN) {
            // We need some voice phone object to be active always, so never
            // delete the phone without anything to replace it with!
            logd("phoneObjectUpdater: Unknown rat ignore, "
                    + " newVoiceRadioTech=Unknown. mActivePhone=" + getPhoneName());
            return;
        }

        boolean oldPowerState = false; // old power state to off
        if (mResetModemOnRadioTechnologyChange) {
            if (mCi.getRadioState().isOn()) {
                oldPowerState = true;
                logd("phoneObjectUpdater: Setting Radio Power to Off");
                mCi.setRadioPower(false, null);
            }
        }

        switchVoiceRadioTech(newVoiceRadioTech);

        if (mResetModemOnRadioTechnologyChange && oldPowerState) { // restore power state
            logd("phoneObjectUpdater: Resetting Radio");
            mCi.setRadioPower(oldPowerState, null);
        }

        // update voice radio tech in icc card proxy
        mIccCardProxy.setVoiceRadioTech(newVoiceRadioTech);

        // Send an Intent to the PhoneApp that we had a radio technology change
        Intent intent = new Intent(TelephonyIntents.ACTION_RADIO_TECHNOLOGY_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intent.putExtra(PhoneConstants.PHONE_NAME_KEY, getPhoneName());
        SubscriptionManager.putPhoneIdAndSubIdExtra(intent, mPhoneId);
        ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);
    }

    private void switchVoiceRadioTech(int newVoiceRadioTech) {

        String outgoingPhoneName = getPhoneName();

        logd("Switching Voice Phone : " + outgoingPhoneName + " >>> "
                + (ServiceState.isGsm(newVoiceRadioTech) ? "GSM" : "CDMA"));

        if (ServiceState.isCdma(newVoiceRadioTech)) {
            switchPhoneType(PhoneConstants.PHONE_TYPE_CDMA_LTE);
        } else if (ServiceState.isGsm(newVoiceRadioTech)) {
            switchPhoneType(PhoneConstants.PHONE_TYPE_GSM);
        } else {
            loge("deleteAndCreatePhone: newVoiceRadioTech=" + newVoiceRadioTech +
                    " is not CDMA or GSM (error) - aborting!");
            return;
        }
    }

    @Override
    public IccSmsInterfaceManager getIccSmsInterfaceManager(){
        return mIccSmsInterfaceManager;
    }

    @Override
    public void updatePhoneObject(int voiceRadioTech) {
        logd("updatePhoneObject: radioTechnology=" + voiceRadioTech);
        sendMessage(obtainMessage(EVENT_UPDATE_PHONE_OBJECT, voiceRadioTech, 0, null));
    }

    @Override
    public void setImsRegistrationState(boolean registered) {
        mSST.setImsRegistrationState(registered);
    }

    @Override
    public boolean getIccRecordsLoaded() {
        return mIccCardProxy.getIccRecordsLoaded();
    }

    @Override
    public IccCard getIccCard() {
        return mIccCardProxy;
    }

    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("GsmCdmaPhone extends:");
        super.dump(fd, pw, args);
        pw.println(" mPrecisePhoneType=" + mPrecisePhoneType);
        pw.println(" mCT=" + mCT);
        pw.println(" mSST=" + mSST);
        pw.println(" mPendingMMIs=" + mPendingMMIs);
        pw.println(" mIccPhoneBookIntManager=" + mIccPhoneBookIntManager);
        if (VDBG) pw.println(" mImei=" + mImei);
        if (VDBG) pw.println(" mImeiSv=" + mImeiSv);
        if (VDBG) pw.println(" mVmNumber=" + mVmNumber);
        pw.println(" mCdmaSSM=" + mCdmaSSM);
        pw.println(" mCdmaSubscriptionSource=" + mCdmaSubscriptionSource);
        pw.println(" mEriManager=" + mEriManager);
        pw.println(" mWakeLock=" + mWakeLock);
        pw.println(" mIsPhoneInEcmState=" + mIsPhoneInEcmState);
        if (VDBG) pw.println(" mEsn=" + mEsn);
        if (VDBG) pw.println(" mMeid=" + mMeid);
        pw.println(" mCarrierOtaSpNumSchema=" + mCarrierOtaSpNumSchema);
        if (!isPhoneTypeGsm()) {
            pw.println(" getCdmaEriIconIndex()=" + getCdmaEriIconIndex());
            pw.println(" getCdmaEriIconMode()=" + getCdmaEriIconMode());
            pw.println(" getCdmaEriText()=" + getCdmaEriText());
            pw.println(" isMinInfoReady()=" + isMinInfoReady());
        }
        pw.println(" isCspPlmnEnabled()=" + isCspPlmnEnabled());
        pw.flush();
        pw.println("++++++++++++++++++++++++++++++++");

        try {
            mIccCardProxy.dump(fd, pw, args);
        } catch (Exception e) {
            e.printStackTrace();
        }
        pw.flush();
        pw.println("++++++++++++++++++++++++++++++++");
    }

    @Override
    public boolean setOperatorBrandOverride(String brand) {
        if (mUiccController == null) {
            return false;
        }

        UiccCard card = mUiccController.getUiccCard(getPhoneId());
        if (card == null) {
            return false;
        }

        boolean status = card.setOperatorBrandOverride(brand);

        // Refresh.
        if (status) {
            IccRecords iccRecords = mIccRecords.get();
            if (iccRecords != null) {
                TelephonyManager.from(mContext).setSimOperatorNameForPhone(
                        getPhoneId(), iccRecords.getServiceProviderName());
            }
            if (mSST != null) {
                mSST.pollState();
            }
        }
        return status;
    }

    /**
     * @return operator numeric.
     */
    private String getOperatorNumeric() {
        String operatorNumeric = null;
        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();
            if (r != null) {
                operatorNumeric = r.getOperatorNumeric();
            }
        } else { //isPhoneTypeCdmaLte()
            IccRecords curIccRecords = null;
            if (mCdmaSubscriptionSource == CDMA_SUBSCRIPTION_NV) {
                operatorNumeric = SystemProperties.get("ro.cdma.home.operator.numeric");
            } else if (mCdmaSubscriptionSource == CDMA_SUBSCRIPTION_RUIM_SIM) {
                curIccRecords = mSimRecords;
                if (curIccRecords != null) {
                    operatorNumeric = curIccRecords.getOperatorNumeric();
                } else {
                    curIccRecords = mIccRecords.get();
                    if (curIccRecords != null && (curIccRecords instanceof RuimRecords)) {
                        RuimRecords csim = (RuimRecords) curIccRecords;
                        operatorNumeric = csim.getRUIMOperatorNumeric();
                    }
                }
            }
            if (operatorNumeric == null) {
                loge("getOperatorNumeric: Cannot retrieve operatorNumeric:"
                        + " mCdmaSubscriptionSource = " + mCdmaSubscriptionSource +
                        " mIccRecords = " + ((curIccRecords != null) ?
                        curIccRecords.getRecordsLoaded() : null));
            }

            logd("getOperatorNumeric: mCdmaSubscriptionSource = " + mCdmaSubscriptionSource
                    + " operatorNumeric = " + operatorNumeric);

        }
        return operatorNumeric;
    }

    public void notifyEcbmTimerReset(Boolean flag) {
        mEcmTimerResetRegistrants.notifyResult(flag);
    }

    /**
     * Registration point for Ecm timer reset
     *
     * @param h handler to notify
     * @param what User-defined message code
     * @param obj placed in Message.obj
     */
    @Override
    public void registerForEcmTimerReset(Handler h, int what, Object obj) {
        mEcmTimerResetRegistrants.addUnique(h, what, obj);
    }

    @Override
    public void unregisterForEcmTimerReset(Handler h) {
        mEcmTimerResetRegistrants.remove(h);
    }

    /**
     * Sets the SIM voice message waiting indicator records.
     * @param line GSM Subscriber Profile Number, one-based. Only '1' is supported
     * @param countWaiting The number of messages waiting, if known. Use
     *                     -1 to indicate that an unknown number of
     *                      messages are waiting
     */
    @Override
    public void setVoiceMessageWaiting(int line, int countWaiting) {
        if (isPhoneTypeGsm()) {
            IccRecords r = mIccRecords.get();
            if (r != null) {
                r.setVoiceMessageWaiting(line, countWaiting);
            } else {
                logd("SIM Records not found, MWI not updated");
            }
        } else {
            setVoiceMessageCount(countWaiting);
        }
    }

    private void logd(String s) {
        Rlog.d(LOG_TAG, "[GsmCdmaPhone] " + s);
    }

    private void loge(String s) {
        Rlog.e(LOG_TAG, "[GsmCdmaPhone] " + s);
    }

    @Override
    public boolean isUtEnabled() {
        Phone imsPhone = mImsPhone;
        if (imsPhone != null) {
            return imsPhone.isUtEnabled();
        } else {
            logd("isUtEnabled: called for GsmCdma");
            return false;
        }
    }

    public String getDtmfToneDelayKey() {
        return isPhoneTypeGsm() ?
                CarrierConfigManager.KEY_GSM_DTMF_TONE_DELAY_INT :
                CarrierConfigManager.KEY_CDMA_DTMF_TONE_DELAY_INT;
    }

    @VisibleForTesting
    public PowerManager.WakeLock getWakeLock() {
        return mWakeLock;
    }

}
