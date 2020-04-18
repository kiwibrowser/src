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

package com.android.internal.telephony.imsphone;

import com.android.ims.ImsCallProfile;

/**
 * Interface implemented by modules which are capable of performing a pull of an external call.
 * This is used to break the dependency between {@link ImsExternalCallTracker} and
 * {@link ImsPhoneCallTracker}.
 *
 * @hide
 */
public interface ImsPullCall {
    /**
     * Initiate a pull of a call which has the specified phone number.
     *
     * @param number The phone number of the call to be pulled.
     * @param videoState The video state of the call to be pulled.
     * @param dialogId The {@link ImsExternalConnection#getCallId()} dialog Id associated with the
     *                 call to be pulled.
     */
    void pullExternalCall(String number, int videoState, int dialogId);
}
