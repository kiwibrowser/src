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

import android.app.Activity;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Message;
import android.provider.Telephony.Sms;
import android.provider.Telephony.Sms.Intents;
import android.telephony.Rlog;
import android.telephony.ServiceState;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.GsmAlphabet;
import com.android.internal.telephony.ImsSMSDispatcher;
import com.android.internal.telephony.InboundSmsHandler;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.SMSDispatcher;
import com.android.internal.telephony.SmsConstants;
import com.android.internal.telephony.SmsHeader;
import com.android.internal.telephony.SmsUsageMonitor;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;

import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

public final class GsmSMSDispatcher extends SMSDispatcher {
    private static final String TAG = "GsmSMSDispatcher";
    private static final boolean VDBG = false;
    protected UiccController mUiccController = null;
    private AtomicReference<IccRecords> mIccRecords = new AtomicReference<IccRecords>();
    private AtomicReference<UiccCardApplication> mUiccApplication =
            new AtomicReference<UiccCardApplication>();
    private GsmInboundSmsHandler mGsmInboundSmsHandler;

    /** Status report received */
    private static final int EVENT_NEW_SMS_STATUS_REPORT = 100;

    public GsmSMSDispatcher(Phone phone, SmsUsageMonitor usageMonitor,
            ImsSMSDispatcher imsSMSDispatcher,
            GsmInboundSmsHandler gsmInboundSmsHandler) {
        super(phone, usageMonitor, imsSMSDispatcher);
        mCi.setOnSmsStatus(this, EVENT_NEW_SMS_STATUS_REPORT, null);
        mGsmInboundSmsHandler = gsmInboundSmsHandler;
        mUiccController = UiccController.getInstance();
        mUiccController.registerForIccChanged(this, EVENT_ICC_CHANGED, null);
        Rlog.d(TAG, "GsmSMSDispatcher created");
    }

    @Override
    public void dispose() {
        super.dispose();
        mCi.unSetOnSmsStatus(this);
        mUiccController.unregisterForIccChanged(this);
    }

    @Override
    protected String getFormat() {
        return SmsConstants.FORMAT_3GPP;
    }

    /**
     * Handles 3GPP format-specific events coming from the phone stack.
     * Other events are handled by {@link SMSDispatcher#handleMessage}.
     *
     * @param msg the message to handle
     */
    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
        case EVENT_NEW_SMS_STATUS_REPORT:
            handleStatusReport((AsyncResult) msg.obj);
            break;

        case EVENT_NEW_ICC_SMS:
        // pass to InboundSmsHandler to process
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS, msg.obj);
        break;

        case EVENT_ICC_CHANGED:
            onUpdateIccAvailability();
            break;

        default:
            super.handleMessage(msg);
        }
    }

    /**
     * Called when a status report is received.  This should correspond to
     * a previously successful SEND.
     *
     * @param ar AsyncResult passed into the message handler.  ar.result should
     *           be a String representing the status report PDU, as ASCII hex.
     */
    private void handleStatusReport(AsyncResult ar) {
        String pduString = (String) ar.result;
        SmsMessage sms = SmsMessage.newFromCDS(pduString);

        if (sms != null) {
            int tpStatus = sms.getStatus();
            int messageRef = sms.mMessageRef;
            for (int i = 0, count = deliveryPendingList.size(); i < count; i++) {
                SmsTracker tracker = deliveryPendingList.get(i);
                if (tracker.mMessageRef == messageRef) {
                    // Found it.  Remove from list and broadcast.
                    if(tpStatus >= Sms.STATUS_FAILED || tpStatus < Sms.STATUS_PENDING ) {
                       deliveryPendingList.remove(i);
                       // Update the message status (COMPLETE or FAILED)
                       tracker.updateSentMessageStatus(mContext, tpStatus);
                    }
                    PendingIntent intent = tracker.mDeliveryIntent;
                    Intent fillIn = new Intent();
                    fillIn.putExtra("pdu", IccUtils.hexStringToBytes(pduString));
                    fillIn.putExtra("format", getFormat());
                    try {
                        intent.send(mContext, Activity.RESULT_OK, fillIn);
                    } catch (CanceledException ex) {}

                    // Only expect to see one tracker matching this messageref
                    break;
                }
            }
        }
        mCi.acknowledgeLastIncomingGsmSms(true, Intents.RESULT_SMS_HANDLED, null);
    }

    /** {@inheritDoc} */
    @Override
    protected void sendData(String destAddr, String scAddr, int destPort,
            byte[] data, PendingIntent sentIntent, PendingIntent deliveryIntent) {
        SmsMessage.SubmitPdu pdu = SmsMessage.getSubmitPdu(
                scAddr, destAddr, destPort, data, (deliveryIntent != null));
        if (pdu != null) {
            HashMap map = getSmsTrackerMap(destAddr, scAddr, destPort, data, pdu);
            SmsTracker tracker = getSmsTracker(map, sentIntent, deliveryIntent, getFormat(),
                    null /*messageUri*/, false /*isExpectMore*/, null /*fullMessageText*/,
                    false /*isText*/, true /*persistMessage*/);

            String carrierPackage = getCarrierAppPackageName();
            if (carrierPackage != null) {
                Rlog.d(TAG, "Found carrier package.");
                DataSmsSender smsSender = new DataSmsSender(tracker);
                smsSender.sendSmsByCarrierApp(carrierPackage, new SmsSenderCallback(smsSender));
            } else {
                Rlog.v(TAG, "No carrier package.");
                sendRawPdu(tracker);
            }
        } else {
            Rlog.e(TAG, "GsmSMSDispatcher.sendData(): getSubmitPdu() returned null");
        }
    }

    /** {@inheritDoc} */
    @VisibleForTesting
    @Override
    public void sendText(String destAddr, String scAddr, String text, PendingIntent sentIntent,
            PendingIntent deliveryIntent, Uri messageUri, String callingPkg,
            boolean persistMessage) {
        SmsMessage.SubmitPdu pdu = SmsMessage.getSubmitPdu(
                scAddr, destAddr, text, (deliveryIntent != null));
        if (pdu != null) {
            HashMap map = getSmsTrackerMap(destAddr, scAddr, text, pdu);
            SmsTracker tracker = getSmsTracker(map, sentIntent, deliveryIntent, getFormat(),
                    messageUri, false /*isExpectMore*/, text /*fullMessageText*/, true /*isText*/,
                    persistMessage);

            String carrierPackage = getCarrierAppPackageName();
            if (carrierPackage != null) {
                Rlog.d(TAG, "Found carrier package.");
                TextSmsSender smsSender = new TextSmsSender(tracker);
                smsSender.sendSmsByCarrierApp(carrierPackage, new SmsSenderCallback(smsSender));
            } else {
                Rlog.v(TAG, "No carrier package.");
                sendRawPdu(tracker);
            }
        } else {
            Rlog.e(TAG, "GsmSMSDispatcher.sendText(): getSubmitPdu() returned null");
        }
    }

    /** {@inheritDoc} */
    @Override
    protected void injectSmsPdu(byte[] pdu, String format, PendingIntent receivedIntent) {
        throw new IllegalStateException("This method must be called only on ImsSMSDispatcher");
    }

    /** {@inheritDoc} */
    @Override
    protected GsmAlphabet.TextEncodingDetails calculateLength(CharSequence messageBody,
            boolean use7bitOnly) {
        return SmsMessage.calculateLength(messageBody, use7bitOnly);
    }

    /** {@inheritDoc} */
    @Override
    protected SmsTracker getNewSubmitPduTracker(String destinationAddress, String scAddress,
            String message, SmsHeader smsHeader, int encoding,
            PendingIntent sentIntent, PendingIntent deliveryIntent, boolean lastPart,
            AtomicInteger unsentPartCount, AtomicBoolean anyPartFailed, Uri messageUri,
            String fullMessageText) {
        SmsMessage.SubmitPdu pdu = SmsMessage.getSubmitPdu(scAddress, destinationAddress,
                message, deliveryIntent != null, SmsHeader.toByteArray(smsHeader),
                encoding, smsHeader.languageTable, smsHeader.languageShiftTable);
        if (pdu != null) {
            HashMap map =  getSmsTrackerMap(destinationAddress, scAddress,
                    message, pdu);
            return getSmsTracker(map, sentIntent,
                    deliveryIntent, getFormat(), unsentPartCount, anyPartFailed, messageUri,
                    smsHeader, !lastPart, fullMessageText, true /*isText*/,
                    false /*persistMessage*/);
        } else {
            Rlog.e(TAG, "GsmSMSDispatcher.sendNewSubmitPdu(): getSubmitPdu() returned null");
            return null;
        }
    }

    @Override
    protected void sendSubmitPdu(SmsTracker tracker) {
        sendRawPdu(tracker);
    }

    /** {@inheritDoc} */
    @Override
    protected void sendSms(SmsTracker tracker) {
        HashMap<String, Object> map = tracker.getData();

        byte pdu[] = (byte[]) map.get("pdu");

        if (tracker.mRetryCount > 0) {
            Rlog.d(TAG, "sendSms: "
                    + " mRetryCount=" + tracker.mRetryCount
                    + " mMessageRef=" + tracker.mMessageRef
                    + " SS=" + mPhone.getServiceState().getState());

            // per TS 23.040 Section 9.2.3.6:  If TP-MTI SMS-SUBMIT (0x01) type
            //   TP-RD (bit 2) is 1 for retry
            //   and TP-MR is set to previously failed sms TP-MR
            if (((0x01 & pdu[0]) == 0x01)) {
                pdu[0] |= 0x04; // TP-RD
                pdu[1] = (byte) tracker.mMessageRef; // TP-MR
            }
        }
        Rlog.d(TAG, "sendSms: "
                + " isIms()=" + isIms()
                + " mRetryCount=" + tracker.mRetryCount
                + " mImsRetry=" + tracker.mImsRetry
                + " mMessageRef=" + tracker.mMessageRef
                + " SS=" + mPhone.getServiceState().getState());

        sendSmsByPstn(tracker);
    }

    /** {@inheritDoc} */
    @Override
    protected void sendSmsByPstn(SmsTracker tracker) {
        int ss = mPhone.getServiceState().getState();
        // if sms over IMS is not supported on data and voice is not available...
        if (!isIms() && ss != ServiceState.STATE_IN_SERVICE) {
            tracker.onFailed(mContext, getNotInServiceError(ss), 0/*errorCode*/);
            return;
        }

        HashMap<String, Object> map = tracker.getData();

        byte smsc[] = (byte[]) map.get("smsc");
        byte[] pdu = (byte[]) map.get("pdu");
        Message reply = obtainMessage(EVENT_SEND_SMS_COMPLETE, tracker);

        // sms over gsm is used:
        //   if sms over IMS is not supported AND
        //   this is not a retry case after sms over IMS failed
        //     indicated by mImsRetry > 0
        if (0 == tracker.mImsRetry && !isIms()) {
            if (tracker.mRetryCount > 0) {
                // per TS 23.040 Section 9.2.3.6:  If TP-MTI SMS-SUBMIT (0x01) type
                //   TP-RD (bit 2) is 1 for retry
                //   and TP-MR is set to previously failed sms TP-MR
                if (((0x01 & pdu[0]) == 0x01)) {
                    pdu[0] |= 0x04; // TP-RD
                    pdu[1] = (byte) tracker.mMessageRef; // TP-MR
                }
            }
            if (tracker.mRetryCount == 0 && tracker.mExpectMore) {
                mCi.sendSMSExpectMore(IccUtils.bytesToHexString(smsc),
                        IccUtils.bytesToHexString(pdu), reply);
            } else {
                mCi.sendSMS(IccUtils.bytesToHexString(smsc),
                        IccUtils.bytesToHexString(pdu), reply);
            }
        } else {
            mCi.sendImsGsmSms(IccUtils.bytesToHexString(smsc),
                    IccUtils.bytesToHexString(pdu), tracker.mImsRetry,
                    tracker.mMessageRef, reply);
            // increment it here, so in case of SMS_FAIL_RETRY over IMS
            // next retry will be sent using IMS request again.
            tracker.mImsRetry++;
        }
    }

    protected UiccCardApplication getUiccCardApplication() {
            Rlog.d(TAG, "GsmSMSDispatcher: subId = " + mPhone.getSubId()
                    + " slotId = " + mPhone.getPhoneId());
                return mUiccController.getUiccCardApplication(mPhone.getPhoneId(),
                        UiccController.APP_FAM_3GPP);
    }

    private void onUpdateIccAvailability() {
        if (mUiccController == null ) {
            return;
        }

        UiccCardApplication newUiccApplication = getUiccCardApplication();

        UiccCardApplication app = mUiccApplication.get();
        if (app != newUiccApplication) {
            if (app != null) {
                Rlog.d(TAG, "Removing stale icc objects.");
                if (mIccRecords.get() != null) {
                    mIccRecords.get().unregisterForNewSms(this);
                }
                mIccRecords.set(null);
                mUiccApplication.set(null);
            }
            if (newUiccApplication != null) {
                Rlog.d(TAG, "New Uicc application found");
                mUiccApplication.set(newUiccApplication);
                mIccRecords.set(newUiccApplication.getIccRecords());
                if (mIccRecords.get() != null) {
                    mIccRecords.get().registerForNewSms(this, EVENT_NEW_ICC_SMS, null);
                }
            }
        }
    }
}
