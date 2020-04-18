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
import android.annotation.Nullable;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.app.PendingIntent.CanceledException;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.database.sqlite.SqliteWrapper;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Binder;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.os.RemoteException;
import android.os.UserHandle;
import android.provider.Settings;
import android.provider.Telephony;
import android.provider.Telephony.Sms;
import android.service.carrier.CarrierMessagingService;
import android.service.carrier.ICarrierMessagingCallback;
import android.service.carrier.ICarrierMessagingService;
import android.telephony.CarrierMessagingServiceManager;
import android.telephony.PhoneNumberUtils;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.text.Html;
import android.text.Spanned;
import android.text.TextUtils;
import android.util.EventLog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.TextView;

import com.android.internal.R;
import com.android.internal.telephony.GsmAlphabet.TextEncodingDetails;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.internal.telephony.uicc.UiccController;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import static android.Manifest.permission.SEND_SMS_NO_CONFIRMATION;
import static android.telephony.SmsManager.RESULT_ERROR_FDN_CHECK_FAILURE;
import static android.telephony.SmsManager.RESULT_ERROR_GENERIC_FAILURE;
import static android.telephony.SmsManager.RESULT_ERROR_LIMIT_EXCEEDED;
import static android.telephony.SmsManager.RESULT_ERROR_NO_SERVICE;
import static android.telephony.SmsManager.RESULT_ERROR_NULL_PDU;
import static android.telephony.SmsManager.RESULT_ERROR_RADIO_OFF;

public abstract class SMSDispatcher extends Handler {
    static final String TAG = "SMSDispatcher";    // accessed from inner class
    static final boolean DBG = false;
    private static final String SEND_NEXT_MSG_EXTRA = "SendNextMsg";

    private static final int PREMIUM_RULE_USE_SIM = 1;
    private static final int PREMIUM_RULE_USE_NETWORK = 2;
    private static final int PREMIUM_RULE_USE_BOTH = 3;
    private final AtomicInteger mPremiumSmsRule = new AtomicInteger(PREMIUM_RULE_USE_SIM);
    private final SettingsObserver mSettingsObserver;

    /** SMS send complete. */
    protected static final int EVENT_SEND_SMS_COMPLETE = 2;

    /** Retry sending a previously failed SMS message */
    private static final int EVENT_SEND_RETRY = 3;

    /** Confirmation required for sending a large number of messages. */
    private static final int EVENT_SEND_LIMIT_REACHED_CONFIRMATION = 4;

    /** Send the user confirmed SMS */
    static final int EVENT_SEND_CONFIRMED_SMS = 5;  // accessed from inner class

    /** Don't send SMS (user did not confirm). */
    static final int EVENT_STOP_SENDING = 7;        // accessed from inner class

    /** Confirmation required for third-party apps sending to an SMS short code. */
    private static final int EVENT_CONFIRM_SEND_TO_POSSIBLE_PREMIUM_SHORT_CODE = 8;

    /** Confirmation required for third-party apps sending to an SMS short code. */
    private static final int EVENT_CONFIRM_SEND_TO_PREMIUM_SHORT_CODE = 9;

    /** Handle status report from {@code CdmaInboundSmsHandler}. */
    protected static final int EVENT_HANDLE_STATUS_REPORT = 10;

    /** Radio is ON */
    protected static final int EVENT_RADIO_ON = 11;

    /** IMS registration/SMS format changed */
    protected static final int EVENT_IMS_STATE_CHANGED = 12;

    /** Callback from RIL_REQUEST_IMS_REGISTRATION_STATE */
    protected static final int EVENT_IMS_STATE_DONE = 13;

    // other
    protected static final int EVENT_NEW_ICC_SMS = 14;
    protected static final int EVENT_ICC_CHANGED = 15;

    protected Phone mPhone;
    protected final Context mContext;
    protected final ContentResolver mResolver;
    protected final CommandsInterface mCi;
    protected final TelephonyManager mTelephonyManager;

    /** Maximum number of times to retry sending a failed SMS. */
    private static final int MAX_SEND_RETRIES = 3;
    /** Delay before next send attempt on a failed SMS, in milliseconds. */
    private static final int SEND_RETRY_DELAY = 2000;
    /** single part SMS */
    private static final int SINGLE_PART_SMS = 1;
    /** Message sending queue limit */
    private static final int MO_MSG_QUEUE_LIMIT = 5;

    /**
     * Message reference for a CONCATENATED_8_BIT_REFERENCE or
     * CONCATENATED_16_BIT_REFERENCE message set.  Should be
     * incremented for each set of concatenated messages.
     * Static field shared by all dispatcher objects.
     */
    private static int sConcatenatedRef = new Random().nextInt(256);

    /** Outgoing message counter. Shared by all dispatchers. */
    private SmsUsageMonitor mUsageMonitor;

    private ImsSMSDispatcher mImsSMSDispatcher;

    /** Number of outgoing SmsTrackers waiting for user confirmation. */
    private int mPendingTrackerCount;

    /* Flags indicating whether the current device allows sms service */
    protected boolean mSmsCapable = true;
    protected boolean mSmsSendDisabled;

    protected static int getNextConcatenatedRef() {
        sConcatenatedRef += 1;
        return sConcatenatedRef;
    }

    /**
     * Create a new SMS dispatcher.
     * @param phone the Phone to use
     * @param usageMonitor the SmsUsageMonitor to use
     */
    protected SMSDispatcher(Phone phone, SmsUsageMonitor usageMonitor,
            ImsSMSDispatcher imsSMSDispatcher) {
        mPhone = phone;
        mImsSMSDispatcher = imsSMSDispatcher;
        mContext = phone.getContext();
        mResolver = mContext.getContentResolver();
        mCi = phone.mCi;
        mUsageMonitor = usageMonitor;
        mTelephonyManager = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
        mSettingsObserver = new SettingsObserver(this, mPremiumSmsRule, mContext);
        mContext.getContentResolver().registerContentObserver(Settings.Global.getUriFor(
                Settings.Global.SMS_SHORT_CODE_RULE), false, mSettingsObserver);

        mSmsCapable = mContext.getResources().getBoolean(
                com.android.internal.R.bool.config_sms_capable);
        mSmsSendDisabled = !mTelephonyManager.getSmsSendCapableForPhone(
                mPhone.getPhoneId(), mSmsCapable);
        Rlog.d(TAG, "SMSDispatcher: ctor mSmsCapable=" + mSmsCapable + " format=" + getFormat()
                + " mSmsSendDisabled=" + mSmsSendDisabled);
    }

    /**
     * Observe the secure setting for updated premium sms determination rules
     */
    private static class SettingsObserver extends ContentObserver {
        private final AtomicInteger mPremiumSmsRule;
        private final Context mContext;
        SettingsObserver(Handler handler, AtomicInteger premiumSmsRule, Context context) {
            super(handler);
            mPremiumSmsRule = premiumSmsRule;
            mContext = context;
            onChange(false); // load initial value;
        }

        @Override
        public void onChange(boolean selfChange) {
            mPremiumSmsRule.set(Settings.Global.getInt(mContext.getContentResolver(),
                    Settings.Global.SMS_SHORT_CODE_RULE, PREMIUM_RULE_USE_SIM));
        }
    }

    protected void updatePhoneObject(Phone phone) {
        mPhone = phone;
        mUsageMonitor = phone.mSmsUsageMonitor;
        Rlog.d(TAG, "Active phone changed to " + mPhone.getPhoneName() );
    }

    /** Unregister for incoming SMS events. */
    public void dispose() {
        mContext.getContentResolver().unregisterContentObserver(mSettingsObserver);
    }

    /**
     * The format of the message PDU in the associated broadcast intent.
     * This will be either "3gpp" for GSM/UMTS/LTE messages in 3GPP format
     * or "3gpp2" for CDMA/LTE messages in 3GPP2 format.
     *
     * Note: All applications which handle incoming SMS messages by processing the
     * SMS_RECEIVED_ACTION broadcast intent MUST pass the "format" extra from the intent
     * into the new methods in {@link android.telephony.SmsMessage} which take an
     * extra format parameter. This is required in order to correctly decode the PDU on
     * devices which require support for both 3GPP and 3GPP2 formats at the same time,
     * such as CDMA/LTE devices and GSM/CDMA world phones.
     *
     * @return the format of the message PDU
     */
    protected abstract String getFormat();

    /**
     * Pass the Message object to subclass to handle. Currently used to pass CDMA status reports
     * from {@link com.android.internal.telephony.cdma.CdmaInboundSmsHandler}.
     * @param o the SmsMessage containing the status report
     */
    protected void handleStatusReport(Object o) {
        Rlog.d(TAG, "handleStatusReport() called with no subclass.");
    }

    /* TODO: Need to figure out how to keep track of status report routing in a
     *       persistent manner. If the phone process restarts (reboot or crash),
     *       we will lose this list and any status reports that come in after
     *       will be dropped.
     */
    /** Sent messages awaiting a delivery status report. */
    protected final ArrayList<SmsTracker> deliveryPendingList = new ArrayList<SmsTracker>();

    /**
     * Handles events coming from the phone stack. Overridden from handler.
     *
     * @param msg the message to handle
     */
    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
        case EVENT_SEND_SMS_COMPLETE:
            // An outbound SMS has been successfully transferred, or failed.
            handleSendComplete((AsyncResult) msg.obj);
            break;

        case EVENT_SEND_RETRY:
            Rlog.d(TAG, "SMS retry..");
            sendRetrySms((SmsTracker) msg.obj);
            break;

        case EVENT_SEND_LIMIT_REACHED_CONFIRMATION:
            handleReachSentLimit((SmsTracker)(msg.obj));
            break;

        case EVENT_CONFIRM_SEND_TO_POSSIBLE_PREMIUM_SHORT_CODE:
            handleConfirmShortCode(false, (SmsTracker)(msg.obj));
            break;

        case EVENT_CONFIRM_SEND_TO_PREMIUM_SHORT_CODE:
            handleConfirmShortCode(true, (SmsTracker)(msg.obj));
            break;

        case EVENT_SEND_CONFIRMED_SMS:
        {
            SmsTracker tracker = (SmsTracker) msg.obj;
            if (tracker.isMultipart()) {
                sendMultipartSms(tracker);
            } else {
                if (mPendingTrackerCount > 1) {
                    tracker.mExpectMore = true;
                } else {
                    tracker.mExpectMore = false;
                }
                sendSms(tracker);
            }
            mPendingTrackerCount--;
            break;
        }

        case EVENT_STOP_SENDING:
        {
            SmsTracker tracker = (SmsTracker) msg.obj;
            tracker.onFailed(mContext, RESULT_ERROR_LIMIT_EXCEEDED, 0/*errorCode*/);
            mPendingTrackerCount--;
            break;
        }

        case EVENT_HANDLE_STATUS_REPORT:
            handleStatusReport(msg.obj);
            break;

        default:
            Rlog.e(TAG, "handleMessage() ignoring message of unexpected type " + msg.what);
        }
    }

    /**
     * Use the carrier messaging service to send a data or text SMS.
     */
    protected abstract class SmsSender extends CarrierMessagingServiceManager {
        protected final SmsTracker mTracker;
        // Initialized in sendSmsByCarrierApp
        protected volatile SmsSenderCallback mSenderCallback;

        protected SmsSender(SmsTracker tracker) {
            mTracker = tracker;
        }

        public void sendSmsByCarrierApp(String carrierPackageName,
                                        SmsSenderCallback senderCallback) {
            mSenderCallback = senderCallback;
            if (!bindToCarrierMessagingService(mContext, carrierPackageName)) {
                Rlog.e(TAG, "bindService() for carrier messaging service failed");
                mSenderCallback.onSendSmsComplete(
                        CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK,
                        0 /* messageRef */);
            } else {
                Rlog.d(TAG, "bindService() for carrier messaging service succeeded");
            }
        }
    }

    private static int getSendSmsFlag(@Nullable PendingIntent deliveryIntent) {
        if (deliveryIntent == null) {
            return 0;
        }
        return CarrierMessagingService.SEND_FLAG_REQUEST_DELIVERY_STATUS;
    }

    /**
     * Use the carrier messaging service to send a text SMS.
     */
    protected final class TextSmsSender extends SmsSender {
        public TextSmsSender(SmsTracker tracker) {
            super(tracker);
        }

        @Override
        protected void onServiceReady(ICarrierMessagingService carrierMessagingService) {
            HashMap<String, Object> map = mTracker.getData();
            String text = (String) map.get("text");

            if (text != null) {
                try {
                    carrierMessagingService.sendTextSms(text, getSubId(),
                            mTracker.mDestAddress, getSendSmsFlag(mTracker.mDeliveryIntent),
                            mSenderCallback);
                } catch (RemoteException e) {
                    Rlog.e(TAG, "Exception sending the SMS: " + e);
                    mSenderCallback.onSendSmsComplete(
                            CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK,
                            0 /* messageRef */);
                }
            } else {
                mSenderCallback.onSendSmsComplete(
                        CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK,
                        0 /* messageRef */);
            }
        }
    }

    /**
     * Use the carrier messaging service to send a data SMS.
     */
    protected final class DataSmsSender extends SmsSender {
        public DataSmsSender(SmsTracker tracker) {
            super(tracker);
        }

        @Override
        protected void onServiceReady(ICarrierMessagingService carrierMessagingService) {
            HashMap<String, Object> map = mTracker.getData();
            byte[] data = (byte[]) map.get("data");
            int destPort = (int) map.get("destPort");

            if (data != null) {
                try {
                    carrierMessagingService.sendDataSms(data, getSubId(),
                            mTracker.mDestAddress, destPort,
                            getSendSmsFlag(mTracker.mDeliveryIntent), mSenderCallback);
                } catch (RemoteException e) {
                    Rlog.e(TAG, "Exception sending the SMS: " + e);
                    mSenderCallback.onSendSmsComplete(
                            CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK,
                            0 /* messageRef */);
                }
            } else {
                mSenderCallback.onSendSmsComplete(
                        CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK,
                        0 /* messageRef */);
            }
        }
    }

    /**
     * Callback for TextSmsSender and DataSmsSender from the carrier messaging service.
     * Once the result is ready, the carrier messaging service connection is disposed.
     */
    protected final class SmsSenderCallback extends ICarrierMessagingCallback.Stub {
        private final SmsSender mSmsSender;

        public SmsSenderCallback(SmsSender smsSender) {
            mSmsSender = smsSender;
        }

        /**
         * This method should be called only once.
         */
        @Override
        public void onSendSmsComplete(int result, int messageRef) {
            checkCallerIsPhoneOrCarrierApp();
            final long identity = Binder.clearCallingIdentity();
            try {
                mSmsSender.disposeConnection(mContext);
                processSendSmsResponse(mSmsSender.mTracker, result, messageRef);
            } finally {
                Binder.restoreCallingIdentity(identity);
            }
        }

        @Override
        public void onSendMultipartSmsComplete(int result, int[] messageRefs) {
            Rlog.e(TAG, "Unexpected onSendMultipartSmsComplete call with result: " + result);
        }

        @Override
        public void onFilterComplete(int result) {
            Rlog.e(TAG, "Unexpected onFilterComplete call with result: " + result);
        }

        @Override
        public void onSendMmsComplete(int result, byte[] sendConfPdu) {
            Rlog.e(TAG, "Unexpected onSendMmsComplete call with result: " + result);
        }

        @Override
        public void onDownloadMmsComplete(int result) {
            Rlog.e(TAG, "Unexpected onDownloadMmsComplete call with result: " + result);
        }
    }

    private void processSendSmsResponse(SmsTracker tracker, int result, int messageRef) {
        if (tracker == null) {
            Rlog.e(TAG, "processSendSmsResponse: null tracker");
            return;
        }

        SmsResponse smsResponse = new SmsResponse(
                messageRef, null /* ackPdu */, -1 /* unknown error code */);

        switch (result) {
        case CarrierMessagingService.SEND_STATUS_OK:
            Rlog.d(TAG, "Sending SMS by IP succeeded.");
            sendMessage(obtainMessage(EVENT_SEND_SMS_COMPLETE,
                                      new AsyncResult(tracker,
                                                      smsResponse,
                                                      null /* exception*/ )));
            break;
        case CarrierMessagingService.SEND_STATUS_ERROR:
            Rlog.d(TAG, "Sending SMS by IP failed.");
            sendMessage(obtainMessage(EVENT_SEND_SMS_COMPLETE,
                    new AsyncResult(tracker, smsResponse,
                            new CommandException(CommandException.Error.GENERIC_FAILURE))));
            break;
        case CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK:
            Rlog.d(TAG, "Sending SMS by IP failed. Retry on carrier network.");
            sendSubmitPdu(tracker);
            break;
        default:
            Rlog.d(TAG, "Unknown result " + result + " Retry on carrier network.");
            sendSubmitPdu(tracker);
        }
    }

    /**
     * Use the carrier messaging service to send a multipart text SMS.
     */
    private final class MultipartSmsSender extends CarrierMessagingServiceManager {
        private final List<String> mParts;
        public final SmsTracker[] mTrackers;
        // Initialized in sendSmsByCarrierApp
        private volatile MultipartSmsSenderCallback mSenderCallback;

        MultipartSmsSender(ArrayList<String> parts, SmsTracker[] trackers) {
            mParts = parts;
            mTrackers = trackers;
        }

        void sendSmsByCarrierApp(String carrierPackageName,
                                 MultipartSmsSenderCallback senderCallback) {
            mSenderCallback = senderCallback;
            if (!bindToCarrierMessagingService(mContext, carrierPackageName)) {
                Rlog.e(TAG, "bindService() for carrier messaging service failed");
                mSenderCallback.onSendMultipartSmsComplete(
                        CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK,
                        null /* smsResponse */);
            } else {
                Rlog.d(TAG, "bindService() for carrier messaging service succeeded");
            }
        }

        @Override
        protected void onServiceReady(ICarrierMessagingService carrierMessagingService) {
            try {
                carrierMessagingService.sendMultipartTextSms(
                        mParts, getSubId(), mTrackers[0].mDestAddress,
                        getSendSmsFlag(mTrackers[0].mDeliveryIntent), mSenderCallback);
            } catch (RemoteException e) {
                Rlog.e(TAG, "Exception sending the SMS: " + e);
                mSenderCallback.onSendMultipartSmsComplete(
                        CarrierMessagingService.SEND_STATUS_RETRY_ON_CARRIER_NETWORK,
                        null /* smsResponse */);
            }
        }
    }

    /**
     * Callback for MultipartSmsSender from the carrier messaging service.
     * Once the result is ready, the carrier messaging service connection is disposed.
     */
    private final class MultipartSmsSenderCallback extends ICarrierMessagingCallback.Stub {
        private final MultipartSmsSender mSmsSender;

        MultipartSmsSenderCallback(MultipartSmsSender smsSender) {
            mSmsSender = smsSender;
        }

        @Override
        public void onSendSmsComplete(int result, int messageRef) {
            Rlog.e(TAG, "Unexpected onSendSmsComplete call with result: " + result);
        }

        /**
         * This method should be called only once.
         */
        @Override
        public void onSendMultipartSmsComplete(int result, int[] messageRefs) {
            mSmsSender.disposeConnection(mContext);

            if (mSmsSender.mTrackers == null) {
                Rlog.e(TAG, "Unexpected onSendMultipartSmsComplete call with null trackers.");
                return;
            }

            checkCallerIsPhoneOrCarrierApp();
            final long identity = Binder.clearCallingIdentity();
            try {
                for (int i = 0; i < mSmsSender.mTrackers.length; i++) {
                    int messageRef = 0;
                    if (messageRefs != null && messageRefs.length > i) {
                        messageRef = messageRefs[i];
                    }
                    processSendSmsResponse(mSmsSender.mTrackers[i], result, messageRef);
                }
            } finally {
                Binder.restoreCallingIdentity(identity);
            }
        }

        @Override
        public void onFilterComplete(int result) {
            Rlog.e(TAG, "Unexpected onFilterComplete call with result: " + result);
        }

        @Override
        public void onSendMmsComplete(int result, byte[] sendConfPdu) {
            Rlog.e(TAG, "Unexpected onSendMmsComplete call with result: " + result);
        }

        @Override
        public void onDownloadMmsComplete(int result) {
            Rlog.e(TAG, "Unexpected onDownloadMmsComplete call with result: " + result);
        }
    }

    /**
     * Send an SMS PDU. Usually just calls {@link sendRawPdu}.
     */
    protected abstract void sendSubmitPdu(SmsTracker tracker);

    /**
     * Called when SMS send completes. Broadcasts a sentIntent on success.
     * On failure, either sets up retries or broadcasts a sentIntent with
     * the failure in the result code.
     *
     * @param ar AsyncResult passed into the message handler.  ar.result should
     *           an SmsResponse instance if send was successful.  ar.userObj
     *           should be an SmsTracker instance.
     */
    protected void handleSendComplete(AsyncResult ar) {
        SmsTracker tracker = (SmsTracker) ar.userObj;
        PendingIntent sentIntent = tracker.mSentIntent;

        if (ar.result != null) {
            tracker.mMessageRef = ((SmsResponse)ar.result).mMessageRef;
        } else {
            Rlog.d(TAG, "SmsResponse was null");
        }

        if (ar.exception == null) {
            if (DBG) Rlog.d(TAG, "SMS send complete. Broadcasting intent: " + sentIntent);

            if (tracker.mDeliveryIntent != null) {
                // Expecting a status report.  Add it to the list.
                deliveryPendingList.add(tracker);
            }
            tracker.onSent(mContext);
        } else {
            if (DBG) Rlog.d(TAG, "SMS send failed");

            int ss = mPhone.getServiceState().getState();

            if ( tracker.mImsRetry > 0 && ss != ServiceState.STATE_IN_SERVICE) {
                // This is retry after failure over IMS but voice is not available.
                // Set retry to max allowed, so no retry is sent and
                //   cause RESULT_ERROR_GENERIC_FAILURE to be returned to app.
                tracker.mRetryCount = MAX_SEND_RETRIES;

                Rlog.d(TAG, "handleSendComplete: Skipping retry: "
                +" isIms()="+isIms()
                +" mRetryCount="+tracker.mRetryCount
                +" mImsRetry="+tracker.mImsRetry
                +" mMessageRef="+tracker.mMessageRef
                +" SS= "+mPhone.getServiceState().getState());
            }

            // if sms over IMS is not supported on data and voice is not available...
            if (!isIms() && ss != ServiceState.STATE_IN_SERVICE) {
                tracker.onFailed(mContext, getNotInServiceError(ss), 0/*errorCode*/);
            } else if ((((CommandException)(ar.exception)).getCommandError()
                    == CommandException.Error.SMS_FAIL_RETRY) &&
                   tracker.mRetryCount < MAX_SEND_RETRIES) {
                // Retry after a delay if needed.
                // TODO: According to TS 23.040, 9.2.3.6, we should resend
                //       with the same TP-MR as the failed message, and
                //       TP-RD set to 1.  However, we don't have a means of
                //       knowing the MR for the failed message (EF_SMSstatus
                //       may or may not have the MR corresponding to this
                //       message, depending on the failure).  Also, in some
                //       implementations this retry is handled by the baseband.
                tracker.mRetryCount++;
                Message retryMsg = obtainMessage(EVENT_SEND_RETRY, tracker);
                sendMessageDelayed(retryMsg, SEND_RETRY_DELAY);
            } else {
                int errorCode = 0;
                if (ar.result != null) {
                    errorCode = ((SmsResponse)ar.result).mErrorCode;
                }
                int error = RESULT_ERROR_GENERIC_FAILURE;
                if (((CommandException)(ar.exception)).getCommandError()
                        == CommandException.Error.FDN_CHECK_FAILURE) {
                    error = RESULT_ERROR_FDN_CHECK_FAILURE;
                }
                tracker.onFailed(mContext, error, errorCode);
            }
        }
    }

    /**
     * Handles outbound message when the phone is not in service.
     *
     * @param ss     Current service state.  Valid values are:
     *                  OUT_OF_SERVICE
     *                  EMERGENCY_ONLY
     *                  POWER_OFF
     * @param sentIntent the PendingIntent to send the error to
     */
    protected static void handleNotInService(int ss, PendingIntent sentIntent) {
        if (sentIntent != null) {
            try {
                if (ss == ServiceState.STATE_POWER_OFF) {
                    sentIntent.send(RESULT_ERROR_RADIO_OFF);
                } else {
                    sentIntent.send(RESULT_ERROR_NO_SERVICE);
                }
            } catch (CanceledException ex) {}
        }
    }

    /**
     * @param ss service state
     * @return The result error based on input service state for not in service error
     */
    protected static int getNotInServiceError(int ss) {
        if (ss == ServiceState.STATE_POWER_OFF) {
            return RESULT_ERROR_RADIO_OFF;
        }
        return RESULT_ERROR_NO_SERVICE;
    }

    /**
     * Send a data based SMS to a specific application port.
     *
     * @param destAddr the address to send the message to
     * @param scAddr is the service center address or null to use
     *  the current default SMSC
     * @param destPort the port to deliver the message to
     * @param data the body of the message to send
     * @param sentIntent if not NULL this <code>PendingIntent</code> is
     *  broadcast when the message is successfully sent, or failed.
     *  The result code will be <code>Activity.RESULT_OK<code> for success,
     *  or one of these errors:<br>
     *  <code>RESULT_ERROR_GENERIC_FAILURE</code><br>
     *  <code>RESULT_ERROR_RADIO_OFF</code><br>
     *  <code>RESULT_ERROR_NULL_PDU</code><br>
     *  <code>RESULT_ERROR_NO_SERVICE</code><br>.
     *  For <code>RESULT_ERROR_GENERIC_FAILURE</code> the sentIntent may include
     *  the extra "errorCode" containing a radio technology specific value,
     *  generally only useful for troubleshooting.<br>
     *  The per-application based SMS control checks sentIntent. If sentIntent
     *  is NULL the caller will be checked against all unknown applications,
     *  which cause smaller number of SMS to be sent in checking period.
     * @param deliveryIntent if not NULL this <code>PendingIntent</code> is
     *  broadcast when the message is delivered to the recipient.  The
     *  raw pdu of the status report is in the extended data ("pdu").
     */
    protected abstract void sendData(String destAddr, String scAddr, int destPort,
            byte[] data, PendingIntent sentIntent, PendingIntent deliveryIntent);

    /**
     * Send a text based SMS.
     *  @param destAddr the address to send the message to
     * @param scAddr is the service center address or null to use
     *  the current default SMSC
     * @param text the body of the message to send
     * @param sentIntent if not NULL this <code>PendingIntent</code> is
     *  broadcast when the message is successfully sent, or failed.
     *  The result code will be <code>Activity.RESULT_OK<code> for success,
     *  or one of these errors:<br>
     *  <code>RESULT_ERROR_GENERIC_FAILURE</code><br>
     *  <code>RESULT_ERROR_RADIO_OFF</code><br>
     *  <code>RESULT_ERROR_NULL_PDU</code><br>
     *  <code>RESULT_ERROR_NO_SERVICE</code><br>.
     *  For <code>RESULT_ERROR_GENERIC_FAILURE</code> the sentIntent may include
     *  the extra "errorCode" containing a radio technology specific value,
     *  generally only useful for troubleshooting.<br>
     *  The per-application based SMS control checks sentIntent. If sentIntent
     *  is NULL the caller will be checked against all unknown applications,
     *  which cause smaller number of SMS to be sent in checking period.
     * @param deliveryIntent if not NULL this <code>PendingIntent</code> is
     *  broadcast when the message is delivered to the recipient.  The
     * @param messageUri optional URI of the message if it is already stored in the system
     * @param callingPkg the calling package name
     * @param persistMessage whether to save the sent message into SMS DB for a
     *   non-default SMS app.
     */
    protected abstract void sendText(String destAddr, String scAddr, String text,
            PendingIntent sentIntent, PendingIntent deliveryIntent, Uri messageUri,
            String callingPkg, boolean persistMessage);

    /**
     * Inject an SMS PDU into the android platform.
     *
     * @param pdu is the byte array of pdu to be injected into android telephony layer
     * @param format is the format of SMS pdu (3gpp or 3gpp2)
     * @param receivedIntent if not NULL this <code>PendingIntent</code> is
     *  broadcast when the message is successfully received by the
     *  android telephony layer. This intent is broadcasted at
     *  the same time an SMS received from radio is responded back.
     */
    protected abstract void injectSmsPdu(byte[] pdu, String format, PendingIntent receivedIntent);

    /**
     * Calculate the number of septets needed to encode the message. This function should only be
     * called for individual segments of multipart message.
     *
     * @param messageBody the message to encode
     * @param use7bitOnly ignore (but still count) illegal characters if true
     * @return TextEncodingDetails
     */
    protected abstract TextEncodingDetails calculateLength(CharSequence messageBody,
            boolean use7bitOnly);

    /**
     * Send a multi-part text based SMS.
     *  @param destAddr the address to send the message to
     * @param scAddr is the service center address or null to use
     *   the current default SMSC
     * @param parts an <code>ArrayList</code> of strings that, in order,
     *   comprise the original message
     * @param sentIntents if not null, an <code>ArrayList</code> of
     *   <code>PendingIntent</code>s (one for each message part) that is
     *   broadcast when the corresponding message part has been sent.
     *   The result code will be <code>Activity.RESULT_OK<code> for success,
     *   or one of these errors:
     *   <code>RESULT_ERROR_GENERIC_FAILURE</code>
     *   <code>RESULT_ERROR_RADIO_OFF</code>
     *   <code>RESULT_ERROR_NULL_PDU</code>
     *   <code>RESULT_ERROR_NO_SERVICE</code>.
     *  The per-application based SMS control checks sentIntent. If sentIntent
     *  is NULL the caller will be checked against all unknown applications,
     *  which cause smaller number of SMS to be sent in checking period.
     * @param deliveryIntents if not null, an <code>ArrayList</code> of
     *   <code>PendingIntent</code>s (one for each message part) that is
     *   broadcast when the corresponding message part has been delivered
     *   to the recipient.  The raw pdu of the status report is in the
     * @param messageUri optional URI of the message if it is already stored in the system
     * @param callingPkg the calling package name
     * @param persistMessage whether to save the sent message into SMS DB for a
     *   non-default SMS app.
     */
    protected void sendMultipartText(String destAddr, String scAddr,
            ArrayList<String> parts, ArrayList<PendingIntent> sentIntents,
            ArrayList<PendingIntent> deliveryIntents, Uri messageUri, String callingPkg,
            boolean persistMessage) {
        final String fullMessageText = getMultipartMessageText(parts);
        int refNumber = getNextConcatenatedRef() & 0x00FF;
        int msgCount = parts.size();
        int encoding = SmsConstants.ENCODING_UNKNOWN;

        TextEncodingDetails[] encodingForParts = new TextEncodingDetails[msgCount];
        for (int i = 0; i < msgCount; i++) {
            TextEncodingDetails details = calculateLength(parts.get(i), false);
            if (encoding != details.codeUnitSize
                    && (encoding == SmsConstants.ENCODING_UNKNOWN
                            || encoding == SmsConstants.ENCODING_7BIT)) {
                encoding = details.codeUnitSize;
            }
            encodingForParts[i] = details;
        }

        SmsTracker[] trackers = new SmsTracker[msgCount];

        // States to track at the message level (for all parts)
        final AtomicInteger unsentPartCount = new AtomicInteger(msgCount);
        final AtomicBoolean anyPartFailed = new AtomicBoolean(false);

        for (int i = 0; i < msgCount; i++) {
            SmsHeader.ConcatRef concatRef = new SmsHeader.ConcatRef();
            concatRef.refNumber = refNumber;
            concatRef.seqNumber = i + 1;  // 1-based sequence
            concatRef.msgCount = msgCount;
            // TODO: We currently set this to true since our messaging app will never
            // send more than 255 parts (it converts the message to MMS well before that).
            // However, we should support 3rd party messaging apps that might need 16-bit
            // references
            // Note:  It's not sufficient to just flip this bit to true; it will have
            // ripple effects (several calculations assume 8-bit ref).
            concatRef.isEightBits = true;
            SmsHeader smsHeader = new SmsHeader();
            smsHeader.concatRef = concatRef;

            // Set the national language tables for 3GPP 7-bit encoding, if enabled.
            if (encoding == SmsConstants.ENCODING_7BIT) {
                smsHeader.languageTable = encodingForParts[i].languageTable;
                smsHeader.languageShiftTable = encodingForParts[i].languageShiftTable;
            }

            PendingIntent sentIntent = null;
            if (sentIntents != null && sentIntents.size() > i) {
                sentIntent = sentIntents.get(i);
            }

            PendingIntent deliveryIntent = null;
            if (deliveryIntents != null && deliveryIntents.size() > i) {
                deliveryIntent = deliveryIntents.get(i);
            }

            trackers[i] =
                getNewSubmitPduTracker(destAddr, scAddr, parts.get(i), smsHeader, encoding,
                        sentIntent, deliveryIntent, (i == (msgCount - 1)),
                        unsentPartCount, anyPartFailed, messageUri, fullMessageText);
            trackers[i].mPersistMessage = persistMessage;
        }

        if (parts == null || trackers == null || trackers.length == 0
                || trackers[0] == null) {
            Rlog.e(TAG, "Cannot send multipart text. parts=" + parts + " trackers=" + trackers);
            return;
        }

        String carrierPackage = getCarrierAppPackageName();
        if (carrierPackage != null) {
            Rlog.d(TAG, "Found carrier package.");
            MultipartSmsSender smsSender = new MultipartSmsSender(parts, trackers);
            smsSender.sendSmsByCarrierApp(carrierPackage, new MultipartSmsSenderCallback(smsSender));
        } else {
            Rlog.v(TAG, "No carrier package.");
            for (SmsTracker tracker : trackers) {
                if (tracker != null) {
                    sendSubmitPdu(tracker);
                } else {
                    Rlog.e(TAG, "Null tracker.");
                }
            }
        }
    }

    /**
     * Create a new SubmitPdu and return the SMS tracker.
     */
    protected abstract SmsTracker getNewSubmitPduTracker(String destinationAddress, String scAddress,
            String message, SmsHeader smsHeader, int encoding,
            PendingIntent sentIntent, PendingIntent deliveryIntent, boolean lastPart,
            AtomicInteger unsentPartCount, AtomicBoolean anyPartFailed, Uri messageUri,
            String fullMessageText);

    /**
     * Send an SMS
     * @param tracker will contain:
     * -smsc the SMSC to send the message through, or NULL for the
     *  default SMSC
     * -pdu the raw PDU to send
     * -sentIntent if not NULL this <code>Intent</code> is
     *  broadcast when the message is successfully sent, or failed.
     *  The result code will be <code>Activity.RESULT_OK<code> for success,
     *  or one of these errors:
     *  <code>RESULT_ERROR_GENERIC_FAILURE</code>
     *  <code>RESULT_ERROR_RADIO_OFF</code>
     *  <code>RESULT_ERROR_NULL_PDU</code>
     *  <code>RESULT_ERROR_NO_SERVICE</code>.
     *  The per-application based SMS control checks sentIntent. If sentIntent
     *  is NULL the caller will be checked against all unknown applications,
     *  which cause smaller number of SMS to be sent in checking period.
     * -deliveryIntent if not NULL this <code>Intent</code> is
     *  broadcast when the message is delivered to the recipient.  The
     *  raw pdu of the status report is in the extended data ("pdu").
     * -param destAddr the destination phone number (for short code confirmation)
     */
    protected void sendRawPdu(SmsTracker tracker) {
        HashMap map = tracker.getData();
        byte pdu[] = (byte[]) map.get("pdu");

        if (mSmsSendDisabled) {
            Rlog.e(TAG, "Device does not support sending sms.");
            tracker.onFailed(mContext, RESULT_ERROR_NO_SERVICE, 0/*errorCode*/);
            return;
        }

        if (pdu == null) {
            Rlog.e(TAG, "Empty PDU");
            tracker.onFailed(mContext, RESULT_ERROR_NULL_PDU, 0/*errorCode*/);
            return;
        }

        // Get calling app package name via UID from Binder call
        PackageManager pm = mContext.getPackageManager();
        String[] packageNames = pm.getPackagesForUid(Binder.getCallingUid());

        if (packageNames == null || packageNames.length == 0) {
            // Refuse to send SMS if we can't get the calling package name.
            Rlog.e(TAG, "Can't get calling app package name: refusing to send SMS");
            tracker.onFailed(mContext, RESULT_ERROR_GENERIC_FAILURE, 0/*errorCode*/);
            return;
        }

        // Get package info via packagemanager
        PackageInfo appInfo;
        try {
            // XXX this is lossy- apps can share a UID
            appInfo = pm.getPackageInfo(packageNames[0], PackageManager.GET_SIGNATURES);
        } catch (PackageManager.NameNotFoundException e) {
            Rlog.e(TAG, "Can't get calling app package info: refusing to send SMS");
            tracker.onFailed(mContext, RESULT_ERROR_GENERIC_FAILURE, 0/*errorCode*/);
            return;
        }

        // checkDestination() returns true if the destination is not a premium short code or the
        // sending app is approved to send to short codes. Otherwise, a message is sent to our
        // handler with the SmsTracker to request user confirmation before sending.
        if (checkDestination(tracker)) {
            // check for excessive outgoing SMS usage by this app
            if (!mUsageMonitor.check(appInfo.packageName, SINGLE_PART_SMS)) {
                sendMessage(obtainMessage(EVENT_SEND_LIMIT_REACHED_CONFIRMATION, tracker));
                return;
            }

            sendSms(tracker);
        }

        if (PhoneNumberUtils.isLocalEmergencyNumber(mContext, tracker.mDestAddress)) {
            new AsyncEmergencyContactNotifier(mContext).execute();
        }
    }

    /**
     * Check if destination is a potential premium short code and sender is not pre-approved to
     * send to short codes.
     *
     * @param tracker the tracker for the SMS to send
     * @return true if the destination is approved; false if user confirmation event was sent
     */
    boolean checkDestination(SmsTracker tracker) {
        if (mContext.checkCallingOrSelfPermission(SEND_SMS_NO_CONFIRMATION)
                == PackageManager.PERMISSION_GRANTED) {
            return true;            // app is pre-approved to send to short codes
        } else {
            int rule = mPremiumSmsRule.get();
            int smsCategory = SmsUsageMonitor.CATEGORY_NOT_SHORT_CODE;
            if (rule == PREMIUM_RULE_USE_SIM || rule == PREMIUM_RULE_USE_BOTH) {
                String simCountryIso = mTelephonyManager.getSimCountryIso();
                if (simCountryIso == null || simCountryIso.length() != 2) {
                    Rlog.e(TAG, "Can't get SIM country Iso: trying network country Iso");
                    simCountryIso = mTelephonyManager.getNetworkCountryIso();
                }

                smsCategory = mUsageMonitor.checkDestination(tracker.mDestAddress, simCountryIso);
            }
            if (rule == PREMIUM_RULE_USE_NETWORK || rule == PREMIUM_RULE_USE_BOTH) {
                String networkCountryIso = mTelephonyManager.getNetworkCountryIso();
                if (networkCountryIso == null || networkCountryIso.length() != 2) {
                    Rlog.e(TAG, "Can't get Network country Iso: trying SIM country Iso");
                    networkCountryIso = mTelephonyManager.getSimCountryIso();
                }

                smsCategory = SmsUsageMonitor.mergeShortCodeCategories(smsCategory,
                        mUsageMonitor.checkDestination(tracker.mDestAddress, networkCountryIso));
            }

            if (smsCategory == SmsUsageMonitor.CATEGORY_NOT_SHORT_CODE
                    || smsCategory == SmsUsageMonitor.CATEGORY_FREE_SHORT_CODE
                    || smsCategory == SmsUsageMonitor.CATEGORY_STANDARD_SHORT_CODE) {
                return true;    // not a premium short code
            }

            // Do not allow any premium sms during SuW
            if (Settings.Global.getInt(mResolver, Settings.Global.DEVICE_PROVISIONED, 0) == 0) {
                Rlog.e(TAG, "Can't send premium sms during Setup Wizard");
                return false;
            }

            // Wait for user confirmation unless the user has set permission to always allow/deny
            int premiumSmsPermission = mUsageMonitor.getPremiumSmsPermission(
                    tracker.mAppInfo.packageName);
            if (premiumSmsPermission == SmsUsageMonitor.PREMIUM_SMS_PERMISSION_UNKNOWN) {
                // First time trying to send to premium SMS.
                premiumSmsPermission = SmsUsageMonitor.PREMIUM_SMS_PERMISSION_ASK_USER;
            }

            switch (premiumSmsPermission) {
                case SmsUsageMonitor.PREMIUM_SMS_PERMISSION_ALWAYS_ALLOW:
                    Rlog.d(TAG, "User approved this app to send to premium SMS");
                    return true;

                case SmsUsageMonitor.PREMIUM_SMS_PERMISSION_NEVER_ALLOW:
                    Rlog.w(TAG, "User denied this app from sending to premium SMS");
                    sendMessage(obtainMessage(EVENT_STOP_SENDING, tracker));
                    return false;   // reject this message

                case SmsUsageMonitor.PREMIUM_SMS_PERMISSION_ASK_USER:
                default:
                    int event;
                    if (smsCategory == SmsUsageMonitor.CATEGORY_POSSIBLE_PREMIUM_SHORT_CODE) {
                        event = EVENT_CONFIRM_SEND_TO_POSSIBLE_PREMIUM_SHORT_CODE;
                    } else {
                        event = EVENT_CONFIRM_SEND_TO_PREMIUM_SHORT_CODE;
                    }
                    sendMessage(obtainMessage(event, tracker));
                    return false;   // wait for user confirmation
            }
        }
    }

    /**
     * Deny sending an SMS if the outgoing queue limit is reached. Used when the message
     * must be confirmed by the user due to excessive usage or potential premium SMS detected.
     * @param tracker the SmsTracker for the message to send
     * @return true if the message was denied; false to continue with send confirmation
     */
    private boolean denyIfQueueLimitReached(SmsTracker tracker) {
        if (mPendingTrackerCount >= MO_MSG_QUEUE_LIMIT) {
            // Deny sending message when the queue limit is reached.
            Rlog.e(TAG, "Denied because queue limit reached");
            tracker.onFailed(mContext, RESULT_ERROR_LIMIT_EXCEEDED, 0/*errorCode*/);
            return true;
        }
        mPendingTrackerCount++;
        return false;
    }

    /**
     * Returns the label for the specified app package name.
     * @param appPackage the package name of the app requesting to send an SMS
     * @return the label for the specified app, or the package name if getApplicationInfo() fails
     */
    private CharSequence getAppLabel(String appPackage) {
        PackageManager pm = mContext.getPackageManager();
        try {
            ApplicationInfo appInfo = pm.getApplicationInfo(appPackage, 0);
            return appInfo.loadSafeLabel(pm);
        } catch (PackageManager.NameNotFoundException e) {
            Rlog.e(TAG, "PackageManager Name Not Found for package " + appPackage);
            return appPackage;  // fall back to package name if we can't get app label
        }
    }

    /**
     * Post an alert when SMS needs confirmation due to excessive usage.
     * @param tracker an SmsTracker for the current message.
     */
    protected void handleReachSentLimit(SmsTracker tracker) {
        if (denyIfQueueLimitReached(tracker)) {
            return;     // queue limit reached; error was returned to caller
        }

        CharSequence appLabel = getAppLabel(tracker.mAppInfo.packageName);
        Resources r = Resources.getSystem();
        Spanned messageText = Html.fromHtml(r.getString(R.string.sms_control_message, appLabel));

        ConfirmDialogListener listener = new ConfirmDialogListener(tracker, null);

        AlertDialog d = new AlertDialog.Builder(mContext)
                .setTitle(R.string.sms_control_title)
                .setIcon(R.drawable.stat_sys_warning)
                .setMessage(messageText)
                .setPositiveButton(r.getString(R.string.sms_control_yes), listener)
                .setNegativeButton(r.getString(R.string.sms_control_no), listener)
                .setOnCancelListener(listener)
                .create();

        d.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        d.show();
    }

    /**
     * Post an alert for user confirmation when sending to a potential short code.
     * @param isPremium true if the destination is known to be a premium short code
     * @param tracker the SmsTracker for the current message.
     */
    protected void handleConfirmShortCode(boolean isPremium, SmsTracker tracker) {
        if (denyIfQueueLimitReached(tracker)) {
            return;     // queue limit reached; error was returned to caller
        }

        int detailsId;
        if (isPremium) {
            detailsId = R.string.sms_premium_short_code_details;
        } else {
            detailsId = R.string.sms_short_code_details;
        }

        CharSequence appLabel = getAppLabel(tracker.mAppInfo.packageName);
        Resources r = Resources.getSystem();
        Spanned messageText = Html.fromHtml(r.getString(R.string.sms_short_code_confirm_message,
                appLabel, tracker.mDestAddress));

        LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        View layout = inflater.inflate(R.layout.sms_short_code_confirmation_dialog, null);

        ConfirmDialogListener listener = new ConfirmDialogListener(tracker,
                (TextView)layout.findViewById(R.id.sms_short_code_remember_undo_instruction));


        TextView messageView = (TextView) layout.findViewById(R.id.sms_short_code_confirm_message);
        messageView.setText(messageText);

        ViewGroup detailsLayout = (ViewGroup) layout.findViewById(
                R.id.sms_short_code_detail_layout);
        TextView detailsView = (TextView) detailsLayout.findViewById(
                R.id.sms_short_code_detail_message);
        detailsView.setText(detailsId);

        CheckBox rememberChoice = (CheckBox) layout.findViewById(
                R.id.sms_short_code_remember_choice_checkbox);
        rememberChoice.setOnCheckedChangeListener(listener);

        AlertDialog d = new AlertDialog.Builder(mContext)
                .setView(layout)
                .setPositiveButton(r.getString(R.string.sms_short_code_confirm_allow), listener)
                .setNegativeButton(r.getString(R.string.sms_short_code_confirm_deny), listener)
                .setOnCancelListener(listener)
                .create();

        d.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        d.show();

        listener.setPositiveButton(d.getButton(DialogInterface.BUTTON_POSITIVE));
        listener.setNegativeButton(d.getButton(DialogInterface.BUTTON_NEGATIVE));
    }

    /**
     * Returns the premium SMS permission for the specified package. If the package has never
     * been seen before, the default {@link SmsUsageMonitor#PREMIUM_SMS_PERMISSION_ASK_USER}
     * will be returned.
     * @param packageName the name of the package to query permission
     * @return one of {@link SmsUsageMonitor#PREMIUM_SMS_PERMISSION_UNKNOWN},
     *  {@link SmsUsageMonitor#PREMIUM_SMS_PERMISSION_ASK_USER},
     *  {@link SmsUsageMonitor#PREMIUM_SMS_PERMISSION_NEVER_ALLOW}, or
     *  {@link SmsUsageMonitor#PREMIUM_SMS_PERMISSION_ALWAYS_ALLOW}
     */
    public int getPremiumSmsPermission(String packageName) {
        return mUsageMonitor.getPremiumSmsPermission(packageName);
    }

    /**
     * Sets the premium SMS permission for the specified package and save the value asynchronously
     * to persistent storage.
     * @param packageName the name of the package to set permission
     * @param permission one of {@link SmsUsageMonitor#PREMIUM_SMS_PERMISSION_ASK_USER},
     *  {@link SmsUsageMonitor#PREMIUM_SMS_PERMISSION_NEVER_ALLOW}, or
     *  {@link SmsUsageMonitor#PREMIUM_SMS_PERMISSION_ALWAYS_ALLOW}
     */
    public void setPremiumSmsPermission(String packageName, int permission) {
        mUsageMonitor.setPremiumSmsPermission(packageName, permission);
    }

    /**
     * Send the message along to the radio.
     *
     * @param tracker holds the SMS message to send
     */
    protected abstract void sendSms(SmsTracker tracker);

    /**
     * Send the SMS via the PSTN network.
     *
     * @param tracker holds the Sms tracker ready to be sent
     */
    protected abstract void sendSmsByPstn(SmsTracker tracker);

    /**
     * Retry the message along to the radio.
     *
     * @param tracker holds the SMS message to send
     */
    public void sendRetrySms(SmsTracker tracker) {
        // re-routing to ImsSMSDispatcher
        if (mImsSMSDispatcher != null) {
            mImsSMSDispatcher.sendRetrySms(tracker);
        } else {
            Rlog.e(TAG, mImsSMSDispatcher + " is null. Retry failed");
        }
    }

    /**
     * Send the multi-part SMS based on multipart Sms tracker
     *
     * @param tracker holds the multipart Sms tracker ready to be sent
     */
    private void sendMultipartSms(SmsTracker tracker) {
        ArrayList<String> parts;
        ArrayList<PendingIntent> sentIntents;
        ArrayList<PendingIntent> deliveryIntents;

        HashMap<String, Object> map = tracker.getData();

        String destinationAddress = (String) map.get("destination");
        String scAddress = (String) map.get("scaddress");

        parts = (ArrayList<String>) map.get("parts");
        sentIntents = (ArrayList<PendingIntent>) map.get("sentIntents");
        deliveryIntents = (ArrayList<PendingIntent>) map.get("deliveryIntents");

        // check if in service
        int ss = mPhone.getServiceState().getState();
        // if sms over IMS is not supported on data and voice is not available...
        if (!isIms() && ss != ServiceState.STATE_IN_SERVICE) {
            for (int i = 0, count = parts.size(); i < count; i++) {
                PendingIntent sentIntent = null;
                if (sentIntents != null && sentIntents.size() > i) {
                    sentIntent = sentIntents.get(i);
                }
                handleNotInService(ss, sentIntent);
            }
            return;
        }

        sendMultipartText(destinationAddress, scAddress, parts, sentIntents, deliveryIntents,
                null/*messageUri*/, null/*callingPkg*/, tracker.mPersistMessage);
    }

    /**
     * Keeps track of an SMS that has been sent to the RIL, until it has
     * successfully been sent, or we're done trying.
     */
    public static class SmsTracker {
        // fields need to be public for derived SmsDispatchers
        private final HashMap<String, Object> mData;
        public int mRetryCount;
        public int mImsRetry; // nonzero indicates initial message was sent over Ims
        public int mMessageRef;
        public boolean mExpectMore;
        String mFormat;

        public final PendingIntent mSentIntent;
        public final PendingIntent mDeliveryIntent;

        public final PackageInfo mAppInfo;
        public final String mDestAddress;

        public final SmsHeader mSmsHeader;

        private long mTimestamp = System.currentTimeMillis();
        public Uri mMessageUri; // Uri of persisted message if we wrote one

        // Reference to states of a multipart message that this part belongs to
        private AtomicInteger mUnsentPartCount;
        private AtomicBoolean mAnyPartFailed;
        // The full message content of a single part message
        // or a multipart message that this part belongs to
        private String mFullMessageText;

        private int mSubId;

        // If this is a text message (instead of data message)
        private boolean mIsText;

        private boolean mPersistMessage;

        private SmsTracker(HashMap<String, Object> data, PendingIntent sentIntent,
                PendingIntent deliveryIntent, PackageInfo appInfo, String destAddr, String format,
                AtomicInteger unsentPartCount, AtomicBoolean anyPartFailed, Uri messageUri,
                SmsHeader smsHeader, boolean isExpectMore, String fullMessageText, int subId,
                boolean isText, boolean persistMessage) {
            mData = data;
            mSentIntent = sentIntent;
            mDeliveryIntent = deliveryIntent;
            mRetryCount = 0;
            mAppInfo = appInfo;
            mDestAddress = destAddr;
            mFormat = format;
            mExpectMore = isExpectMore;
            mImsRetry = 0;
            mMessageRef = 0;
            mUnsentPartCount = unsentPartCount;
            mAnyPartFailed = anyPartFailed;
            mMessageUri = messageUri;
            mSmsHeader = smsHeader;
            mFullMessageText = fullMessageText;
            mSubId = subId;
            mIsText = isText;
            mPersistMessage = persistMessage;
        }

        /**
         * Returns whether this tracker holds a multi-part SMS.
         * @return true if the tracker holds a multi-part SMS; false otherwise
         */
        boolean isMultipart() {
            return mData.containsKey("parts");
        }

        public HashMap<String, Object> getData() {
            return mData;
        }

        /**
         * Update the status of this message if we persisted it
         */
        public void updateSentMessageStatus(Context context, int status) {
            if (mMessageUri != null) {
                // If we wrote this message in writeSentMessage, update it now
                ContentValues values = new ContentValues(1);
                values.put(Sms.STATUS, status);
                SqliteWrapper.update(context, context.getContentResolver(),
                        mMessageUri, values, null, null);
            }
        }

        /**
         * Set the final state of a message: FAILED or SENT
         *
         * @param context The Context
         * @param messageType The final message type
         * @param errorCode The error code
         */
        private void updateMessageState(Context context, int messageType, int errorCode) {
            if (mMessageUri == null) {
                return;
            }
            final ContentValues values = new ContentValues(2);
            values.put(Sms.TYPE, messageType);
            values.put(Sms.ERROR_CODE, errorCode);
            final long identity = Binder.clearCallingIdentity();
            try {
                if (SqliteWrapper.update(context, context.getContentResolver(), mMessageUri, values,
                        null/*where*/, null/*selectionArgs*/) != 1) {
                    Rlog.e(TAG, "Failed to move message to " + messageType);
                }
            } finally {
                Binder.restoreCallingIdentity(identity);
            }
        }

        /**
         * Persist a sent SMS if required:
         * 1. It is a text message
         * 2. SmsApplication tells us to persist: sent from apps that are not default-SMS app or
         *    bluetooth
         *
         * @param context
         * @param messageType The folder to store (FAILED or SENT)
         * @param errorCode The current error code for this SMS or SMS part
         * @return The telephony provider URI if stored
         */
        private Uri persistSentMessageIfRequired(Context context, int messageType, int errorCode) {
            if (!mIsText || !mPersistMessage ||
                    !SmsApplication.shouldWriteMessageForPackage(mAppInfo.packageName, context)) {
                return null;
            }
            Rlog.d(TAG, "Persist SMS into "
                    + (messageType == Sms.MESSAGE_TYPE_FAILED ? "FAILED" : "SENT"));
            final ContentValues values = new ContentValues();
            values.put(Sms.SUBSCRIPTION_ID, mSubId);
            values.put(Sms.ADDRESS, mDestAddress);
            values.put(Sms.BODY, mFullMessageText);
            values.put(Sms.DATE, System.currentTimeMillis()); // milliseconds
            values.put(Sms.SEEN, 1);
            values.put(Sms.READ, 1);
            final String creator = mAppInfo != null ? mAppInfo.packageName : null;
            if (!TextUtils.isEmpty(creator)) {
                values.put(Sms.CREATOR, creator);
            }
            if (mDeliveryIntent != null) {
                values.put(Sms.STATUS, Telephony.Sms.STATUS_PENDING);
            }
            if (errorCode != 0) {
                values.put(Sms.ERROR_CODE, errorCode);
            }
            final long identity = Binder.clearCallingIdentity();
            final ContentResolver resolver = context.getContentResolver();
            try {
                final Uri uri =  resolver.insert(Telephony.Sms.Sent.CONTENT_URI, values);
                if (uri != null && messageType == Sms.MESSAGE_TYPE_FAILED) {
                    // Since we can't persist a message directly into FAILED box,
                    // we have to update the column after we persist it into SENT box.
                    // The gap between the state change is tiny so I would not expect
                    // it to cause any serious problem
                    // TODO: we should add a "failed" URI for this in SmsProvider?
                    final ContentValues updateValues = new ContentValues(1);
                    updateValues.put(Sms.TYPE, Sms.MESSAGE_TYPE_FAILED);
                    resolver.update(uri, updateValues, null/*where*/, null/*selectionArgs*/);
                }
                return uri;
            } catch (Exception e) {
                Rlog.e(TAG, "writeOutboxMessage: Failed to persist outbox message", e);
                return null;
            } finally {
                Binder.restoreCallingIdentity(identity);
            }
        }

        /**
         * Persist or update an SMS depending on if we send a new message or a stored message
         *
         * @param context
         * @param messageType The message folder for this SMS, FAILED or SENT
         * @param errorCode The current error code for this SMS or SMS part
         */
        private void persistOrUpdateMessage(Context context, int messageType, int errorCode) {
            if (mMessageUri != null) {
                updateMessageState(context, messageType, errorCode);
            } else {
                mMessageUri = persistSentMessageIfRequired(context, messageType, errorCode);
            }
        }

        /**
         * Handle a failure of a single part message or a part of a multipart message
         *
         * @param context The Context
         * @param error The error to send back with
         * @param errorCode
         */
        public void onFailed(Context context, int error, int errorCode) {
            if (mAnyPartFailed != null) {
                mAnyPartFailed.set(true);
            }
            // is single part or last part of multipart message
            boolean isSinglePartOrLastPart = true;
            if (mUnsentPartCount != null) {
                isSinglePartOrLastPart = mUnsentPartCount.decrementAndGet() == 0;
            }
            if (isSinglePartOrLastPart) {
                persistOrUpdateMessage(context, Sms.MESSAGE_TYPE_FAILED, errorCode);
            }
            if (mSentIntent != null) {
                try {
                    // Extra information to send with the sent intent
                    Intent fillIn = new Intent();
                    if (mMessageUri != null) {
                        // Pass this to SMS apps so that they know where it is stored
                        fillIn.putExtra("uri", mMessageUri.toString());
                    }
                    if (errorCode != 0) {
                        fillIn.putExtra("errorCode", errorCode);
                    }
                    if (mUnsentPartCount != null && isSinglePartOrLastPart) {
                        // Is multipart and last part
                        fillIn.putExtra(SEND_NEXT_MSG_EXTRA, true);
                    }
                    mSentIntent.send(context, error, fillIn);
                } catch (CanceledException ex) {
                    Rlog.e(TAG, "Failed to send result");
                }
            }
        }

        /**
         * Handle the sent of a single part message or a part of a multipart message
         *
         * @param context The Context
         */
        public void onSent(Context context) {
            // is single part or last part of multipart message
            boolean isSinglePartOrLastPart = true;
            if (mUnsentPartCount != null) {
                isSinglePartOrLastPart = mUnsentPartCount.decrementAndGet() == 0;
            }
            if (isSinglePartOrLastPart) {
                int messageType = Sms.MESSAGE_TYPE_SENT;
                if (mAnyPartFailed != null && mAnyPartFailed.get()) {
                    messageType = Sms.MESSAGE_TYPE_FAILED;
                }
                persistOrUpdateMessage(context, messageType, 0/*errorCode*/);
            }
            if (mSentIntent != null) {
                try {
                    // Extra information to send with the sent intent
                    Intent fillIn = new Intent();
                    if (mMessageUri != null) {
                        // Pass this to SMS apps so that they know where it is stored
                        fillIn.putExtra("uri", mMessageUri.toString());
                    }
                    if (mUnsentPartCount != null && isSinglePartOrLastPart) {
                        // Is multipart and last part
                        fillIn.putExtra(SEND_NEXT_MSG_EXTRA, true);
                    }
                    mSentIntent.send(context, Activity.RESULT_OK, fillIn);
                } catch (CanceledException ex) {
                    Rlog.e(TAG, "Failed to send result");
                }
            }
        }
    }

    protected SmsTracker getSmsTracker(HashMap<String, Object> data, PendingIntent sentIntent,
            PendingIntent deliveryIntent, String format, AtomicInteger unsentPartCount,
            AtomicBoolean anyPartFailed, Uri messageUri, SmsHeader smsHeader,
            boolean isExpectMore, String fullMessageText, boolean isText, boolean persistMessage) {
        // Get calling app package name via UID from Binder call
        PackageManager pm = mContext.getPackageManager();
        String[] packageNames = pm.getPackagesForUid(Binder.getCallingUid());

        // Get package info via packagemanager
        PackageInfo appInfo = null;
        if (packageNames != null && packageNames.length > 0) {
            try {
                // XXX this is lossy- apps can share a UID
                appInfo = pm.getPackageInfo(packageNames[0], PackageManager.GET_SIGNATURES);
            } catch (PackageManager.NameNotFoundException e) {
                // error will be logged in sendRawPdu
            }
        }
        // Strip non-digits from destination phone number before checking for short codes
        // and before displaying the number to the user if confirmation is required.
        String destAddr = PhoneNumberUtils.extractNetworkPortion((String) data.get("destAddr"));
        return new SmsTracker(data, sentIntent, deliveryIntent, appInfo, destAddr, format,
                unsentPartCount, anyPartFailed, messageUri, smsHeader, isExpectMore,
                fullMessageText, getSubId(), isText, persistMessage);
    }

    protected SmsTracker getSmsTracker(HashMap<String, Object> data, PendingIntent sentIntent,
            PendingIntent deliveryIntent, String format, Uri messageUri, boolean isExpectMore,
            String fullMessageText, boolean isText, boolean persistMessage) {
        return getSmsTracker(data, sentIntent, deliveryIntent, format, null/*unsentPartCount*/,
                null/*anyPartFailed*/, messageUri, null/*smsHeader*/, isExpectMore,
                fullMessageText, isText, persistMessage);
    }

    protected HashMap<String, Object> getSmsTrackerMap(String destAddr, String scAddr,
            String text, SmsMessageBase.SubmitPduBase pdu) {
        HashMap<String, Object> map = new HashMap<String, Object>();
        map.put("destAddr", destAddr);
        map.put("scAddr", scAddr);
        map.put("text", text);
        map.put("smsc", pdu.encodedScAddress);
        map.put("pdu", pdu.encodedMessage);
        return map;
    }

    protected HashMap<String, Object> getSmsTrackerMap(String destAddr, String scAddr,
            int destPort, byte[] data, SmsMessageBase.SubmitPduBase pdu) {
        HashMap<String, Object> map = new HashMap<String, Object>();
        map.put("destAddr", destAddr);
        map.put("scAddr", scAddr);
        map.put("destPort", destPort);
        map.put("data", data);
        map.put("smsc", pdu.encodedScAddress);
        map.put("pdu", pdu.encodedMessage);
        return map;
    }

    /**
     * Dialog listener for SMS confirmation dialog.
     */
    private final class ConfirmDialogListener
            implements DialogInterface.OnClickListener, DialogInterface.OnCancelListener,
            CompoundButton.OnCheckedChangeListener {

        private final SmsTracker mTracker;
        private Button mPositiveButton;
        private Button mNegativeButton;
        private boolean mRememberChoice;    // default is unchecked
        private final TextView mRememberUndoInstruction;

        ConfirmDialogListener(SmsTracker tracker, TextView textView) {
            mTracker = tracker;
            mRememberUndoInstruction = textView;
        }

        void setPositiveButton(Button button) {
            mPositiveButton = button;
        }

        void setNegativeButton(Button button) {
            mNegativeButton = button;
        }

        @Override
        public void onClick(DialogInterface dialog, int which) {
            // Always set the SMS permission so that Settings will show a permission setting
            // for the app (it won't be shown until after the app tries to send to a short code).
            int newSmsPermission = SmsUsageMonitor.PREMIUM_SMS_PERMISSION_ASK_USER;

            if (which == DialogInterface.BUTTON_POSITIVE) {
                Rlog.d(TAG, "CONFIRM sending SMS");
                // XXX this is lossy- apps can have more than one signature
                EventLog.writeEvent(EventLogTags.EXP_DET_SMS_SENT_BY_USER,
                                    mTracker.mAppInfo.applicationInfo == null ?
                                    -1 : mTracker.mAppInfo.applicationInfo.uid);
                sendMessage(obtainMessage(EVENT_SEND_CONFIRMED_SMS, mTracker));
                if (mRememberChoice) {
                    newSmsPermission = SmsUsageMonitor.PREMIUM_SMS_PERMISSION_ALWAYS_ALLOW;
                }
            } else if (which == DialogInterface.BUTTON_NEGATIVE) {
                Rlog.d(TAG, "DENY sending SMS");
                // XXX this is lossy- apps can have more than one signature
                EventLog.writeEvent(EventLogTags.EXP_DET_SMS_DENIED_BY_USER,
                                    mTracker.mAppInfo.applicationInfo == null ?
                                    -1 :  mTracker.mAppInfo.applicationInfo.uid);
                sendMessage(obtainMessage(EVENT_STOP_SENDING, mTracker));
                if (mRememberChoice) {
                    newSmsPermission = SmsUsageMonitor.PREMIUM_SMS_PERMISSION_NEVER_ALLOW;
                }
            }
            setPremiumSmsPermission(mTracker.mAppInfo.packageName, newSmsPermission);
        }

        @Override
        public void onCancel(DialogInterface dialog) {
            Rlog.d(TAG, "dialog dismissed: don't send SMS");
            sendMessage(obtainMessage(EVENT_STOP_SENDING, mTracker));
        }

        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
            Rlog.d(TAG, "remember this choice: " + isChecked);
            mRememberChoice = isChecked;
            if (isChecked) {
                mPositiveButton.setText(R.string.sms_short_code_confirm_always_allow);
                mNegativeButton.setText(R.string.sms_short_code_confirm_never_allow);
                if (mRememberUndoInstruction != null) {
                    mRememberUndoInstruction.
                            setText(R.string.sms_short_code_remember_undo_instruction);
                    mRememberUndoInstruction.setPadding(0,0,0,32);
                }
            } else {
                mPositiveButton.setText(R.string.sms_short_code_confirm_allow);
                mNegativeButton.setText(R.string.sms_short_code_confirm_deny);
                if (mRememberUndoInstruction != null) {
                    mRememberUndoInstruction.setText("");
                    mRememberUndoInstruction.setPadding(0,0,0,0);
                }
            }
        }
    }

    public boolean isIms() {
        if (mImsSMSDispatcher != null) {
            return mImsSMSDispatcher.isIms();
        } else {
            Rlog.e(TAG, mImsSMSDispatcher + " is null");
            return false;
        }
    }

    public String getImsSmsFormat() {
        if (mImsSMSDispatcher != null) {
            return mImsSMSDispatcher.getImsSmsFormat();
        } else {
            Rlog.e(TAG, mImsSMSDispatcher + " is null");
            return null;
        }
    }

    private String getMultipartMessageText(ArrayList<String> parts) {
        final StringBuilder sb = new StringBuilder();
        for (String part : parts) {
            if (part != null) {
                sb.append(part);
            }
        }
        return sb.toString();
    }

    protected String getCarrierAppPackageName() {
        UiccCard card = UiccController.getInstance().getUiccCard(mPhone.getPhoneId());
        if (card == null) {
            return null;
        }

        List<String> carrierPackages = card.getCarrierPackageNamesForIntent(
            mContext.getPackageManager(), new Intent(CarrierMessagingService.SERVICE_INTERFACE));
        return (carrierPackages != null && carrierPackages.size() == 1) ?
                carrierPackages.get(0) : null;
    }

    protected int getSubId() {
        return SubscriptionController.getInstance().getSubIdUsingPhoneId(mPhone.getPhoneId());
    }

    private void checkCallerIsPhoneOrCarrierApp() {
        int uid = Binder.getCallingUid();
        int appId = UserHandle.getAppId(uid);
        if (appId == Process.PHONE_UID || uid == 0) {
            return;
        }
        try {
            PackageManager pm = mContext.getPackageManager();
            ApplicationInfo ai = pm.getApplicationInfo(getCarrierAppPackageName(), 0);
            if (!UserHandle.isSameApp(ai.uid, Binder.getCallingUid())) {
                throw new SecurityException("Caller is not phone or carrier app!");
            }
        } catch (PackageManager.NameNotFoundException re) {
            throw new SecurityException("Caller is not phone or carrier app!");
        }
    }
}
