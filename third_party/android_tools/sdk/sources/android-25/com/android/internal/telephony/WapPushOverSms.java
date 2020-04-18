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

import static com.google.android.mms.pdu.PduHeaders.MESSAGE_TYPE_DELIVERY_IND;
import static com.google.android.mms.pdu.PduHeaders.MESSAGE_TYPE_NOTIFICATION_IND;
import static com.google.android.mms.pdu.PduHeaders.MESSAGE_TYPE_READ_ORIG_IND;
import android.app.Activity;
import android.app.AppOpsManager;
import android.app.BroadcastOptions;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SqliteWrapper;
import android.net.Uri;
import android.os.Bundle;
import android.os.IBinder;
import android.os.IDeviceIdleController;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.Telephony;
import android.provider.Telephony.Sms.Intents;
import android.telephony.Rlog;
import android.telephony.SmsManager;
import android.telephony.SubscriptionManager;
import android.util.Log;

import com.android.internal.telephony.uicc.IccUtils;

import java.util.HashMap;

import com.google.android.mms.MmsException;
import com.google.android.mms.pdu.DeliveryInd;
import com.google.android.mms.pdu.GenericPdu;
import com.google.android.mms.pdu.NotificationInd;
import com.google.android.mms.pdu.PduHeaders;
import com.google.android.mms.pdu.PduParser;
import com.google.android.mms.pdu.PduPersister;
import com.google.android.mms.pdu.ReadOrigInd;

/**
 * WAP push handler class.
 *
 * @hide
 */
public class WapPushOverSms implements ServiceConnection {
    private static final String TAG = "WAP PUSH";
    private static final boolean DBG = false;

    private final Context mContext;
    private IDeviceIdleController mDeviceIdleController;

    private String mWapPushManagerPackage;

    /** Assigned from ServiceConnection callback on main threaad. */
    private volatile IWapPushManager mWapPushManager;

    /** Broadcast receiver that binds to WapPushManager when the user unlocks the phone for the
     *  first time after reboot and the credential-encrypted storage is available.
     */
    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(final Context context, Intent intent) {
            Rlog.d(TAG, "Received broadcast " + intent.getAction());
            if (Intent.ACTION_USER_UNLOCKED.equals(intent.getAction())) {
                new BindServiceThread(mContext).start();
            }
        }
    };

    private class BindServiceThread extends Thread {
        private final Context context;

        private BindServiceThread(Context context) {
            this.context = context;
        }

        @Override
        public void run() {
            bindWapPushManagerService(context);
        }
    }

    private void bindWapPushManagerService(Context context) {
        Intent intent = new Intent(IWapPushManager.class.getName());
        ComponentName comp = intent.resolveSystemService(context.getPackageManager(), 0);
        intent.setComponent(comp);
        if (comp == null || !context.bindService(intent, this, Context.BIND_AUTO_CREATE)) {
            Rlog.e(TAG, "bindService() for wappush manager failed");
        } else {
            synchronized (this) {
                mWapPushManagerPackage = comp.getPackageName();
            }
            if (DBG) Rlog.v(TAG, "bindService() for wappush manager succeeded");
        }
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        mWapPushManager = IWapPushManager.Stub.asInterface(service);
        if (DBG) Rlog.v(TAG, "wappush manager connected to " + hashCode());
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        mWapPushManager = null;
        if (DBG) Rlog.v(TAG, "wappush manager disconnected.");
    }

    public WapPushOverSms(Context context) {
        mContext = context;
        mDeviceIdleController = TelephonyComponentFactory.getInstance().getIDeviceIdleController();

        UserManager userManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);

        if (userManager.isUserUnlocked()) {
            bindWapPushManagerService(mContext);
        } else {
            IntentFilter userFilter = new IntentFilter();
            userFilter.addAction(Intent.ACTION_USER_UNLOCKED);
            context.registerReceiver(mBroadcastReceiver, userFilter);
        }
    }

    public void dispose() {
        if (mWapPushManager != null) {
            if (DBG) Rlog.v(TAG, "dispose: unbind wappush manager");
            mContext.unbindService(this);
        } else {
            Rlog.e(TAG, "dispose: not bound to a wappush manager");
        }
    }

    /**
     * Decodes the wap push pdu. The decoded result is wrapped inside the {@link DecodedResult}
     * object. The caller of this method should check {@link DecodedResult#statusCode} for the
     * decoding status. It  can have the following values.
     *
     * Activity.RESULT_OK - the wap push pdu is successfully decoded and should be further processed
     * Intents.RESULT_SMS_HANDLED - the wap push pdu should be ignored.
     * Intents.RESULT_SMS_GENERIC_ERROR - the pdu is invalid.
     */
    private DecodedResult decodeWapPdu(byte[] pdu, InboundSmsHandler handler) {
        DecodedResult result = new DecodedResult();
        if (DBG) Rlog.d(TAG, "Rx: " + IccUtils.bytesToHexString(pdu));

        try {
            int index = 0;
            int transactionId = pdu[index++] & 0xFF;
            int pduType = pdu[index++] & 0xFF;

            // Should we "abort" if no subId for now just no supplying extra param below
            int phoneId = handler.getPhone().getPhoneId();

            if ((pduType != WspTypeDecoder.PDU_TYPE_PUSH) &&
                    (pduType != WspTypeDecoder.PDU_TYPE_CONFIRMED_PUSH)) {
                index = mContext.getResources().getInteger(
                        com.android.internal.R.integer.config_valid_wappush_index);
                if (index != -1) {
                    transactionId = pdu[index++] & 0xff;
                    pduType = pdu[index++] & 0xff;
                    if (DBG)
                        Rlog.d(TAG, "index = " + index + " PDU Type = " + pduType +
                                " transactionID = " + transactionId);

                    // recheck wap push pduType
                    if ((pduType != WspTypeDecoder.PDU_TYPE_PUSH)
                            && (pduType != WspTypeDecoder.PDU_TYPE_CONFIRMED_PUSH)) {
                        if (DBG) Rlog.w(TAG, "Received non-PUSH WAP PDU. Type = " + pduType);
                        result.statusCode = Intents.RESULT_SMS_HANDLED;
                        return result;
                    }
                } else {
                    if (DBG) Rlog.w(TAG, "Received non-PUSH WAP PDU. Type = " + pduType);
                    result.statusCode = Intents.RESULT_SMS_HANDLED;
                    return result;
                }
            }

            WspTypeDecoder pduDecoder =
                    TelephonyComponentFactory.getInstance().makeWspTypeDecoder(pdu);

            /**
             * Parse HeaderLen(unsigned integer).
             * From wap-230-wsp-20010705-a section 8.1.2
             * The maximum size of a uintvar is 32 bits.
             * So it will be encoded in no more than 5 octets.
             */
            if (pduDecoder.decodeUintvarInteger(index) == false) {
                if (DBG) Rlog.w(TAG, "Received PDU. Header Length error.");
                result.statusCode = Intents.RESULT_SMS_GENERIC_ERROR;
                return result;
            }
            int headerLength = (int) pduDecoder.getValue32();
            index += pduDecoder.getDecodedDataLength();

            int headerStartIndex = index;

            /**
             * Parse Content-Type.
             * From wap-230-wsp-20010705-a section 8.4.2.24
             *
             * Content-type-value = Constrained-media | Content-general-form
             * Content-general-form = Value-length Media-type
             * Media-type = (Well-known-media | Extension-Media) *(Parameter)
             * Value-length = Short-length | (Length-quote Length)
             * Short-length = <Any octet 0-30>   (octet <= WAP_PDU_SHORT_LENGTH_MAX)
             * Length-quote = <Octet 31>         (WAP_PDU_LENGTH_QUOTE)
             * Length = Uintvar-integer
             */
            if (pduDecoder.decodeContentType(index) == false) {
                if (DBG) Rlog.w(TAG, "Received PDU. Header Content-Type error.");
                result.statusCode = Intents.RESULT_SMS_GENERIC_ERROR;
                return result;
            }

            String mimeType = pduDecoder.getValueString();
            long binaryContentType = pduDecoder.getValue32();
            index += pduDecoder.getDecodedDataLength();

            byte[] header = new byte[headerLength];
            System.arraycopy(pdu, headerStartIndex, header, 0, header.length);

            byte[] intentData;

            if (mimeType != null && mimeType.equals(WspTypeDecoder.CONTENT_TYPE_B_PUSH_CO)) {
                intentData = pdu;
            } else {
                int dataIndex = headerStartIndex + headerLength;
                intentData = new byte[pdu.length - dataIndex];
                System.arraycopy(pdu, dataIndex, intentData, 0, intentData.length);
            }

            int[] subIds = SubscriptionManager.getSubId(phoneId);
            int subId = (subIds != null) && (subIds.length > 0) ? subIds[0]
                    : SmsManager.getDefaultSmsSubscriptionId();

            // Continue if PDU parsing fails: the default messaging app may successfully parse the
            // same PDU.
            GenericPdu parsedPdu = null;
            try {
                parsedPdu = new PduParser(intentData, shouldParseContentDisposition(subId)).parse();
            } catch (Exception e) {
                Rlog.e(TAG, "Unable to parse PDU: " + e.toString());
            }

            if (parsedPdu != null && parsedPdu.getMessageType() == MESSAGE_TYPE_NOTIFICATION_IND) {
                final NotificationInd nInd = (NotificationInd) parsedPdu;
                if (nInd.getFrom() != null
                        && BlockChecker.isBlocked(mContext, nInd.getFrom().getString())) {
                    result.statusCode = Intents.RESULT_SMS_HANDLED;
                    return result;
                }
            }

            /**
             * Seek for application ID field in WSP header.
             * If application ID is found, WapPushManager substitute the message
             * processing. Since WapPushManager is optional module, if WapPushManager
             * is not found, legacy message processing will be continued.
             */
            if (pduDecoder.seekXWapApplicationId(index, index + headerLength - 1)) {
                index = (int) pduDecoder.getValue32();
                pduDecoder.decodeXWapApplicationId(index);
                String wapAppId = pduDecoder.getValueString();
                if (wapAppId == null) {
                    wapAppId = Integer.toString((int) pduDecoder.getValue32());
                }
                result.wapAppId = wapAppId;
                String contentType = ((mimeType == null) ?
                        Long.toString(binaryContentType) : mimeType);
                result.contentType = contentType;
                if (DBG) Rlog.v(TAG, "appid found: " + wapAppId + ":" + contentType);
            }

            result.subId = subId;
            result.phoneId = phoneId;
            result.parsedPdu = parsedPdu;
            result.mimeType = mimeType;
            result.transactionId = transactionId;
            result.pduType = pduType;
            result.header = header;
            result.intentData = intentData;
            result.contentTypeParameters = pduDecoder.getContentParameters();
            result.statusCode = Activity.RESULT_OK;
        } catch (ArrayIndexOutOfBoundsException aie) {
            // 0-byte WAP PDU or other unexpected WAP PDU contents can easily throw this;
            // log exception string without stack trace and return false.
            Rlog.e(TAG, "ignoring dispatchWapPdu() array index exception: " + aie);
            result.statusCode = Intents.RESULT_SMS_GENERIC_ERROR;
        }
        return result;
    }

    /**
     * Dispatches inbound messages that are in the WAP PDU format. See
     * wap-230-wsp-20010705-a section 8 for details on the WAP PDU format.
     *
     * @param pdu The WAP PDU, made up of one or more SMS PDUs
     * @return a result code from {@link android.provider.Telephony.Sms.Intents}, or
     *         {@link Activity#RESULT_OK} if the message has been broadcast
     *         to applications
     */
    public int dispatchWapPdu(byte[] pdu, BroadcastReceiver receiver, InboundSmsHandler handler) {
        DecodedResult result = decodeWapPdu(pdu, handler);
        if (result.statusCode != Activity.RESULT_OK) {
            return result.statusCode;
        }

        if (SmsManager.getDefault().getAutoPersisting()) {
            // Store the wap push data in telephony
            writeInboxMessage(result.subId, result.parsedPdu);
        }

        /**
         * If the pdu has application ID, WapPushManager substitute the message
         * processing. Since WapPushManager is optional module, if WapPushManager
         * is not found, legacy message processing will be continued.
         */
        if (result.wapAppId != null) {
            try {
                boolean processFurther = true;
                IWapPushManager wapPushMan = mWapPushManager;

                if (wapPushMan == null) {
                    if (DBG) Rlog.w(TAG, "wap push manager not found!");
                } else {
                    synchronized (this) {
                        mDeviceIdleController.addPowerSaveTempWhitelistAppForMms(
                                mWapPushManagerPackage, 0, "mms-mgr");
                    }

                    Intent intent = new Intent();
                    intent.putExtra("transactionId", result.transactionId);
                    intent.putExtra("pduType", result.pduType);
                    intent.putExtra("header", result.header);
                    intent.putExtra("data", result.intentData);
                    intent.putExtra("contentTypeParameters", result.contentTypeParameters);
                    SubscriptionManager.putPhoneIdAndSubIdExtra(intent, result.phoneId);

                    int procRet = wapPushMan.processMessage(
                        result.wapAppId, result.contentType, intent);
                    if (DBG) Rlog.v(TAG, "procRet:" + procRet);
                    if ((procRet & WapPushManagerParams.MESSAGE_HANDLED) > 0
                            && (procRet & WapPushManagerParams.FURTHER_PROCESSING) == 0) {
                        processFurther = false;
                    }
                }
                if (!processFurther) {
                    return Intents.RESULT_SMS_HANDLED;
                }
            } catch (RemoteException e) {
                if (DBG) Rlog.w(TAG, "remote func failed...");
            }
        }
        if (DBG) Rlog.v(TAG, "fall back to existing handler");

        if (result.mimeType == null) {
            if (DBG) Rlog.w(TAG, "Header Content-Type error.");
            return Intents.RESULT_SMS_GENERIC_ERROR;
        }

        Intent intent = new Intent(Intents.WAP_PUSH_DELIVER_ACTION);
        intent.setType(result.mimeType);
        intent.putExtra("transactionId", result.transactionId);
        intent.putExtra("pduType", result.pduType);
        intent.putExtra("header", result.header);
        intent.putExtra("data", result.intentData);
        intent.putExtra("contentTypeParameters", result.contentTypeParameters);
        SubscriptionManager.putPhoneIdAndSubIdExtra(intent, result.phoneId);

        // Direct the intent to only the default MMS app. If we can't find a default MMS app
        // then sent it to all broadcast receivers.
        ComponentName componentName = SmsApplication.getDefaultMmsApplication(mContext, true);
        Bundle options = null;
        if (componentName != null) {
            // Deliver MMS message only to this receiver
            intent.setComponent(componentName);
            if (DBG) Rlog.v(TAG, "Delivering MMS to: " + componentName.getPackageName() +
                    " " + componentName.getClassName());
            try {
                long duration = mDeviceIdleController.addPowerSaveTempWhitelistAppForMms(
                        componentName.getPackageName(), 0, "mms-app");
                BroadcastOptions bopts = BroadcastOptions.makeBasic();
                bopts.setTemporaryAppWhitelistDuration(duration);
                options = bopts.toBundle();
            } catch (RemoteException e) {
            }
        }

        handler.dispatchIntent(intent, getPermissionForType(result.mimeType),
                getAppOpsPermissionForIntent(result.mimeType), options, receiver,
                UserHandle.SYSTEM);
        return Activity.RESULT_OK;
    }

    /**
     * Check whether the pdu is a MMS WAP push pdu that should be dispatched to the SMS app.
     */
    public boolean isWapPushForMms(byte[] pdu, InboundSmsHandler handler) {
        DecodedResult result = decodeWapPdu(pdu, handler);
        return result.statusCode == Activity.RESULT_OK
            && WspTypeDecoder.CONTENT_TYPE_B_MMS.equals(result.mimeType);
    }

    private static boolean shouldParseContentDisposition(int subId) {
        return SmsManager
                .getSmsManagerForSubscriptionId(subId)
                .getCarrierConfigValues()
                .getBoolean(SmsManager.MMS_CONFIG_SUPPORT_MMS_CONTENT_DISPOSITION, true);
    }

    private void writeInboxMessage(int subId, GenericPdu pdu) {
        if (pdu == null) {
            Rlog.e(TAG, "Invalid PUSH PDU");
        }
        final PduPersister persister = PduPersister.getPduPersister(mContext);
        final int type = pdu.getMessageType();
        try {
            switch (type) {
                case MESSAGE_TYPE_DELIVERY_IND:
                case MESSAGE_TYPE_READ_ORIG_IND: {
                    final long threadId = getDeliveryOrReadReportThreadId(mContext, pdu);
                    if (threadId == -1) {
                        // The associated SendReq isn't found, therefore skip
                        // processing this PDU.
                        Rlog.e(TAG, "Failed to find delivery or read report's thread id");
                        break;
                    }
                    final Uri uri = persister.persist(
                            pdu,
                            Telephony.Mms.Inbox.CONTENT_URI,
                            true/*createThreadId*/,
                            true/*groupMmsEnabled*/,
                            null/*preOpenedFiles*/);
                    if (uri == null) {
                        Rlog.e(TAG, "Failed to persist delivery or read report");
                        break;
                    }
                    // Update thread ID for ReadOrigInd & DeliveryInd.
                    final ContentValues values = new ContentValues(1);
                    values.put(Telephony.Mms.THREAD_ID, threadId);
                    if (SqliteWrapper.update(
                            mContext,
                            mContext.getContentResolver(),
                            uri,
                            values,
                            null/*where*/,
                            null/*selectionArgs*/) != 1) {
                        Rlog.e(TAG, "Failed to update delivery or read report thread id");
                    }
                    break;
                }
                case MESSAGE_TYPE_NOTIFICATION_IND: {
                    final NotificationInd nInd = (NotificationInd) pdu;

                    Bundle configs = SmsManager.getSmsManagerForSubscriptionId(subId)
                            .getCarrierConfigValues();
                    if (configs != null && configs.getBoolean(
                        SmsManager.MMS_CONFIG_APPEND_TRANSACTION_ID, false)) {
                        final byte [] contentLocation = nInd.getContentLocation();
                        if ('=' == contentLocation[contentLocation.length - 1]) {
                            byte [] transactionId = nInd.getTransactionId();
                            byte [] contentLocationWithId = new byte [contentLocation.length
                                    + transactionId.length];
                            System.arraycopy(contentLocation, 0, contentLocationWithId,
                                    0, contentLocation.length);
                            System.arraycopy(transactionId, 0, contentLocationWithId,
                                    contentLocation.length, transactionId.length);
                            nInd.setContentLocation(contentLocationWithId);
                        }
                    }
                    if (!isDuplicateNotification(mContext, nInd)) {
                        final Uri uri = persister.persist(
                                pdu,
                                Telephony.Mms.Inbox.CONTENT_URI,
                                true/*createThreadId*/,
                                true/*groupMmsEnabled*/,
                                null/*preOpenedFiles*/);
                        if (uri == null) {
                            Rlog.e(TAG, "Failed to save MMS WAP push notification ind");
                        }
                    } else {
                        Rlog.d(TAG, "Skip storing duplicate MMS WAP push notification ind: "
                                + new String(nInd.getContentLocation()));
                    }
                    break;
                }
                default:
                    Log.e(TAG, "Received unrecognized WAP Push PDU.");
            }
        } catch (MmsException e) {
            Log.e(TAG, "Failed to save MMS WAP push data: type=" + type, e);
        } catch (RuntimeException e) {
            Log.e(TAG, "Unexpected RuntimeException in persisting MMS WAP push data", e);
        }

    }

    private static final String THREAD_ID_SELECTION =
            Telephony.Mms.MESSAGE_ID + "=? AND " + Telephony.Mms.MESSAGE_TYPE + "=?";

    private static long getDeliveryOrReadReportThreadId(Context context, GenericPdu pdu) {
        String messageId;
        if (pdu instanceof DeliveryInd) {
            messageId = new String(((DeliveryInd) pdu).getMessageId());
        } else if (pdu instanceof ReadOrigInd) {
            messageId = new String(((ReadOrigInd) pdu).getMessageId());
        } else {
            Rlog.e(TAG, "WAP Push data is neither delivery or read report type: "
                    + pdu.getClass().getCanonicalName());
            return -1L;
        }
        Cursor cursor = null;
        try {
            cursor = SqliteWrapper.query(
                    context,
                    context.getContentResolver(),
                    Telephony.Mms.CONTENT_URI,
                    new String[]{ Telephony.Mms.THREAD_ID },
                    THREAD_ID_SELECTION,
                    new String[]{
                            DatabaseUtils.sqlEscapeString(messageId),
                            Integer.toString(PduHeaders.MESSAGE_TYPE_SEND_REQ)
                    },
                    null/*sortOrder*/);
            if (cursor != null && cursor.moveToFirst()) {
                return cursor.getLong(0);
            }
        } catch (SQLiteException e) {
            Rlog.e(TAG, "Failed to query delivery or read report thread id", e);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return -1L;
    }

    private static final String LOCATION_SELECTION =
            Telephony.Mms.MESSAGE_TYPE + "=? AND " + Telephony.Mms.CONTENT_LOCATION + " =?";

    private static boolean isDuplicateNotification(Context context, NotificationInd nInd) {
        final byte[] rawLocation = nInd.getContentLocation();
        if (rawLocation != null) {
            String location = new String(rawLocation);
            String[] selectionArgs = new String[] { location };
            Cursor cursor = null;
            try {
                cursor = SqliteWrapper.query(
                        context,
                        context.getContentResolver(),
                        Telephony.Mms.CONTENT_URI,
                        new String[]{Telephony.Mms._ID},
                        LOCATION_SELECTION,
                        new String[]{
                                Integer.toString(PduHeaders.MESSAGE_TYPE_NOTIFICATION_IND),
                                new String(rawLocation)
                        },
                        null/*sortOrder*/);
                if (cursor != null && cursor.getCount() > 0) {
                    // We already received the same notification before.
                    return true;
                }
            } catch (SQLiteException e) {
                Rlog.e(TAG, "failed to query existing notification ind", e);
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }
        return false;
    }

    public static String getPermissionForType(String mimeType) {
        String permission;
        if (WspTypeDecoder.CONTENT_TYPE_B_MMS.equals(mimeType)) {
            permission = android.Manifest.permission.RECEIVE_MMS;
        } else {
            permission = android.Manifest.permission.RECEIVE_WAP_PUSH;
        }
        return permission;
    }

    public static int getAppOpsPermissionForIntent(String mimeType) {
        int appOp;
        if (WspTypeDecoder.CONTENT_TYPE_B_MMS.equals(mimeType)) {
            appOp = AppOpsManager.OP_RECEIVE_MMS;
        } else {
            appOp = AppOpsManager.OP_RECEIVE_WAP_PUSH;
        }
        return appOp;
    }

    /**
     * Place holder for decoded Wap pdu data.
     */
    private final class DecodedResult {
        String mimeType;
        String contentType;
        int transactionId;
        int pduType;
        int phoneId;
        int subId;
        byte[] header;
        String wapAppId;
        byte[] intentData;
        HashMap<String, String> contentTypeParameters;
        GenericPdu parsedPdu;
        int statusCode;
    }
}
