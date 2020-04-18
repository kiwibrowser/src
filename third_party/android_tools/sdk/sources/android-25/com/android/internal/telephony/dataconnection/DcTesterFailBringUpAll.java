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
 * A package level call that causes all DataConnection bringUp calls to fail a specific
 * number of times. Here is an example that sets counter to 2 and cause to -3 for all instances:
 *    adb shell am broadcast -a com.android.internal.telephony.dataconnection.action_fail_bringup \
 *     --ei counter 2 --ei fail_cause -3
 *
 * Also you can add a suggested retry time if desired:
 *     --ei suggested_retry_time 5000
 *
 * The fail_cause is one of {@link DcFailCause}
 */
public class DcTesterFailBringUpAll {
    private static final String LOG_TAG = "DcTesterFailBrinupAll";
    private static final boolean DBG = true;

    private Phone mPhone;

    private String mActionFailBringUp = DcFailBringUp.INTENT_BASE + "."
            + DcFailBringUp.ACTION_FAIL_BRINGUP;

    // The saved FailBringUp data from the intent
    private DcFailBringUp mFailBringUp = new DcFailBringUp();

    // The static intent receiver one for all instances.
    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
            @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (DBG) log("sIntentReceiver.onReceive: action=" + action);
            if (action.equals(mActionFailBringUp)) {
                mFailBringUp.saveParameters(intent, "sFailBringUp");
            } else if (action.equals(mPhone.getActionDetached())) {
                // Counter is MAX, bringUp/retry will always fail
                log("simulate detaching");
                mFailBringUp.saveParameters(Integer.MAX_VALUE,
                        DcFailCause.LOST_CONNECTION.getErrorCode(),
                        DcFailBringUp.DEFAULT_SUGGESTED_RETRY_TIME);
            } else if (action.equals(mPhone.getActionAttached())) {
                // Counter is 0 next bringUp/retry will succeed
                log("simulate attaching");
                mFailBringUp.saveParameters(0, DcFailCause.NONE.getErrorCode(),
                        DcFailBringUp.DEFAULT_SUGGESTED_RETRY_TIME);
            } else {
                if (DBG) log("onReceive: unknown action=" + action);
            }
        }
    };

    DcTesterFailBringUpAll(Phone phone, Handler handler) {
        mPhone = phone;
        if (Build.IS_DEBUGGABLE) {
            IntentFilter filter = new IntentFilter();

            filter.addAction(mActionFailBringUp);
            log("register for intent action=" + mActionFailBringUp);

            filter.addAction(mPhone.getActionDetached());
            log("register for intent action=" + mPhone.getActionDetached());

            filter.addAction(mPhone.getActionAttached());
            log("register for intent action=" + mPhone.getActionAttached());

            phone.getContext().registerReceiver(mIntentReceiver, filter, null, handler);
        }
    }

    void dispose() {
        if (Build.IS_DEBUGGABLE) {
            mPhone.getContext().unregisterReceiver(mIntentReceiver);
        }
    }

    public DcFailBringUp getDcFailBringUp() {
        return mFailBringUp;
    }

    private void log(String s) {
        Rlog.d(LOG_TAG, s);
    }
}
