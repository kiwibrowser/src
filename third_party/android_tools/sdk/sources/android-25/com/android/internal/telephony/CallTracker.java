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

package com.android.internal.telephony;

import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.text.TextUtils;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;


/**
 * {@hide}
 */
public abstract class CallTracker extends Handler {

    private static final boolean DBG_POLL = false;

    //***** Constants

    static final int POLL_DELAY_MSEC = 250;

    protected int mPendingOperations;
    protected boolean mNeedsPoll;
    protected Message mLastRelevantPoll;
    protected ArrayList<Connection> mHandoverConnections = new ArrayList<Connection>();

    public CommandsInterface mCi;

    protected boolean mNumberConverted = false;
    private final int VALID_COMPARE_LENGTH   = 3;

    //***** Events

    protected static final int EVENT_POLL_CALLS_RESULT             = 1;
    protected static final int EVENT_CALL_STATE_CHANGE             = 2;
    protected static final int EVENT_REPOLL_AFTER_DELAY            = 3;
    protected static final int EVENT_OPERATION_COMPLETE            = 4;
    protected static final int EVENT_GET_LAST_CALL_FAIL_CAUSE      = 5;

    protected static final int EVENT_SWITCH_RESULT                 = 8;
    protected static final int EVENT_RADIO_AVAILABLE               = 9;
    protected static final int EVENT_RADIO_NOT_AVAILABLE           = 10;
    protected static final int EVENT_CONFERENCE_RESULT             = 11;
    protected static final int EVENT_SEPARATE_RESULT               = 12;
    protected static final int EVENT_ECT_RESULT                    = 13;
    protected static final int EVENT_EXIT_ECM_RESPONSE_CDMA        = 14;
    protected static final int EVENT_CALL_WAITING_INFO_CDMA        = 15;
    protected static final int EVENT_THREE_WAY_DIAL_L2_RESULT_CDMA = 16;
    protected static final int EVENT_THREE_WAY_DIAL_BLANK_FLASH    = 20;

    protected void pollCallsWhenSafe() {
        mNeedsPoll = true;

        if (checkNoOperationsPending()) {
            mLastRelevantPoll = obtainMessage(EVENT_POLL_CALLS_RESULT);
            mCi.getCurrentCalls(mLastRelevantPoll);
        }
    }

    protected void
    pollCallsAfterDelay() {
        Message msg = obtainMessage();

        msg.what = EVENT_REPOLL_AFTER_DELAY;
        sendMessageDelayed(msg, POLL_DELAY_MSEC);
    }

    protected boolean
    isCommandExceptionRadioNotAvailable(Throwable e) {
        return e != null && e instanceof CommandException
                && ((CommandException)e).getCommandError()
                        == CommandException.Error.RADIO_NOT_AVAILABLE;
    }

    protected abstract void handlePollCalls(AsyncResult ar);

    protected Connection getHoConnection(DriverCall dc) {
        for (Connection hoConn : mHandoverConnections) {
            log("getHoConnection - compare number: hoConn= " + hoConn.toString());
            if (hoConn.getAddress() != null && hoConn.getAddress().contains(dc.number)) {
                log("getHoConnection: Handover connection match found = " + hoConn.toString());
                return hoConn;
            }
        }
        for (Connection hoConn : mHandoverConnections) {
            log("getHoConnection: compare state hoConn= " + hoConn.toString());
            if (hoConn.getStateBeforeHandover() == Call.stateFromDCState(dc.state)) {
                log("getHoConnection: Handover connection match found = " + hoConn.toString());
                return hoConn;
            }
        }
        return null;
    }

    protected void notifySrvccState(Call.SrvccState state, ArrayList<Connection> c) {
        if (state == Call.SrvccState.STARTED && c != null) {
            // SRVCC started. Prepare handover connections list
            mHandoverConnections.addAll(c);
        } else if (state != Call.SrvccState.COMPLETED) {
            // SRVCC FAILED/CANCELED. Clear the handover connections list
            // Individual connections will be removed from the list in handlePollCalls()
            mHandoverConnections.clear();
        }
        log("notifySrvccState: mHandoverConnections= " + mHandoverConnections.toString());
    }

    protected void handleRadioAvailable() {
        pollCallsWhenSafe();
    }

    /**
     * Obtain a complete message that indicates that this operation
     * does not require polling of getCurrentCalls(). However, if other
     * operations that do need getCurrentCalls() are pending or are
     * scheduled while this operation is pending, the invocation
     * of getCurrentCalls() will be postponed until this
     * operation is also complete.
     */
    protected Message
    obtainNoPollCompleteMessage(int what) {
        mPendingOperations++;
        mLastRelevantPoll = null;
        return obtainMessage(what);
    }

    /**
     * @return true if we're idle or there's a call to getCurrentCalls() pending
     * but nothing else
     */
    private boolean
    checkNoOperationsPending() {
        if (DBG_POLL) log("checkNoOperationsPending: pendingOperations=" +
                mPendingOperations);
        return mPendingOperations == 0;
    }

    /**
     * Routine called from dial to check if the number is a test Emergency number
     * and if so remap the number. This allows a short emergency number to be remapped
     * to a regular number for testing how the frameworks handles emergency numbers
     * without actually calling an emergency number.
     *
     * This is not a full test and is not a substitute for testing real emergency
     * numbers but can be useful.
     *
     * To use this feature set a system property ril.test.emergencynumber to a pair of
     * numbers separated by a colon. If the first number matches the number parameter
     * this routine returns the second number. Example:
     *
     * ril.test.emergencynumber=112:1-123-123-45678
     *
     * To test Dial 112 take call then hang up on MO device to enter ECM
     * see RIL#processSolicited RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND
     *
     * @param dialString to test if it should be remapped
     * @return the same number or the remapped number.
     */
    protected String checkForTestEmergencyNumber(String dialString) {
        String testEn = SystemProperties.get("ril.test.emergencynumber");
        if (DBG_POLL) {
            log("checkForTestEmergencyNumber: dialString=" + dialString +
                " testEn=" + testEn);
        }
        if (!TextUtils.isEmpty(testEn)) {
            String values[] = testEn.split(":");
            log("checkForTestEmergencyNumber: values.length=" + values.length);
            if (values.length == 2) {
                if (values[0].equals(
                        android.telephony.PhoneNumberUtils.stripSeparators(dialString))) {
                    // mCi will be null for ImsPhoneCallTracker.
                    if (mCi != null) {
                        mCi.testingEmergencyCall();
                    }
                    log("checkForTestEmergencyNumber: remap " +
                            dialString + " to " + values[1]);
                    dialString = values[1];
                }
            }
        }
        return dialString;
    }

    protected String convertNumberIfNecessary(Phone phone, String dialNumber) {
        if (dialNumber == null) {
            return dialNumber;
        }
        String[] convertMaps = phone.getContext().getResources().getStringArray(
                com.android.internal.R.array.dial_string_replace);
        log("convertNumberIfNecessary Roaming"
            + " convertMaps.length " + convertMaps.length
            + " dialNumber.length() " + dialNumber.length());

        if (convertMaps.length < 1 || dialNumber.length() < VALID_COMPARE_LENGTH) {
            return dialNumber;
        }

        String[] entry;
        String[] tmpArray;
        String outNumber = "";
        boolean needConvert = false;
        for(String convertMap : convertMaps) {
            log("convertNumberIfNecessary: " + convertMap);
            entry = convertMap.split(":");
            if (entry.length > 1) {
                tmpArray = entry[1].split(",");
                if (!TextUtils.isEmpty(entry[0]) && dialNumber.equals(entry[0])) {
                    if (tmpArray.length >= 2 && !TextUtils.isEmpty(tmpArray[1])) {
                        if (compareGid1(phone, tmpArray[1])) {
                            needConvert = true;
                        }
                    } else if (outNumber.isEmpty()) {
                        needConvert = true;
                    }

                    if (needConvert) {
                        if(!TextUtils.isEmpty(tmpArray[0]) && tmpArray[0].endsWith("MDN")) {
                            String mdn = phone.getLine1Number();
                            if (!TextUtils.isEmpty(mdn) ) {
                                if (mdn.startsWith("+")) {
                                    outNumber = mdn;
                                } else {
                                    outNumber = tmpArray[0].substring(0, tmpArray[0].length() -3)
                                            + mdn;
                                }
                            }
                        } else {
                            outNumber = tmpArray[0];
                        }
                        needConvert = false;
                    }
                }
            }
        }

        if (!TextUtils.isEmpty(outNumber)) {
            log("convertNumberIfNecessary: convert service number");
            mNumberConverted = true;
            return outNumber;
        }

        return dialNumber;

    }

    private boolean compareGid1(Phone phone, String serviceGid1) {
        String gid1 = phone.getGroupIdLevel1();
        int gid_length = serviceGid1.length();
        boolean ret = true;

        if (serviceGid1 == null || serviceGid1.equals("")) {
            log("compareGid1 serviceGid is empty, return " + ret);
            return ret;
        }
        // Check if gid1 match service GID1
        if (!((gid1 != null) && (gid1.length() >= gid_length) &&
                gid1.substring(0, gid_length).equalsIgnoreCase(serviceGid1))) {
            log(" gid1 " + gid1 + " serviceGid1 " + serviceGid1);
            ret = false;
        }
        log("compareGid1 is " + (ret?"Same":"Different"));
        return ret;
    }

    //***** Overridden from Handler
    @Override
    public abstract void handleMessage (Message msg);
    public abstract void registerForVoiceCallStarted(Handler h, int what, Object obj);
    public abstract void unregisterForVoiceCallStarted(Handler h);
    public abstract void registerForVoiceCallEnded(Handler h, int what, Object obj);
    public abstract void unregisterForVoiceCallEnded(Handler h);
    public abstract PhoneConstants.State getState();
    protected abstract void log(String msg);

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("CallTracker:");
        pw.println(" mPendingOperations=" + mPendingOperations);
        pw.println(" mNeedsPoll=" + mNeedsPoll);
        pw.println(" mLastRelevantPoll=" + mLastRelevantPoll);
    }
}
