/*
 * Copyright (C) 2012 The Android Open Source Project
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

import android.telephony.Rlog;
import android.text.TextUtils;

import com.android.internal.telephony.metrics.TelephonyMetrics;

import java.io.FileDescriptor;
import java.io.PrintWriter;

/**
 * A debug service that will dump telephony's state
 *
 * Currently this "Service" has a proxy in the phone app
 * com.android.phone.TelephonyDebugService which actually
 * invokes the dump method.
 */
public class DebugService {
    private static String TAG = "DebugService";

    /** Constructor */
    public DebugService() {
        log("DebugService:");
    }

    /**
     * Dump the state of various objects, add calls to other objects as desired.
     */
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        if (args != null && args.length > 0) {
            if (TextUtils.equals(args[0], "--metrics") ||
                    TextUtils.equals(args[0], "--metricsproto"))
            {
                log("Collecting telephony metrics..");
                TelephonyMetrics.getInstance().dump(fd, pw, args);
                return;
            }
        }
        log("Dump telephony.");
        PhoneFactory.dump(fd, pw, args);
    }

    private static void log(String s) {
        Rlog.d(TAG, "DebugService " + s);
    }
}
