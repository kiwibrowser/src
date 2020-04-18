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

package com.android.internal.telephony.cdma;

import android.Manifest;
import android.app.Activity;
import android.app.AppOpsManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Message;
import android.provider.Telephony.Sms.Intents;
import android.telephony.PhoneNumberUtils;
import android.telephony.SubscriptionManager;
import android.telephony.cdma.CdmaSmsCbProgramData;
import android.telephony.cdma.CdmaSmsCbProgramResults;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.WakeLockStateMachine;
import com.android.internal.telephony.cdma.sms.BearerData;
import com.android.internal.telephony.cdma.sms.CdmaSmsAddress;
import com.android.internal.telephony.cdma.sms.SmsEnvelope;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;

/**
 * Handle CDMA Service Category Program Data requests and responses.
 */
public final class CdmaServiceCategoryProgramHandler extends WakeLockStateMachine {

    final CommandsInterface mCi;

    /**
     * Create a new CDMA inbound SMS handler.
     */
    CdmaServiceCategoryProgramHandler(Context context, CommandsInterface commandsInterface) {
        super("CdmaServiceCategoryProgramHandler", context, null);
        mContext = context;
        mCi = commandsInterface;
    }

    /**
     * Create a new State machine for SCPD requests.
     * @param context the context to use
     * @param commandsInterface the radio commands interface
     * @return the new SCPD handler
     */
    static CdmaServiceCategoryProgramHandler makeScpHandler(Context context,
            CommandsInterface commandsInterface) {
        CdmaServiceCategoryProgramHandler handler = new CdmaServiceCategoryProgramHandler(
                context, commandsInterface);
        handler.start();
        return handler;
    }

    /**
     * Handle Cell Broadcast messages from {@code CdmaInboundSmsHandler}.
     * 3GPP-format Cell Broadcast messages sent from radio are handled in the subclass.
     *
     * @param message the message to process
     * @return true if an ordered broadcast was sent; false on failure
     */
    @Override
    protected boolean handleSmsMessage(Message message) {
        if (message.obj instanceof SmsMessage) {
            return handleServiceCategoryProgramData((SmsMessage) message.obj);
        } else {
            loge("handleMessage got object of type: " + message.obj.getClass().getName());
            return false;
        }
    }


    /**
     * Send SCPD request to CellBroadcastReceiver as an ordered broadcast.
     * @param sms the CDMA SmsMessage containing the SCPD request
     * @return true if an ordered broadcast was sent; false on failure
     */
    private boolean handleServiceCategoryProgramData(SmsMessage sms) {
        ArrayList<CdmaSmsCbProgramData> programDataList = sms.getSmsCbProgramData();
        if (programDataList == null) {
            loge("handleServiceCategoryProgramData: program data list is null!");
            return false;
        }

        Intent intent = new Intent(Intents.SMS_SERVICE_CATEGORY_PROGRAM_DATA_RECEIVED_ACTION);
        intent.putExtra("sender", sms.getOriginatingAddress());
        intent.putParcelableArrayListExtra("program_data", programDataList);
        SubscriptionManager.putPhoneIdAndSubIdExtra(intent, mPhone.getPhoneId());
        mContext.sendOrderedBroadcast(intent, Manifest.permission.RECEIVE_SMS,
                AppOpsManager.OP_RECEIVE_SMS, mScpResultsReceiver,
                getHandler(), Activity.RESULT_OK, null, null);
        return true;
    }

    /**
     * Broadcast receiver to handle results of ordered broadcast. Sends the SCPD results
     * as a reply SMS, then sends a message to state machine to transition to idle.
     */
    private final BroadcastReceiver mScpResultsReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            sendScpResults();
            if (DBG) log("mScpResultsReceiver finished");
            sendMessage(EVENT_BROADCAST_COMPLETE);
        }

        private void sendScpResults() {
            int resultCode = getResultCode();
            if ((resultCode != Activity.RESULT_OK) && (resultCode != Intents.RESULT_SMS_HANDLED)) {
                loge("SCP results error: result code = " + resultCode);
                return;
            }
            Bundle extras = getResultExtras(false);
            if (extras == null) {
                loge("SCP results error: missing extras");
                return;
            }
            String sender = extras.getString("sender");
            if (sender == null) {
                loge("SCP results error: missing sender extra.");
                return;
            }
            ArrayList<CdmaSmsCbProgramResults> results
                    = extras.getParcelableArrayList("results");
            if (results == null) {
                loge("SCP results error: missing results extra.");
                return;
            }

            BearerData bData = new BearerData();
            bData.messageType = BearerData.MESSAGE_TYPE_SUBMIT;
            bData.messageId = SmsMessage.getNextMessageId();
            bData.serviceCategoryProgramResults = results;
            byte[] encodedBearerData = BearerData.encode(bData);

            ByteArrayOutputStream baos = new ByteArrayOutputStream(100);
            DataOutputStream dos = new DataOutputStream(baos);
            try {
                dos.writeInt(SmsEnvelope.TELESERVICE_SCPT);
                dos.writeInt(0); //servicePresent
                dos.writeInt(0); //serviceCategory
                CdmaSmsAddress destAddr = CdmaSmsAddress.parse(
                        PhoneNumberUtils.cdmaCheckAndProcessPlusCodeForSms(sender));
                dos.write(destAddr.digitMode);
                dos.write(destAddr.numberMode);
                dos.write(destAddr.ton); // number_type
                dos.write(destAddr.numberPlan);
                dos.write(destAddr.numberOfDigits);
                dos.write(destAddr.origBytes, 0, destAddr.origBytes.length); // digits
                // Subaddress is not supported.
                dos.write(0); //subaddressType
                dos.write(0); //subaddr_odd
                dos.write(0); //subaddr_nbr_of_digits
                dos.write(encodedBearerData.length);
                dos.write(encodedBearerData, 0, encodedBearerData.length);
                // Ignore the RIL response. TODO: implement retry if SMS send fails.
                mCi.sendCdmaSms(baos.toByteArray(), null);
            } catch (IOException e) {
                loge("exception creating SCP results PDU", e);
            } finally {
                try {
                    dos.close();
                } catch (IOException ignored) {
                }
            }
        }
    };
}
