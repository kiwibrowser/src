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

import android.Manifest;
import android.app.Activity;
import android.app.AppOpsManager;
import android.content.Context;
import android.content.Intent;
import android.os.Message;
import android.os.UserHandle;
import android.provider.Telephony;
import android.telephony.SubscriptionManager;
import android.telephony.SmsCbMessage;

/**
 * Dispatch new Cell Broadcasts to receivers. Acquires a private wakelock until the broadcast
 * completes and our result receiver is called.
 */
public class CellBroadcastHandler extends WakeLockStateMachine {

    private CellBroadcastHandler(Context context, Phone phone) {
        this("CellBroadcastHandler", context, phone);
    }

    protected CellBroadcastHandler(String debugTag, Context context, Phone phone) {
        super(debugTag, context, phone);
    }

    /**
     * Create a new CellBroadcastHandler.
     * @param context the context to use for dispatching Intents
     * @return the new handler
     */
    public static CellBroadcastHandler makeCellBroadcastHandler(Context context, Phone phone) {
        CellBroadcastHandler handler = new CellBroadcastHandler(context, phone);
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
        if (message.obj instanceof SmsCbMessage) {
            handleBroadcastSms((SmsCbMessage) message.obj);
            return true;
        } else {
            loge("handleMessage got object of type: " + message.obj.getClass().getName());
            return false;
        }
    }

    /**
     * Dispatch a Cell Broadcast message to listeners.
     * @param message the Cell Broadcast to broadcast
     */
    protected void handleBroadcastSms(SmsCbMessage message) {
        String receiverPermission;
        int appOp;

        Intent intent;
        if (message.isEmergencyMessage()) {
            log("Dispatching emergency SMS CB, SmsCbMessage is: " + message);
            intent = new Intent(Telephony.Sms.Intents.SMS_EMERGENCY_CB_RECEIVED_ACTION);
            receiverPermission = Manifest.permission.RECEIVE_EMERGENCY_BROADCAST;
            appOp = AppOpsManager.OP_RECEIVE_EMERGECY_SMS;
        } else {
            log("Dispatching SMS CB, SmsCbMessage is: " + message);
            intent = new Intent(Telephony.Sms.Intents.SMS_CB_RECEIVED_ACTION);
            receiverPermission = Manifest.permission.RECEIVE_SMS;
            appOp = AppOpsManager.OP_RECEIVE_SMS;
        }
        intent.putExtra("message", message);
        SubscriptionManager.putPhoneIdAndSubIdExtra(intent, mPhone.getPhoneId());
        mContext.sendOrderedBroadcastAsUser(intent, UserHandle.ALL, receiverPermission, appOp,
                mReceiver, getHandler(), Activity.RESULT_OK, null, null);
    }
}
