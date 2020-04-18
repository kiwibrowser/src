/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.internal.telephony;

import android.content.Context;
import android.os.AsyncTask;
import android.provider.BlockedNumberContract;
import android.telephony.Rlog;

/**
 * An {@link AsyncTask} that notifies the Blocked number provider that emergency services were
 * contacted. See {@link BlockedNumberContract.SystemContract#notifyEmergencyContact(Context)}
 * for details.
 * {@hide}
 */
public class AsyncEmergencyContactNotifier extends AsyncTask<Void, Void, Void> {
    private static final String TAG = "AsyncEmergencyContactNotifier";

    private final Context mContext;

    public AsyncEmergencyContactNotifier(Context context) {
        mContext = context;
    }

    @Override
    protected Void doInBackground(Void... params) {
        try {
            BlockedNumberContract.SystemContract.notifyEmergencyContact(mContext);
        } catch (Exception e) {
            Rlog.e(TAG, "Exception notifying emergency contact: " + e);
        }
        return null;
    }
}
