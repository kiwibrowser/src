/*
 * Copyright (C) 2011 The Android Open Source Project
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
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.provider.Telephony.Sms.Intents;
import android.telephony.PhoneNumberUtils;
import android.telephony.Rlog;
import android.telephony.SmsManager;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.cat.ComprehensionTlvTag;
import com.android.internal.telephony.uicc.IccIoResult;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.UsimServiceTable;

/**
 * Handler for SMS-PP data download messages.
 * See 3GPP TS 31.111 section 7.1.1
 */
public class UsimDataDownloadHandler extends Handler {
    private static final String TAG = "UsimDataDownloadHandler";

    /** BER-TLV tag for SMS-PP download. TS 31.111 section 9.1. */
    private static final int BER_SMS_PP_DOWNLOAD_TAG      = 0xd1;

    /** Device identity value for UICC (destination). */
    private static final int DEV_ID_UICC        = 0x81;

    /** Device identity value for network (source). */
    private static final int DEV_ID_NETWORK     = 0x83;

    /** Message containing new SMS-PP message to process. */
    private static final int EVENT_START_DATA_DOWNLOAD = 1;

    /** Response to SMS-PP download envelope command. */
    private static final int EVENT_SEND_ENVELOPE_RESPONSE = 2;

    /** Result of writing SM to UICC (when SMS-PP service is not available). */
    private static final int EVENT_WRITE_SMS_COMPLETE = 3;

    private final CommandsInterface mCi;

    public UsimDataDownloadHandler(CommandsInterface commandsInterface) {
        mCi = commandsInterface;
    }

    /**
     * Handle SMS-PP data download messages. Normally these are automatically handled by the
     * radio, but we may have to deal with this type of SM arriving via the IMS stack. If the
     * data download service is not enabled, try to write to the USIM as an SMS, and send the
     * UICC response as the acknowledgment to the SMSC.
     *
     * @param ust the UsimServiceTable, to check if data download is enabled
     * @param smsMessage the SMS message to process
     * @return {@code Activity.RESULT_OK} on success; {@code RESULT_SMS_GENERIC_ERROR} on failure
     */
    int handleUsimDataDownload(UsimServiceTable ust, SmsMessage smsMessage) {
        // If we receive an SMS-PP message before the UsimServiceTable has been loaded,
        // assume that the data download service is not present. This is very unlikely to
        // happen because the IMS connection will not be established until after the ISIM
        // records have been loaded, after the USIM service table has been loaded.
        if (ust != null && ust.isAvailable(
                UsimServiceTable.UsimService.DATA_DL_VIA_SMS_PP)) {
            Rlog.d(TAG, "Received SMS-PP data download, sending to UICC.");
            return startDataDownload(smsMessage);
        } else {
            Rlog.d(TAG, "DATA_DL_VIA_SMS_PP service not available, storing message to UICC.");
            String smsc = IccUtils.bytesToHexString(
                    PhoneNumberUtils.networkPortionToCalledPartyBCDWithLength(
                            smsMessage.getServiceCenterAddress()));
            mCi.writeSmsToSim(SmsManager.STATUS_ON_ICC_UNREAD, smsc,
                    IccUtils.bytesToHexString(smsMessage.getPdu()),
                    obtainMessage(EVENT_WRITE_SMS_COMPLETE));
            return Activity.RESULT_OK;  // acknowledge after response from write to USIM
        }

    }

    /**
     * Start an SMS-PP data download for the specified message. Can be called from a different
     * thread than this Handler is running on.
     *
     * @param smsMessage the message to process
     * @return {@code Activity.RESULT_OK} on success; {@code RESULT_SMS_GENERIC_ERROR} on failure
     */
    public int startDataDownload(SmsMessage smsMessage) {
        if (sendMessage(obtainMessage(EVENT_START_DATA_DOWNLOAD, smsMessage))) {
            return Activity.RESULT_OK;  // we will send SMS ACK/ERROR based on UICC response
        } else {
            Rlog.e(TAG, "startDataDownload failed to send message to start data download.");
            return Intents.RESULT_SMS_GENERIC_ERROR;
        }
    }

    private void handleDataDownload(SmsMessage smsMessage) {
        int dcs = smsMessage.getDataCodingScheme();
        int pid = smsMessage.getProtocolIdentifier();
        byte[] pdu = smsMessage.getPdu();           // includes SC address

        int scAddressLength = pdu[0] & 0xff;
        int tpduIndex = scAddressLength + 1;        // start of TPDU
        int tpduLength = pdu.length - tpduIndex;

        int bodyLength = getEnvelopeBodyLength(scAddressLength, tpduLength);

        // Add 1 byte for SMS-PP download tag and 1-2 bytes for BER-TLV length.
        // See ETSI TS 102 223 Annex C for encoding of length and tags.
        int totalLength = bodyLength + 1 + (bodyLength > 127 ? 2 : 1);

        byte[] envelope = new byte[totalLength];
        int index = 0;

        // SMS-PP download tag and length (assumed to be < 256 bytes).
        envelope[index++] = (byte) BER_SMS_PP_DOWNLOAD_TAG;
        if (bodyLength > 127) {
            envelope[index++] = (byte) 0x81;    // length 128-255 encoded as 0x81 + length
        }
        envelope[index++] = (byte) bodyLength;

        // Device identities TLV
        envelope[index++] = (byte) (0x80 | ComprehensionTlvTag.DEVICE_IDENTITIES.value());
        envelope[index++] = (byte) 2;
        envelope[index++] = (byte) DEV_ID_NETWORK;
        envelope[index++] = (byte) DEV_ID_UICC;

        // Address TLV (if present). Encoded length is assumed to be < 127 bytes.
        if (scAddressLength != 0) {
            envelope[index++] = (byte) ComprehensionTlvTag.ADDRESS.value();
            envelope[index++] = (byte) scAddressLength;
            System.arraycopy(pdu, 1, envelope, index, scAddressLength);
            index += scAddressLength;
        }

        // SMS TPDU TLV. Length is assumed to be < 256 bytes.
        envelope[index++] = (byte) (0x80 | ComprehensionTlvTag.SMS_TPDU.value());
        if (tpduLength > 127) {
            envelope[index++] = (byte) 0x81;    // length 128-255 encoded as 0x81 + length
        }
        envelope[index++] = (byte) tpduLength;
        System.arraycopy(pdu, tpduIndex, envelope, index, tpduLength);
        index += tpduLength;

        // Verify that we calculated the payload size correctly.
        if (index != envelope.length) {
            Rlog.e(TAG, "startDataDownload() calculated incorrect envelope length, aborting.");
            acknowledgeSmsWithError(CommandsInterface.GSM_SMS_FAIL_CAUSE_UNSPECIFIED_ERROR);
            return;
        }

        String encodedEnvelope = IccUtils.bytesToHexString(envelope);
        mCi.sendEnvelopeWithStatus(encodedEnvelope, obtainMessage(
                EVENT_SEND_ENVELOPE_RESPONSE, new int[]{ dcs, pid }));
    }

    /**
     * Return the size in bytes of the envelope to send to the UICC, excluding the
     * SMS-PP download tag byte and length byte(s). If the size returned is <= 127,
     * the BER-TLV length will be encoded in 1 byte, otherwise 2 bytes are required.
     *
     * @param scAddressLength the length of the SMSC address, or zero if not present
     * @param tpduLength the length of the TPDU from the SMS-PP message
     * @return the number of bytes to allocate for the envelope command
     */
    private static int getEnvelopeBodyLength(int scAddressLength, int tpduLength) {
        // Add 4 bytes for device identities TLV + 1 byte for SMS TPDU tag byte
        int length = tpduLength + 5;
        // Add 1 byte for TPDU length, or 2 bytes if length > 127
        length += (tpduLength > 127 ? 2 : 1);
        // Add length of address tag, if present (+ 2 bytes for tag and length)
        if (scAddressLength != 0) {
            length = length + 2 + scAddressLength;
        }
        return length;
    }

    /**
     * Handle the response to the ENVELOPE command.
     * @param response UICC response encoded as hexadecimal digits. First two bytes are the
     *  UICC SW1 and SW2 status bytes.
     */
    private void sendSmsAckForEnvelopeResponse(IccIoResult response, int dcs, int pid) {
        int sw1 = response.sw1;
        int sw2 = response.sw2;

        boolean success;
        if ((sw1 == 0x90 && sw2 == 0x00) || sw1 == 0x91) {
            Rlog.d(TAG, "USIM data download succeeded: " + response.toString());
            success = true;
        } else if (sw1 == 0x93 && sw2 == 0x00) {
            Rlog.e(TAG, "USIM data download failed: Toolkit busy");
            acknowledgeSmsWithError(CommandsInterface.GSM_SMS_FAIL_CAUSE_USIM_APP_TOOLKIT_BUSY);
            return;
        } else if (sw1 == 0x62 || sw1 == 0x63) {
            Rlog.e(TAG, "USIM data download failed: " + response.toString());
            success = false;
        } else {
            Rlog.e(TAG, "Unexpected SW1/SW2 response from UICC: " + response.toString());
            success = false;
        }

        byte[] responseBytes = response.payload;
        if (responseBytes == null || responseBytes.length == 0) {
            if (success) {
                mCi.acknowledgeLastIncomingGsmSms(true, 0, null);
            } else {
                acknowledgeSmsWithError(
                        CommandsInterface.GSM_SMS_FAIL_CAUSE_USIM_DATA_DOWNLOAD_ERROR);
            }
            return;
        }

        byte[] smsAckPdu;
        int index = 0;
        if (success) {
            smsAckPdu = new byte[responseBytes.length + 5];
            smsAckPdu[index++] = 0x00;      // TP-MTI, TP-UDHI
            smsAckPdu[index++] = 0x07;      // TP-PI: TP-PID, TP-DCS, TP-UDL present
        } else {
            smsAckPdu = new byte[responseBytes.length + 6];
            smsAckPdu[index++] = 0x00;      // TP-MTI, TP-UDHI
            smsAckPdu[index++] = (byte)
                    CommandsInterface.GSM_SMS_FAIL_CAUSE_USIM_DATA_DOWNLOAD_ERROR;  // TP-FCS
            smsAckPdu[index++] = 0x07;      // TP-PI: TP-PID, TP-DCS, TP-UDL present
        }

        smsAckPdu[index++] = (byte) pid;
        smsAckPdu[index++] = (byte) dcs;

        if (is7bitDcs(dcs)) {
            int septetCount = responseBytes.length * 8 / 7;
            smsAckPdu[index++] = (byte) septetCount;
        } else {
            smsAckPdu[index++] = (byte) responseBytes.length;
        }

        System.arraycopy(responseBytes, 0, smsAckPdu, index, responseBytes.length);

        mCi.acknowledgeIncomingGsmSmsWithPdu(success,
                IccUtils.bytesToHexString(smsAckPdu), null);
    }

    private void acknowledgeSmsWithError(int cause) {
        mCi.acknowledgeLastIncomingGsmSms(false, cause, null);
    }

    /**
     * Returns whether the DCS is 7 bit. If so, set TP-UDL to the septet count of TP-UD;
     * otherwise, set TP-UDL to the octet count of TP-UD.
     * @param dcs the TP-Data-Coding-Scheme field from the original download SMS
     * @return true if the DCS specifies 7 bit encoding; false otherwise
     */
    private static boolean is7bitDcs(int dcs) {
        // See 3GPP TS 23.038 section 4
        return ((dcs & 0x8C) == 0x00) || ((dcs & 0xF4) == 0xF0);
    }

    /**
     * Handle UICC envelope response and send SMS acknowledgement.
     *
     * @param msg the message to handle
     */
    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;

        switch (msg.what) {
            case EVENT_START_DATA_DOWNLOAD:
                handleDataDownload((SmsMessage) msg.obj);
                break;

            case EVENT_SEND_ENVELOPE_RESPONSE:
                ar = (AsyncResult) msg.obj;

                if (ar.exception != null) {
                    Rlog.e(TAG, "UICC Send Envelope failure, exception: " + ar.exception);
                    acknowledgeSmsWithError(
                            CommandsInterface.GSM_SMS_FAIL_CAUSE_USIM_DATA_DOWNLOAD_ERROR);
                    return;
                }

                int[] dcsPid = (int[]) ar.userObj;
                sendSmsAckForEnvelopeResponse((IccIoResult) ar.result, dcsPid[0], dcsPid[1]);
                break;

            case EVENT_WRITE_SMS_COMPLETE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    Rlog.d(TAG, "Successfully wrote SMS-PP message to UICC");
                    mCi.acknowledgeLastIncomingGsmSms(true, 0, null);
                } else {
                    Rlog.d(TAG, "Failed to write SMS-PP message to UICC", ar.exception);
                    mCi.acknowledgeLastIncomingGsmSms(false,
                            CommandsInterface.GSM_SMS_FAIL_CAUSE_UNSPECIFIED_ERROR, null);
                }
                break;

            default:
                Rlog.e(TAG, "Ignoring unexpected message, what=" + msg.what);
        }
    }
}
