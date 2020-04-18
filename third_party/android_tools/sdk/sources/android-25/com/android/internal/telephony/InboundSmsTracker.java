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

import android.content.ContentValues;
import android.database.Cursor;

import com.android.internal.util.HexDump;

import java.util.Arrays;
import java.util.Date;

/**
 * Tracker for an incoming SMS message ready to broadcast to listeners.
 * This is similar to {@link com.android.internal.telephony.SMSDispatcher.SmsTracker} used for
 * outgoing messages.
 */
public class InboundSmsTracker {

    // Fields for single and multi-part messages
    private final byte[] mPdu;
    private final long mTimestamp;
    private final int mDestPort;
    private final boolean mIs3gpp2;
    private final boolean mIs3gpp2WapPdu;
    private final String mMessageBody;
    // Copied from SmsMessageBase#getDisplayOriginatingAddress used for blocking messages.
    private final String mAddress;

    // Fields for concatenating multi-part SMS messages
    private final int mReferenceNumber;
    private final int mSequenceNumber;
    private final int mMessageCount;

    // Fields for deleting this message after delivery
    private String mDeleteWhere;
    private String[] mDeleteWhereArgs;

    /** Destination port flag bit for no destination port. */
    private static final int DEST_PORT_FLAG_NO_PORT = (1 << 16);

    /** Destination port flag bit to indicate 3GPP format message. */
    private static final int DEST_PORT_FLAG_3GPP = (1 << 17);

    /** Destination port flag bit to indicate 3GPP2 format message. */
    private static final int DEST_PORT_FLAG_3GPP2 = (1 << 18);

    /** Destination port flag bit to indicate 3GPP2 format WAP message. */
    private static final int DEST_PORT_FLAG_3GPP2_WAP_PDU = (1 << 19);

    /** Destination port mask (16-bit unsigned value on GSM and CDMA). */
    private static final int DEST_PORT_MASK = 0xffff;

    /**
     * Create a tracker for a single-part SMS.
     *
     * @param pdu the message PDU
     * @param timestamp the message timestamp
     * @param destPort the destination port
     * @param is3gpp2 true for 3GPP2 format; false for 3GPP format
     * @param is3gpp2WapPdu true for 3GPP2 format WAP PDU; false otherwise
     * @param address originating address, or email if this message was from an email gateway
     */
    public InboundSmsTracker(byte[] pdu, long timestamp, int destPort, boolean is3gpp2,
            boolean is3gpp2WapPdu, String address, String messageBody) {
        mPdu = pdu;
        mTimestamp = timestamp;
        mDestPort = destPort;
        mIs3gpp2 = is3gpp2;
        mIs3gpp2WapPdu = is3gpp2WapPdu;
        mMessageBody = messageBody;
        mAddress = address;
        // fields for multi-part SMS
        mReferenceNumber = -1;
        mSequenceNumber = getIndexOffset();     // 0 or 1, depending on type
        mMessageCount = 1;
    }

    /**
     * Create a tracker for a multi-part SMS. Sequence numbers start at 1 for 3GPP and regular
     * concatenated 3GPP2 messages, but CDMA WAP push sequence numbers start at 0. The caller will
     * subtract 1 if necessary so that the sequence number is always 0-based. When loading and
     * saving to the raw table, the sequence number is adjusted if necessary for backwards
     * compatibility.
     *
     * @param pdu the message PDU
     * @param timestamp the message timestamp
     * @param destPort the destination port
     * @param is3gpp2 true for 3GPP2 format; false for 3GPP format
     * @param address originating address, or email if this message was from an email gateway
     * @param referenceNumber the concatenated reference number
     * @param sequenceNumber the sequence number of this segment (0-based)
     * @param messageCount the total number of segments
     * @param is3gpp2WapPdu true for 3GPP2 format WAP PDU; false otherwise
     */
    public InboundSmsTracker(byte[] pdu, long timestamp, int destPort, boolean is3gpp2,
            String address, int referenceNumber, int sequenceNumber, int messageCount,
            boolean is3gpp2WapPdu, String messageBody) {
        mPdu = pdu;
        mTimestamp = timestamp;
        mDestPort = destPort;
        mIs3gpp2 = is3gpp2;
        mIs3gpp2WapPdu = is3gpp2WapPdu;
        mMessageBody = messageBody;
        mAddress = address;
        // fields for multi-part SMS
        mReferenceNumber = referenceNumber;
        mSequenceNumber = sequenceNumber;
        mMessageCount = messageCount;
    }

    /**
     * Create a new tracker from the row of the raw table pointed to by Cursor.
     * Since this constructor is used only for recovery during startup, the Dispatcher is null.
     * @param cursor a Cursor pointing to the row to construct this SmsTracker for
     */
    public InboundSmsTracker(Cursor cursor, boolean isCurrentFormat3gpp2) {
        mPdu = HexDump.hexStringToByteArray(cursor.getString(InboundSmsHandler.PDU_COLUMN));

        if (cursor.isNull(InboundSmsHandler.DESTINATION_PORT_COLUMN)) {
            mDestPort = -1;
            mIs3gpp2 = isCurrentFormat3gpp2;
            mIs3gpp2WapPdu = false;
        } else {
            int destPort = cursor.getInt(InboundSmsHandler.DESTINATION_PORT_COLUMN);
            if ((destPort & DEST_PORT_FLAG_3GPP) != 0) {
                mIs3gpp2 = false;
            } else if ((destPort & DEST_PORT_FLAG_3GPP2) != 0) {
                mIs3gpp2 = true;
            } else {
                mIs3gpp2 = isCurrentFormat3gpp2;
            }
            mIs3gpp2WapPdu = ((destPort & DEST_PORT_FLAG_3GPP2_WAP_PDU) != 0);
            mDestPort = getRealDestPort(destPort);
        }

        mTimestamp = cursor.getLong(InboundSmsHandler.DATE_COLUMN);
        mAddress = cursor.getString(InboundSmsHandler.ADDRESS_COLUMN);

        if (cursor.isNull(InboundSmsHandler.COUNT_COLUMN)) {
            // single-part message
            long rowId = cursor.getLong(InboundSmsHandler.ID_COLUMN);
            mReferenceNumber = -1;
            mSequenceNumber = getIndexOffset();     // 0 or 1, depending on type
            mMessageCount = 1;
            mDeleteWhere = InboundSmsHandler.SELECT_BY_ID;
            mDeleteWhereArgs = new String[]{Long.toString(rowId)};
        } else {
            // multi-part message
            mReferenceNumber = cursor.getInt(InboundSmsHandler.REFERENCE_NUMBER_COLUMN);
            mMessageCount = cursor.getInt(InboundSmsHandler.COUNT_COLUMN);

            // GSM sequence numbers start at 1; CDMA WDP datagram sequence numbers start at 0
            mSequenceNumber = cursor.getInt(InboundSmsHandler.SEQUENCE_COLUMN);
            int index = mSequenceNumber - getIndexOffset();

            if (index < 0 || index >= mMessageCount) {
                throw new IllegalArgumentException("invalid PDU sequence " + mSequenceNumber
                        + " of " + mMessageCount);
            }

            mDeleteWhere = InboundSmsHandler.SELECT_BY_REFERENCE;
            mDeleteWhereArgs = new String[]{mAddress,
                    Integer.toString(mReferenceNumber), Integer.toString(mMessageCount)};
        }
        mMessageBody = cursor.getString(InboundSmsHandler.MESSAGE_BODY_COLUMN);
    }

    public ContentValues getContentValues() {
        ContentValues values = new ContentValues();
        values.put("pdu", HexDump.toHexString(mPdu));
        values.put("date", mTimestamp);
        // Always set the destination port, since it now contains message format flags.
        // Port is a 16-bit value, or -1, so clear the upper bits before setting flags.
        int destPort;
        if (mDestPort == -1) {
            destPort = DEST_PORT_FLAG_NO_PORT;
        } else {
            destPort = mDestPort & DEST_PORT_MASK;
        }
        if (mIs3gpp2) {
            destPort |= DEST_PORT_FLAG_3GPP2;
        } else {
            destPort |= DEST_PORT_FLAG_3GPP;
        }
        if (mIs3gpp2WapPdu) {
            destPort |= DEST_PORT_FLAG_3GPP2_WAP_PDU;
        }
        values.put("destination_port", destPort);
        if (mAddress != null) {
            values.put("address", mAddress);
            values.put("reference_number", mReferenceNumber);
            values.put("sequence", mSequenceNumber);
            values.put("count", mMessageCount);
        }
        values.put("message_body", mMessageBody);
        return values;
    }

    /**
     * Get the port number, or -1 if there is no destination port.
     * @param destPort the destination port value, with flags
     * @return the real destination port, or -1 for no port
     */
    public static int getRealDestPort(int destPort) {
        if ((destPort & DEST_PORT_FLAG_NO_PORT) != 0) {
            return -1;
        } else {
           return destPort & DEST_PORT_MASK;
        }
    }

    /**
     * Update the values to delete all rows of the message from raw table.
     * @param deleteWhere the selection to use
     * @param deleteWhereArgs the selection args to use
     */
    public void setDeleteWhere(String deleteWhere, String[] deleteWhereArgs) {
        mDeleteWhere = deleteWhere;
        mDeleteWhereArgs = deleteWhereArgs;
    }

    public String toString() {
        StringBuilder builder = new StringBuilder("SmsTracker{timestamp=");
        builder.append(new Date(mTimestamp));
        builder.append(" destPort=").append(mDestPort);
        builder.append(" is3gpp2=").append(mIs3gpp2);
        if (mAddress != null) {
            builder.append(" address=").append(mAddress);
            builder.append(" refNumber=").append(mReferenceNumber);
            builder.append(" seqNumber=").append(mSequenceNumber);
            builder.append(" msgCount=").append(mMessageCount);
        }
        if (mDeleteWhere != null) {
            builder.append(" deleteWhere(").append(mDeleteWhere);
            builder.append(") deleteArgs=(").append(Arrays.toString(mDeleteWhereArgs));
            builder.append(')');
        }
        builder.append('}');
        return builder.toString();
    }

    public byte[] getPdu() {
        return mPdu;
    }

    public long getTimestamp() {
        return mTimestamp;
    }

    public int getDestPort() {
        return mDestPort;
    }

    public boolean is3gpp2() {
        return mIs3gpp2;
    }

    public String getFormat() {
        return mIs3gpp2 ? SmsConstants.FORMAT_3GPP2 : SmsConstants.FORMAT_3GPP;
    }

    /**
     * Sequence numbers for concatenated messages start at 1. The exception is CDMA WAP PDU
     * messages, which use a 0-based index.
     * @return the offset to use to convert between mIndex and the sequence number
     */
    public int getIndexOffset() {
        return (mIs3gpp2 && mIs3gpp2WapPdu) ? 0 : 1;
    }

    public String getAddress() {
        return mAddress;
    }

    public String getMessageBody() {
        return mMessageBody;
    }

    public int getReferenceNumber() {
        return mReferenceNumber;
    }

    public int getSequenceNumber() {
        return mSequenceNumber;
    }

    public int getMessageCount() {
        return mMessageCount;
    }

    public String getDeleteWhere() {
        return mDeleteWhere;
    }

    public String[] getDeleteWhereArgs() {
        return mDeleteWhereArgs;
    }
}
