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

import android.content.Intent;
import android.telephony.Rlog;

/**
 * A package visible class for supporting testing failing bringUp commands. This
 * saves the parameters from a action_fail_bringup intent. See
 * {@link DataConnection#doOnConnect} and {@see DcTesterFailBringUpAll} for more info.
 */
public class DcFailBringUp {
    private static final String LOG_TAG = "DcFailBringUp";
    private static final boolean DBG = true;

    static final String INTENT_BASE = DataConnection.class.getPackage().getName();

    static final String ACTION_FAIL_BRINGUP = "action_fail_bringup";

    // counter with its --ei option name and default value
    static final String COUNTER = "counter";
    static final int DEFAULT_COUNTER = 2;
    int mCounter;

    // failCause with its --ei option name and default value
    static final String FAIL_CAUSE = "fail_cause";
    static final DcFailCause DEFAULT_FAIL_CAUSE = DcFailCause.ERROR_UNSPECIFIED;
    DcFailCause mFailCause;

    // suggestedRetryTime with its --ei option name and default value
    static final String SUGGESTED_RETRY_TIME = "suggested_retry_time";
    static final int DEFAULT_SUGGESTED_RETRY_TIME = -1;
    int mSuggestedRetryTime;

    // Get the Extra Intent parameters
    void saveParameters(Intent intent, String s) {
        if (DBG) log(s + ".saveParameters: action=" + intent.getAction());
        mCounter = intent.getIntExtra(COUNTER, DEFAULT_COUNTER);
        mFailCause = DcFailCause.fromInt(
                intent.getIntExtra(FAIL_CAUSE, DEFAULT_FAIL_CAUSE.getErrorCode()));
        mSuggestedRetryTime =
                intent.getIntExtra(SUGGESTED_RETRY_TIME, DEFAULT_SUGGESTED_RETRY_TIME);
        if (DBG) {
            log(s + ".saveParameters: " + this);
        }
    }

    public void saveParameters(int counter, int failCause, int suggestedRetryTime) {
        mCounter = counter;
        mFailCause = DcFailCause.fromInt(failCause);
        mSuggestedRetryTime = suggestedRetryTime;
    }

    @Override
    public String toString() {
        return "{mCounter=" + mCounter +
                " mFailCause=" + mFailCause +
                " mSuggestedRetryTime=" + mSuggestedRetryTime + "}";

    }

    private static void log(String s) {
        Rlog.d(LOG_TAG, s);
    }
}
