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
import com.android.internal.telephony.TelephonyProto.ImsReasonInfo;
import com.android.internal.telephony.TelephonyProto.RilDataCall;
import com.android.internal.telephony.TelephonyProto.TelephonyCallSession;
import com.android.internal.telephony.TelephonyProto.TelephonyServiceState;
import com.android.internal.telephony.TelephonyProto.TelephonySettings;

public class CallSessionEventBuilder {
    private final TelephonyCallSession.Event mEvent = new TelephonyCallSession.Event();

    public TelephonyCallSession.Event build() {
        return mEvent;
    }

    public CallSessionEventBuilder(int type) {
        mEvent.setType(type);
    }

    public CallSessionEventBuilder setDelay(int delay) {
        mEvent.setDelay(delay);
        return this;
    }

    public CallSessionEventBuilder setRilRequest(int rilRequestType) {
        mEvent.setRilRequest(rilRequestType);
        return this;
    }

    public CallSessionEventBuilder setRilRequestId(int rilRequestId) {
        mEvent.setRilRequestId(rilRequestId);
        return this;
    }

    public CallSessionEventBuilder setRilError(int rilError) {
        mEvent.setError(rilError);
        return this;
    }

    public CallSessionEventBuilder setCallIndex(int callIndex) {
        mEvent.setCallIndex(callIndex);
        return this;
    }

    public CallSessionEventBuilder setCallState(int state) {
        mEvent.setCallState(state);
        return this;
    }

    public CallSessionEventBuilder setSrvccState(int srvccState) {
        mEvent.setSrvccState(srvccState);
        return this;
    }

    public CallSessionEventBuilder setImsCommand(int imsCommand) {
        mEvent.setImsCommand(imsCommand);
        return this;
    }

    public CallSessionEventBuilder setImsReasonInfo(ImsReasonInfo reasonInfo) {
        mEvent.reasonInfo = reasonInfo;
        return this;
    }

    public CallSessionEventBuilder setSrcAccessTech(int tech) {
        mEvent.setSrcAccessTech(tech);
        return this;
    }

    public CallSessionEventBuilder setTargetAccessTech(int tech) {
        mEvent.setTargetAccessTech(tech);
        return this;
    }

    public CallSessionEventBuilder setSettings(TelephonySettings settings) {
        mEvent.settings = settings;
        return this;
    }

    public CallSessionEventBuilder setServiceState(TelephonyServiceState state) {
        mEvent.serviceState = state;
        return this;
    }

    public CallSessionEventBuilder setImsConnectionState(ImsConnectionState state) {
        mEvent.imsConnectionState = state;
        return this;
    }

    public CallSessionEventBuilder setImsCapabilities(ImsCapabilities capabilities) {
        mEvent.imsCapabilities = capabilities;
        return this;
    }

    public CallSessionEventBuilder setDataCalls(RilDataCall[] dataCalls) {
        mEvent.dataCalls = dataCalls;
        return this;
    }

    public CallSessionEventBuilder setPhoneState(int phoneState) {
        mEvent.setPhoneState(phoneState);
        return this;
    }

    public CallSessionEventBuilder setNITZ(long timestamp) {
        mEvent.setNitzTimestampMillis(timestamp);
        return this;
    }
}
