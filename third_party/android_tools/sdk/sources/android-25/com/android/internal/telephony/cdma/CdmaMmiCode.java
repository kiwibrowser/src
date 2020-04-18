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

package com.android.internal.telephony.cdma;

import android.content.Context;

import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppState;
import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.Phone;

import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.telephony.Rlog;

import java.util.regex.Pattern;
import java.util.regex.Matcher;

/**
 * This class can handle Puk code Mmi
 *
 * {@hide}
 *
 */
public final class CdmaMmiCode  extends Handler implements MmiCode {
    static final String LOG_TAG = "CdmaMmiCode";

    // Constants

    // From TS 22.030 6.5.2
    static final String ACTION_REGISTER = "**";

    // Supplementary Service codes for PIN/PIN2/PUK/PUK2 from TS 22.030 Annex B
    static final String SC_PIN          = "04";
    static final String SC_PIN2         = "042";
    static final String SC_PUK          = "05";
    static final String SC_PUK2         = "052";

    // Event Constant

    static final int EVENT_SET_COMPLETE = 1;

    // Instance Variables

    GsmCdmaPhone mPhone;
    Context mContext;
    UiccCardApplication mUiccApplication;

    String mAction;              // ACTION_REGISTER
    String mSc;                  // Service Code
    String mSia, mSib, mSic;     // Service Info a,b,c
    String mPoundString;         // Entire MMI string up to and including #
    String mDialingNumber;
    String mPwd;                 // For password registration

    State mState = State.PENDING;
    CharSequence mMessage;

    // Class Variables

    static Pattern sPatternSuppService = Pattern.compile(
        "((\\*|#|\\*#|\\*\\*|##)(\\d{2,3})(\\*([^*#]*)(\\*([^*#]*)(\\*([^*#]*)(\\*([^*#]*))?)?)?)?#)(.*)");
/*       1  2                    3          4  5       6   7         8    9     10  11             12

         1 = Full string up to and including #
         2 = action
         3 = service code
         5 = SIA
         7 = SIB
         9 = SIC
         10 = dialing number
*/

    static final int MATCH_GROUP_POUND_STRING = 1;
    static final int MATCH_GROUP_ACTION = 2;
    static final int MATCH_GROUP_SERVICE_CODE = 3;
    static final int MATCH_GROUP_SIA = 5;
    static final int MATCH_GROUP_SIB = 7;
    static final int MATCH_GROUP_SIC = 9;
    static final int MATCH_GROUP_PWD_CONFIRM = 11;
    static final int MATCH_GROUP_DIALING_NUMBER = 12;


    // Public Class methods

    /**
     * Check if provided string contains Mmi code in it and create corresponding
     * Mmi if it does
     */

    public static CdmaMmiCode
    newFromDialString(String dialString, GsmCdmaPhone phone, UiccCardApplication app) {
        Matcher m;
        CdmaMmiCode ret = null;

        m = sPatternSuppService.matcher(dialString);

        // Is this formatted like a standard supplementary service code?
        if (m.matches()) {
            ret = new CdmaMmiCode(phone,app);
            ret.mPoundString = makeEmptyNull(m.group(MATCH_GROUP_POUND_STRING));
            ret.mAction = makeEmptyNull(m.group(MATCH_GROUP_ACTION));
            ret.mSc = makeEmptyNull(m.group(MATCH_GROUP_SERVICE_CODE));
            ret.mSia = makeEmptyNull(m.group(MATCH_GROUP_SIA));
            ret.mSib = makeEmptyNull(m.group(MATCH_GROUP_SIB));
            ret.mSic = makeEmptyNull(m.group(MATCH_GROUP_SIC));
            ret.mPwd = makeEmptyNull(m.group(MATCH_GROUP_PWD_CONFIRM));
            ret.mDialingNumber = makeEmptyNull(m.group(MATCH_GROUP_DIALING_NUMBER));

        }

        return ret;
    }

    // Private Class methods

    /** make empty strings be null.
     *  Regexp returns empty strings for empty groups
     */
    private static String
    makeEmptyNull (String s) {
        if (s != null && s.length() == 0) return null;

        return s;
    }

    // Constructor

    CdmaMmiCode (GsmCdmaPhone phone, UiccCardApplication app) {
        super(phone.getHandler().getLooper());
        mPhone = phone;
        mContext = phone.getContext();
        mUiccApplication = app;
    }

    // MmiCode implementation

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
        mPhone.onMMIDone (this);
    }

    @Override
    public boolean isCancelable() {
        return false;
    }

    // Instance Methods

    /**
     * @return true if the Service Code is PIN/PIN2/PUK/PUK2-related
     */
    public boolean isPinPukCommand() {
        return mSc != null && (mSc.equals(SC_PIN) || mSc.equals(SC_PIN2)
                              || mSc.equals(SC_PUK) || mSc.equals(SC_PUK2));
    }

    boolean isRegister() {
        return mAction != null && mAction.equals(ACTION_REGISTER);
    }

    @Override
    public boolean isUssdRequest() {
        Rlog.w(LOG_TAG, "isUssdRequest is not implemented in CdmaMmiCode");
        return false;
    }

    /** Process a MMI PUK code */
    public void
    processCode() {
        try {
            if (isPinPukCommand()) {
                // TODO: This is the same as the code in GsmMmiCode.java,
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
                            throw new RuntimeException("Unsupported service code=" + mSc);
                        }
                    } else {
                        throw new RuntimeException("No application mUiccApplicaiton is null");
                    }
                } else {
                    throw new RuntimeException ("Ivalid register/action=" + mAction);
                }
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

    @Override
    public void
    handleMessage (Message msg) {
        AsyncResult ar;

        if (msg.what == EVENT_SET_COMPLETE) {
            ar = (AsyncResult) (msg.obj);
            onSetComplete(msg, ar);
        } else {
            Rlog.e(LOG_TAG, "Unexpected reply");
        }
    }
    // Private instance methods

    private CharSequence getScString() {
        if (mSc != null) {
            if (isPinPukCommand()) {
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
                } else {
                    sb.append(mContext.getText(
                            com.android.internal.R.string.mmiError));
                }
            } else {
                sb.append(mContext.getText(
                        com.android.internal.R.string.mmiError));
            }
        } else if (isRegister()) {
            mState = State.COMPLETE;
            sb.append(mContext.getText(
                    com.android.internal.R.string.serviceRegistered));
        } else {
            mState = State.FAILED;
            sb.append(mContext.getText(
                    com.android.internal.R.string.mmiError));
        }

        mMessage = sb;
        mPhone.onMMIDone(this);
    }

}
