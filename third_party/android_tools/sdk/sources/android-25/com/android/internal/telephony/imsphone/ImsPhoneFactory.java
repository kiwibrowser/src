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

package com.android.internal.telephony.imsphone;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneNotifier;

import android.content.Context;
import android.telephony.Rlog;

/**
 * {@hide}
 */
public class ImsPhoneFactory {

    /**
     * Makes a {@link ImsPhone} object.
     * @param context {@code Context} needed to create a Phone object
     * @param phoneNotifier {@code PhoneNotifier} needed to create a Phone
     *      object
     * @return the {@code ImsPhone} object
     */
    public static ImsPhone makePhone(Context context,
            PhoneNotifier phoneNotifier, Phone defaultPhone) {

        try {
            return new ImsPhone(context, phoneNotifier, defaultPhone);
        } catch (Exception e) {
            Rlog.e("VoltePhoneFactory", "makePhone", e);
            return null;
        }
    }
}
