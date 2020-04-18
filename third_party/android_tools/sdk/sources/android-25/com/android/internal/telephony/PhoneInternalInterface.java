/*
 * Copyright (C) 2007 The Android Open Source Project
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
import android.net.LinkProperties;
import android.net.NetworkCapabilities;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.telephony.CellInfo;
import android.telephony.CellLocation;
import android.telephony.CarrierConfigManager;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;

import com.android.internal.telephony.imsphone.ImsPhone;
import com.android.internal.telephony.RadioCapability;
import com.android.internal.telephony.test.SimulatedRadioControl;
import com.android.internal.telephony.uicc.IsimRecords;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.internal.telephony.uicc.UsimServiceTable;

import com.android.internal.telephony.PhoneConstants.*; // ????

import java.util.List;
import java.util.Locale;

/**
 * Internal interface used to control the phone; SDK developers cannot
 * obtain this interface.
 *
 * {@hide}
 *
 */
public interface PhoneInternalInterface {

    /** used to enable additional debug messages */
    static final boolean DEBUG_PHONE = true;

    public enum DataActivityState {
        /**
         * The state of a data activity.
         * <ul>
         * <li>NONE = No traffic</li>
         * <li>DATAIN = Receiving IP ppp traffic</li>
         * <li>DATAOUT = Sending IP ppp traffic</li>
         * <li>DATAINANDOUT = Both receiving and sending IP ppp traffic</li>
         * <li>DORMANT = The data connection is still active,
                                     but physical link is down</li>
         * </ul>
         */
        NONE, DATAIN, DATAOUT, DATAINANDOUT, DORMANT;
    }

    enum SuppService {
      UNKNOWN, SWITCH, SEPARATE, TRANSFER, CONFERENCE, REJECT, HANGUP, RESUME, HOLD;
    }

    // "Features" accessible through the connectivity manager
    static final String FEATURE_ENABLE_MMS = "enableMMS";
    static final String FEATURE_ENABLE_SUPL = "enableSUPL";
    static final String FEATURE_ENABLE_DUN = "enableDUN";
    static final String FEATURE_ENABLE_HIPRI = "enableHIPRI";
    static final String FEATURE_ENABLE_DUN_ALWAYS = "enableDUNAlways";
    static final String FEATURE_ENABLE_FOTA = "enableFOTA";
    static final String FEATURE_ENABLE_IMS = "enableIMS";
    static final String FEATURE_ENABLE_CBS = "enableCBS";
    static final String FEATURE_ENABLE_EMERGENCY = "enableEmergency";

    /**
     * Optional reasons for disconnect and connect
     */
    static final String REASON_ROAMING_ON = "roamingOn";
    static final String REASON_ROAMING_OFF = "roamingOff";
    static final String REASON_DATA_DISABLED = "dataDisabled";
    static final String REASON_DATA_ENABLED = "dataEnabled";
    static final String REASON_DATA_ATTACHED = "dataAttached";
    static final String REASON_DATA_DETACHED = "dataDetached";
    static final String REASON_CDMA_DATA_ATTACHED = "cdmaDataAttached";
    static final String REASON_CDMA_DATA_DETACHED = "cdmaDataDetached";
    static final String REASON_APN_CHANGED = "apnChanged";
    static final String REASON_APN_SWITCHED = "apnSwitched";
    static final String REASON_APN_FAILED = "apnFailed";
    static final String REASON_RESTORE_DEFAULT_APN = "restoreDefaultApn";
    static final String REASON_RADIO_TURNED_OFF = "radioTurnedOff";
    static final String REASON_PDP_RESET = "pdpReset";
    static final String REASON_VOICE_CALL_ENDED = "2GVoiceCallEnded";
    static final String REASON_VOICE_CALL_STARTED = "2GVoiceCallStarted";
    static final String REASON_PS_RESTRICT_ENABLED = "psRestrictEnabled";
    static final String REASON_PS_RESTRICT_DISABLED = "psRestrictDisabled";
    static final String REASON_SIM_LOADED = "simLoaded";
    static final String REASON_NW_TYPE_CHANGED = "nwTypeChanged";
    static final String REASON_DATA_DEPENDENCY_MET = "dependencyMet";
    static final String REASON_DATA_DEPENDENCY_UNMET = "dependencyUnmet";
    static final String REASON_LOST_DATA_CONNECTION = "lostDataConnection";
    static final String REASON_CONNECTED = "connected";
    static final String REASON_SINGLE_PDN_ARBITRATION = "SinglePdnArbitration";
    static final String REASON_DATA_SPECIFIC_DISABLED = "specificDisabled";
    static final String REASON_SIM_NOT_READY = "simNotReady";
    static final String REASON_IWLAN_AVAILABLE = "iwlanAvailable";
    static final String REASON_CARRIER_CHANGE = "carrierChange";
    static final String REASON_CARRIER_ACTION_DISABLE_METERED_APN =
            "carrierActionDisableMeteredApn";

    // Used for band mode selection methods
    static final int BM_UNSPECIFIED = RILConstants.BAND_MODE_UNSPECIFIED; // automatic
    static final int BM_EURO_BAND   = RILConstants.BAND_MODE_EURO;
    static final int BM_US_BAND     = RILConstants.BAND_MODE_USA;
    static final int BM_JPN_BAND    = RILConstants.BAND_MODE_JPN;
    static final int BM_AUS_BAND    = RILConstants.BAND_MODE_AUS;
    static final int BM_AUS2_BAND   = RILConstants.BAND_MODE_AUS_2;
    static final int BM_CELL_800    = RILConstants.BAND_MODE_CELL_800;
    static final int BM_PCS         = RILConstants.BAND_MODE_PCS;
    static final int BM_JTACS       = RILConstants.BAND_MODE_JTACS;
    static final int BM_KOREA_PCS   = RILConstants.BAND_MODE_KOREA_PCS;
    static final int BM_4_450M      = RILConstants.BAND_MODE_5_450M;
    static final int BM_IMT2000     = RILConstants.BAND_MODE_IMT2000;
    static final int BM_7_700M2     = RILConstants.BAND_MODE_7_700M_2;
    static final int BM_8_1800M     = RILConstants.BAND_MODE_8_1800M;
    static final int BM_9_900M      = RILConstants.BAND_MODE_9_900M;
    static final int BM_10_800M_2   = RILConstants.BAND_MODE_10_800M_2;
    static final int BM_EURO_PAMR   = RILConstants.BAND_MODE_EURO_PAMR_400M;
    static final int BM_AWS         = RILConstants.BAND_MODE_AWS;
    static final int BM_US_2500M    = RILConstants.BAND_MODE_USA_2500M;
    static final int BM_NUM_BAND_MODES = 19; //Total number of band modes

    // Used for preferred network type
    // Note NT_* substitute RILConstants.NETWORK_MODE_* above the Phone
    int NT_MODE_WCDMA_PREF   = RILConstants.NETWORK_MODE_WCDMA_PREF;
    int NT_MODE_GSM_ONLY     = RILConstants.NETWORK_MODE_GSM_ONLY;
    int NT_MODE_WCDMA_ONLY   = RILConstants.NETWORK_MODE_WCDMA_ONLY;
    int NT_MODE_GSM_UMTS     = RILConstants.NETWORK_MODE_GSM_UMTS;

    int NT_MODE_CDMA         = RILConstants.NETWORK_MODE_CDMA;

    int NT_MODE_CDMA_NO_EVDO = RILConstants.NETWORK_MODE_CDMA_NO_EVDO;
    int NT_MODE_EVDO_NO_CDMA = RILConstants.NETWORK_MODE_EVDO_NO_CDMA;
    int NT_MODE_GLOBAL       = RILConstants.NETWORK_MODE_GLOBAL;

    int NT_MODE_LTE_CDMA_AND_EVDO        = RILConstants.NETWORK_MODE_LTE_CDMA_EVDO;
    int NT_MODE_LTE_GSM_WCDMA            = RILConstants.NETWORK_MODE_LTE_GSM_WCDMA;
    int NT_MODE_LTE_CDMA_EVDO_GSM_WCDMA  = RILConstants.NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA;
    int NT_MODE_LTE_ONLY                 = RILConstants.NETWORK_MODE_LTE_ONLY;
    int NT_MODE_LTE_WCDMA                = RILConstants.NETWORK_MODE_LTE_WCDMA;

    int NT_MODE_TDSCDMA_ONLY            = RILConstants.NETWORK_MODE_TDSCDMA_ONLY;
    int NT_MODE_TDSCDMA_WCDMA           = RILConstants.NETWORK_MODE_TDSCDMA_WCDMA;
    int NT_MODE_LTE_TDSCDMA             = RILConstants.NETWORK_MODE_LTE_TDSCDMA;
    int NT_MODE_TDSCDMA_GSM             = RILConstants.NETWORK_MODE_TDSCDMA_GSM;
    int NT_MODE_LTE_TDSCDMA_GSM         = RILConstants.NETWORK_MODE_LTE_TDSCDMA_GSM;
    int NT_MODE_TDSCDMA_GSM_WCDMA       = RILConstants.NETWORK_MODE_TDSCDMA_GSM_WCDMA;
    int NT_MODE_LTE_TDSCDMA_WCDMA       = RILConstants.NETWORK_MODE_LTE_TDSCDMA_WCDMA;
    int NT_MODE_LTE_TDSCDMA_GSM_WCDMA   = RILConstants.NETWORK_MODE_LTE_TDSCDMA_GSM_WCDMA;
    int NT_MODE_TDSCDMA_CDMA_EVDO_GSM_WCDMA = RILConstants.NETWORK_MODE_TDSCDMA_CDMA_EVDO_GSM_WCDMA;
    int NT_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA = RILConstants.NETWORK_MODE_LTE_TDSCDMA_CDMA_EVDO_GSM_WCDMA;

    int PREFERRED_NT_MODE                = RILConstants.PREFERRED_NETWORK_MODE;

    // Used for CDMA roaming mode
    // Home Networks only, as defined in PRL
    static final int CDMA_RM_HOME        = CarrierConfigManager.CDMA_ROAMING_MODE_HOME;
    // Roaming an Affiliated networks, as defined in PRL
    static final int CDMA_RM_AFFILIATED  = CarrierConfigManager.CDMA_ROAMING_MODE_AFFILIATED;
    // Roaming on Any Network, as defined in PRL
    static final int CDMA_RM_ANY         = CarrierConfigManager.CDMA_ROAMING_MODE_ANY;

    // Used for CDMA subscription mode
    static final int CDMA_SUBSCRIPTION_UNKNOWN  =-1; // Unknown
    static final int CDMA_SUBSCRIPTION_RUIM_SIM = 0; // RUIM/SIM (default)
    static final int CDMA_SUBSCRIPTION_NV       = 1; // NV -> non-volatile memory

    static final int PREFERRED_CDMA_SUBSCRIPTION = CDMA_SUBSCRIPTION_NV;

    static final int TTY_MODE_OFF = 0;
    static final int TTY_MODE_FULL = 1;
    static final int TTY_MODE_HCO = 2;
    static final int TTY_MODE_VCO = 3;

     /**
     * CDMA OTA PROVISION STATUS, the same as RIL_CDMA_OTA_Status in ril.h
     */

    public static final int CDMA_OTA_PROVISION_STATUS_SPL_UNLOCKED = 0;
    public static final int CDMA_OTA_PROVISION_STATUS_SPC_RETRIES_EXCEEDED = 1;
    public static final int CDMA_OTA_PROVISION_STATUS_A_KEY_EXCHANGED = 2;
    public static final int CDMA_OTA_PROVISION_STATUS_SSD_UPDATED = 3;
    public static final int CDMA_OTA_PROVISION_STATUS_NAM_DOWNLOADED = 4;
    public static final int CDMA_OTA_PROVISION_STATUS_MDN_DOWNLOADED = 5;
    public static final int CDMA_OTA_PROVISION_STATUS_IMSI_DOWNLOADED = 6;
    public static final int CDMA_OTA_PROVISION_STATUS_PRL_DOWNLOADED = 7;
    public static final int CDMA_OTA_PROVISION_STATUS_COMMITTED = 8;
    public static final int CDMA_OTA_PROVISION_STATUS_OTAPA_STARTED = 9;
    public static final int CDMA_OTA_PROVISION_STATUS_OTAPA_STOPPED = 10;
    public static final int CDMA_OTA_PROVISION_STATUS_OTAPA_ABORTED = 11;


    /**
     * Get the current ServiceState. Use
     * <code>registerForServiceStateChanged</code> to be informed of
     * updates.
     */
    ServiceState getServiceState();

    /**
     * Get the current CellLocation.
     */
    CellLocation getCellLocation();

    /**
     * Get the current DataState. No change notification exists at this
     * interface -- use
     * {@link android.telephony.PhoneStateListener} instead.
     * @param apnType specify for which apn to get connection state info.
     */
    DataState getDataConnectionState(String apnType);

    /**
     * Get the current DataActivityState. No change notification exists at this
     * interface -- use
     * {@link android.telephony.TelephonyManager} instead.
     */
    DataActivityState getDataActivityState();

    /**
     * Returns a list of MMI codes that are pending. (They have initiated
     * but have not yet completed).
     * Presently there is only ever one.
     * Use <code>registerForMmiInitiate</code>
     * and <code>registerForMmiComplete</code> for change notification.
     */
    public List<? extends MmiCode> getPendingMmiCodes();

    /**
     * Sends user response to a USSD REQUEST message.  An MmiCode instance
     * representing this response is sent to handlers registered with
     * registerForMmiInitiate.
     *
     * @param ussdMessge    Message to send in the response.
     */
    public void sendUssdResponse(String ussdMessge);

    /**
     * Register for Supplementary Service notifications from the network.
     * Message.obj will contain an AsyncResult.
     * AsyncResult.result will be a SuppServiceNotification instance.
     *
     * @param h Handler that receives the notification message.
     * @param what User-defined message code.
     * @param obj User object.
     */
    void registerForSuppServiceNotification(Handler h, int what, Object obj);

    /**
     * Unregisters for Supplementary Service notifications.
     * Extraneous calls are tolerated silently
     *
     * @param h Handler to be removed from the registrant list.
     */
    void unregisterForSuppServiceNotification(Handler h);

    /**
     * Answers a ringing or waiting call. Active calls, if any, go on hold.
     * Answering occurs asynchronously, and final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @param videoState The video state in which to answer the call.
     * @exception CallStateException when no call is ringing or waiting
     */
    void acceptCall(int videoState) throws CallStateException;

    /**
     * Reject (ignore) a ringing call. In GSM, this means UDUB
     * (User Determined User Busy). Reject occurs asynchronously,
     * and final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @exception CallStateException when no call is ringing or waiting
     */
    void rejectCall() throws CallStateException;

    /**
     * Places any active calls on hold, and makes any held calls
     *  active. Switch occurs asynchronously and may fail.
     * Final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @exception CallStateException if a call is ringing, waiting, or
     * dialing/alerting. In these cases, this operation may not be performed.
     */
    void switchHoldingAndActive() throws CallStateException;

    /**
     * Whether or not the phone can conference in the current phone
     * state--that is, one call holding and one call active.
     * @return true if the phone can conference; false otherwise.
     */
    boolean canConference();

    /**
     * Conferences holding and active. Conference occurs asynchronously
     * and may fail. Final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @exception CallStateException if canConference() would return false.
     * In these cases, this operation may not be performed.
     */
    void conference() throws CallStateException;

    /**
     * Whether or not the phone can do explicit call transfer in the current
     * phone state--that is, one call holding and one call active.
     * @return true if the phone can do explicit call transfer; false otherwise.
     */
    boolean canTransfer();

    /**
     * Connects the two calls and disconnects the subscriber from both calls
     * Explicit Call Transfer occurs asynchronously
     * and may fail. Final notification occurs via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     *
     * @exception CallStateException if canTransfer() would return false.
     * In these cases, this operation may not be performed.
     */
    void explicitCallTransfer() throws CallStateException;

    /**
     * Clears all DISCONNECTED connections from Call connection lists.
     * Calls that were in the DISCONNECTED state become idle. This occurs
     * synchronously.
     */
    void clearDisconnected();

    /**
     * Gets the foreground call object, which represents all connections that
     * are dialing or active (all connections
     * that have their audio path connected).<p>
     *
     * The foreground call is a singleton object. It is constant for the life
     * of this phone. It is never null.<p>
     *
     * The foreground call will only ever be in one of these states:
     * IDLE, ACTIVE, DIALING, ALERTING, or DISCONNECTED.
     *
     * State change notification is available via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     */
    Call getForegroundCall();

    /**
     * Gets the background call object, which represents all connections that
     * are holding (all connections that have been accepted or connected, but
     * do not have their audio path connected). <p>
     *
     * The background call is a singleton object. It is constant for the life
     * of this phone object . It is never null.<p>
     *
     * The background call will only ever be in one of these states:
     * IDLE, HOLDING or DISCONNECTED.
     *
     * State change notification is available via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     */
    Call getBackgroundCall();

    /**
     * Gets the ringing call object, which represents an incoming
     * connection (if present) that is pending answer/accept. (This connection
     * may be RINGING or WAITING, and there may be only one.)<p>

     * The ringing call is a singleton object. It is constant for the life
     * of this phone. It is never null.<p>
     *
     * The ringing call will only ever be in one of these states:
     * IDLE, INCOMING, WAITING or DISCONNECTED.
     *
     * State change notification is available via
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}.
     */
    Call getRingingCall();

    /**
     * Initiate a new voice connection. This happens asynchronously, so you
     * cannot assume the audio path is connected (or a call index has been
     * assigned) until PhoneStateChanged notification has occurred.
     *
     * @param dialString The dial string.
     * @param videoState The desired video state for the connection.
     * @exception CallStateException if a new outgoing call is not currently
     * possible because no more call slots exist or a call exists that is
     * dialing, alerting, ringing, or waiting.  Other errors are
     * handled asynchronously.
     */
    Connection dial(String dialString, int videoState) throws CallStateException;

    /**
     * Initiate a new voice connection with supplementary User to User
     * Information. This happens asynchronously, so you cannot assume the audio
     * path is connected (or a call index has been assigned) until
     * PhoneStateChanged notification has occurred.
     *
     * NOTE: If adding another parameter, consider creating a DialArgs parameter instead to
     * encapsulate all dial arguments and decrease scaffolding headache.
     *
     * @param dialString The dial string.
     * @param uusInfo The UUSInfo.
     * @param videoState The desired video state for the connection.
     * @param intentExtras The extras from the original CALL intent.
     * @exception CallStateException if a new outgoing call is not currently
     *                possible because no more call slots exist or a call exists
     *                that is dialing, alerting, ringing, or waiting. Other
     *                errors are handled asynchronously.
     */
    Connection dial(String dialString, UUSInfo uusInfo, int videoState, Bundle intentExtras)
            throws CallStateException;

    /**
     * Handles PIN MMI commands (PIN/PIN2/PUK/PUK2), which are initiated
     * without SEND (so <code>dial</code> is not appropriate).
     *
     * @param dialString the MMI command to be executed.
     * @return true if MMI command is executed.
     */
    boolean handlePinMmi(String dialString);

    /**
     * Handles in-call MMI commands. While in a call, or while receiving a
     * call, use this to execute MMI commands.
     * see 3GPP 20.030, section 6.5.5.1 for specs on the allowed MMI commands.
     *
     * @param command the MMI command to be executed.
     * @return true if the MMI command is executed.
     * @throws CallStateException
     */
    boolean handleInCallMmiCommands(String command) throws CallStateException;

    /**
     * Play a DTMF tone on the active call. Ignored if there is no active call.
     * @param c should be one of 0-9, '*' or '#'. Other values will be
     * silently ignored.
     */
    void sendDtmf(char c);

    /**
     * Start to paly a DTMF tone on the active call. Ignored if there is no active call
     * or there is a playing DTMF tone.
     * @param c should be one of 0-9, '*' or '#'. Other values will be
     * silently ignored.
     */
    void startDtmf(char c);

    /**
     * Stop the playing DTMF tone. Ignored if there is no playing DTMF
     * tone or no active call.
     */
    void stopDtmf();

    /**
     * Sets the radio power on/off state (off is sometimes
     * called "airplane mode"). Current state can be gotten via
     * {@link #getServiceState()}.{@link
     * android.telephony.ServiceState#getState() getState()}.
     * <strong>Note: </strong>This request is asynchronous.
     * getServiceState().getState() will not change immediately after this call.
     * registerForServiceStateChanged() to find out when the
     * request is complete.
     *
     * @param power true means "on", false means "off".
     */
    void setRadioPower(boolean power);

    /**
     * Get the line 1 phone number (MSISDN). For CDMA phones, the MDN is returned
     * and {@link #getMsisdn()} will return the MSISDN on CDMA/LTE phones.<p>
     *
     * @return phone number. May return null if not
     * available or the SIM is not ready
     */
    String getLine1Number();

    /**
     * Returns the alpha tag associated with the msisdn number.
     * If there is no alpha tag associated or the record is not yet available,
     * returns a default localized string. <p>
     */
    String getLine1AlphaTag();

    /**
     * Sets the MSISDN phone number in the SIM card.
     *
     * @param alphaTag the alpha tag associated with the MSISDN phone number
     *        (see getMsisdnAlphaTag)
     * @param number the new MSISDN phone number to be set on the SIM.
     * @param onComplete a callback message when the action is completed.
     *
     * @return true if req is sent, false otherwise. If req is not sent there will be no response,
     * that is, onComplete will never be sent.
     */
    boolean setLine1Number(String alphaTag, String number, Message onComplete);

    /**
     * Get the voice mail access phone number. Typically dialed when the
     * user holds the "1" key in the phone app. May return null if not
     * available or the SIM is not ready.<p>
     */
    String getVoiceMailNumber();

    /**
     * Returns the alpha tag associated with the voice mail number.
     * If there is no alpha tag associated or the record is not yet available,
     * returns a default localized string. <p>
     *
     * Please use this value instead of some other localized string when
     * showing a name for this number in the UI. For example, call log
     * entries should show this alpha tag. <p>
     *
     * Usage of this alpha tag in the UI is a common carrier requirement.
     */
    String getVoiceMailAlphaTag();

    /**
     * setVoiceMailNumber
     * sets the voicemail number in the SIM card.
     *
     * @param alphaTag the alpha tag associated with the voice mail number
     *        (see getVoiceMailAlphaTag)
     * @param voiceMailNumber the new voicemail number to be set on the SIM.
     * @param onComplete a callback message when the action is completed.
     */
    void setVoiceMailNumber(String alphaTag,
                            String voiceMailNumber,
                            Message onComplete);

    /**
     * getCallForwardingOptions
     * gets a call forwarding option. The return value of
     * ((AsyncResult)onComplete.obj) is an array of CallForwardInfo.
     *
     * @param commandInterfaceCFReason is one of the valid call forwarding
     *        CF_REASONS, as defined in
     *        <code>com.android.internal.telephony.CommandsInterface.</code>
     * @param onComplete a callback message when the action is completed.
     *        @see com.android.internal.telephony.CallForwardInfo for details.
     */
    void getCallForwardingOption(int commandInterfaceCFReason,
                                  Message onComplete);

    /**
     * setCallForwardingOptions
     * sets a call forwarding option.
     *
     * @param commandInterfaceCFReason is one of the valid call forwarding
     *        CF_REASONS, as defined in
     *        <code>com.android.internal.telephony.CommandsInterface.</code>
     * @param commandInterfaceCFAction is one of the valid call forwarding
     *        CF_ACTIONS, as defined in
     *        <code>com.android.internal.telephony.CommandsInterface.</code>
     * @param dialingNumber is the target phone number to forward calls to
     * @param timerSeconds is used by CFNRy to indicate the timeout before
     *        forwarding is attempted.
     * @param onComplete a callback message when the action is completed.
     */
    void setCallForwardingOption(int commandInterfaceCFReason,
                                 int commandInterfaceCFAction,
                                 String dialingNumber,
                                 int timerSeconds,
                                 Message onComplete);

    /**
     * getOutgoingCallerIdDisplay
     * gets outgoing caller id display. The return value of
     * ((AsyncResult)onComplete.obj) is an array of int, with a length of 2.
     *
     * @param onComplete a callback message when the action is completed.
     *        @see com.android.internal.telephony.CommandsInterface#getCLIR for details.
     */
    void getOutgoingCallerIdDisplay(Message onComplete);

    /**
     * setOutgoingCallerIdDisplay
     * sets a call forwarding option.
     *
     * @param commandInterfaceCLIRMode is one of the valid call CLIR
     *        modes, as defined in
     *        <code>com.android.internal.telephony.CommandsInterface./code>
     * @param onComplete a callback message when the action is completed.
     */
    void setOutgoingCallerIdDisplay(int commandInterfaceCLIRMode,
                                    Message onComplete);

    /**
     * getCallWaiting
     * gets call waiting activation state. The return value of
     * ((AsyncResult)onComplete.obj) is an array of int, with a length of 1.
     *
     * @param onComplete a callback message when the action is completed.
     *        @see com.android.internal.telephony.CommandsInterface#queryCallWaiting for details.
     */
    void getCallWaiting(Message onComplete);

    /**
     * setCallWaiting
     * sets a call forwarding option.
     *
     * @param enable is a boolean representing the state that you are
     *        requesting, true for enabled, false for disabled.
     * @param onComplete a callback message when the action is completed.
     */
    void setCallWaiting(boolean enable, Message onComplete);

    /**
     * Scan available networks. This method is asynchronous; .
     * On completion, <code>response.obj</code> is set to an AsyncResult with
     * one of the following members:.<p>
     *<ul>
     * <li><code>response.obj.result</code> will be a <code>List</code> of
     * <code>OperatorInfo</code> objects, or</li>
     * <li><code>response.obj.exception</code> will be set with an exception
     * on failure.</li>
     * </ul>
     */
    void getAvailableNetworks(Message response);

    /**
     * Query neighboring cell IDs.  <code>response</code> is dispatched when
     * this is complete.  <code>response.obj</code> will be an AsyncResult,
     * and <code>response.obj.exception</code> will be non-null on failure.
     * On success, <code>AsyncResult.result</code> will be a <code>String[]</code>
     * containing the neighboring cell IDs.  Index 0 will contain the count
     * of available cell IDs.  Cell IDs are in hexadecimal format.
     *
     * @param response callback message that is dispatched when the query
     * completes.
     */
    void getNeighboringCids(Message response);

    /**
     * Mutes or unmutes the microphone for the active call. The microphone
     * is automatically unmuted if a call is answered, dialed, or resumed
     * from a holding state.
     *
     * @param muted true to mute the microphone,
     * false to activate the microphone.
     */

    void setMute(boolean muted);

    /**
     * Gets current mute status. Use
     * {@link #registerForPreciseCallStateChanged(android.os.Handler, int,
     * java.lang.Object) registerForPreciseCallStateChanged()}
     * as a change notifcation, although presently phone state changed is not
     * fired when setMute() is called.
     *
     * @return true is muting, false is unmuting
     */
    boolean getMute();

    /**
     * Get the current active Data Call list
     *
     * @param response <strong>On success</strong>, "response" bytes is
     * made available as:
     * (String[])(((AsyncResult)response.obj).result).
     * <strong>On failure</strong>,
     * (((AsyncResult)response.obj).result) == null and
     * (((AsyncResult)response.obj).exception) being an instance of
     * com.android.internal.telephony.gsm.CommandException
     */
    void getDataCallList(Message response);

    /**
     * Update the ServiceState CellLocation for current network registration.
     */
    void updateServiceLocation();

    /**
     * Enable location update notifications.
     */
    void enableLocationUpdates();

    /**
     * Disable location update notifications.
     */
    void disableLocationUpdates();

    /**
     * @return true if enable data connection on roaming
     */
    boolean getDataRoamingEnabled();

    /**
     * @param enable set true if enable data connection on roaming
     */
    void setDataRoamingEnabled(boolean enable);

    /**
     * @return true if user has enabled data
     */
    boolean getDataEnabled();

    /**
     * @param @enable set {@code true} if enable data connection
     */
    void setDataEnabled(boolean enable);

    /**
     * Retrieves the unique device ID, e.g., IMEI for GSM phones and MEID for CDMA phones.
     */
    String getDeviceId();

    /**
     * Retrieves the software version number for the device, e.g., IMEI/SV
     * for GSM phones.
     */
    String getDeviceSvn();

    /**
     * Retrieves the unique subscriber ID, e.g., IMSI for GSM phones.
     */
    String getSubscriberId();

    /**
     * Retrieves the Group Identifier Level1 for GSM phones.
     */
    String getGroupIdLevel1();

    /**
     * Retrieves the Group Identifier Level2 for phones.
     */
    String getGroupIdLevel2();

    /* CDMA support methods */

    /**
     * Retrieves the ESN for CDMA phones.
     */
    String getEsn();

    /**
     * Retrieves MEID for CDMA phones.
     */
    String getMeid();

    /**
     * Retrieves IMEI for phones. Returns null if IMEI is not set.
     */
    String getImei();

    /**
     * Retrieves the IccPhoneBookInterfaceManager of the Phone
     */
    public IccPhoneBookInterfaceManager getIccPhoneBookInterfaceManager();

    /**
     * Activate or deactivate cell broadcast SMS.
     *
     * @param activate
     *            0 = activate, 1 = deactivate
     * @param response
     *            Callback message is empty on completion
     */
    void activateCellBroadcastSms(int activate, Message response);

    /**
     * Query the current configuration of cdma cell broadcast SMS.
     *
     * @param response
     *            Callback message is empty on completion
     */
    void getCellBroadcastSmsConfig(Message response);

    /**
     * Configure cell broadcast SMS.
     *
     * TODO: Change the configValuesArray to a RIL_BroadcastSMSConfig
     *
     * @param response
     *            Callback message is empty on completion
     */
    public void setCellBroadcastSmsConfig(int[] configValuesArray, Message response);
}
