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

import static android.telephony.SmsManager.RESULT_ERROR_GENERIC_FAILURE;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Message;
import android.provider.Telephony.Sms.Intents;
import android.telephony.Rlog;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.cdma.CdmaInboundSmsHandler;
import com.android.internal.telephony.cdma.CdmaSMSDispatcher;
import com.android.internal.telephony.gsm.GsmInboundSmsHandler;
import com.android.internal.telephony.gsm.GsmSMSDispatcher;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public class ImsSMSDispatcher extends SMSDispatcher {
    private static final String TAG = "RIL_ImsSms";

    private SMSDispatcher mCdmaDispatcher;
    private SMSDispatcher mGsmDispatcher;

    private GsmInboundSmsHandler mGsmInboundSmsHandler;
    private CdmaInboundSmsHandler mCdmaInboundSmsHandler;


    /** true if IMS is registered and sms is supported, false otherwise.*/
    private boolean mIms = false;
    private String mImsSmsFormat = SmsConstants.FORMAT_UNKNOWN;

    public ImsSMSDispatcher(Phone phone, SmsStorageMonitor storageMonitor,
            SmsUsageMonitor usageMonitor) {
        super(phone, usageMonitor, null);
        Rlog.d(TAG, "ImsSMSDispatcher created");

        // Create dispatchers, inbound SMS handlers and
        // broadcast undelivered messages in raw table.
        mCdmaDispatcher = new CdmaSMSDispatcher(phone, usageMonitor, this);
        mGsmInboundSmsHandler = GsmInboundSmsHandler.makeInboundSmsHandler(phone.getContext(),
                storageMonitor, phone);
        mCdmaInboundSmsHandler = CdmaInboundSmsHandler.makeInboundSmsHandler(phone.getContext(),
                storageMonitor, phone, (CdmaSMSDispatcher) mCdmaDispatcher);
        mGsmDispatcher = new GsmSMSDispatcher(phone, usageMonitor, this, mGsmInboundSmsHandler);
        SmsBroadcastUndelivered.initialize(phone.getContext(),
            mGsmInboundSmsHandler, mCdmaInboundSmsHandler);
        InboundSmsHandler.registerNewMessageNotificationActionHandler(phone.getContext());

        mCi.registerForOn(this, EVENT_RADIO_ON, null);
        mCi.registerForImsNetworkStateChanged(this, EVENT_IMS_STATE_CHANGED, null);
    }

    /* Updates the phone object when there is a change */
    @Override
    protected void updatePhoneObject(Phone phone) {
        Rlog.d(TAG, "In IMS updatePhoneObject ");
        super.updatePhoneObject(phone);
        mCdmaDispatcher.updatePhoneObject(phone);
        mGsmDispatcher.updatePhoneObject(phone);
        mGsmInboundSmsHandler.updatePhoneObject(phone);
        mCdmaInboundSmsHandler.updatePhoneObject(phone);
    }

    public void dispose() {
        mCi.unregisterForOn(this);
        mCi.unregisterForImsNetworkStateChanged(this);
        mGsmDispatcher.dispose();
        mCdmaDispatcher.dispose();
        mGsmInboundSmsHandler.dispose();
        mCdmaInboundSmsHandler.dispose();
    }

    /**
     * Handles events coming from the phone stack. Overridden from handler.
     *
     * @param msg the message to handle
     */
    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;

        switch (msg.what) {
        case EVENT_RADIO_ON:
        case EVENT_IMS_STATE_CHANGED: // received unsol
            mCi.getImsRegistrationState(this.obtainMessage(EVENT_IMS_STATE_DONE));
            break;

        case EVENT_IMS_STATE_DONE:
            ar = (AsyncResult) msg.obj;

            if (ar.exception == null) {
                updateImsInfo(ar);
            } else {
                Rlog.e(TAG, "IMS State query failed with exp "
                        + ar.exception);
            }
            break;

        default:
            super.handleMessage(msg);
        }
    }

    private void setImsSmsFormat(int format) {
        // valid format?
        switch (format) {
            case PhoneConstants.PHONE_TYPE_GSM:
                mImsSmsFormat = "3gpp";
                break;
            case PhoneConstants.PHONE_TYPE_CDMA:
                mImsSmsFormat = "3gpp2";
                break;
            default:
                mImsSmsFormat = "unknown";
                break;
        }
    }

    private void updateImsInfo(AsyncResult ar) {
        int[] responseArray = (int[])ar.result;

        mIms = false;
        if (responseArray[0] == 1) {  // IMS is registered
            Rlog.d(TAG, "IMS is registered!");
            mIms = true;
        } else {
            Rlog.d(TAG, "IMS is NOT registered!");
        }

        setImsSmsFormat(responseArray[1]);

        if (("unknown".equals(mImsSmsFormat))) {
            Rlog.e(TAG, "IMS format was unknown!");
            // failed to retrieve valid IMS SMS format info, set IMS to unregistered
            mIms = false;
        }
    }

    @Override
    public void sendData(String destAddr, String scAddr, int destPort,
            byte[] data, PendingIntent sentIntent, PendingIntent deliveryIntent) {
        if (isCdmaMo()) {
            mCdmaDispatcher.sendData(destAddr, scAddr, destPort,
                    data, sentIntent, deliveryIntent);
        } else {
            mGsmDispatcher.sendData(destAddr, scAddr, destPort,
                    data, sentIntent, deliveryIntent);
        }
    }

    @Override
    public void sendMultipartText(String destAddr, String scAddr,
            ArrayList<String> parts, ArrayList<PendingIntent> sentIntents,
            ArrayList<PendingIntent> deliveryIntents, Uri messageUri, String callingPkg,
            boolean persistMessage) {
        if (isCdmaMo()) {
            mCdmaDispatcher.sendMultipartText(destAddr, scAddr,
                    parts, sentIntents, deliveryIntents, messageUri, callingPkg, persistMessage);
        } else {
            mGsmDispatcher.sendMultipartText(destAddr, scAddr,
                    parts, sentIntents, deliveryIntents, messageUri, callingPkg, persistMessage);
        }
    }

    @Override
    protected void sendSms(SmsTracker tracker) {
        //  sendSms is a helper function to other send functions, sendText/Data...
        //  it is not part of ISms.stub
        Rlog.e(TAG, "sendSms should never be called from here!");
    }

    @Override
    protected void sendSmsByPstn(SmsTracker tracker) {
        // This function should be defined in Gsm/CdmaDispatcher.
        Rlog.e(TAG, "sendSmsByPstn should never be called from here!");
    }

    @Override
    public void sendText(String destAddr, String scAddr, String text, PendingIntent sentIntent,
            PendingIntent deliveryIntent, Uri messageUri, String callingPkg,
            boolean persistMessage) {
        Rlog.d(TAG, "sendText");
        if (isCdmaMo()) {
            mCdmaDispatcher.sendText(destAddr, scAddr,
                    text, sentIntent, deliveryIntent, messageUri, callingPkg, persistMessage);
        } else {
            mGsmDispatcher.sendText(destAddr, scAddr,
                    text, sentIntent, deliveryIntent, messageUri, callingPkg, persistMessage);
        }
    }

    @VisibleForTesting
    @Override
    public void injectSmsPdu(byte[] pdu, String format, PendingIntent receivedIntent) {
        Rlog.d(TAG, "ImsSMSDispatcher:injectSmsPdu");
        try {
            // TODO We need to decide whether we should allow injecting GSM(3gpp)
            // SMS pdus when the phone is camping on CDMA(3gpp2) network and vice versa.
            android.telephony.SmsMessage msg =
                    android.telephony.SmsMessage.createFromPdu(pdu, format);

            // Only class 1 SMS are allowed to be injected.
            if (msg == null ||
                    msg.getMessageClass() != android.telephony.SmsMessage.MessageClass.CLASS_1) {
                if (msg == null) {
                    Rlog.e(TAG, "injectSmsPdu: createFromPdu returned null");
                }
                if (receivedIntent != null) {
                    receivedIntent.send(Intents.RESULT_SMS_GENERIC_ERROR);
                }
                return;
            }

            AsyncResult ar = new AsyncResult(receivedIntent, msg, null);

            if (format.equals(SmsConstants.FORMAT_3GPP)) {
                Rlog.i(TAG, "ImsSMSDispatcher:injectSmsText Sending msg=" + msg +
                        ", format=" + format + "to mGsmInboundSmsHandler");
                mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_INJECT_SMS, ar);
            } else if (format.equals(SmsConstants.FORMAT_3GPP2)) {
                Rlog.i(TAG, "ImsSMSDispatcher:injectSmsText Sending msg=" + msg +
                        ", format=" + format + "to mCdmaInboundSmsHandler");
                mCdmaInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_INJECT_SMS, ar);
            } else {
                // Invalid pdu format.
                Rlog.e(TAG, "Invalid pdu format: " + format);
                if (receivedIntent != null)
                    receivedIntent.send(Intents.RESULT_SMS_GENERIC_ERROR);
            }
        } catch (Exception e) {
            Rlog.e(TAG, "injectSmsPdu failed: ", e);
            try {
                if (receivedIntent != null)
                    receivedIntent.send(Intents.RESULT_SMS_GENERIC_ERROR);
            } catch (CanceledException ex) {}
        }
    }

    @Override
    public void sendRetrySms(SmsTracker tracker) {
        String oldFormat = tracker.mFormat;

        // newFormat will be based on voice technology
        String newFormat =
            (PhoneConstants.PHONE_TYPE_CDMA == mPhone.getPhoneType()) ?
                    mCdmaDispatcher.getFormat() :
                        mGsmDispatcher.getFormat();

        // was previously sent sms format match with voice tech?
        if (oldFormat.equals(newFormat)) {
            if (isCdmaFormat(newFormat)) {
                Rlog.d(TAG, "old format matched new format (cdma)");
                mCdmaDispatcher.sendSms(tracker);
                return;
            } else {
                Rlog.d(TAG, "old format matched new format (gsm)");
                mGsmDispatcher.sendSms(tracker);
                return;
            }
        }

        // format didn't match, need to re-encode.
        HashMap map = tracker.getData();

        // to re-encode, fields needed are:  scAddr, destAddr, and
        //   text if originally sent as sendText or
        //   data and destPort if originally sent as sendData.
        if (!( map.containsKey("scAddr") && map.containsKey("destAddr") &&
               ( map.containsKey("text") ||
                       (map.containsKey("data") && map.containsKey("destPort"))))) {
            // should never come here...
            Rlog.e(TAG, "sendRetrySms failed to re-encode per missing fields!");
            tracker.onFailed(mContext, RESULT_ERROR_GENERIC_FAILURE, 0/*errorCode*/);
            return;
        }
        String scAddr = (String)map.get("scAddr");
        String destAddr = (String)map.get("destAddr");

        SmsMessageBase.SubmitPduBase pdu = null;
        //    figure out from tracker if this was sendText/Data
        if (map.containsKey("text")) {
            Rlog.d(TAG, "sms failed was text");
            String text = (String)map.get("text");

            if (isCdmaFormat(newFormat)) {
                Rlog.d(TAG, "old format (gsm) ==> new format (cdma)");
                pdu = com.android.internal.telephony.cdma.SmsMessage.getSubmitPdu(
                        scAddr, destAddr, text, (tracker.mDeliveryIntent != null), null);
            } else {
                Rlog.d(TAG, "old format (cdma) ==> new format (gsm)");
                pdu = com.android.internal.telephony.gsm.SmsMessage.getSubmitPdu(
                        scAddr, destAddr, text, (tracker.mDeliveryIntent != null), null);
            }
        } else if (map.containsKey("data")) {
            Rlog.d(TAG, "sms failed was data");
            byte[] data = (byte[])map.get("data");
            Integer destPort = (Integer)map.get("destPort");

            if (isCdmaFormat(newFormat)) {
                Rlog.d(TAG, "old format (gsm) ==> new format (cdma)");
                pdu = com.android.internal.telephony.cdma.SmsMessage.getSubmitPdu(
                            scAddr, destAddr, destPort.intValue(), data,
                            (tracker.mDeliveryIntent != null));
            } else {
                Rlog.d(TAG, "old format (cdma) ==> new format (gsm)");
                pdu = com.android.internal.telephony.gsm.SmsMessage.getSubmitPdu(
                            scAddr, destAddr, destPort.intValue(), data,
                            (tracker.mDeliveryIntent != null));
            }
        }

        // replace old smsc and pdu with newly encoded ones
        map.put("smsc", pdu.encodedScAddress);
        map.put("pdu", pdu.encodedMessage);

        SMSDispatcher dispatcher = (isCdmaFormat(newFormat)) ?
                mCdmaDispatcher : mGsmDispatcher;

        tracker.mFormat = dispatcher.getFormat();
        dispatcher.sendSms(tracker);
    }

    @Override
    protected void sendSubmitPdu(SmsTracker tracker) {
        sendRawPdu(tracker);
    }

    @Override
    protected String getFormat() {
        // this function should be defined in Gsm/CdmaDispatcher.
        Rlog.e(TAG, "getFormat should never be called from here!");
        return "unknown";
    }

    @Override
    protected GsmAlphabet.TextEncodingDetails calculateLength(
            CharSequence messageBody, boolean use7bitOnly) {
        Rlog.e(TAG, "Error! Not implemented for IMS.");
        return null;
    }

    @Override
    protected SmsTracker getNewSubmitPduTracker(String destinationAddress, String scAddress,
            String message, SmsHeader smsHeader, int format, PendingIntent sentIntent,
            PendingIntent deliveryIntent, boolean lastPart,
            AtomicInteger unsentPartCount, AtomicBoolean anyPartFailed, Uri messageUri,
            String fullMessageText) {
        Rlog.e(TAG, "Error! Not implemented for IMS.");
        return null;
    }

    @Override
    public boolean isIms() {
        return mIms;
    }

    @Override
    public String getImsSmsFormat() {
        return mImsSmsFormat;
    }

    /**
     * Determines whether or not to use CDMA format for MO SMS.
     * If SMS over IMS is supported, then format is based on IMS SMS format,
     * otherwise format is based on current phone type.
     *
     * @return true if Cdma format should be used for MO SMS, false otherwise.
     */
    private boolean isCdmaMo() {
        if (!isIms()) {
            // IMS is not registered, use Voice technology to determine SMS format.
            return (PhoneConstants.PHONE_TYPE_CDMA == mPhone.getPhoneType());
        }
        // IMS is registered with SMS support
        return isCdmaFormat(mImsSmsFormat);
    }

    /**
     * Determines whether or not format given is CDMA format.
     *
     * @param format
     * @return true if format given is CDMA format, false otherwise.
     */
    private boolean isCdmaFormat(String format) {
        return (mCdmaDispatcher.getFormat().equals(format));
    }
}
