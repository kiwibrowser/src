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
import android.content.res.Resources;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.telephony.PhoneNumberUtils;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.telephony.Rlog;

import com.android.ims.ImsException;
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsSsInfo;
import com.android.ims.ImsUtInterface;
import com.android.internal.telephony.CallForwardInfo;
import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.uicc.IccRecords;

import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_NONE;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_VOICE;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_DATA;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_FAX;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_SMS;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_DATA_SYNC;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_DATA_ASYNC;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_PACKET;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_PAD;
import static com.android.internal.telephony.CommandsInterface.SERVICE_CLASS_MAX;

import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.Phone;

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
public final class ImsPhoneMmiCode extends Handler implements MmiCode {
    static final String LOG_TAG = "ImsPhoneMmiCode";

    //***** Constants

    // Max Size of the Short Code (aka Short String from TS 22.030 6.5.2)
    private static final int MAX_LENGTH_SHORT_CODE = 2;

    // TS 22.030 6.5.2 Every Short String USSD command will end with #-key
    // (known as #-String)
    private static final char END_OF_USSD_COMMAND = '#';

    // From TS 22.030 6.5.2
    private static final String ACTION_ACTIVATE = "*";
    private static final String ACTION_DEACTIVATE = "#";
    private static final String ACTION_INTERROGATE = "*#";
    private static final String ACTION_REGISTER = "**";
    private static final String ACTION_ERASURE = "##";

    // Supp Service codes from TS 22.030 Annex B

    //Called line presentation
    private static final String SC_CLIP    = "30";
    private static final String SC_CLIR    = "31";
    private static final String SC_COLP    = "76";
    private static final String SC_COLR    = "77";

    //Calling name presentation
    private static final String SC_CNAP    = "300";

    // Call Forwarding
    private static final String SC_CFU     = "21";
    private static final String SC_CFB     = "67";
    private static final String SC_CFNRy   = "61";
    private static final String SC_CFNR    = "62";
    // Call Forwarding unconditional Timer
    private static final String SC_CFUT     = "22";

    private static final String SC_CF_All = "002";
    private static final String SC_CF_All_Conditional = "004";

    // Call Waiting
    private static final String SC_WAIT     = "43";

    // Call Barring
    private static final String SC_BAOC         = "33";
    private static final String SC_BAOIC        = "331";
    private static final String SC_BAOICxH      = "332";
    private static final String SC_BAIC         = "35";
    private static final String SC_BAICr        = "351";

    private static final String SC_BA_ALL       = "330";
    private static final String SC_BA_MO        = "333";
    private static final String SC_BA_MT        = "353";

    // Incoming/Anonymous call barring
    private static final String SC_BS_MT        = "156";
    private static final String SC_BAICa        = "157";

    // Supp Service Password registration
    private static final String SC_PWD          = "03";

    // PIN/PIN2/PUK/PUK2
    private static final String SC_PIN          = "04";
    private static final String SC_PIN2         = "042";
    private static final String SC_PUK          = "05";
    private static final String SC_PUK2         = "052";

    //***** Event Constants

    private static final int EVENT_SET_COMPLETE            = 0;
    private static final int EVENT_QUERY_CF_COMPLETE       = 1;
    private static final int EVENT_USSD_COMPLETE           = 2;
    private static final int EVENT_QUERY_COMPLETE          = 3;
    private static final int EVENT_SET_CFF_COMPLETE        = 4;
    private static final int EVENT_USSD_CANCEL_COMPLETE    = 5;
    private static final int EVENT_GET_CLIR_COMPLETE       = 6;
    private static final int EVENT_SUPP_SVC_QUERY_COMPLETE = 7;
    private static final int EVENT_QUERY_ICB_COMPLETE      = 10;

    //***** Calling Line Presentation Constants
    private static final int NUM_PRESENTATION_ALLOWED     = 0;
    private static final int NUM_PRESENTATION_RESTRICTED  = 1;

    //***** Supplementary Service Query Bundle Keys
    // Used by IMS Service layer to put supp. serv. query
    // responses into the ssInfo Bundle.
    public static final String UT_BUNDLE_KEY_CLIR = "queryClir";
    public static final String UT_BUNDLE_KEY_SSINFO = "imsSsInfo";

    //***** Calling Line Identity Restriction Constants
    // The 'm' parameter from TS 27.007 7.7
    private static final int CLIR_NOT_PROVISIONED                    = 0;
    private static final int CLIR_PROVISIONED_PERMANENT              = 1;
    private static final int CLIR_PRESENTATION_RESTRICTED_TEMPORARY  = 3;
    private static final int CLIR_PRESENTATION_ALLOWED_TEMPORARY     = 4;
    // The 'n' parameter from TS 27.007 7.7
    private static final int CLIR_DEFAULT     = 0;
    private static final int CLIR_INVOCATION  = 1;
    private static final int CLIR_SUPPRESSION = 2;

    //***** Instance Variables

    private ImsPhone mPhone;
    private Context mContext;
    private IccRecords mIccRecords;

    private String mAction;              // One of ACTION_*
    private String mSc;                  // Service Code
    private String mSia, mSib, mSic;       // Service Info a,b,c
    private String mPoundString;         // Entire MMI string up to and including #
    private String mDialingNumber;
    private String mPwd;                 // For password registration

    private boolean mIsPendingUSSD;

    private boolean mIsUssdRequest;

    private boolean mIsCallFwdReg;
    private State mState = State.PENDING;
    private CharSequence mMessage;
    //resgister/erasure of ICB (Specific DN)
    static final String IcbDnMmi = "Specific Incoming Call Barring";
    //ICB (Anonymous)
    static final String IcbAnonymousMmi = "Anonymous Incoming Call Barring";
    //***** Class Variables


    // See TS 22.030 6.5.2 "Structure of the MMI"

    private static Pattern sPatternSuppService = Pattern.compile(
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

    private static final int MATCH_GROUP_POUND_STRING = 1;

    private static final int MATCH_GROUP_ACTION = 2;
                        //(activation/interrogation/registration/erasure)

    private static final int MATCH_GROUP_SERVICE_CODE = 3;
    private static final int MATCH_GROUP_SIA = 5;
    private static final int MATCH_GROUP_SIB = 7;
    private static final int MATCH_GROUP_SIC = 9;
    private static final int MATCH_GROUP_PWD_CONFIRM = 11;
    private static final int MATCH_GROUP_DIALING_NUMBER = 12;
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

    static ImsPhoneMmiCode
    newFromDialString(String dialString, ImsPhone phone) {
        Matcher m;
        ImsPhoneMmiCode ret = null;

        m = sPatternSuppService.matcher(dialString);

        // Is this formatted like a standard supplementary service code?
        if (m.matches()) {
            ret = new ImsPhoneMmiCode(phone);
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
            if (ret.mDialingNumber != null &&
                    ret.mDialingNumber.endsWith("#") &&
                    dialString.endsWith("#")){
                ret = new ImsPhoneMmiCode(phone);
                ret.mPoundString = dialString;
            }
        } else if (dialString.endsWith("#")) {
            // TS 22.030 sec 6.5.3.2
            // "Entry of any characters defined in the 3GPP TS 23.038 [8] Default Alphabet
            // (up to the maximum defined in 3GPP TS 24.080 [10]), followed by #SEND".

            ret = new ImsPhoneMmiCode(phone);
            ret.mPoundString = dialString;
        } else if (isTwoDigitShortCode(phone.getContext(), dialString)) {
            //Is a country-specific exception to short codes as defined in TS 22.030, 6.5.3.2
            ret = null;
        } else if (isShortCode(dialString, phone)) {
            // this may be a short code, as defined in TS 22.030, 6.5.3.2
            ret = new ImsPhoneMmiCode(phone);
            ret.mDialingNumber = dialString;
        }

        return ret;
    }

    static ImsPhoneMmiCode
    newNetworkInitiatedUssd(String ussdMessage, boolean isUssdRequest, ImsPhone phone) {
        ImsPhoneMmiCode ret;

        ret = new ImsPhoneMmiCode(phone);

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

    static ImsPhoneMmiCode newFromUssdUserInput(String ussdMessge, ImsPhone phone) {
        ImsPhoneMmiCode ret = new ImsPhoneMmiCode(phone);

        ret.mMessage = ussdMessge;
        ret.mState = State.PENDING;
        ret.mIsPendingUSSD = true;

        return ret;
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

    static boolean isScMatchesSuppServType(String dialString) {
        boolean isMatch = false;
        Matcher m = sPatternSuppService.matcher(dialString);
        if (m.matches()) {
            String sc = makeEmptyNull(m.group(MATCH_GROUP_SERVICE_CODE));
            if (sc.equals(SC_CFUT)) {
                isMatch = true;
            } else if(sc.equals(SC_BS_MT)) {
                isMatch = true;
            }
        }
        return isMatch;
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

    ImsPhoneMmiCode(ImsPhone phone) {
        // The telephony unit-test cases may create ImsPhoneMmiCode's
        // in secondary threads
        super(phone.getHandler().getLooper());
        mPhone = phone;
        mContext = phone.getContext();
        mIccRecords = mPhone.mDefaultPhone.getIccRecords();
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

    @Override
    public Phone getPhone() { return mPhone; }

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
            mPhone.cancelUSSD();
        } else {
            mPhone.onMMIDone (this);
        }

    }

    @Override
    public boolean isCancelable() {
        /* Can only cancel pending USSD sessions. */
        return mIsPendingUSSD;
    }

    //***** Instance Methods

    String getDialingNumber() {
        return mDialingNumber;
    }

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
    static private boolean isShortCode(String dialString, ImsPhone phone) {
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
    static private boolean isShortCodeUSSD(String dialString, ImsPhone phone) {
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
    boolean
    isTemporaryModeCLIR() {
        return mSc != null && mSc.equals(SC_CLIR) && mDialingNumber != null
                && (isActivate() || isDeactivate());
    }

    /**
     * returns CommandsInterface.CLIR_*
     * See also isTemporaryModeCLIR()
     */
    int
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

    boolean
    isSupportedOverImsPhone() {
        if (isShortCode()) return true;
        else if (mDialingNumber != null) return false;
        else if (isServiceCodeCallForwarding(mSc)
                || isServiceCodeCallBarring(mSc)
                || (mSc != null && mSc.equals(SC_WAIT))
                || (mSc != null && mSc.equals(SC_CLIR))
                || (mSc != null && mSc.equals(SC_CLIP))
                || (mSc != null && mSc.equals(SC_COLR))
                || (mSc != null && mSc.equals(SC_COLP))
                || (mSc != null && mSc.equals(SC_BS_MT))
                || (mSc != null && mSc.equals(SC_BAICa))) {

            try {
                int serviceClass = siToServiceClass(mSib);
                if (serviceClass != SERVICE_CLASS_NONE
                        && serviceClass != SERVICE_CLASS_VOICE) {
                    return false;
                }
                return true;
            } catch (RuntimeException exc) {
                Rlog.d(LOG_TAG, "Invalid service class " + exc);
            }
        } else if (isPinPukCommand()
                || (mSc != null
                    && (mSc.equals(SC_PWD) || mSc.equals(SC_CLIP) || mSc.equals(SC_CLIR)))) {
            return false;
        } else if (mPoundString != null) return true;

        return false;
    }

    /*
     * The below actions are IMS/Volte CallBarring actions.We have not defined
     * these actions in ImscommandInterface.However we have reused existing
     * actions of CallForwarding as, both CF and CB actions are used for same
     * purpose.
     */
    public int callBarAction(String dialingNumber) {
        if (isActivate()) {
            return CommandsInterface.CF_ACTION_ENABLE;
        } else if (isDeactivate()) {
            return CommandsInterface.CF_ACTION_DISABLE;
        } else if (isRegister()) {
            if (!isEmptyOrNull(dialingNumber)) {
                return CommandsInterface.CF_ACTION_REGISTRATION;
            } else {
                throw new RuntimeException ("invalid action");
            }
        } else if (isErasure()) {
            return CommandsInterface.CF_ACTION_ERASURE;
        } else {
            throw new RuntimeException ("invalid action");
        }
    }

    /** Process a MMI code or short code...anything that isn't a dialing number */
    public void
    processCode () throws CallStateException {
        try {
            if (isShortCode()) {
                Rlog.d(LOG_TAG, "isShortCode");

                // These just get treated as USSD.
                Rlog.d(LOG_TAG, "Sending short code '"
                       + mDialingNumber + "' over CS pipe.");
                throw new CallStateException(Phone.CS_FALLBACK);
            } else if (isServiceCodeCallForwarding(mSc)) {
                Rlog.d(LOG_TAG, "is CF");

                String dialingNumber = mSia;
                int reason = scToCallForwardReason(mSc);
                int serviceClass = siToServiceClass(mSib);
                int time = siToTime(mSic);

                if (isInterrogate()) {
                    mPhone.getCallForwardingOption(reason,
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

                    int isSettingUnconditional =
                            ((reason == CommandsInterface.CF_REASON_UNCONDITIONAL) ||
                             (reason == CommandsInterface.CF_REASON_ALL)) ? 1 : 0;

                    int isEnableDesired =
                        ((cfAction == CommandsInterface.CF_ACTION_ENABLE) ||
                                (cfAction == CommandsInterface.CF_ACTION_REGISTRATION)) ? 1 : 0;

                    Rlog.d(LOG_TAG, "is CF setCallForward");
                    mPhone.setCallForwardingOption(cfAction, reason,
                            dialingNumber, serviceClass, time, obtainMessage(
                                    EVENT_SET_CFF_COMPLETE,
                                    isSettingUnconditional,
                                    isEnableDesired, this));
                }
            } else if (isServiceCodeCallBarring(mSc)) {
                // sia = password
                // sib = basic service group
                // service group is not supported

                String password = mSia;
                String facility = scToBarringFacility(mSc);

                if (isInterrogate()) {
                    mPhone.getCallBarring(facility,
                            obtainMessage(EVENT_SUPP_SVC_QUERY_COMPLETE, this));
                } else if (isActivate() || isDeactivate()) {
                    mPhone.setCallBarring(facility, isActivate(), password,
                            obtainMessage(EVENT_SET_COMPLETE, this));
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }
            } else if (mSc != null && mSc.equals(SC_CLIR)) {
                // NOTE: Since these supplementary services are accessed only
                //       via MMI codes, methods have not been added to ImsPhone.
                //       Only the UT interface handle is used.
                if (isActivate()) {
                    try {
                        mPhone.mCT.getUtInterface().updateCLIR(CommandsInterface.CLIR_INVOCATION,
                            obtainMessage(EVENT_SET_COMPLETE, this));
                    } catch (ImsException e) {
                        Rlog.d(LOG_TAG, "Could not get UT handle for updateCLIR.");
                    }
                } else if (isDeactivate()) {
                    try {
                        mPhone.mCT.getUtInterface().updateCLIR(CommandsInterface.CLIR_SUPPRESSION,
                            obtainMessage(EVENT_SET_COMPLETE, this));
                    } catch (ImsException e) {
                        Rlog.d(LOG_TAG, "Could not get UT handle for updateCLIR.");
                    }
                } else if (isInterrogate()) {
                    try {
                        mPhone.mCT.getUtInterface()
                            .queryCLIR(obtainMessage(EVENT_GET_CLIR_COMPLETE, this));
                    } catch (ImsException e) {
                        Rlog.d(LOG_TAG, "Could not get UT handle for queryCLIR.");
                    }
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }
            } else if (mSc != null && mSc.equals(SC_CLIP)) {
                // NOTE: Refer to the note above.
                if (isInterrogate()) {
                    try {
                        mPhone.mCT.getUtInterface()
                            .queryCLIP(obtainMessage(EVENT_SUPP_SVC_QUERY_COMPLETE, this));
                    } catch (ImsException e) {
                        Rlog.d(LOG_TAG, "Could not get UT handle for queryCLIP.");
                    }
                } else if (isActivate() || isDeactivate()) {
                    try {
                        mPhone.mCT.getUtInterface().updateCLIP(isActivate(),
                                obtainMessage(EVENT_SET_COMPLETE, this));
                    } catch (ImsException e) {
                        Rlog.d(LOG_TAG, "Could not get UT handle for updateCLIP.");
                    }
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }
            } else if (mSc != null && mSc.equals(SC_COLP)) {
                // NOTE: Refer to the note above.
                if (isInterrogate()) {
                    try {
                        mPhone.mCT.getUtInterface()
                            .queryCOLP(obtainMessage(EVENT_SUPP_SVC_QUERY_COMPLETE, this));
                    } catch (ImsException e) {
                        Rlog.d(LOG_TAG, "Could not get UT handle for queryCOLP.");
                    }
                } else if (isActivate() || isDeactivate()) {
                    try {
                        mPhone.mCT.getUtInterface().updateCOLP(isActivate(),
                                 obtainMessage(EVENT_SET_COMPLETE, this));
                     } catch (ImsException e) {
                         Rlog.d(LOG_TAG, "Could not get UT handle for updateCOLP.");
                     }
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }
            } else if (mSc != null && mSc.equals(SC_COLR)) {
                // NOTE: Refer to the note above.
                if (isActivate()) {
                    try {
                        mPhone.mCT.getUtInterface().updateCOLR(NUM_PRESENTATION_RESTRICTED,
                                obtainMessage(EVENT_SET_COMPLETE, this));
                    } catch (ImsException e) {
                        Rlog.d(LOG_TAG, "Could not get UT handle for updateCOLR.");
                    }
                } else if (isDeactivate()) {
                    try {
                        mPhone.mCT.getUtInterface().updateCOLR(NUM_PRESENTATION_ALLOWED,
                                obtainMessage(EVENT_SET_COMPLETE, this));
                    } catch (ImsException e) {
                        Rlog.d(LOG_TAG, "Could not get UT handle for updateCOLR.");
                    }
                } else if (isInterrogate()) {
                    try {
                        mPhone.mCT.getUtInterface()
                            .queryCOLR(obtainMessage(EVENT_SUPP_SVC_QUERY_COMPLETE, this));
                    } catch (ImsException e) {
                        Rlog.d(LOG_TAG, "Could not get UT handle for queryCOLR.");
                    }
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }
            } else if (mSc != null && (mSc.equals(SC_BS_MT))) {
                try {
                    if (isInterrogate()) {
                        mPhone.mCT.getUtInterface()
                        .queryCallBarring(ImsUtInterface.CB_BS_MT,
                                          obtainMessage(EVENT_QUERY_ICB_COMPLETE,this));
                    } else {
                        processIcbMmiCodeForUpdate();
                    }
                 // TODO: isRegister() case needs to be handled.
                } catch (ImsException e) {
                    Rlog.d(LOG_TAG, "Could not get UT handle for ICB.");
                }
            } else if (mSc != null && mSc.equals(SC_BAICa)) {
                int callAction =0;
                // TODO: Should we route through queryCallBarring() here?
                try {
                    if (isInterrogate()) {
                        mPhone.mCT.getUtInterface()
                        .queryCallBarring(ImsUtInterface.CB_BIC_ACR,
                                          obtainMessage(EVENT_QUERY_ICB_COMPLETE,this));
                    } else {
                        if (isActivate()) {
                            callAction = CommandsInterface.CF_ACTION_ENABLE;
                        } else if (isDeactivate()) {
                            callAction = CommandsInterface.CF_ACTION_DISABLE;
                        }
                        mPhone.mCT.getUtInterface()
                                .updateCallBarring(ImsUtInterface.CB_BIC_ACR,
                                callAction,
                                obtainMessage(EVENT_SET_COMPLETE,this),
                                null);
                    }
                } catch (ImsException e) {
                    Rlog.d(LOG_TAG, "Could not get UT handle for ICBa.");
                }
            } else if (mSc != null && mSc.equals(SC_WAIT)) {
                // sia = basic service group
                int serviceClass = siToServiceClass(mSib);

                if (isActivate() || isDeactivate()) {
                    mPhone.setCallWaiting(isActivate(), serviceClass,
                            obtainMessage(EVENT_SET_COMPLETE, this));
                } else if (isInterrogate()) {
                    mPhone.getCallWaiting(obtainMessage(EVENT_QUERY_COMPLETE, this));
                } else {
                    throw new RuntimeException ("Invalid or Unsupported MMI Code");
                }
            } else if (mPoundString != null) {
                Rlog.d(LOG_TAG, "Sending pound string '"
                       + mDialingNumber + "' over CS pipe.");
                throw new CallStateException(Phone.CS_FALLBACK);
            } else {
                throw new RuntimeException ("Invalid or Unsupported MMI Code");
            }
        } catch (RuntimeException exc) {
            mState = State.FAILED;
            mMessage = mContext.getText(com.android.internal.R.string.mmiError);
            mPhone.onMMIDone(this);
        }
    }

    /**
     * Called from ImsPhone
     *
     * An unsolicited USSD NOTIFY or REQUEST has come in matching
     * up with this pending USSD request
     *
     * Note: If REQUEST, this exchange is complete, but the session remains
     *       active (ie, the network expects user input).
     */
    void
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
     * Called from ImsPhone
     *
     * The radio has reset, and this is still pending
     */

    void
    onUssdFinishedError() {
        if (mState == State.PENDING) {
            mState = State.FAILED;
            mMessage = mContext.getText(com.android.internal.R.string.mmiError);

            mPhone.onMMIDone(this);
        }
    }

    void sendUssd(String ussdMessage) {
        // Treat this as a USSD string
        mIsPendingUSSD = true;

        // Note that unlike most everything else, the USSD complete
        // response does not complete this MMI code...we wait for
        // an unsolicited USSD "Notify" or "Request".
        // The matching up of this is done in ImsPhone.

        mPhone.sendUSSD(ussdMessage,
            obtainMessage(EVENT_USSD_COMPLETE, this));
    }

    /** Called from ImsPhone.handleMessage; not a Handler subclass */
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
                // The matching up of this is done in ImsPhone.

                break;

            case EVENT_USSD_CANCEL_COMPLETE:
                mPhone.onMMIDone(this);
                break;

            case EVENT_SUPP_SVC_QUERY_COMPLETE:
                ar = (AsyncResult) (msg.obj);
                onSuppSvcQueryComplete(ar);
                break;

            case EVENT_QUERY_ICB_COMPLETE:
                ar = (AsyncResult) (msg.obj);
                onIcbQueryComplete(ar);
                break;

            case EVENT_GET_CLIR_COMPLETE:
                ar = (AsyncResult) (msg.obj);
                onQueryClirComplete(ar);
                break;

            default:
                break;
        }
    }

    //***** Private instance methods

    private void
    processIcbMmiCodeForUpdate () {
        String dialingNumber = mSia;
        String[] icbNum = null;
        int callAction;
        if (dialingNumber != null) {
            icbNum = dialingNumber.split("\\$");
        }
        callAction = callBarAction(dialingNumber);

        try {
            mPhone.mCT.getUtInterface()
            .updateCallBarring(ImsUtInterface.CB_BS_MT,
                               callAction,
                               obtainMessage(EVENT_SET_COMPLETE,this),
                               icbNum);
        } catch (ImsException e) {
            Rlog.d(LOG_TAG, "Could not get UT handle for updating ICB.");
        }
    }

    private CharSequence getErrorMessage(AsyncResult ar) {
        return mContext.getText(com.android.internal.R.string.mmiError);
    }

    private CharSequence getScString() {
        if (mSc != null) {
            if (isServiceCodeCallBarring(mSc)) {
                return mContext.getText(com.android.internal.R.string.BaMmi);
            } else if (isServiceCodeCallForwarding(mSc)) {
                return mContext.getText(com.android.internal.R.string.CfMmi);
            } else if (mSc.equals(SC_PWD)) {
                return mContext.getText(com.android.internal.R.string.PwdMmi);
            } else if (mSc.equals(SC_WAIT)) {
                return mContext.getText(com.android.internal.R.string.CwMmi);
            } else if (mSc.equals(SC_CLIP)) {
                return mContext.getText(com.android.internal.R.string.ClipMmi);
            } else if (mSc.equals(SC_CLIR)) {
                return mContext.getText(com.android.internal.R.string.ClirMmi);
            } else if (mSc.equals(SC_COLP)) {
                return mContext.getText(com.android.internal.R.string.ColpMmi);
            } else if (mSc.equals(SC_COLR)) {
                return mContext.getText(com.android.internal.R.string.ColrMmi);
            } else if (mSc.equals(SC_BS_MT)) {
                return IcbDnMmi;
            } else if (mSc.equals(SC_BAICa)) {
                return IcbAnonymousMmi;
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
                CommandException err = (CommandException) ar.exception;
                if (err.getCommandError() == CommandException.Error.PASSWORD_INCORRECT) {
                    sb.append(mContext.getText(
                            com.android.internal.R.string.passwordIncorrect));
                } else if (err.getMessage() != null) {
                    sb.append(err.getMessage());
                } else {
                    sb.append(mContext.getText(com.android.internal.R.string.mmiError));
                }
            } else {
                ImsException error = (ImsException) ar.exception;
                if (error.getMessage() != null) {
                    sb.append(error.getMessage());
                } else {
                    sb.append(getErrorMessage(ar));
                }
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
        } else if (isDeactivate()) {
            mState = State.COMPLETE;
            sb.append(mContext.getText(
                    com.android.internal.R.string.serviceDisabled));
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
        destinations[1] = PhoneNumberUtils.stringFromStringAndTOA(info.number, info.toa);
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


    private void
    onQueryCfComplete(AsyncResult ar) {
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");

        if (ar.exception != null) {
            mState = State.FAILED;

            if (ar.exception instanceof ImsException) {
                ImsException error = (ImsException) ar.exception;
                if (error.getMessage() != null) {
                    sb.append(error.getMessage());
                } else {
                    sb.append(getErrorMessage(ar));
                }
            }
            else {
                sb.append(getErrorMessage(ar));
            }
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

    private void onSuppSvcQueryComplete(AsyncResult ar) {
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");

        if (ar.exception != null) {
            mState = State.FAILED;

            if (ar.exception instanceof ImsException) {
                ImsException error = (ImsException) ar.exception;
                if (error.getMessage() != null) {
                    sb.append(error.getMessage());
                } else {
                    sb.append(getErrorMessage(ar));
                }
            } else {
                sb.append(getErrorMessage(ar));
            }
        } else {
            mState = State.FAILED;
            ImsSsInfo ssInfo = null;
            if (ar.result instanceof Bundle) {
                Rlog.d(LOG_TAG, "Received CLIP/COLP/COLR Response.");
                // Response for CLIP, COLP and COLR queries.
                Bundle ssInfoResp = (Bundle) ar.result;
                ssInfo = (ImsSsInfo) ssInfoResp.getParcelable(UT_BUNDLE_KEY_SSINFO);
                if (ssInfo != null) {
                    Rlog.d(LOG_TAG, "ImsSsInfo mStatus = " + ssInfo.mStatus);
                    if (ssInfo.mStatus == ImsSsInfo.DISABLED) {
                        sb.append(mContext.getText(com.android.internal.R.string.serviceDisabled));
                        mState = State.COMPLETE;
                    } else if (ssInfo.mStatus == ImsSsInfo.ENABLED) {
                        sb.append(mContext.getText(com.android.internal.R.string.serviceEnabled));
                        mState = State.COMPLETE;
                    } else {
                        sb.append(mContext.getText(com.android.internal.R.string.mmiError));
                    }
                } else {
                    sb.append(mContext.getText(com.android.internal.R.string.mmiError));
                }

            } else {
                Rlog.d(LOG_TAG, "Received Call Barring Response.");
                // Response for Call Barring queries.
                int[] cbInfos = (int[]) ar.result;
                if (cbInfos[0] == 1) {
                    sb.append(mContext.getText(com.android.internal.R.string.serviceEnabled));
                    mState = State.COMPLETE;
                } else {
                    sb.append(mContext.getText(com.android.internal.R.string.serviceDisabled));
                    mState = State.COMPLETE;
                }
            }
        }

        mMessage = sb;
        mPhone.onMMIDone(this);
    }

    private void onIcbQueryComplete(AsyncResult ar) {
        Rlog.d(LOG_TAG, "onIcbQueryComplete ");
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");

        if (ar.exception != null) {
            mState = State.FAILED;

            if (ar.exception instanceof ImsException) {
                ImsException error = (ImsException) ar.exception;
                if (error.getMessage() != null) {
                    sb.append(error.getMessage());
                } else {
                    sb.append(getErrorMessage(ar));
                }
            } else {
                sb.append(getErrorMessage(ar));
            }
        } else {
            ImsSsInfo[] infos = (ImsSsInfo[])ar.result;
            if (infos.length == 0) {
                sb.append(mContext.getText(com.android.internal.R.string.serviceDisabled));
            } else {
                for (int i = 0, s = infos.length; i < s ; i++) {
                    if (infos[i].mIcbNum !=null) {
                        sb.append("Num: " + infos[i].mIcbNum + " status: "
                                + infos[i].mStatus + "\n");
                    } else if (infos[i].mStatus == 1) {
                        sb.append(mContext.getText(com.android.internal
                                .R.string.serviceEnabled));
                    } else {
                        sb.append(mContext.getText(com.android.internal
                                .R.string.serviceDisabled));
                    }
                }
            }
            mState = State.COMPLETE;
        }
        mMessage = sb;
        mPhone.onMMIDone(this);
    }

    private void onQueryClirComplete(AsyncResult ar) {
        StringBuilder sb = new StringBuilder(getScString());
        sb.append("\n");
        mState = State.FAILED;

        if (ar.exception != null) {

            if (ar.exception instanceof ImsException) {
                ImsException error = (ImsException) ar.exception;
                if (error.getMessage() != null) {
                    sb.append(error.getMessage());
                } else {
                    sb.append(getErrorMessage(ar));
                }
            }
        } else {
            Bundle ssInfo = (Bundle) ar.result;
            int[] clirInfo = ssInfo.getIntArray(UT_BUNDLE_KEY_CLIR);
            // clirInfo[0] = The 'n' parameter from TS 27.007 7.7
            // clirInfo[1] = The 'm' parameter from TS 27.007 7.7
            Rlog.d(LOG_TAG, "CLIR param n=" + clirInfo[0]
                    + " m=" + clirInfo[1]);

            // 'm' parameter.
            switch (clirInfo[1]) {
                case CLIR_NOT_PROVISIONED:
                    sb.append(mContext.getText(
                            com.android.internal.R.string.serviceNotProvisioned));
                    mState = State.COMPLETE;
                    break;
                case CLIR_PROVISIONED_PERMANENT:
                    sb.append(mContext.getText(
                            com.android.internal.R.string.CLIRPermanent));
                    mState = State.COMPLETE;
                    break;
                case CLIR_PRESENTATION_RESTRICTED_TEMPORARY:
                    // 'n' parameter.
                    switch (clirInfo[0]) {
                        case CLIR_DEFAULT:
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOnNextCallOn));
                            mState = State.COMPLETE;
                            break;
                        case CLIR_INVOCATION:
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOnNextCallOn));
                            mState = State.COMPLETE;
                            break;
                        case CLIR_SUPPRESSION:
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOnNextCallOff));
                            mState = State.COMPLETE;
                            break;
                        default:
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.mmiError));
                            mState = State.FAILED;
                    }
                    break;
                case CLIR_PRESENTATION_ALLOWED_TEMPORARY:
                    // 'n' parameter.
                    switch (clirInfo[0]) {
                        case CLIR_DEFAULT:
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOffNextCallOff));
                            mState = State.COMPLETE;
                            break;
                        case CLIR_INVOCATION:
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOffNextCallOn));
                            mState = State.COMPLETE;
                            break;
                        case CLIR_SUPPRESSION:
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.CLIRDefaultOffNextCallOff));
                            mState = State.COMPLETE;
                            break;
                        default:
                            sb.append(mContext.getText(
                                    com.android.internal.R.string.mmiError));
                            mState = State.FAILED;
                    }
                    break;
                default:
                    sb.append(mContext.getText(
                            com.android.internal.R.string.mmiError));
                    mState = State.FAILED;
            }
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

            if (ar.exception instanceof ImsException) {
                ImsException error = (ImsException) ar.exception;
                if (error.getMessage() != null) {
                    sb.append(error.getMessage());
                } else {
                    sb.append(getErrorMessage(ar));
                }
            } else {
                sb.append(getErrorMessage(ar));
            }

        } else {
            int[] ints = (int[])ar.result;

            if (ints.length != 0) {
                if (ints[0] == 0) {
                    sb.append(mContext.getText(com.android.internal.R.string.serviceDisabled));
                } else if (mSc.equals(SC_WAIT)) {
                    // Call Waiting includes additional data in the response.
                    sb.append(createQueryCallWaitingResultMessage(ints[1]));
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
        StringBuilder sb = new StringBuilder(
                mContext.getText(com.android.internal.R.string.serviceEnabledFor));

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
        StringBuilder sb = new StringBuilder("ImsPhoneMmiCode {");

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
