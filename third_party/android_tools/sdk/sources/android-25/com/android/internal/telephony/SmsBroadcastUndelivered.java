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

package com.android.internal.telephony;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.database.SQLException;
import android.os.UserHandle;
import android.os.UserManager;
import android.telephony.Rlog;

import com.android.internal.telephony.cdma.CdmaInboundSmsHandler;
import com.android.internal.telephony.gsm.GsmInboundSmsHandler;

import java.util.HashMap;
import java.util.HashSet;

/**
 * Called when the credential-encrypted storage is unlocked, collecting all acknowledged messages
 * and deleting any partial message segments older than 30 days. Called from a worker thread to
 * avoid delaying phone app startup. The last step is to broadcast the first pending message from
 * the main thread, then the remaining pending messages will be broadcast after the previous
 * ordered broadcast completes.
 */
public class SmsBroadcastUndelivered {
    private static final String TAG = "SmsBroadcastUndelivered";
    private static final boolean DBG = InboundSmsHandler.DBG;

    /** Delete any partial message segments older than 30 days. */
    static final long PARTIAL_SEGMENT_EXPIRE_AGE = (long) (60 * 60 * 1000) * 24 * 30;

    /**
     * Query projection for dispatching pending messages at boot time.
     * Column order must match the {@code *_COLUMN} constants in {@link InboundSmsHandler}.
     */
    private static final String[] PDU_PENDING_MESSAGE_PROJECTION = {
            "pdu",
            "sequence",
            "destination_port",
            "date",
            "reference_number",
            "count",
            "address",
            "_id",
            "message_body"
    };

    private static SmsBroadcastUndelivered instance;

    /** Content resolver to use to access raw table from SmsProvider. */
    private final ContentResolver mResolver;

    /** Handler for 3GPP-format messages (may be null). */
    private final GsmInboundSmsHandler mGsmInboundSmsHandler;

    /** Handler for 3GPP2-format messages (may be null). */
    private final CdmaInboundSmsHandler mCdmaInboundSmsHandler;

    /** Broadcast receiver that processes the raw table when the user unlocks the phone for the
     *  first time after reboot and the credential-encrypted storage is available.
     */
    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(final Context context, Intent intent) {
            Rlog.d(TAG, "Received broadcast " + intent.getAction());
            if (Intent.ACTION_USER_UNLOCKED.equals(intent.getAction())) {
                new ScanRawTableThread(context).start();
            }
        }
    };

    private class ScanRawTableThread extends Thread {
        private final Context context;

        private ScanRawTableThread(Context context) {
            this.context = context;
        }

        @Override
        public void run() {
            scanRawTable();
            InboundSmsHandler.cancelNewMessageNotification(context);
        }
    }

    public static void initialize(Context context, GsmInboundSmsHandler gsmInboundSmsHandler,
        CdmaInboundSmsHandler cdmaInboundSmsHandler) {
        if (instance == null) {
            instance = new SmsBroadcastUndelivered(
                context, gsmInboundSmsHandler, cdmaInboundSmsHandler);
        }

        // Tell handlers to start processing new messages and transit from the startup state to the
        // idle state. This method may be called multiple times for multi-sim devices. We must make
        // sure the state transition happen to all inbound sms handlers.
        if (gsmInboundSmsHandler != null) {
            gsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_START_ACCEPTING_SMS);
        }
        if (cdmaInboundSmsHandler != null) {
            cdmaInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_START_ACCEPTING_SMS);
        }
    }

    private SmsBroadcastUndelivered(Context context, GsmInboundSmsHandler gsmInboundSmsHandler,
            CdmaInboundSmsHandler cdmaInboundSmsHandler) {
        mResolver = context.getContentResolver();
        mGsmInboundSmsHandler = gsmInboundSmsHandler;
        mCdmaInboundSmsHandler = cdmaInboundSmsHandler;

        UserManager userManager = (UserManager) context.getSystemService(Context.USER_SERVICE);

        if (userManager.isUserUnlocked()) {
            new ScanRawTableThread(context).start();
        } else {
            IntentFilter userFilter = new IntentFilter();
            userFilter.addAction(Intent.ACTION_USER_UNLOCKED);
            context.registerReceiver(mBroadcastReceiver, userFilter);
        }
    }

    /**
     * Scan the raw table for complete SMS messages to broadcast, and old PDUs to delete.
     */
    private void scanRawTable() {
        if (DBG) Rlog.d(TAG, "scanning raw table for undelivered messages");
        long startTime = System.nanoTime();
        HashMap<SmsReferenceKey, Integer> multiPartReceivedCount =
                new HashMap<SmsReferenceKey, Integer>(4);
        HashSet<SmsReferenceKey> oldMultiPartMessages = new HashSet<SmsReferenceKey>(4);
        Cursor cursor = null;
        try {
            // query only non-deleted ones
            cursor = mResolver.query(InboundSmsHandler.sRawUri, PDU_PENDING_MESSAGE_PROJECTION,
                    "deleted = 0", null,
                    null);
            if (cursor == null) {
                Rlog.e(TAG, "error getting pending message cursor");
                return;
            }

            boolean isCurrentFormat3gpp2 = InboundSmsHandler.isCurrentFormat3gpp2();
            while (cursor.moveToNext()) {
                InboundSmsTracker tracker;
                try {
                    tracker = TelephonyComponentFactory.getInstance().makeInboundSmsTracker(cursor,
                            isCurrentFormat3gpp2);
                } catch (IllegalArgumentException e) {
                    Rlog.e(TAG, "error loading SmsTracker: " + e);
                    continue;
                }

                if (tracker.getMessageCount() == 1) {
                    // deliver single-part message
                    broadcastSms(tracker);
                } else {
                    SmsReferenceKey reference = new SmsReferenceKey(tracker);
                    Integer receivedCount = multiPartReceivedCount.get(reference);
                    if (receivedCount == null) {
                        multiPartReceivedCount.put(reference, 1);    // first segment seen
                        if (tracker.getTimestamp() <
                                (System.currentTimeMillis() - PARTIAL_SEGMENT_EXPIRE_AGE)) {
                            // older than 30 days; delete if we don't find all the segments
                            oldMultiPartMessages.add(reference);
                        }
                    } else {
                        int newCount = receivedCount + 1;
                        if (newCount == tracker.getMessageCount()) {
                            // looks like we've got all the pieces; send a single tracker
                            // to state machine which will find the other pieces to broadcast
                            if (DBG) Rlog.d(TAG, "found complete multi-part message");
                            broadcastSms(tracker);
                            // don't delete this old message until after we broadcast it
                            oldMultiPartMessages.remove(reference);
                        } else {
                            multiPartReceivedCount.put(reference, newCount);
                        }
                    }
                }
            }
            // Delete old incomplete message segments
            for (SmsReferenceKey message : oldMultiPartMessages) {
                // delete permanently
                int rows = mResolver.delete(InboundSmsHandler.sRawUriPermanentDelete,
                        InboundSmsHandler.SELECT_BY_REFERENCE, message.getDeleteWhereArgs());
                if (rows == 0) {
                    Rlog.e(TAG, "No rows were deleted from raw table!");
                } else if (DBG) {
                    Rlog.d(TAG, "Deleted " + rows + " rows from raw table for incomplete "
                            + message.mMessageCount + " part message");
                }
            }
        } catch (SQLException e) {
            Rlog.e(TAG, "error reading pending SMS messages", e);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            if (DBG) Rlog.d(TAG, "finished scanning raw table in "
                    + ((System.nanoTime() - startTime) / 1000000) + " ms");
        }
    }

    /**
     * Send tracker to appropriate (3GPP or 3GPP2) inbound SMS handler for broadcast.
     */
    private void broadcastSms(InboundSmsTracker tracker) {
        InboundSmsHandler handler;
        if (tracker.is3gpp2()) {
            handler = mCdmaInboundSmsHandler;
        } else {
            handler = mGsmInboundSmsHandler;
        }
        if (handler != null) {
            handler.sendMessage(InboundSmsHandler.EVENT_BROADCAST_SMS, tracker);
        } else {
            Rlog.e(TAG, "null handler for " + tracker.getFormat() + " format, can't deliver.");
        }
    }

    /**
     * Used as the HashMap key for matching concatenated message segments.
     */
    private static class SmsReferenceKey {
        final String mAddress;
        final int mReferenceNumber;
        final int mMessageCount;

        SmsReferenceKey(InboundSmsTracker tracker) {
            mAddress = tracker.getAddress();
            mReferenceNumber = tracker.getReferenceNumber();
            mMessageCount = tracker.getMessageCount();
        }

        String[] getDeleteWhereArgs() {
            return new String[]{mAddress, Integer.toString(mReferenceNumber),
                    Integer.toString(mMessageCount)};
        }

        @Override
        public int hashCode() {
            return ((mReferenceNumber * 31) + mMessageCount) * 31 + mAddress.hashCode();
        }

        @Override
        public boolean equals(Object o) {
            if (o instanceof SmsReferenceKey) {
                SmsReferenceKey other = (SmsReferenceKey) o;
                return other.mAddress.equals(mAddress)
                        && (other.mReferenceNumber == mReferenceNumber)
                        && (other.mMessageCount == mMessageCount);
            }
            return false;
        }
    }
}
