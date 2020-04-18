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

package com.android.internal.telephony.dataconnection;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Handler;
import android.telephony.Rlog;

import com.android.internal.telephony.Phone;

/**
 * To bring down all DC's send the following intent:
 *
 * adb shell am broadcast -a com.android.internal.telephony.dataconnection.action_deactivate_all
 */
public class DcTesterDeactivateAll {
    private static final String LOG_TAG = "DcTesterDeacativateAll";
    private static final boolean DBG = true;

    private Phone mPhone;
    private DcController mDcc;

    public static String sActionDcTesterDeactivateAll =
            "com.android.internal.telephony.dataconnection.action_deactivate_all";


    // The static intent receiver one for all instances and we assume this
    // is running on the same thread as Dcc.
    protected BroadcastReceiver sIntentReceiver = new BroadcastReceiver() {
            @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (DBG) log("sIntentReceiver.onReceive: action=" + action);
            if (action.equals(sActionDcTesterDeactivateAll)
                    || action.equals(mPhone.getActionDetached())) {
                log("Send DEACTIVATE to all Dcc's");
                if (mDcc != null) {
                    for (DataConnection dc : mDcc.mDcListAll) {
                        dc.tearDownNow();
                    }
                } else {
                    if (DBG) log("onReceive: mDcc is null, ignoring");
                }
            } else {
                if (DBG) log("onReceive: unknown action=" + action);
            }
        }
    };

    DcTesterDeactivateAll(Phone phone, DcController dcc, Handler handler) {
        mPhone = phone;
        mDcc = dcc;

        if (Build.IS_DEBUGGABLE) {
            IntentFilter filter = new IntentFilter();

            filter.addAction(sActionDcTesterDeactivateAll);
            log("register for intent action=" + sActionDcTesterDeactivateAll);

            filter.addAction(mPhone.getActionDetached());
            log("register for intent action=" + mPhone.getActionDetached());

            phone.getContext().registerReceiver(sIntentReceiver, filter, null, handler);
        }
    }

    void dispose() {
        if (Build.IS_DEBUGGABLE) {
            mPhone.getContext().unregisterReceiver(sIntentReceiver);
        }
    }

    private static void log(String s) {
        Rlog.d(LOG_TAG, s);
    }
}
