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

package com.android.internal.telephony;

import static android.telephony.SmsManager.STATUS_ON_ICC_FREE;
import static android.telephony.SmsManager.STATUS_ON_ICC_READ;
import static android.telephony.SmsManager.STATUS_ON_ICC_UNREAD;

import android.Manifest;
import android.app.AppOpsManager;
import android.app.PendingIntent;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Binder;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.os.UserManager;
import android.provider.Telephony;
import android.telephony.Rlog;
import android.telephony.SmsManager;
import android.telephony.SmsMessage;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.telephony.cdma.CdmaSmsBroadcastConfigInfo;
import com.android.internal.telephony.gsm.SmsBroadcastConfigInfo;
import com.android.internal.telephony.uicc.IccConstants;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.util.HexDump;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * IccSmsInterfaceManager to provide an inter-process communication to
 * access Sms in Icc.
 */
public class IccSmsInterfaceManager {
    static final String LOG_TAG = "IccSmsInterfaceManager";
    static final boolean DBG = true;

    protected final Object mLock = new Object();
    protected boolean mSuccess;
    private List<SmsRawData> mSms;

    private CellBroadcastRangeManager mCellBroadcastRangeManager =
            new CellBroadcastRangeManager();
    private CdmaBroadcastRangeManager mCdmaBroadcastRangeManager =
            new CdmaBroadcastRangeManager();

    private static final int EVENT_LOAD_DONE = 1;
    private static final int EVENT_UPDATE_DONE = 2;
    protected static final int EVENT_SET_BROADCAST_ACTIVATION_DONE = 3;
    protected static final int EVENT_SET_BROADCAST_CONFIG_DONE = 4;
    private static final int SMS_CB_CODE_SCHEME_MIN = 0;
    private static final int SMS_CB_CODE_SCHEME_MAX = 255;

    protected Phone mPhone;
    final protected Context mContext;
    final protected AppOpsManager mAppOps;
    final private UserManager mUserManager;
    protected SMSDispatcher mDispatcher;

    protected Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            AsyncResult ar;

            switch (msg.what) {
                case EVENT_UPDATE_DONE:
                    ar = (AsyncResult) msg.obj;
                    synchronized (mLock) {
                        mSuccess = (ar.exception == null);
                        mLock.notifyAll();
                    }
                    break;
                case EVENT_LOAD_DONE:
                    ar = (AsyncResult)msg.obj;
                    synchronized (mLock) {
                        if (ar.exception == null) {
                            mSms = buildValidRawData((ArrayList<byte[]>) ar.result);
                            //Mark SMS as read after importing it from card.
                            markMessagesAsRead((ArrayList<byte[]>) ar.result);
                        } else {
                            if (Rlog.isLoggable("SMS", Log.DEBUG)) {
                                log("Cannot load Sms records");
                            }
                            mSms = null;
                        }
                        mLock.notifyAll();
                    }
                    break;
                case EVENT_SET_BROADCAST_ACTIVATION_DONE:
                case EVENT_SET_BROADCAST_CONFIG_DONE:
                    ar = (AsyncResult) msg.obj;
                    synchronized (mLock) {
                        mSuccess = (ar.exception == null);
                        mLock.notifyAll();
                    }
                    break;
            }
        }
    };

    protected IccSmsInterfaceManager(Phone phone) {
        mPhone = phone;
        mContext = phone.getContext();
        mAppOps = (AppOpsManager) mContext.getSystemService(Context.APP_OPS_SERVICE);
        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mDispatcher = new ImsSMSDispatcher(phone,
                phone.mSmsStorageMonitor, phone.mSmsUsageMonitor);
    }

    protected void markMessagesAsRead(ArrayList<byte[]> messages) {
        if (messages == null) {
            return;
        }

        //IccFileHandler can be null, if icc card is absent.
        IccFileHandler fh = mPhone.getIccFileHandler();
        if (fh == null) {
            //shouldn't really happen, as messages are marked as read, only
            //after importing it from icc.
            if (Rlog.isLoggable("SMS", Log.DEBUG)) {
                log("markMessagesAsRead - aborting, no icc card present.");
            }
            return;
        }

        int count = messages.size();

        for (int i = 0; i < count; i++) {
             byte[] ba = messages.get(i);
             if (ba[0] == STATUS_ON_ICC_UNREAD) {
                 int n = ba.length;
                 byte[] nba = new byte[n - 1];
                 System.arraycopy(ba, 1, nba, 0, n - 1);
                 byte[] record = makeSmsRecordData(STATUS_ON_ICC_READ, nba);
                 fh.updateEFLinearFixed(IccConstants.EF_SMS, i + 1, record, null, null);
                 if (Rlog.isLoggable("SMS", Log.DEBUG)) {
                     log("SMS " + (i + 1) + " marked as read");
                 }
             }
        }
    }

    protected void updatePhoneObject(Phone phone) {
        mPhone = phone;
        mDispatcher.updatePhoneObject(phone);
    }

    protected void enforceReceiveAndSend(String message) {
        mContext.enforceCallingOrSelfPermission(
                Manifest.permission.RECEIVE_SMS, message);
        mContext.enforceCallingOrSelfPermission(
                Manifest.permission.SEND_SMS, message);
    }

    /**
     * Update the specified message on the Icc.
     *
     * @param index record index of message to update
     * @param status new message status (STATUS_ON_ICC_READ,
     *                  STATUS_ON_ICC_UNREAD, STATUS_ON_ICC_SENT,
     *                  STATUS_ON_ICC_UNSENT, STATUS_ON_ICC_FREE)
     * @param pdu the raw PDU to store
     * @return success or not
     *
     */

    public boolean
    updateMessageOnIccEf(String callingPackage, int index, int status, byte[] pdu) {
        if (DBG) log("updateMessageOnIccEf: index=" + index +
                " status=" + status + " ==> " +
                "("+ Arrays.toString(pdu) + ")");
        enforceReceiveAndSend("Updating message on Icc");
        if (mAppOps.noteOp(AppOpsManager.OP_WRITE_ICC_SMS, Binder.getCallingUid(),
                callingPackage) != AppOpsManager.MODE_ALLOWED) {
            return false;
        }
        synchronized(mLock) {
            mSuccess = false;
            Message response = mHandler.obtainMessage(EVENT_UPDATE_DONE);

            if (status == STATUS_ON_ICC_FREE) {
                // RIL_REQUEST_DELETE_SMS_ON_SIM vs RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM
                // Special case FREE: call deleteSmsOnSim/Ruim instead of
                // manipulating the record
                // Will eventually fail if icc card is not present.
                if (PhoneConstants.PHONE_TYPE_GSM == mPhone.getPhoneType()) {
                    mPhone.mCi.deleteSmsOnSim(index, response);
                } else {
                    mPhone.mCi.deleteSmsOnRuim(index, response);
                }
            } else {
                //IccFilehandler can be null if ICC card is not present.
                IccFileHandler fh = mPhone.getIccFileHandler();
                if (fh == null) {
                    response.recycle();
                    return mSuccess; /* is false */
                }
                byte[] record = makeSmsRecordData(status, pdu);
                fh.updateEFLinearFixed(
                        IccConstants.EF_SMS,
                        index, record, null, response);
            }
            try {
                mLock.wait();
            } catch (InterruptedException e) {
                log("interrupted while trying to update by index");
            }
        }
        return mSuccess;
    }

    /**
     * Copy a raw SMS PDU to the Icc.
     *
     * @param pdu the raw PDU to store
     * @param status message status (STATUS_ON_ICC_READ, STATUS_ON_ICC_UNREAD,
     *               STATUS_ON_ICC_SENT, STATUS_ON_ICC_UNSENT)
     * @return success or not
     *
     */
    public boolean copyMessageToIccEf(String callingPackage, int status, byte[] pdu, byte[] smsc) {
        //NOTE smsc not used in RUIM
        if (DBG) log("copyMessageToIccEf: status=" + status + " ==> " +
                "pdu=("+ Arrays.toString(pdu) +
                "), smsc=(" + Arrays.toString(smsc) +")");
        enforceReceiveAndSend("Copying message to Icc");
        if (mAppOps.noteOp(AppOpsManager.OP_WRITE_ICC_SMS, Binder.getCallingUid(),
                callingPackage) != AppOpsManager.MODE_ALLOWED) {
            return false;
        }
        synchronized(mLock) {
            mSuccess = false;
            Message response = mHandler.obtainMessage(EVENT_UPDATE_DONE);

            //RIL_REQUEST_WRITE_SMS_TO_SIM vs RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM
            if (PhoneConstants.PHONE_TYPE_GSM == mPhone.getPhoneType()) {
                mPhone.mCi.writeSmsToSim(status, IccUtils.bytesToHexString(smsc),
                        IccUtils.bytesToHexString(pdu), response);
            } else {
                mPhone.mCi.writeSmsToRuim(status, IccUtils.bytesToHexString(pdu),
                        response);
            }

            try {
                mLock.wait();
            } catch (InterruptedException e) {
                log("interrupted while trying to update by index");
            }
        }
        return mSuccess;
    }

    /**
     * Retrieves all messages currently stored on Icc.
     *
     * @return list of SmsRawData of all sms on Icc
     */

    public List<SmsRawData> getAllMessagesFromIccEf(String callingPackage) {
        if (DBG) log("getAllMessagesFromEF");

        mContext.enforceCallingOrSelfPermission(
                Manifest.permission.RECEIVE_SMS,
                "Reading messages from Icc");
        if (mAppOps.noteOp(AppOpsManager.OP_READ_ICC_SMS, Binder.getCallingUid(),
                callingPackage) != AppOpsManager.MODE_ALLOWED) {
            return new ArrayList<SmsRawData>();
        }
        synchronized(mLock) {

            IccFileHandler fh = mPhone.getIccFileHandler();
            if (fh == null) {
                Rlog.e(LOG_TAG, "Cannot load Sms records. No icc card?");
                mSms = null;
                return mSms;
            }

            Message response = mHandler.obtainMessage(EVENT_LOAD_DONE);
            fh.loadEFLinearFixedAll(IccConstants.EF_SMS, response);

            try {
                mLock.wait();
            } catch (InterruptedException e) {
                log("interrupted while trying to load from the Icc");
            }
        }
        return mSms;
    }

    /**
     * A permissions check before passing to {@link IccSmsInterfaceManager#sendDataInternal}.
     * This method checks if the calling package or itself has the permission to send the data sms.
     */
    public void sendDataWithSelfPermissions(String callingPackage, String destAddr, String scAddr,
            int destPort, byte[] data, PendingIntent sentIntent, PendingIntent deliveryIntent) {
        mPhone.getContext().enforceCallingOrSelfPermission(
                Manifest.permission.SEND_SMS,
                "Sending SMS message");
        sendDataInternal(callingPackage, destAddr, scAddr, destPort, data, sentIntent,
                deliveryIntent);
    }

    /**
     * A permissions check before passing to {@link IccSmsInterfaceManager#sendDataInternal}.
     * This method checks only if the calling package has the permission to send the data sms.
     */
    public void sendData(String callingPackage, String destAddr, String scAddr, int destPort,
            byte[] data, PendingIntent sentIntent, PendingIntent deliveryIntent) {
        mPhone.getContext().enforceCallingPermission(
                Manifest.permission.SEND_SMS,
                "Sending SMS message");
        sendDataInternal(callingPackage, destAddr, scAddr, destPort, data, sentIntent,
                deliveryIntent);
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

    private void sendDataInternal(String callingPackage, String destAddr, String scAddr,
            int destPort, byte[] data, PendingIntent sentIntent, PendingIntent deliveryIntent) {
        if (Rlog.isLoggable("SMS", Log.VERBOSE)) {
            log("sendData: destAddr=" + destAddr + " scAddr=" + scAddr + " destPort=" +
                destPort + " data='"+ HexDump.toHexString(data)  + "' sentIntent=" +
                sentIntent + " deliveryIntent=" + deliveryIntent);
        }
        if (mAppOps.noteOp(AppOpsManager.OP_SEND_SMS, Binder.getCallingUid(),
                callingPackage) != AppOpsManager.MODE_ALLOWED) {
            return;
        }
        destAddr = filterDestAddress(destAddr);
        mDispatcher.sendData(destAddr, scAddr, destPort, data, sentIntent, deliveryIntent);
    }

    /**
     * A permissions check before passing to {@link IccSmsInterfaceManager#sendTextInternal}.
     * This method checks only if the calling package has the permission to send the sms.
     */
    public void sendText(String callingPackage, String destAddr, String scAddr,
            String text, PendingIntent sentIntent, PendingIntent deliveryIntent,
            boolean persistMessageForNonDefaultSmsApp) {
        mPhone.getContext().enforceCallingPermission(
                Manifest.permission.SEND_SMS,
                "Sending SMS message");
        sendTextInternal(callingPackage, destAddr, scAddr, text, sentIntent, deliveryIntent,
            persistMessageForNonDefaultSmsApp);
    }

    /**
     * A permissions check before passing to {@link IccSmsInterfaceManager#sendTextInternal}.
     * This method checks if the calling package or itself has the permission to send the sms.
     */
    public void sendTextWithSelfPermissions(String callingPackage, String destAddr, String scAddr,
            String text, PendingIntent sentIntent, PendingIntent deliveryIntent,
            boolean persistMessage) {
        mPhone.getContext().enforceCallingOrSelfPermission(
                Manifest.permission.SEND_SMS,
                "Sending SMS message");
        sendTextInternal(callingPackage, destAddr, scAddr, text, sentIntent, deliveryIntent,
            persistMessage);
    }

    /**
     * Send a text based SMS.
     *
     * @param destAddr the address to send the message to
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

    private void sendTextInternal(String callingPackage, String destAddr, String scAddr,
            String text, PendingIntent sentIntent, PendingIntent deliveryIntent,
            boolean persistMessageForNonDefaultSmsApp) {
        if (Rlog.isLoggable("SMS", Log.VERBOSE)) {
            log("sendText: destAddr=" + destAddr + " scAddr=" + scAddr +
                " text='"+ text + "' sentIntent=" +
                sentIntent + " deliveryIntent=" + deliveryIntent);
        }
        if (mAppOps.noteOp(AppOpsManager.OP_SEND_SMS, Binder.getCallingUid(),
                callingPackage) != AppOpsManager.MODE_ALLOWED) {
            return;
        }
        if (!persistMessageForNonDefaultSmsApp) {
            // Only allow carrier app or phone process to skip auto message persistence.
            enforceCarrierOrPhonePrivilege();
        }
        destAddr = filterDestAddress(destAddr);
        mDispatcher.sendText(destAddr, scAddr, text, sentIntent, deliveryIntent,
                null/*messageUri*/, callingPackage, persistMessageForNonDefaultSmsApp);
    }

    /**
     * Inject an SMS PDU into the android application framework.
     *
     * @param pdu is the byte array of pdu to be injected into android application framework
     * @param format is the format of SMS pdu (3gpp or 3gpp2)
     * @param receivedIntent if not NULL this <code>PendingIntent</code> is
     *  broadcast when the message is successfully received by the
     *  android application framework. This intent is broadcasted at
     *  the same time an SMS received from radio is acknowledged back.
     */
    public void injectSmsPdu(byte[] pdu, String format, PendingIntent receivedIntent) {
        enforceCarrierPrivilege();
        if (Rlog.isLoggable("SMS", Log.VERBOSE)) {
            log("pdu: " + pdu +
                "\n format=" + format +
                "\n receivedIntent=" + receivedIntent);
        }
        mDispatcher.injectSmsPdu(pdu, format, receivedIntent);
    }

    /**
     * Send a multi-part text based SMS.
     *
     * @param destAddr the address to send the message to
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
     *   <code>RESULT_ERROR_NULL_PDU</code>.
     *  The per-application based SMS control checks sentIntent. If sentIntent
     *  is NULL the caller will be checked against all unknown applications,
     *  which cause smaller number of SMS to be sent in checking period.
     * @param deliveryIntents if not null, an <code>ArrayList</code> of
     *   <code>PendingIntent</code>s (one for each message part) that is
     *   broadcast when the corresponding message part has been delivered
     *   to the recipient.  The raw pdu of the status report is in the
     *   extended data ("pdu").
     */

    public void sendMultipartText(String callingPackage, String destAddr, String scAddr,
            List<String> parts, List<PendingIntent> sentIntents,
            List<PendingIntent> deliveryIntents, boolean persistMessageForNonDefaultSmsApp) {
        mPhone.getContext().enforceCallingPermission(
                Manifest.permission.SEND_SMS,
                "Sending SMS message");
        if (!persistMessageForNonDefaultSmsApp) {
            // Only allow carrier app to skip auto message persistence.
            enforceCarrierPrivilege();
        }
        if (Rlog.isLoggable("SMS", Log.VERBOSE)) {
            int i = 0;
            for (String part : parts) {
                log("sendMultipartText: destAddr=" + destAddr + ", srAddr=" + scAddr +
                        ", part[" + (i++) + "]=" + part);
            }
        }
        if (mAppOps.noteOp(AppOpsManager.OP_SEND_SMS, Binder.getCallingUid(),
                callingPackage) != AppOpsManager.MODE_ALLOWED) {
            return;
        }

        destAddr = filterDestAddress(destAddr);

        if (parts.size() > 1 && parts.size() < 10 && !SmsMessage.hasEmsSupport()) {
            for (int i = 0; i < parts.size(); i++) {
                // If EMS is not supported, we have to break down EMS into single segment SMS
                // and add page info " x/y".
                String singlePart = parts.get(i);
                if (SmsMessage.shouldAppendPageNumberAsPrefix()) {
                    singlePart = String.valueOf(i + 1) + '/' + parts.size() + ' ' + singlePart;
                } else {
                    singlePart = singlePart.concat(' ' + String.valueOf(i + 1) + '/' + parts.size());
                }

                PendingIntent singleSentIntent = null;
                if (sentIntents != null && sentIntents.size() > i) {
                    singleSentIntent = sentIntents.get(i);
                }

                PendingIntent singleDeliveryIntent = null;
                if (deliveryIntents != null && deliveryIntents.size() > i) {
                    singleDeliveryIntent = deliveryIntents.get(i);
                }

                mDispatcher.sendText(destAddr, scAddr, singlePart,
                        singleSentIntent, singleDeliveryIntent,
                        null/*messageUri*/, callingPackage,
                        persistMessageForNonDefaultSmsApp);
            }
            return;
        }

        mDispatcher.sendMultipartText(destAddr, scAddr, (ArrayList<String>) parts,
                (ArrayList<PendingIntent>) sentIntents, (ArrayList<PendingIntent>) deliveryIntents,
                null/*messageUri*/, callingPackage, persistMessageForNonDefaultSmsApp);
    }


    public int getPremiumSmsPermission(String packageName) {
        return mDispatcher.getPremiumSmsPermission(packageName);
    }


    public void setPremiumSmsPermission(String packageName, int permission) {
        mDispatcher.setPremiumSmsPermission(packageName, permission);
    }

    /**
     * create SmsRawData lists from all sms record byte[]
     * Use null to indicate "free" record
     *
     * @param messages List of message records from EF_SMS.
     * @return SmsRawData list of all in-used records
     */
    protected ArrayList<SmsRawData> buildValidRawData(ArrayList<byte[]> messages) {
        int count = messages.size();
        ArrayList<SmsRawData> ret;

        ret = new ArrayList<SmsRawData>(count);

        for (int i = 0; i < count; i++) {
            byte[] ba = messages.get(i);
            if (ba[0] == STATUS_ON_ICC_FREE) {
                ret.add(null);
            } else {
                ret.add(new SmsRawData(messages.get(i)));
            }
        }

        return ret;
    }

    /**
     * Generates an EF_SMS record from status and raw PDU.
     *
     * @param status Message status.  See TS 51.011 10.5.3.
     * @param pdu Raw message PDU.
     * @return byte array for the record.
     */
    protected byte[] makeSmsRecordData(int status, byte[] pdu) {
        byte[] data;
        if (PhoneConstants.PHONE_TYPE_GSM == mPhone.getPhoneType()) {
            data = new byte[IccConstants.SMS_RECORD_LENGTH];
        } else {
            data = new byte[IccConstants.CDMA_SMS_RECORD_LENGTH];
        }

        // Status bits for this record.  See TS 51.011 10.5.3
        data[0] = (byte)(status & 7);

        System.arraycopy(pdu, 0, data, 1, pdu.length);

        // Pad out with 0xFF's.
        for (int j = pdu.length+1; j < data.length; j++) {
            data[j] = -1;
        }

        return data;
    }

    public boolean enableCellBroadcast(int messageIdentifier, int ranType) {
        return enableCellBroadcastRange(messageIdentifier, messageIdentifier, ranType);
    }

    public boolean disableCellBroadcast(int messageIdentifier, int ranType) {
        return disableCellBroadcastRange(messageIdentifier, messageIdentifier, ranType);
    }

    public boolean enableCellBroadcastRange(int startMessageId, int endMessageId, int ranType) {
        if (ranType == SmsManager.CELL_BROADCAST_RAN_TYPE_GSM) {
            return enableGsmBroadcastRange(startMessageId, endMessageId);
        } else if (ranType == SmsManager.CELL_BROADCAST_RAN_TYPE_CDMA) {
            return enableCdmaBroadcastRange(startMessageId, endMessageId);
        } else {
            throw new IllegalArgumentException("Not a supportted RAN Type");
        }
    }

    public boolean disableCellBroadcastRange(int startMessageId, int endMessageId, int ranType) {
        if (ranType == SmsManager.CELL_BROADCAST_RAN_TYPE_GSM ) {
            return disableGsmBroadcastRange(startMessageId, endMessageId);
        } else if (ranType == SmsManager.CELL_BROADCAST_RAN_TYPE_CDMA)  {
            return disableCdmaBroadcastRange(startMessageId, endMessageId);
        } else {
            throw new IllegalArgumentException("Not a supportted RAN Type");
        }
    }

    synchronized public boolean enableGsmBroadcastRange(int startMessageId, int endMessageId) {

        Context context = mPhone.getContext();

        context.enforceCallingPermission(
                "android.permission.RECEIVE_SMS",
                "Enabling cell broadcast SMS");

        String client = context.getPackageManager().getNameForUid(
                Binder.getCallingUid());

        if (!mCellBroadcastRangeManager.enableRange(startMessageId, endMessageId, client)) {
            log("Failed to add GSM cell broadcast subscription for MID range " + startMessageId
                    + " to " + endMessageId + " from client " + client);
            return false;
        }

        if (DBG)
            log("Added GSM cell broadcast subscription for MID range " + startMessageId
                    + " to " + endMessageId + " from client " + client);

        setCellBroadcastActivation(!mCellBroadcastRangeManager.isEmpty());

        return true;
    }

    synchronized public boolean disableGsmBroadcastRange(int startMessageId, int endMessageId) {

        Context context = mPhone.getContext();

        context.enforceCallingPermission(
                "android.permission.RECEIVE_SMS",
                "Disabling cell broadcast SMS");

        String client = context.getPackageManager().getNameForUid(
                Binder.getCallingUid());

        if (!mCellBroadcastRangeManager.disableRange(startMessageId, endMessageId, client)) {
            log("Failed to remove GSM cell broadcast subscription for MID range " + startMessageId
                    + " to " + endMessageId + " from client " + client);
            return false;
        }

        if (DBG)
            log("Removed GSM cell broadcast subscription for MID range " + startMessageId
                    + " to " + endMessageId + " from client " + client);

        setCellBroadcastActivation(!mCellBroadcastRangeManager.isEmpty());

        return true;
    }

    synchronized public boolean enableCdmaBroadcastRange(int startMessageId, int endMessageId) {

        Context context = mPhone.getContext();

        context.enforceCallingPermission(
                "android.permission.RECEIVE_SMS",
                "Enabling cdma broadcast SMS");

        String client = context.getPackageManager().getNameForUid(
                Binder.getCallingUid());

        if (!mCdmaBroadcastRangeManager.enableRange(startMessageId, endMessageId, client)) {
            log("Failed to add cdma broadcast subscription for MID range " + startMessageId
                    + " to " + endMessageId + " from client " + client);
            return false;
        }

        if (DBG)
            log("Added cdma broadcast subscription for MID range " + startMessageId
                    + " to " + endMessageId + " from client " + client);

        setCdmaBroadcastActivation(!mCdmaBroadcastRangeManager.isEmpty());

        return true;
    }

    synchronized public boolean disableCdmaBroadcastRange(int startMessageId, int endMessageId) {

        Context context = mPhone.getContext();

        context.enforceCallingPermission(
                "android.permission.RECEIVE_SMS",
                "Disabling cell broadcast SMS");

        String client = context.getPackageManager().getNameForUid(
                Binder.getCallingUid());

        if (!mCdmaBroadcastRangeManager.disableRange(startMessageId, endMessageId, client)) {
            log("Failed to remove cdma broadcast subscription for MID range " + startMessageId
                    + " to " + endMessageId + " from client " + client);
            return false;
        }

        if (DBG)
            log("Removed cdma broadcast subscription for MID range " + startMessageId
                    + " to " + endMessageId + " from client " + client);

        setCdmaBroadcastActivation(!mCdmaBroadcastRangeManager.isEmpty());

        return true;
    }

    class CellBroadcastRangeManager extends IntRangeManager {
        private ArrayList<SmsBroadcastConfigInfo> mConfigList =
                new ArrayList<SmsBroadcastConfigInfo>();

        /**
         * Called when the list of enabled ranges has changed. This will be
         * followed by zero or more calls to {@link #addRange} followed by
         * a call to {@link #finishUpdate}.
         */
        protected void startUpdate() {
            mConfigList.clear();
        }

        /**
         * Called after {@link #startUpdate} to indicate a range of enabled
         * values.
         * @param startId the first id included in the range
         * @param endId the last id included in the range
         */
        protected void addRange(int startId, int endId, boolean selected) {
            mConfigList.add(new SmsBroadcastConfigInfo(startId, endId,
                        SMS_CB_CODE_SCHEME_MIN, SMS_CB_CODE_SCHEME_MAX, selected));
        }

        /**
         * Called to indicate the end of a range update started by the
         * previous call to {@link #startUpdate}.
         * @return true if successful, false otherwise
         */
        protected boolean finishUpdate() {
            if (mConfigList.isEmpty()) {
                return true;
            } else {
                SmsBroadcastConfigInfo[] configs =
                        mConfigList.toArray(new SmsBroadcastConfigInfo[mConfigList.size()]);
                return setCellBroadcastConfig(configs);
            }
        }
    }

    class CdmaBroadcastRangeManager extends IntRangeManager {
        private ArrayList<CdmaSmsBroadcastConfigInfo> mConfigList =
                new ArrayList<CdmaSmsBroadcastConfigInfo>();

        /**
         * Called when the list of enabled ranges has changed. This will be
         * followed by zero or more calls to {@link #addRange} followed by a
         * call to {@link #finishUpdate}.
         */
        protected void startUpdate() {
            mConfigList.clear();
        }

        /**
         * Called after {@link #startUpdate} to indicate a range of enabled
         * values.
         * @param startId the first id included in the range
         * @param endId the last id included in the range
         */
        protected void addRange(int startId, int endId, boolean selected) {
            mConfigList.add(new CdmaSmsBroadcastConfigInfo(startId, endId,
                    1, selected));
        }

        /**
         * Called to indicate the end of a range update started by the previous
         * call to {@link #startUpdate}.
         * @return true if successful, false otherwise
         */
        protected boolean finishUpdate() {
            if (mConfigList.isEmpty()) {
                return true;
            } else {
                CdmaSmsBroadcastConfigInfo[] configs =
                        mConfigList.toArray(new CdmaSmsBroadcastConfigInfo[mConfigList.size()]);
                return setCdmaBroadcastConfig(configs);
            }
        }
    }

    private boolean setCellBroadcastConfig(SmsBroadcastConfigInfo[] configs) {
        if (DBG)
            log("Calling setGsmBroadcastConfig with " + configs.length + " configurations");

        synchronized (mLock) {
            Message response = mHandler.obtainMessage(EVENT_SET_BROADCAST_CONFIG_DONE);

            mSuccess = false;
            mPhone.mCi.setGsmBroadcastConfig(configs, response);

            try {
                mLock.wait();
            } catch (InterruptedException e) {
                log("interrupted while trying to set cell broadcast config");
            }
        }

        return mSuccess;
    }

    private boolean setCellBroadcastActivation(boolean activate) {
        if (DBG)
            log("Calling setCellBroadcastActivation(" + activate + ')');

        synchronized (mLock) {
            Message response = mHandler.obtainMessage(EVENT_SET_BROADCAST_ACTIVATION_DONE);

            mSuccess = false;
            mPhone.mCi.setGsmBroadcastActivation(activate, response);

            try {
                mLock.wait();
            } catch (InterruptedException e) {
                log("interrupted while trying to set cell broadcast activation");
            }
        }

        return mSuccess;
    }

    private boolean setCdmaBroadcastConfig(CdmaSmsBroadcastConfigInfo[] configs) {
        if (DBG)
            log("Calling setCdmaBroadcastConfig with " + configs.length + " configurations");

        synchronized (mLock) {
            Message response = mHandler.obtainMessage(EVENT_SET_BROADCAST_CONFIG_DONE);

            mSuccess = false;
            mPhone.mCi.setCdmaBroadcastConfig(configs, response);

            try {
                mLock.wait();
            } catch (InterruptedException e) {
                log("interrupted while trying to set cdma broadcast config");
            }
        }

        return mSuccess;
    }

    private boolean setCdmaBroadcastActivation(boolean activate) {
        if (DBG)
            log("Calling setCdmaBroadcastActivation(" + activate + ")");

        synchronized (mLock) {
            Message response = mHandler.obtainMessage(EVENT_SET_BROADCAST_ACTIVATION_DONE);

            mSuccess = false;
            mPhone.mCi.setCdmaBroadcastActivation(activate, response);

            try {
                mLock.wait();
            } catch (InterruptedException e) {
                log("interrupted while trying to set cdma broadcast activation");
            }
        }

        return mSuccess;
    }

    protected void log(String msg) {
        Log.d(LOG_TAG, "[IccSmsInterfaceManager] " + msg);
    }

    public boolean isImsSmsSupported() {
        return mDispatcher.isIms();
    }

    public String getImsSmsFormat() {
        return mDispatcher.getImsSmsFormat();
    }

    public void sendStoredText(String callingPkg, Uri messageUri, String scAddress,
            PendingIntent sentIntent, PendingIntent deliveryIntent) {
        mPhone.getContext().enforceCallingPermission(Manifest.permission.SEND_SMS,
                "Sending SMS message");
        if (Rlog.isLoggable("SMS", Log.VERBOSE)) {
            log("sendStoredText: scAddr=" + scAddress + " messageUri=" + messageUri
                    + " sentIntent=" + sentIntent + " deliveryIntent=" + deliveryIntent);
        }
        if (mAppOps.noteOp(AppOpsManager.OP_SEND_SMS, Binder.getCallingUid(), callingPkg)
                != AppOpsManager.MODE_ALLOWED) {
            return;
        }
        final ContentResolver resolver = mPhone.getContext().getContentResolver();
        if (!isFailedOrDraft(resolver, messageUri)) {
            Log.e(LOG_TAG, "[IccSmsInterfaceManager]sendStoredText: not FAILED or DRAFT message");
            returnUnspecifiedFailure(sentIntent);
            return;
        }
        final String[] textAndAddress = loadTextAndAddress(resolver, messageUri);
        if (textAndAddress == null) {
            Log.e(LOG_TAG, "[IccSmsInterfaceManager]sendStoredText: can not load text");
            returnUnspecifiedFailure(sentIntent);
            return;
        }
        textAndAddress[1] = filterDestAddress(textAndAddress[1]);
        mDispatcher.sendText(textAndAddress[1], scAddress, textAndAddress[0],
                sentIntent, deliveryIntent, messageUri, callingPkg,
                true /* persistMessageForNonDefaultSmsApp */);
    }

    public void sendStoredMultipartText(String callingPkg, Uri messageUri, String scAddress,
            List<PendingIntent> sentIntents, List<PendingIntent> deliveryIntents) {
        mPhone.getContext().enforceCallingPermission(Manifest.permission.SEND_SMS,
                "Sending SMS message");
        if (mAppOps.noteOp(AppOpsManager.OP_SEND_SMS, Binder.getCallingUid(), callingPkg)
                != AppOpsManager.MODE_ALLOWED) {
            return;
        }
        final ContentResolver resolver = mPhone.getContext().getContentResolver();
        if (!isFailedOrDraft(resolver, messageUri)) {
            Log.e(LOG_TAG, "[IccSmsInterfaceManager]sendStoredMultipartText: "
                    + "not FAILED or DRAFT message");
            returnUnspecifiedFailure(sentIntents);
            return;
        }
        final String[] textAndAddress = loadTextAndAddress(resolver, messageUri);
        if (textAndAddress == null) {
            Log.e(LOG_TAG, "[IccSmsInterfaceManager]sendStoredMultipartText: can not load text");
            returnUnspecifiedFailure(sentIntents);
            return;
        }
        final ArrayList<String> parts = SmsManager.getDefault().divideMessage(textAndAddress[0]);
        if (parts == null || parts.size() < 1) {
            Log.e(LOG_TAG, "[IccSmsInterfaceManager]sendStoredMultipartText: can not divide text");
            returnUnspecifiedFailure(sentIntents);
            return;
        }

        textAndAddress[1] = filterDestAddress(textAndAddress[1]);

        if (parts.size() > 1 && parts.size() < 10 && !SmsMessage.hasEmsSupport()) {
            for (int i = 0; i < parts.size(); i++) {
                // If EMS is not supported, we have to break down EMS into single segment SMS
                // and add page info " x/y".
                String singlePart = parts.get(i);
                if (SmsMessage.shouldAppendPageNumberAsPrefix()) {
                    singlePart = String.valueOf(i + 1) + '/' + parts.size() + ' ' + singlePart;
                } else {
                    singlePart = singlePart.concat(' ' + String.valueOf(i + 1) + '/' + parts.size());
                }

                PendingIntent singleSentIntent = null;
                if (sentIntents != null && sentIntents.size() > i) {
                    singleSentIntent = sentIntents.get(i);
                }

                PendingIntent singleDeliveryIntent = null;
                if (deliveryIntents != null && deliveryIntents.size() > i) {
                    singleDeliveryIntent = deliveryIntents.get(i);
                }

                mDispatcher.sendText(textAndAddress[1], scAddress, singlePart,
                        singleSentIntent, singleDeliveryIntent, messageUri, callingPkg,
                        true  /* persistMessageForNonDefaultSmsApp */);
            }
            return;
        }

        mDispatcher.sendMultipartText(
                textAndAddress[1], // destAddress
                scAddress,
                parts,
                (ArrayList<PendingIntent>) sentIntents,
                (ArrayList<PendingIntent>) deliveryIntents,
                messageUri,
                callingPkg,
                true  /* persistMessageForNonDefaultSmsApp */);
    }

    private boolean isFailedOrDraft(ContentResolver resolver, Uri messageUri) {
        // Clear the calling identity and query the database using the phone user id
        // Otherwise the AppOps check in TelephonyProvider would complain about mismatch
        // between the calling uid and the package uid
        final long identity = Binder.clearCallingIdentity();
        Cursor cursor = null;
        try {
            cursor = resolver.query(
                    messageUri,
                    new String[]{ Telephony.Sms.TYPE },
                    null/*selection*/,
                    null/*selectionArgs*/,
                    null/*sortOrder*/);
            if (cursor != null && cursor.moveToFirst()) {
                final int type = cursor.getInt(0);
                return type == Telephony.Sms.MESSAGE_TYPE_DRAFT
                        || type == Telephony.Sms.MESSAGE_TYPE_FAILED;
            }
        } catch (SQLiteException e) {
            Log.e(LOG_TAG, "[IccSmsInterfaceManager]isFailedOrDraft: query message type failed", e);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            Binder.restoreCallingIdentity(identity);
        }
        return false;
    }

    // Return an array including both the SMS text (0) and address (1)
    private String[] loadTextAndAddress(ContentResolver resolver, Uri messageUri) {
        // Clear the calling identity and query the database using the phone user id
        // Otherwise the AppOps check in TelephonyProvider would complain about mismatch
        // between the calling uid and the package uid
        final long identity = Binder.clearCallingIdentity();
        Cursor cursor = null;
        try {
            cursor = resolver.query(
                    messageUri,
                    new String[]{
                            Telephony.Sms.BODY,
                            Telephony.Sms.ADDRESS
                    },
                    null/*selection*/,
                    null/*selectionArgs*/,
                    null/*sortOrder*/);
            if (cursor != null && cursor.moveToFirst()) {
                return new String[]{ cursor.getString(0), cursor.getString(1) };
            }
        } catch (SQLiteException e) {
            Log.e(LOG_TAG, "[IccSmsInterfaceManager]loadText: query message text failed", e);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            Binder.restoreCallingIdentity(identity);
        }
        return null;
    }

    private void returnUnspecifiedFailure(PendingIntent pi) {
        if (pi != null) {
            try {
                pi.send(SmsManager.RESULT_ERROR_GENERIC_FAILURE);
            } catch (PendingIntent.CanceledException e) {
                // ignore
            }
        }
    }

    private void returnUnspecifiedFailure(List<PendingIntent> pis) {
        if (pis == null) {
            return;
        }
        for (PendingIntent pi : pis) {
            returnUnspecifiedFailure(pi);
        }
    }

    private void enforceCarrierPrivilege() {
        UiccController controller = UiccController.getInstance();
        if (controller == null || controller.getUiccCard(mPhone.getPhoneId()) == null) {
            throw new SecurityException("No Carrier Privilege: No UICC");
        }
        if (controller.getUiccCard(mPhone.getPhoneId()).getCarrierPrivilegeStatusForCurrentTransaction(
                mContext.getPackageManager()) !=
                    TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS) {
            throw new SecurityException("No Carrier Privilege.");
        }
    }

    private void enforceCarrierOrPhonePrivilege() {
        int callingUid = Binder.getCallingUid();
        if (callingUid != Process.PHONE_UID) {
            enforceCarrierPrivilege();
        }
    }

    private String filterDestAddress(String destAddr) {
        String result  = null;
        result = SmsNumberUtils.filterDestAddr(mPhone, destAddr);
        return result != null ? result : destAddr;
    }

}
