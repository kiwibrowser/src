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
 * limitations under the License.
 */

package com.android.internal.telephony.metrics;

import com.android.internal.telephony.TelephonyProto.ImsCapabilities;
import com.android.internal.telephony.TelephonyProto.ImsConnectionState;
import com.android.internal.telephony.TelephonyProto.RilDataCall;
import com.android.internal.telephony.TelephonyProto.SmsSession;
import com.android.internal.telephony.TelephonyProto.TelephonyServiceState;
import com.android.internal.telephony.TelephonyProto.TelephonySettings;

public class SmsSessionEventBuilder {
    SmsSession.Event mEvent = new SmsSession.Event();

    public SmsSession.Event build() {
        return mEvent;
    }

    public SmsSessionEventBuilder(int type) {
        mEvent.setType(type);
    }

    public SmsSessionEventBuilder setDelay(int delay) {
        mEvent.setDelay(delay);
        return this;
    }

    public SmsSessionEventBuilder setTech(int tech) {
        mEvent.setTech(tech);
        return this;
    }

    public SmsSessionEventBuilder setErrorCode(int code) {
        mEvent.setErrorCode(code);
        return this;
    }

    public SmsSessionEventBuilder setRilErrno(int errno) {
        mEvent.setError(errno);
        return this;
    }

    public SmsSessionEventBuilder setSettings(TelephonySettings settings) {
        mEvent.settings = settings;
        return this;
    }

    public SmsSessionEventBuilder setServiceState(TelephonyServiceState state) {
        mEvent.serviceState = state;
        return this;
    }

    public SmsSessionEventBuilder setImsConnectionState(ImsConnectionState state) {
        mEvent.imsConnectionState = state;
        return this;
    }

    public SmsSessionEventBuilder setImsCapabilities(ImsCapabilities capabilities) {
        mEvent.imsCapabilities = capabilities;
        return this;
    }

    public SmsSessionEventBuilder setDataCalls(RilDataCall[] dataCalls) {
        mEvent.dataCalls = dataCalls;
        return this;
    }

    public SmsSessionEventBuilder setRilRequestId(int id) {
        mEvent.setRilRequestId(id);
        return this;
    }

    public SmsSessionEventBuilder setFormat(int format) {
        mEvent.setFormat(format);
        return this;
    }
}
