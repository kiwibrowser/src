/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.app.Activity;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.content.Intent;
import android.net.Uri;
import android.os.Message;
import android.os.SystemProperties;
import android.provider.Telephony.Sms;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.SmsManager;
import android.telephony.TelephonyManager;

import com.android.internal.telephony.GsmAlphabet;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.internal.telephony.ImsSMSDispatcher;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.SMSDispatcher;
import com.android.internal.telephony.SmsConstants;
import com.android.internal.telephony.SmsHeader;
import com.android.internal.telephony.SmsUsageMonitor;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.cdma.sms.UserData;

import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public class CdmaSMSDispatcher extends SMSDispatcher {
    private static final String TAG = "CdmaSMSDispatcher";
    private static final boolean VDBG = false;

    public CdmaSMSDispatcher(Phone phone, SmsUsageMonitor usageMonitor,
            ImsSMSDispatcher imsSMSDispatcher) {
        super(phone, usageMonitor, imsSMSDispatcher);
        Rlog.d(TAG, "CdmaSMSDispatcher created");
    }

    @Override
    public String getFormat() {
        return SmsConstants.FORMAT_3GPP2;
    }

    /**
     * Send the SMS status report to the dispatcher thread to process.
     * @param sms the CDMA SMS message containing the status report
     */
    public void sendStatusReportMessage(SmsMessage sms) {
        if (VDBG) Rlog.d(TAG, "sending EVENT_HANDLE_STATUS_REPORT message");
        sendMessage(obtainMessage(EVENT_HANDLE_STATUS_REPORT, sms));
    }

    @Override
    protected void handleStatusReport(Object o) {
        if (o instanceof SmsMessage) {
            if (VDBG) Rlog.d(TAG, "calling handleCdmaStatusReport()");
            handleCdmaStatusReport((SmsMessage) o);
        } else {
            Rlog.e(TAG, "handleStatusReport() called for object type " + o.getClass().getName());
        }
    }

    /**
     * Called from parent class to handle status report from {@code CdmaInboundSmsHandler}.
     * @param sms the CDMA SMS message to process
     */
    private void handleCdmaStatusReport(SmsMessage sms) {
        for (int i = 0, count = deliveryPendingList.size(); i < count; i++) {
            SmsTracker tracker = deliveryPendingList.get(i);
            if (tracker.mMessageRef == sms.mMessageRef) {
                // Found it.  Remove from list and broadcast.
                deliveryPendingList.remove(i);
                // Update the message status (COMPLETE)
                tracker.updateSentMessageStatus(mContext, Sms.STATUS_COMPLETE);

                PendingIntent intent = tracker.mDeliveryIntent;
                Intent fillIn = new Intent();
                fillIn.putExtra("pdu", sms.getPdu());
                fillIn.putExtra("format", getFormat());
                try {
                    intent.send(mContext, Activity.RESULT_OK, fillIn);
                } catch (CanceledException ex) {}
                break;  // Only expect to see one tracker matching this message.
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void sendData(String destAddr, String scAddr, int destPort,
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
                sendSubmitPdu(tracker);
            }
        } else {
            Rlog.e(TAG, "CdmaSMSDispatcher.sendData(): getSubmitPdu() returned null");
            if (sentIntent != null) {
                try {
                    sentIntent.send(SmsManager.RESULT_ERROR_GENERIC_FAILURE);
                } catch (CanceledException ex) {
                    Rlog.e(TAG, "Intent has been canceled!");
                }
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public void sendText(String destAddr, String scAddr, String text, PendingIntent sentIntent,
            PendingIntent deliveryIntent, Uri messageUri, String callingPkg,
            boolean persistMessage) {
        SmsMessage.SubmitPdu pdu = SmsMessage.getSubmitPdu(
                scAddr, destAddr, text, (deliveryIntent != null), null);
        if (pdu != null) {
            HashMap map = getSmsTrackerMap(destAddr, scAddr, text, pdu);
            SmsTracker tracker = getSmsTracker(map, sentIntent, deliveryIntent, getFormat(),
                    messageUri, false /*isExpectMore*/, text, true /*isText*/, persistMessage);

            String carrierPackage = getCarrierAppPackageName();
            if (carrierPackage != null) {
                Rlog.d(TAG, "Found carrier package.");
                TextSmsSender smsSender = new TextSmsSender(tracker);
                smsSender.sendSmsByCarrierApp(carrierPackage, new SmsSenderCallback(smsSender));
            } else {
                Rlog.v(TAG, "No carrier package.");
                sendSubmitPdu(tracker);
            }
        } else {
            Rlog.e(TAG, "CdmaSMSDispatcher.sendText(): getSubmitPdu() returned null");
            if (sentIntent != null) {
                try {
                    sentIntent.send(SmsManager.RESULT_ERROR_GENERIC_FAILURE);
                } catch (CanceledException ex) {
                    Rlog.e(TAG, "Intent has been canceled!");
                }
            }
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
        return SmsMessage.calculateLength(messageBody, use7bitOnly, false);
    }

    /** {@inheritDoc} */
    @Override
    protected SmsTracker getNewSubmitPduTracker(String destinationAddress, String scAddress,
            String message, SmsHeader smsHeader, int encoding,
            PendingIntent sentIntent, PendingIntent deliveryIntent, boolean lastPart,
            AtomicInteger unsentPartCount, AtomicBoolean anyPartFailed, Uri messageUri,
            String fullMessageText) {
        UserData uData = new UserData();
        uData.payloadStr = message;
        uData.userDataHeader = smsHeader;
        if (encoding == SmsConstants.ENCODING_7BIT) {
            uData.msgEncoding = UserData.ENCODING_GSM_7BIT_ALPHABET;
        } else { // assume UTF-16
            uData.msgEncoding = UserData.ENCODING_UNICODE_16;
        }
        uData.msgEncodingSet = true;

        /* By setting the statusReportRequested bit only for the
         * last message fragment, this will result in only one
         * callback to the sender when that last fragment delivery
         * has been acknowledged. */
        SmsMessage.SubmitPdu submitPdu = SmsMessage.getSubmitPdu(destinationAddress,
                uData, (deliveryIntent != null) && lastPart);

        HashMap map = getSmsTrackerMap(destinationAddress, scAddress,
                message, submitPdu);
        return getSmsTracker(map, sentIntent, deliveryIntent,
                getFormat(), unsentPartCount, anyPartFailed, messageUri, smsHeader,
                false /*isExpextMore*/, fullMessageText, true /*isText*/,
                true /*persistMessage*/);
    }

    @Override
    protected void sendSubmitPdu(SmsTracker tracker) {
        if (SystemProperties.getBoolean(TelephonyProperties.PROPERTY_INECM_MODE, false)) {
            if (VDBG) {
                Rlog.d(TAG, "Block SMS in Emergency Callback mode");
            }
            tracker.onFailed(mContext, SmsManager.RESULT_ERROR_NO_SERVICE, 0/*errorCode*/);
            return;
        }
        sendRawPdu(tracker);
    }

    /** {@inheritDoc} */
    @Override
    public void sendSms(SmsTracker tracker) {
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

        Message reply = obtainMessage(EVENT_SEND_SMS_COMPLETE, tracker);
        byte[] pdu = (byte[]) tracker.getData().get("pdu");

        int currentDataNetwork = mPhone.getServiceState().getDataNetworkType();
        boolean imsSmsDisabled = (currentDataNetwork == TelephonyManager.NETWORK_TYPE_EHRPD
                    || (ServiceState.isLte(currentDataNetwork)
                    && !mPhone.getServiceStateTracker().isConcurrentVoiceAndDataAllowed()))
                    && mPhone.getServiceState().getVoiceNetworkType()
                    == TelephonyManager.NETWORK_TYPE_1xRTT
                    && ((GsmCdmaPhone) mPhone).mCT.mState != PhoneConstants.State.IDLE;

        // sms over cdma is used:
        //   if sms over IMS is not supported AND
        //   this is not a retry case after sms over IMS failed
        //     indicated by mImsRetry > 0
        if (0 == tracker.mImsRetry && !isIms() || imsSmsDisabled) {
            mCi.sendCdmaSms(pdu, reply);
        } else {
            mCi.sendImsCdmaSms(pdu, tracker.mImsRetry, tracker.mMessageRef, reply);
            // increment it here, so in case of SMS_FAIL_RETRY over IMS
            // next retry will be sent using IMS request again.
            tracker.mImsRetry++;
        }
    }
}
