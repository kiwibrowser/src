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

package com.android.internal.telephony.gsm;

import android.content.Context;
import android.content.res.Resources;
import com.android.internal.telephony.*;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppState;

import android.os.*;
import android.telephony.PhoneNumberUtils;
import android.text.SpannableStringBuilder;
import android.text.BidiFormatter;
import android.text.TextDirectionHeuristics;
import android.text.TextUtils;
import android.telephony.Rlog;

import static com.android.internal.telephony.CommandsInterface.*;
import com.android.internal.telephony.gsm.SsData;

import java.util.regex.Pattern;
import java.util.regex.Matcher;

/**
 * The motto for this file is:
 *
 * "NOTE:    By using the # as a separator, most cases are expected to be unambiguous."
 *   -- TS 22.030 6.5.2
 *
 * {@hide}
 *
 */
public final class GsmMmiCode extends Handler implements MmiCode {
    static final String LOG_TAG = "GsmMmiCode";

    //***** Constants

    // Max Size of the Short Code (aka Short String from TS 22.030 6.5.2)
    static final int MAX_LENGTH_SHORT_CODE = 2;

    // TS 22.030 6.5.2 Every Short String USSD command will end with #-key
    // (known as #-String)
    static final char END_OF_USSD_COMMAND = '#';

    // From TS 22.030 6.5.2
    static final String ACTION_ACTIVATE = "*";
    static final String ACTION_DEACTIVATE = "#";
    static final String ACTION_INTERROGATE = "*#";
    static final String ACTION_REGISTER = "**";
    static final String ACTION_ERASURE = "##";

    // Supp Service codes from TS 22.030 Annex B

    //Called line presentation
    static final String SC_CLIP    = "30";
    static final String SC_CLIR    = "31";

    // Call Forwarding
    static final String SC_CFU     = "21";
    static final String SC_CFB     = "67";
    static final String SC_CFNRy   = "61";
    static final String SC_CFNR    = "62";

    static final String SC_CF_All = "002";
    static final String SC_CF_All_Conditional = "004";

    // Call Waiting
    static final String SC_WAIT     = "43";

    // Call Barring
    static final String SC_BAOC         = "33";
    static final String SC_BAOIC        = "331";
    static final String SC_BAOICxH      = "332";
    static final String SC_BAIC         = "35";
    static final String SC_BAICr        = "351";

    static final String SC_BA_ALL       = "330";
    static final String SC_BA_MO        = "333";
    static final String SC_BA_MT        = "353";

    // Supp Service Password registration
    static final String SC_PWD          = "03";

    // PIN/PIN2/PUK/PUK2
    static final String SC_PIN          = "04";
    static final String SC_PIN2         = "042";
    static final String SC_PUK          = "05";
    static final String SC_PUK2         = "052";

    //***** Event Constants

    static final int EVENT_SET_COMPLETE         = 1;
    static final int EVENT_GET_CLIR_COMPLETE    = 2;
    static final int EVENT_QUERY_CF_COMPLETE    = 3;
    static final int EVENT_USSD_COMPLETE        = 4;
    static final int EVENT_QUERY_COMPLETE       = 5;
    static final int EVENT_SET_CFF_COMPLETE     = 6;
    static final int EVENT_USSD_CANCEL_COMPLETE = 7;

    //***** Instance Variables

    GsmCdmaPhone mPhone;
    Context mContext;
    UiccCardApplication mUiccApplication;
    IccRecords mIccRecords;

    String mAction;              // One of ACTION_*
    String mSc;                  // Service Code
    String mSia, mSib, mSic;       // Service Info a,b,c
    String mPoundString;         // Entire MMI string up to and including #
    public String mDialingNumber;
    String mPwd;                 // For password registration

    /** Set to true in processCode, not at newFromDialString time */
    private boolean mIsPendingUSSD;

    private boolean mIsUssdRequest;

    private boolean mIsCallFwdReg;
    State mState = State.PENDING;
    CharSequence mMessage;
    private boolean mIsSsInfo = false;


    //***** Class Variables


    // See TS 22.030 6.5.2 "Structure of the MMI"

    static Pattern sPatternSuppService = Pattern.compile(
        "((\\*|#|\\*#|\\*\\*|##)(\\d{2,3})(\\*([^*#]*)(\\*([^*#]*)(\\*([^*#]*)(\\*([^*#]*))?)?)?)?#)(.*)");
/*       1  2                    3          4  5       6   7         8    9     10  11             12

         1 = Full string up to and including #
         2 = action (activation/interrogation/registration/erasure)
         3 = service code
         5 = SIA
         7 = SIB
         9 = SIC
         10 = dialing number
*/

    static final int MATCH_GROUP_POUND_STRING = 1;

    static final int MATCH_GROUP_ACTION = 2;
                        //(activation/interrogation/registration/erasure)

    static final int MATCH_GROUP_SERVICE_CODE = 3;
    static final int MATCH_GROUP_SIA = 5;
    static final int MATCH_GROUP_SIB = 7;
    static final int MATCH_GROUP_SIC = 9;
    static final int MATCH_GROUP_PWD_CONFIRM = 11;
    static final int MATCH_GROUP_DIALING_NUMBER = 12;
    static private String[] sTwoDigitNumberPattern;

    //***** Public Class methods

    /**
     * Some dial strings in GSM are defined to do non-call setup
     * things, such as modify or query supplementary service settings (eg, call
     * forwarding). These are generally referred to as "MMI codes".
     * We look to see if the dial string contains a valid MMI code (potentially
     * with a dial string at the end as well) and return info here.
     *
     * If the dial string contains no MMI code, we return an instance with
     * only "dialingNumber" set
     *
     * Please see flow chart in TS 22.030 6.5.3.2
     */

    public static GsmMmiCode
    newFromDialString(String dialString, GsmCdmaPhone phone, UiccCardApplication app) {
        Matcher m;
        GsmMmiCode ret = null;

        m = sPatternSuppService.matcher(dialString);

        // Is this formatted like a standard supplementary service code?
        if (m.matches()) {
            ret = new GsmMmiCode(phone, app);
            ret.mPoundString = makeEmptyNull(m.group(MATCH_GROUP_POUND_STRING));
            ret.mAction = makeEmptyNull(m.group(MATCH_GROUP_ACTION));
            ret.mSc = makeEmptyNull(m.group(MATCH_GROUP_SERVICE_CODE));
            ret.mSia = makeEmptyNull(m.group(MATCH_GROUP_SIA));
            ret.mSib = makeEmptyNull(m.group(MATCH_GROUP_SIB));
            ret.mSic = makeEmptyNull(m.group(MATCH_GROUP_SIC));
            ret.mPwd = makeEmptyNull(m.group(MATCH_GROUP_PWD_CONFIRM));
            ret.mDialingNumber = makeEmptyNull(m.group(MATCH_GROUP_DIALING_NUMBER));
            // According to TS 22.030 6.5.2 "Structure of the MMI",
            // the dialing number should not ending with #.
            // The dialing number ending # is treated as unique USSD,
            // eg, *400#16 digit number# to recharge the prepaid card
            // in India operator(Mumbai MTNL)
            if(ret.mDialingNumber != null &&
                    ret.mDialingNumber.endsWith("#") &&
                    dialString.endsWith("#")){
                ret = new GsmMmiCode(phone, app);
                ret.mPoundString = dialString;
            }
        } else if (dialString.endsWith("#")) {
            // TS 22.030 sec 6.5.3.2
            // "Entry of any characters defined in the 3GPP TS 23.038 [8] Default Alphabet
            // (up to the maximum defined in 3GPP TS 24.080 [10]), followed by #SEND".

            ret = new GsmMmiCode(phone, app);
            ret.mPoundString = dialString;
        } else if (isTwoDigitShortCode(phone.getContext(), dialString)) {
            //Is a country-specific exception to short codes as defined in TS 22.030, 6.5.3.2
            ret = null;
        } else if (isShortCode(dialString, phone)) {
            // this may be a short code, as defined in TS 22.030, 6.5.3.2
            ret = new GsmMmiCode(phone, app);
            ret.mDialingNumber = dialString;
        }

        return ret;
    }

    public static GsmMmiCode
    newNetworkInitiatedUssd(String ussdMessage,
                            boolean isUssdRequest, GsmCdmaPhone phone, UiccCardApplication app) {
        GsmMmiCode ret;

        ret = new GsmMmiCode(phone, app);

        ret.mMessage = ussdMessage;
        ret.mIsUssdRequest = isUssdRequest;

        // If it's a request, set to PENDING so that it's cancelable.
        if (isUssdRequest) {
            ret.mIsPendingUSSD = true;
            ret.mState = State.PENDING;
        } else {
            ret.mState = State.COMPLETE;
        }

        return ret;
    }

    public static GsmMmiCode newFromUssdUserInput(String ussdMessge,
                                                  GsmCdmaPhone phone,
                                                  UiccCardApplication app) {
        GsmMmiCode ret = new GsmMmiCode(phone, app);

        ret.mMessage = ussdMessge;
        ret.mState = State.PENDING;
        ret.mIsPendingUSSD = true;

        return ret;
    }

    /** Process SS Data */
    public void
    processSsData(AsyncResult data) {
        Rlog.d(LOG_TAG, "In processSsData");

        mIsSsInfo = true;
        try {
            SsData ssData = (SsData)data.result;
            parseSsData(ssData);
        } catch (ClassCastException ex) {
            Rlog.e(LOG_TAG, "Class Cast Exception in parsing SS Data : " + ex);
        } catch (NullPointerException ex) {
            Rlog.e(LOG_TAG, "Null Pointer Exception in parsing SS Data : " + ex);
        }
    }

    void parseSsData(SsData ssData) {
        CommandException ex;

        ex = CommandException.fromRilErrno(ssData.result);
        mSc = getScStringFromScType(ssData.serviceType);
        mAction = getActionStringFromReqType(ssData.requestType);
        Rlog.d(LOG_TAG, "parseSsData msc = " + mSc + ", action = " + mAction + ", ex = " + ex);

        switch (ssData.requestType) {
            case SS_ACTIVATION:
            case SS_DEACTIVATION:
            case SS_REGISTRATION:
            case SS_ERASURE:
                if ((ssData.result == RILConstants.SUCCESS) &&
                      ssData.serviceType.isTypeUnConditional()) {
                    /*
                     * When ServiceType is SS_CFU/SS_CF_ALL and RequestType is activate/register
                     * and ServiceClass is Voice/None, set IccRecords.setVoiceCallForwardingFlag.
                     * Only CF status can be set here since number is not available.
                     */
                    boolean cffEnabled = ((ssData.requestType == SsData.RequestType.SS_ACTIVATION ||
                            ssData.requestType == SsData.RequestType.SS_REGISTRATION) &&
                            isServiceClassVoiceorNone(ssData.serviceClass));

                    Rlog.d(LOG_TAG, "setVoiceCallForwardingFlag cffEnabled: " + cffEnabled);
                    if (mIccRecords != null) {
                        mPhone.setVoiceCallForwardingFlag(1, cffEnabled, null);
                        Rlog.d(LOG_TAG, "setVoiceCallForwardingFlag done from SS Info.");
                    } else {
                        Rlog.e(LOG_TAG, "setVoiceCallForwardingFlag aborted. sim records is null.");
                    }
                }
                onSetComplete(null, new AsyncResult(null, ssData.cfInfo, ex));
                break;
            case SS_INTERROGATION:
                if (ssData.serviceType.isTypeClir()) {
                    Rlog.d(LOG_TAG, "CLIR INTERROGATION");
                    onGetClirComplete(new AsyncResult(null, ssData.ssInfo, ex));
                } else if (ssData.serviceType.isTypeCF()) {
                    Rlog.d(LOG_TAG, "CALL FORWARD INTERROGATION");
                    onQueryCfComplete(new AsyncResult(null, ssData.cfInfo, ex));
                } else {
                    onQueryComplete(new AsyncResult(null, ssData.ssInfo, ex));
                }
                break;
            default:
                Rlog.e(LOG_TAG, "Invaid requestType in SSData : " + ssData.requestType);
                break;
        }
    }

    private String getScStringFromScType(SsData.ServiceType sType) {
        switch (sType) {
            case SS_CFU:
                return SC_CFU;
            case SS_CF_BUSY:
                return SC_CFB;
            case SS_CF_NO_REPLY:
                return SC_CFNRy;
            case SS_CF_NOT_REACHABLE:
                return SC_CFNR;
            case SS_CF_ALL:
                return SC_CF_All;
            case SS_CF_ALL_CONDITIONAL:
                return SC_CF_All_Conditional;
            case SS_CLIP:
                return SC_CLIP;
            case SS_CLIR:
                return SC_CLIR;
            case SS_WAIT:
                return SC_WAIT;
            case SS_BAOC:
                return SC_BAOC;
            case SS_BAOIC:
                return SC_BAOIC;
            case SS_BAOIC_EXC_HOME:
                return SC_BAOICxH;
            case SS_BAIC:
                return SC_BAIC;
            case SS_BAIC_ROAMING:
                return SC_BAICr;
            case SS_ALL_BARRING:
                return SC_BA_ALL;
            case SS_OUTGOING_BARRING:
                return SC_BA_MO;
            case SS_INCOMING_BARRING:
                return SC_BA_MT;
        }

        return "";
    }

    private String getActionStringFromReqType(SsData.RequestType rType) {
        switch (rType) {
            case SS_ACTIVATION:
                return ACTION_ACTIVATE;
            case SS_DEACTIVATION:
                return ACTION_DEACTIVATE;
            case SS_INTERROGATION:
                return ACTION_INTERROGATE;
            case SS_REGISTRATION:
                return ACTION_REGISTER;
            case SS_ERASURE:
                return ACTION_ERASURE;
        }

        return "";
    }

    private boolean isServiceClassVoiceorNone(int serviceClass) {
        return (((serviceClass & CommandsInterface.SERVICE_CLASS_VOICE) != 0) ||
                (serviceClass == CommandsInterface.SERVICE_CLASS_NONE));
    }

    //***** Private Class methods

    /** make empty strings be null.
     *  Regexp returns empty strings for empty groups
     */
    private static String
    makeEmptyNull (String s) {
        if (s != null && s.length() == 0) return null;

        return s;
    }

    /** returns true of the string is empty or null */
    private static boolean
    isEmptyOrNull(CharSequence s) {
        return s == null || (s.length() == 0);
    }


    private static int
    scToCallForwardReason(String sc) {
        if (sc == null) {
            throw new RuntimeException ("invalid call forward sc");
        }

        if (sc.equals(SC_CF_All)) {
           return CommandsInterface.CF_REASON_ALL;
        } else if (sc.equals(SC_CFU)) {
            return CommandsInterface.CF_REASON_UNCONDITIONAL;
        } else if (sc.equals(SC_CFB)) {
            return CommandsInterface.CF_REASON_BUSY;
        } else if (sc.equals(SC_CFNR)) {
            return CommandsInterface.CF_REASON_NOT_REACHABLE;
        } else if (sc.equals(SC_CFNRy)) {
            return CommandsInterface.CF_REASON_NO_REPLY;
        } else if (sc.equals(SC_CF_All_Conditional)) {
           return CommandsInterface.CF_REASON_ALL_CONDITIONAL;
        } else {
            throw new RuntimeException ("invalid call forward sc");
        }
    }

    private static int
    siToServiceClass(String si) {
        if (si == null || si.length() == 0) {
                return  SERVICE_CLASS_NONE;
        } else {
            // NumberFormatException should cause MMI fail
            int serviceCode = Integer.parseInt(si, 10);

            switch (serviceCode) {
                case 10: return SERVICE_CLASS_SMS + SERVICE_CLASS_FAX  + SERVICE_CLASS_VOICE;
                case 11: return SERVICE_CLASS_VOICE;
                case 12: return SERVICE_CLASS_SMS + SERVICE_CLASS_FAX;
                case 13: return SERVICE_CLASS_FAX;

                case 16: return SERVICE_CLASS_SMS;

                case 19: return SERVICE_CLASS_FAX + SERVICE_CLASS_VOICE;
/*
    Note for code 20:
     From TS 22.030 Annex C:
                "All GPRS bearer services" are not included in "All tele and bearer services"
                    and "All bearer services"."
....so SERVICE_CLASS_DATA, which (according to 27.007) includes GPRS
*/
                case 20: return SERVICE_CLASS_DATA_ASYNC + SERVICE_CLASS_DATA_SYNC;

                case 21: return SERVICE_CLASS_PAD + SERVICE_CLASS_DATA_ASYNC;
                case 22: return SERVICE_CLASS_PACKET + SERVICE_CLASS_DATA_SYNC;
                case 24: return SERVICE_CLASS_DATA_SYNC;
                case 25: return SERVICE_CLASS_DATA_ASYNC;
                case 26: return SERVICE_CLASS_DATA_SYNC + SERVICE_CLASS_VOICE;
                case 99: return SERVICE_CLASS_PACKET;

                default:
                    throw new RuntimeException("unsupported MMI service code " + si);
            }
        }
    }

    private static int
    siToTime (String si) {
        if (si == null || si.length() == 0) {
            return 0;
        } else {
            // NumberFormatException should cause MMI fail
            return Integer.parseInt(si, 10);
        }
    }

    static boolean
    isServiceCodeCallForwarding(String sc) {
        return sc != null &&
                (sc.equals(SC_CFU)
                || sc.equals(SC_CFB) || sc.equals(SC_CFNRy)
                || sc.equals(SC_CFNR) || sc.equals(SC_CF_All)
                || sc.equals(SC_CF_All_Conditional));
    }

    static boolean
    isServiceCodeCallBarring(String sc) {
        Resources resource = Resources.getSystem();
        if (sc != null) {
            String[] barringMMI = resource.getStringArray(
                com.android.internal.R.array.config_callBarringMMI);
            if (barringMMI != null) {
                for (String match : barringMMI) {
                    if (sc.equals(match)) return true;
                }
            }
        }
        return false;
    }

    static String
    scToBarringFacility(String sc) {
        if (sc == null) {
            throw new RuntimeException ("invalid call barring sc");
        }

        if (sc.equals(SC_BAOC)) {
            return CommandsInterface.CB_FACILITY_BAOC;
        } else if (sc.equals(SC_BAOIC)) {
            return CommandsInterface.CB_FACILITY_BAOIC;
        } else if (sc.equals(SC_BAOICxH)) {
            return CommandsInterface.CB_FACILITY_BAOICxH;
        } else if (sc.equals(SC_BAIC)) {
            return CommandsInterface.CB_FACILITY_BAIC;
        } else if (sc.equals(SC_BAICr)) {
            return CommandsInterface.CB_FACILITY_BAICr;
        } else if (sc.equals(SC_BA_ALL)) {
            return CommandsInterface.CB_FACILITY_BA_ALL;
        } else if (sc.equals(SC_BA_MO)) {
            return CommandsInterface.CB_FACILITY_BA_MO;
        } else if (sc.equals(SC_BA_MT)) {
            return CommandsInterface.CB_FACILITY_BA_MT;
        } else {
            throw new RuntimeException ("invalid call barring sc");
        }
    }

    //***** Constructor

    public GsmMmiCode(GsmCdmaPhone phone, UiccCardApplication app) {
        // The telephony unit-test cases may create GsmMmiCode's
        // in secondary threads
        super(phone.getHandler().getLooper());
        mPhone = phone;
        mContext = phone.getContext();
        mUiccApplication = app;
        if (app != null) {
            mIccRecords = app.getIccRecords();
        }
    }

    //***** MmiCode implementation

    @Override
    public State
    getState() {
        return mState;
    }

    @Override
    public CharSequence
    getMessage() {
        return mMessage;
    }

    public Phone
    getPhone() {
        return ((Phone) mPhone);
    }

    // inherited javadoc suffices
    @Override
    public void
    cancel() {
        // Complete or failed cannot be cancelled
        if (mState == State.COMPLETE || mState == State.FAILED) {
            return;
        }

        mState = State.CANCELLED;

        if (mIsPendingUSSD) {
            /*
             * There can only be one pending USSD session, so tell the radio to
             * cancel it.
             */
            mPhone.mCi.cancelPendingUssd(obtainMessage(EVENT_USSD_CANCEL_COMPLETE, this));

            /*
             * Don't call phone.onMMIDone here; wait for CANCEL_COMPLETE notice
             * from RIL.
             */
        } else {
            // TODO in cases other than USSD, it would be nice to cancel
            // the pending radio operation. This requires RIL cancellation
            // support, which does not presently exist.

            mPhone.onMMIDone (this);
        }

    }

    @Override
    public boolean isCancelable() {
        /* Can only cancel pending USSD sessions. */
        return mIsPendingUSSD;
    }

    //***** Instance Methods

    /** Does this dial string contain a structured or unstructured MMI code? */
    boolean
    isMMI() {
        return mPoundString != null;
    }

    /* Is this a 1 or 2 digit "short code" as defined in TS 22.030 sec 6.5.3.2? */
    boolean
    isShortCode() {
        return mPoundString == null
                    && mDialingNumber != null && mDialingNumber.length() <= 2;

    }

    static private boolean
    isTwoDigitShortCode(Context context, String dialString) {
        Rlog.d(LOG_TAG, "isTwoDigitShortCode");

        if (dialString == null || dialString.length() > 2) return false;

        if (sTwoDigitNumberPattern == null) {
            sTwoDigitNumberPattern = context.getResources().getStringArray(
                    com.android.internal.R.array.config_twoDigitNumberPattern);
        }

        for (String dialnumber : sTwoDigitNumberPattern) {
            Rlog.d(LOG_TAG, "Two Digit Number Pattern " + dialnumber);
            if (dialString.equals(dialnumber)) {
                Rlog.d(LOG_TAG, "Two Digit Number Pattern -true");
                return true;
            }
        }
        Rlog.d(LOG_TAG, "Two Digit Number Pattern -false");
        return false;
    }

    /**
     * Helper function for newFromDialString. Returns true if dialString appears
     * to be a short code AND conditions are correct for it to be treated as
     * such.
     */
    static private boolean isShortCode(String dialString, GsmCdmaPhone phone) {
        // Refer to TS 22.030 Figure 3.5.3.2:
        if (dialString == null) {
            return false;
        }

        // Illegal dial string characters will give a ZERO length.
        // At this point we do not want to crash as any application with
        // call privileges may send a non dial string.
        // It return false as when the dialString is equal to NULL.
        if (dialString.length() == 0) {
            return false;
        }

        if (PhoneNumberUtils.isLocalEmergencyNumber(phone.getContext(), dialString)) {
            return false;
        } else {
            return isShortCodeUSSD(dialString, phone);
        }
    }

    /**
     * Helper function for isShortCode. Returns true if dialString appears to be
     * a short code and it is a USSD structure
     *
     * According to the 3PGG TS 22.030 specification Figure 3.5.3.2: A 1 or 2
     * digit "short code" is treated as USSD if it is entered while on a call or
     * does not satisfy the condition (exactly 2 digits && starts with '1'), there
     * are however exceptions to this rule (see below)
     *
     * Exception (1) to Call initiation is: If the user of the device is already in a call
     * and enters a Short String without any #-key at the end and the length of the Short String is
     * equal or less then the MAX_LENGTH_SHORT_CODE [constant that is equal to 2]
     *
     * The phone shall initiate a USSD/SS commands.
     */
    static private boolean isShortCodeUSSD(String dialString, GsmCdmaPhone phone) {
        if (dialString != null && dialString.length() <= MAX_LENGTH_SHORT_CODE) {
            if (phone.isInCall()) {
                return true;
            }

            if (dialString.length() != MAX_LENGTH_SHORT_CODE ||
                    dialString.charAt(0) != '1') {
                return true;
            }
        }
        return false;
    }

    /**
     * @return true if the Service Code is PIN/PIN2/PUK/PUK2-related
     */
    public boolean isPinPukCommand() {
        return mSc != null && (mSc.equals(SC_PIN) || mSc.equals(SC_PIN2)
                              || mSc.equals(SC_PUK) || mSc.equals(SC_PUK2));
     }

    /**
     * See TS 22.030 Annex B.
     * In temporary mode, to suppress CLIR for a single call, enter:
     *      " * 31 # [called number] SEND "
     *  In temporary mode, to invoke CLIR for a single call enter:
     *       " # 31 # [called number] SEND "
     */
    public boolean
    isTemporaryModeCLIR() {
        return mSc != null && mSc.equals(SC_CLIR) && mDialingNumber != null
                && (isActivate() || isDeactivate());
    }

    /**
     * returns CommandsInterface.CLIR_*
     * See also isTemporaryModeCLIR()
     */
    public int
    getCLIRMode() {
        if (mSc != null && mSc.equals(SC_CLIR)) {
            if (isActivate()) {
                return CommandsInterface.CLIR_SUPPRESSION;
            } else if (isDeactivate()) {
                return CommandsInterface.CLIR_INVOCATION;
            }
        }

        return CommandsInterface.CLIR_DEFAULT;
    }

    boolean isActivate() {
        return mAction != null && mAction.equals(ACTION_ACTIVATE);
    }

    boolean isDeactivate() {
        return mAction != null && mAction.equals(ACTION_DEACTIVATE);
    }

    boolean isInterrogate() {
        return mAction != null && mAction.equals(ACTION_INTERROGATE);
    }

    boolean isRegister() {
        return mAction != null && mAction.equals(ACTION_REGISTER);
    }

    boolean isErasure() {
        return mAction != null && mAction.equals(ACTION_ERASURE);
    }

    /**
     * Returns true if this is a USSD code that's been submitted to the
     * network...eg, after processCode() is called
     */
    public boolean isPendingUSSD() {
        return mIsPendingUSSD;
    }

    @Override
    public boolean isUssdRequest() {
        return mIsUssdRequest;
    }

    public boolean isSsInfo() {
        return mIsSsInfo;
    }

    /** Process a MMI code or short code...anything that isn't a dialing number */
    public void
    processCode() throws CallStateException {
        try {
            if (isShortCode()) {
                Rlog.d(LOG_TAG, "isShortCode");
                // These just get treated as USSD.
                sendUssd(mDialingNumber);
            } else if (mDialingNumber != null) {
                // We should have no dialing numbers here
                throw new RuntimeException ("Invalid or Unsupported MMI Code");
            } else if (mSc != null && mSc.equals(SC_CLIP)) {
                Rlog.d(LOG_TAG, "is CLIP");
                if (isInterrogate()) {
                    mPhone.mCi.queryCLIP(
                            obtainMessage(EVENT_QUERY_COMPLETE, this));
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }
            } else if (mSc != null && mSc.equals(SC_CLIR)) {
                Rlog.d(LOG_TAG, "is CLIR");
                if (isActivate()) {
                    mPhone.mCi.setCLIR(CommandsInterface.CLIR_INVOCATION,
                        obtainMessage(EVENT_SET_COMPLETE, this));
                } else if (isDeactivate()) {
                    mPhone.mCi.setCLIR(CommandsInterface.CLIR_SUPPRESSION,
                        obtainMessage(EVENT_SET_COMPLETE, this));
                } else if (isInterrogate()) {
                    mPhone.mCi.getCLIR(
                        obtainMessage(EVENT_GET_CLIR_COMPLETE, this));
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }
            } else if (isServiceCodeCallForwarding(mSc)) {
                Rlog.d(LOG_TAG, "is CF");

                String dialingNumber = mSia;
                int serviceClass = siToServiceClass(mSib);
                int reason = scToCallForwardReason(mSc);
                int time = siToTime(mSic);

                if (isInterrogate()) {
                    mPhone.mCi.queryCallForwardStatus(
                            reason, serviceClass,  dialingNumber,
                                obtainMessage(EVENT_QUERY_CF_COMPLETE, this));
                } else {
                    int cfAction;

                    if (isActivate()) {
                        // 3GPP TS 22.030 6.5.2
                        // a call forwarding request with a single * would be
                        // interpreted as registration if containing a forwarded-to
                        // number, or an activation if not
                        if (isEmptyOrNull(dialingNumber)) {
                            cfAction = CommandsInterface.CF_ACTION_ENABLE;
                            mIsCallFwdReg = false;
                        } else {
                            cfAction = CommandsInterface.CF_ACTION_REGISTRATION;
                            mIsCallFwdReg = true;
                        }
                    } else if (isDeactivate()) {
                        cfAction = CommandsInterface.CF_ACTION_DISABLE;
                    } else if (isRegister()) {
                        cfAction = CommandsInterface.CF_ACTION_REGISTRATION;
                    } else if (isErasure()) {
                        cfAction = CommandsInterface.CF_ACTION_ERASURE;
                    } else {
                        throw new RuntimeException ("invalid action");
                    }

                    int isSettingUnconditionalVoice =
                        (((reason == CommandsInterface.CF_REASON_UNCONDITIONAL) ||
                                (reason == CommandsInterface.CF_REASON_ALL)) &&
                                (((serviceClass & CommandsInterface.SERVICE_CLASS_VOICE) != 0) ||
                                 (serviceClass == CommandsInterface.SERVICE_CLASS_NONE))) ? 1 : 0;

                    int isEnableDesired =
                        ((cfAction == CommandsInterface.CF_ACTION_ENABLE) ||
                                (cfAction == CommandsInterface.CF_ACTION_REGISTRATION)) ? 1 : 0;

                    Rlog.d(LOG_TAG, "is CF setCallForward");
                    mPhone.mCi.setCallForward(cfAction, reason, serviceClass,
                            dialingNumber, time, obtainMessage(
                                    EVENT_SET_CFF_COMPLETE,
                                    isSettingUnconditionalVoice,
                                    isEnableDesired, this));
                }
            } else if (isServiceCodeCallBarring(mSc)) {
                // sia = password
                // sib = basic service group

                String password = mSia;
                int serviceClass = siToServiceClass(mSib);
                String facility = scToBarringFacility(mSc);

                if (isInterrogate()) {
                    mPhone.mCi.queryFacilityLock(facility, password,
                            serviceClass, obtainMessage(EVENT_QUERY_COMPLETE, this));
                } else if (isActivate() || isDeactivate()) {
                    mPhone.mCi.setFacilityLock(facility, isActivate(), password,
                            serviceClass, obtainMessage(EVENT_SET_COMPLETE, this));
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }

            } else if (mSc != null && mSc.equals(SC_PWD)) {
                // sia = fac
                // sib = old pwd
                // sic = new pwd
                // pwd = new pwd
                String facility;
                String oldPwd = mSib;
                String newPwd = mSic;
                if (isActivate() || isRegister()) {
                    // Even though ACTIVATE is acceptable, this is really termed a REGISTER
                    mAction = ACTION_REGISTER;

                    if (mSia == null) {
                        // If sc was not specified, treat it as BA_ALL.
                        facility = CommandsInterface.CB_FACILITY_BA_ALL;
                    } else {
                        facility = scToBarringFacility(mSia);
                    }
                    if (newPwd.equals(mPwd)) {
                        mPhone.mCi.changeBarringPassword(facility, oldPwd,
                                newPwd, obtainMessage(EVENT_SET_COMPLETE, this));
                    } else {
                        // password mismatch; return error
                        handlePasswordError(com.android.internal.R.string.passwordIncorrect);
                    }
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }

            } else if (mSc != null && mSc.equals(SC_WAIT)) {
                // sia = basic service group
                int serviceClass = siToServiceClass(mSia);

                if (isActivate() || isDeactivate()) {
                    mPhone.mCi.setCallWaiting(isActivate(), serviceClass,
                            obtainMessage(EVENT_SET_COMPLETE, this));
                } else if (isInterrogate()) {
                    mPhone.mCi.queryCallWaiting(serviceClass,
                            obtainMessage(EVENT_QUERY_COMPLETE, this));
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }
            } else if (isPinPukCommand()) {
                // TODO: This is the same as the code in CmdaMmiCode.java,
                // MmiCode should be an abstract or base class and this and
                // other common variables and code should be promoted.

                // sia = old PIN or PUK
                // sib = new PIN
                // sic = new PIN
                String oldPinOrPuk = mSia;
                String newPinOrPuk = mSib;
                int pinLen = newPinOrPuk.length();
                if (isRegister()) {
                    if (!newPinOrPuk.equals(mSic)) {
                        // password mismatch; return error
                        handlePasswordError(com.android.internal.R.string.mismatchPin);
                    } else if (pinLen < 4 || pinLen > 8 ) {
                        // invalid length
                        handlePasswordError(com.android.internal.R.string.invalidPin);
                    } else if (mSc.equals(SC_PIN)
                            && mUiccApplication != null
                            && mUiccApplication.getState() == AppState.APPSTATE_PUK) {
                        // Sim is puk-locked
                        handlePasswordError(com.android.internal.R.string.needPuk);
                    } else if (mUiccApplication != null) {
                        Rlog.d(LOG_TAG, "process mmi service code using UiccApp sc=" + mSc);

                        // We have an app and the pre-checks are OK
                        if (mSc.equals(SC_PIN)) {
                            mUiccApplication.changeIccLockPassword(oldPinOrPuk, newPinOrPuk,
                                    obtainMessage(EVENT_SET_COMPLETE, this));
                        } else if (mSc.equals(SC_PIN2)) {
                            mUiccApplication.changeIccFdnPassword(oldPinOrPuk, newPinOrPuk,
                                    obtainMessage(EVENT_SET_COMPLETE, this));
                        } else if (mSc.equals(SC_PUK)) {
                            mUiccApplication.supplyPuk(oldPinOrPuk, newPinOrPuk,
                                    obtainMessage(EVENT_SET_COMPLETE, this));
                        } else if (mSc.equals(SC_PUK2)) {
                            mUiccApplication.supplyPuk2(oldPinOrPuk, newPinOrPuk,
                                    obtainMessage(EVENT_SET_COMPLETE, this));
                        } else {
                            throw new RuntimeException("uicc unsupported service code=" + mSc);
                        }
                    } else {
                        throw new RuntimeException("No application mUiccApplicaiton is null");
                    }
                } else {
                    throw new RuntimeException ("Ivalid register/action=" + mAction);
                }
            } else if (mPoundString != null) {
                sendUssd(mPoundString);
            } else {
                throw new RuntimeException ("Invalid or Unsupported MMI Code");
            }
        } catch (RuntimeException exc) {
            mState = State.FAILED;
            mMessage = mContext.getText(com.android.internal.R.string.mmiError);
            mPhone.onMMIDone(this);
        }
    }

    private void handlePasswordError(int res) {
        mState = State.FAILED;
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");
        sb.append(mContext.getText(res));
        mMessage = sb;
        mPhone.onMMIDone(this);
    }

    /**
     * Called from GsmCdmaPhone
     *
     * An unsolicited USSD NOTIFY or REQUEST has come in matching
     * up with this pending USSD request
     *
     * Note: If REQUEST, this exchange is complete, but the session remains
     *       active (ie, the network expects user input).
     */
    public void
    onUssdFinished(String ussdMessage, boolean isUssdRequest) {
        if (mState == State.PENDING) {
            if (ussdMessage == null) {
                mMessage = mContext.getText(com.android.internal.R.string.mmiComplete);
            } else {
                mMessage = ussdMessage;
            }
            mIsUssdRequest = isUssdRequest;
            // If it's a request, leave it PENDING so that it's cancelable.
            if (!isUssdRequest) {
                mState = State.COMPLETE;
            }

            mPhone.onMMIDone(this);
        }
    }

    /**
     * Called from GsmCdmaPhone
     *
     * The radio has reset, and this is still pending
     */

    public void
    onUssdFinishedError() {
        if (mState == State.PENDING) {
            mState = State.FAILED;
            mMessage = mContext.getText(com.android.internal.R.string.mmiError);

            mPhone.onMMIDone(this);
        }
    }

    /**
     * Called from GsmCdmaPhone
     *
     * An unsolicited USSD NOTIFY or REQUEST has come in matching
     * up with this pending USSD request
     *
     * Note: If REQUEST, this exchange is complete, but the session remains
     *       active (ie, the network expects user input).
     */
    public void
    onUssdRelease() {
        if (mState == State.PENDING) {
            mState = State.COMPLETE;
            mMessage = null;

            mPhone.onMMIDone(this);
        }
    }

    public void sendUssd(String ussdMessage) {
        // Treat this as a USSD string
        mIsPendingUSSD = true;

        // Note that unlike most everything else, the USSD complete
        // response does not complete this MMI code...we wait for
        // an unsolicited USSD "Notify" or "Request".
        // The matching up of this is done in GsmCdmaPhone.

        mPhone.mCi.sendUSSD(ussdMessage,
            obtainMessage(EVENT_USSD_COMPLETE, this));
    }

    /** Called from GsmCdmaPhone.handleMessage; not a Handler subclass */
    @Override
    public void
    handleMessage (Message msg) {
        AsyncResult ar;

        switch (msg.what) {
            case EVENT_SET_COMPLETE:
                ar = (AsyncResult) (msg.obj);

                onSetComplete(msg, ar);
                break;

            case EVENT_SET_CFF_COMPLETE:
                ar = (AsyncResult) (msg.obj);

                /*
                * msg.arg1 = 1 means to set unconditional voice call forwarding
                * msg.arg2 = 1 means to enable voice call forwarding
                */
                if ((ar.exception == null) && (msg.arg1 == 1)) {
                    boolean cffEnabled = (msg.arg2 == 1);
                    if (mIccRecords != null) {
                        mPhone.setVoiceCallForwardingFlag(1, cffEnabled, mDialingNumber);
                    }
                }

                onSetComplete(msg, ar);
                break;

            case EVENT_GET_CLIR_COMPLETE:
                ar = (AsyncResult) (msg.obj);
                onGetClirComplete(ar);
            break;

            case EVENT_QUERY_CF_COMPLETE:
                ar = (AsyncResult) (msg.obj);
                onQueryCfComplete(ar);
            break;

            case EVENT_QUERY_COMPLETE:
                ar = (AsyncResult) (msg.obj);
                onQueryComplete(ar);
            break;

            case EVENT_USSD_COMPLETE:
                ar = (AsyncResult) (msg.obj);

                if (ar.exception != null) {
                    mState = State.FAILED;
                    mMessage = getErrorMessage(ar);

                    mPhone.onMMIDone(this);
                }

                // Note that unlike most everything else, the USSD complete
                // response does not complete this MMI code...we wait for
                // an unsolicited USSD "Notify" or "Request".
                // The matching up of this is done in GsmCdmaPhone.

            break;

            case EVENT_USSD_CANCEL_COMPLETE:
                mPhone.onMMIDone(this);
            break;
        }
    }
    //***** Private instance methods

    private CharSequence getErrorMessage(AsyncResult ar) {

        if (ar.exception instanceof CommandException) {
            CommandException.Error err = ((CommandException)(ar.exception)).getCommandError();
            if (err == CommandException.Error.FDN_CHECK_FAILURE) {
                Rlog.i(LOG_TAG, "FDN_CHECK_FAILURE");
                return mContext.getText(com.android.internal.R.string.mmiFdnError);
            } else if (err == CommandException.Error.USSD_MODIFIED_TO_DIAL) {
                Rlog.i(LOG_TAG, "USSD_MODIFIED_TO_DIAL");
                return mContext.getText(com.android.internal.R.string.stk_cc_ussd_to_dial);
            } else if (err == CommandException.Error.USSD_MODIFIED_TO_SS) {
                Rlog.i(LOG_TAG, "USSD_MODIFIED_TO_SS");
                return mContext.getText(com.android.internal.R.string.stk_cc_ussd_to_ss);
            } else if (err == CommandException.Error.USSD_MODIFIED_TO_USSD) {
                Rlog.i(LOG_TAG, "USSD_MODIFIED_TO_USSD");
                return mContext.getText(com.android.internal.R.string.stk_cc_ussd_to_ussd);
            } else if (err == CommandException.Error.SS_MODIFIED_TO_DIAL) {
                Rlog.i(LOG_TAG, "SS_MODIFIED_TO_DIAL");
                return mContext.getText(com.android.internal.R.string.stk_cc_ss_to_dial);
            } else if (err == CommandException.Error.SS_MODIFIED_TO_USSD) {
                Rlog.i(LOG_TAG, "SS_MODIFIED_TO_USSD");
                return mContext.getText(com.android.internal.R.string.stk_cc_ss_to_ussd);
            } else if (err == CommandException.Error.SS_MODIFIED_TO_SS) {
                Rlog.i(LOG_TAG, "SS_MODIFIED_TO_SS");
                return mContext.getText(com.android.internal.R.string.stk_cc_ss_to_ss);
            }
        }

        return mContext.getText(com.android.internal.R.string.mmiError);
    }

    private CharSequence getScString() {
        if (mSc != null) {
            if (isServiceCodeCallBarring(mSc)) {
                return mContext.getText(com.android.internal.R.string.BaMmi);
            } else if (isServiceCodeCallForwarding(mSc)) {
                return mContext.getText(com.android.internal.R.string.CfMmi);
            } else if (mSc.equals(SC_CLIP)) {
                return mContext.getText(com.android.internal.R.string.ClipMmi);
            } else if (mSc.equals(SC_CLIR)) {
                return mContext.getText(com.android.internal.R.string.ClirMmi);
            } else if (mSc.equals(SC_PWD)) {
                return mContext.getText(com.android.internal.R.string.PwdMmi);
            } else if (mSc.equals(SC_WAIT)) {
                return mContext.getText(com.android.internal.R.string.CwMmi);
            } else if (isPinPukCommand()) {
                return mContext.getText(com.android.internal.R.string.PinMmi);
            }
        }

        return "";
    }

    private void
    onSetComplete(Message msg, AsyncResult ar){
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");

        if (ar.exception != null) {
            mState = State.FAILED;
            if (ar.exception instanceof CommandException) {
                CommandException.Error err = ((CommandException)(ar.exception)).getCommandError();
                if (err == CommandException.Error.PASSWORD_INCORRECT) {
                    if (isPinPukCommand()) {
                        // look specifically for the PUK commands and adjust
                        // the message accordingly.
                        if (mSc.equals(SC_PUK) || mSc.equals(SC_PUK2)) {
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.badPuk));
                        } else {
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.badPin));
                        }
                        // Get the No. of retries remaining to unlock PUK/PUK2
                        int attemptsRemaining = msg.arg1;
                        if (attemptsRemaining <= 0) {
                            Rlog.d(LOG_TAG, "onSetComplete: PUK locked,"
                                    + " cancel as lock screen will handle this");
                            mState = State.CANCELLED;
                        } else if (attemptsRemaining > 0) {
                            Rlog.d(LOG_TAG, "onSetComplete: attemptsRemaining="+attemptsRemaining);
                            sb.append(mContext.getResources().getQuantityString(
                                    com.android.internal.R.plurals.pinpuk_attempts,
                                    attemptsRemaining, attemptsRemaining));
                        }
                    } else {
                        sb.append(mContext.getText(
                                com.android.internal.R.string.passwordIncorrect));
                    }
                } else if (err == CommandException.Error.SIM_PUK2) {
                    sb.append(mContext.getText(
                            com.android.internal.R.string.badPin));
                    sb.append("\n");
                    sb.append(mContext.getText(
                            com.android.internal.R.string.needPuk2));
                } else if (err == CommandException.Error.REQUEST_NOT_SUPPORTED) {
                    if (mSc.equals(SC_PIN)) {
                        sb.append(mContext.getText(com.android.internal.R.string.enablePin));
                    }
                } else if (err == CommandException.Error.FDN_CHECK_FAILURE) {
                    Rlog.i(LOG_TAG, "FDN_CHECK_FAILURE");
                    sb.append(mContext.getText(com.android.internal.R.string.mmiFdnError));
                } else {
                    sb.append(getErrorMessage(ar));
                }
            } else {
                sb.append(mContext.getText(
                        com.android.internal.R.string.mmiError));
            }
        } else if (isActivate()) {
            mState = State.COMPLETE;
            if (mIsCallFwdReg) {
                sb.append(mContext.getText(
                        com.android.internal.R.string.serviceRegistered));
            } else {
                sb.append(mContext.getText(
                        com.android.internal.R.string.serviceEnabled));
            }
            // Record CLIR setting
            if (mSc.equals(SC_CLIR)) {
                mPhone.saveClirSetting(CommandsInterface.CLIR_INVOCATION);
            }
        } else if (isDeactivate()) {
            mState = State.COMPLETE;
            sb.append(mContext.getText(
                    com.android.internal.R.string.serviceDisabled));
            // Record CLIR setting
            if (mSc.equals(SC_CLIR)) {
                mPhone.saveClirSetting(CommandsInterface.CLIR_SUPPRESSION);
            }
        } else if (isRegister()) {
            mState = State.COMPLETE;
            sb.append(mContext.getText(
                    com.android.internal.R.string.serviceRegistered));
        } else if (isErasure()) {
            mState = State.COMPLETE;
            sb.append(mContext.getText(
                    com.android.internal.R.string.serviceErased));
        } else {
            mState = State.FAILED;
            sb.append(mContext.getText(
                    com.android.internal.R.string.mmiError));
        }

        mMessage = sb;
        mPhone.onMMIDone(this);
    }

    private void
    onGetClirComplete(AsyncResult ar) {
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");

        if (ar.exception != null) {
            mState = State.FAILED;
            sb.append(getErrorMessage(ar));
        } else {
            int clirArgs[];

            clirArgs = (int[])ar.result;

            // the 'm' parameter from TS 27.007 7.7
            switch (clirArgs[1]) {
                case 0: // CLIR not provisioned
                    sb.append(mContext.getText(
                                com.android.internal.R.string.serviceNotProvisioned));
                    mState = State.COMPLETE;
                break;

                case 1: // CLIR provisioned in permanent mode
                    sb.append(mContext.getText(
                                com.android.internal.R.string.CLIRPermanent));
                    mState = State.COMPLETE;
                break;

                case 2: // unknown (e.g. no network, etc.)
                    sb.append(mContext.getText(
                                com.android.internal.R.string.mmiError));
                    mState = State.FAILED;
                break;

                case 3: // CLIR temporary mode presentation restricted

                    // the 'n' parameter from TS 27.007 7.7
                    switch (clirArgs[0]) {
                        default:
                        case 0: // Default
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOnNextCallOn));
                        break;
                        case 1: // CLIR invocation
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOnNextCallOn));
                        break;
                        case 2: // CLIR suppression
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOnNextCallOff));
                        break;
                    }
                    mState = State.COMPLETE;
                break;

                case 4: // CLIR temporary mode presentation allowed
                    // the 'n' parameter from TS 27.007 7.7
                    switch (clirArgs[0]) {
                        default:
                        case 0: // Default
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOffNextCallOff));
                        break;
                        case 1: // CLIR invocation
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOffNextCallOn));
                        break;
                        case 2: // CLIR suppression
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOffNextCallOff));
                        break;
                    }

                    mState = State.COMPLETE;
                break;
            }
        }

        mMessage = sb;
        mPhone.onMMIDone(this);
    }

    /**
     * @param serviceClass 1 bit of the service class bit vectory
     * @return String to be used for call forward query MMI response text.
     *        Returns null if unrecognized
     */

    private CharSequence
    serviceClassToCFString (int serviceClass) {
        switch (serviceClass) {
            case SERVICE_CLASS_VOICE:
                return mContext.getText(com.android.internal.R.string.serviceClassVoice);
            case SERVICE_CLASS_DATA:
                return mContext.getText(com.android.internal.R.string.serviceClassData);
            case SERVICE_CLASS_FAX:
                return mContext.getText(com.android.internal.R.string.serviceClassFAX);
            case SERVICE_CLASS_SMS:
                return mContext.getText(com.android.internal.R.string.serviceClassSMS);
            case SERVICE_CLASS_DATA_SYNC:
                return mContext.getText(com.android.internal.R.string.serviceClassDataSync);
            case SERVICE_CLASS_DATA_ASYNC:
                return mContext.getText(com.android.internal.R.string.serviceClassDataAsync);
            case SERVICE_CLASS_PACKET:
                return mContext.getText(com.android.internal.R.string.serviceClassPacket);
            case SERVICE_CLASS_PAD:
                return mContext.getText(com.android.internal.R.string.serviceClassPAD);
            default:
                return null;
        }
    }


    /** one CallForwardInfo + serviceClassMask -> one line of text */
    private CharSequence
    makeCFQueryResultMessage(CallForwardInfo info, int serviceClassMask) {
        CharSequence template;
        String sources[] = {"{0}", "{1}", "{2}"};
        CharSequence destinations[] = new CharSequence[3];
        boolean needTimeTemplate;

        // CF_REASON_NO_REPLY also has a time value associated with
        // it. All others don't.

        needTimeTemplate =
            (info.reason == CommandsInterface.CF_REASON_NO_REPLY);

        if (info.status == 1) {
            if (needTimeTemplate) {
                template = mContext.getText(
                        com.android.internal.R.string.cfTemplateForwardedTime);
            } else {
                template = mContext.getText(
                        com.android.internal.R.string.cfTemplateForwarded);
            }
        } else if (info.status == 0 && isEmptyOrNull(info.number)) {
            template = mContext.getText(
                        com.android.internal.R.string.cfTemplateNotForwarded);
        } else { /* (info.status == 0) && !isEmptyOrNull(info.number) */
            // A call forward record that is not active but contains
            // a phone number is considered "registered"

            if (needTimeTemplate) {
                template = mContext.getText(
                        com.android.internal.R.string.cfTemplateRegisteredTime);
            } else {
                template = mContext.getText(
                        com.android.internal.R.string.cfTemplateRegistered);
            }
        }

        // In the template (from strings.xmls)
        //         {0} is one of "bearerServiceCode*"
        //        {1} is dialing number
        //      {2} is time in seconds

        destinations[0] = serviceClassToCFString(info.serviceClass & serviceClassMask);
        destinations[1] = formatLtr(
                PhoneNumberUtils.stringFromStringAndTOA(info.number, info.toa));
        destinations[2] = Integer.toString(info.timeSeconds);

        if (info.reason == CommandsInterface.CF_REASON_UNCONDITIONAL &&
                (info.serviceClass & serviceClassMask)
                        == CommandsInterface.SERVICE_CLASS_VOICE) {
            boolean cffEnabled = (info.status == 1);
            if (mIccRecords != null) {
                mPhone.setVoiceCallForwardingFlag(1, cffEnabled, info.number);
            }
        }

        return TextUtils.replace(template, sources, destinations);
    }

    /**
     * Used to format a string that should be displayed as LTR even in RTL locales
     */
    private String formatLtr(String str) {
        BidiFormatter fmt = BidiFormatter.getInstance();
        return str == null ? str : fmt.unicodeWrap(str, TextDirectionHeuristics.LTR, true);
    }

    private void
    onQueryCfComplete(AsyncResult ar) {
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");

        if (ar.exception != null) {
            mState = State.FAILED;
            sb.append(getErrorMessage(ar));
        } else {
            CallForwardInfo infos[];

            infos = (CallForwardInfo[]) ar.result;

            if (infos.length == 0) {
                // Assume the default is not active
                sb.append(mContext.getText(com.android.internal.R.string.serviceDisabled));

                // Set unconditional CFF in SIM to false
                if (mIccRecords != null) {
                    mPhone.setVoiceCallForwardingFlag(1, false, null);
                }
            } else {

                SpannableStringBuilder tb = new SpannableStringBuilder();

                // Each bit in the service class gets its own result line
                // The service classes may be split up over multiple
                // CallForwardInfos. So, for each service class, find out
                // which CallForwardInfo represents it and then build
                // the response text based on that

                for (int serviceClassMask = 1
                            ; serviceClassMask <= SERVICE_CLASS_MAX
                            ; serviceClassMask <<= 1
                ) {
                    for (int i = 0, s = infos.length; i < s ; i++) {
                        if ((serviceClassMask & infos[i].serviceClass) != 0) {
                            tb.append(makeCFQueryResultMessage(infos[i],
                                            serviceClassMask));
                            tb.append("\n");
                        }
                    }
                }
                sb.append(tb);
            }

            mState = State.COMPLETE;
        }

        mMessage = sb;
        mPhone.onMMIDone(this);

    }

    private void
    onQueryComplete(AsyncResult ar) {
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");

        if (ar.exception != null) {
            mState = State.FAILED;
            sb.append(getErrorMessage(ar));
        } else {
            int[] ints = (int[])ar.result;

            if (ints.length != 0) {
                if (ints[0] == 0) {
                    sb.append(mContext.getText(com.android.internal.R.string.serviceDisabled));
                } else if (mSc.equals(SC_WAIT)) {
                    // Call Waiting includes additional data in the response.
                    sb.append(createQueryCallWaitingResultMessage(ints[1]));
                } else if (isServiceCodeCallBarring(mSc)) {
                    // ints[0] for Call Barring is a bit vector of services
                    sb.append(createQueryCallBarringResultMessage(ints[0]));
                } else if (ints[0] == 1) {
                    // for all other services, treat it as a boolean
                    sb.append(mContext.getText(com.android.internal.R.string.serviceEnabled));
                } else {
                    sb.append(mContext.getText(com.android.internal.R.string.mmiError));
                }
            } else {
                sb.append(mContext.getText(com.android.internal.R.string.mmiError));
            }
            mState = State.COMPLETE;
        }

        mMessage = sb;
        mPhone.onMMIDone(this);
    }

    private CharSequence
    createQueryCallWaitingResultMessage(int serviceClass) {
        StringBuilder sb =
                new StringBuilder(mContext.getText(com.android.internal.R.string.serviceEnabledFor));

        for (int classMask = 1
                    ; classMask <= SERVICE_CLASS_MAX
                    ; classMask <<= 1
        ) {
            if ((classMask & serviceClass) != 0) {
                sb.append("\n");
                sb.append(serviceClassToCFString(classMask & serviceClass));
            }
        }
        return sb;
    }
    private CharSequence
    createQueryCallBarringResultMessage(int serviceClass)
    {
        StringBuilder sb = new StringBuilder(mContext.getText(com.android.internal.R.string.serviceEnabledFor));

        for (int classMask = 1
                    ; classMask <= SERVICE_CLASS_MAX
                    ; classMask <<= 1
        ) {
            if ((classMask & serviceClass) != 0) {
                sb.append("\n");
                sb.append(serviceClassToCFString(classMask & serviceClass));
            }
        }
        return sb;
    }

    /***
     * TODO: It would be nice to have a method here that can take in a dialstring and
     * figure out if there is an MMI code embedded within it.  This code would replace
     * some of the string parsing functionality in the Phone App's
     * SpecialCharSequenceMgr class.
     */

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("GsmMmiCode {");

        sb.append("State=" + getState());
        if (mAction != null) sb.append(" action=" + mAction);
        if (mSc != null) sb.append(" sc=" + mSc);
        if (mSia != null) sb.append(" sia=" + mSia);
        if (mSib != null) sb.append(" sib=" + mSib);
        if (mSic != null) sb.append(" sic=" + mSic);
        if (mPoundString != null) sb.append(" poundString=" + mPoundString);
        if (mDialingNumber != null) sb.append(" dialingNumber=" + mDialingNumber);
        if (mPwd != null) sb.append(" pwd=" + mPwd);
        sb.append("}");
        return sb.toString();
    }
}
