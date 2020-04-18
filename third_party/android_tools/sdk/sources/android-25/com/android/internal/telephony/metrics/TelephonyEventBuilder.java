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

import android.os.SystemClock;

import static com.android.internal.telephony.TelephonyProto.ImsCapabilities;
import static com.android.internal.telephony.TelephonyProto.ImsConnectionState;
import static com.android.internal.telephony.TelephonyProto.RilDataCall;
import static com.android.internal.telephony.TelephonyProto.TelephonyEvent;
import static com.android.internal.telephony.TelephonyProto.TelephonyEvent.RilDeactivateDataCall;
import static com.android.internal.telephony.TelephonyProto.TelephonyEvent.RilSetupDataCall;
import static com.android.internal.telephony.TelephonyProto.TelephonyEvent.RilSetupDataCallResponse;
import static com.android.internal.telephony.TelephonyProto.TelephonyServiceState;
import static com.android.internal.telephony.TelephonyProto.TelephonySettings;

public class TelephonyEventBuilder {
    private final TelephonyEvent mEvent = new TelephonyEvent();

    public TelephonyEvent build() {
        return mEvent;
    }

    public TelephonyEventBuilder(int phoneId) {
        this(SystemClock.elapsedRealtime(), phoneId);
    }

    public TelephonyEventBuilder(long timestamp, int phoneId) {
        mEvent.setTimestampMillis(timestamp);
        mEvent.setPhoneId(phoneId);
    }

    public TelephonyEventBuilder setSettings(TelephonySettings settings) {
        mEvent.setType(TelephonyEvent.Type.SETTINGS_CHANGED);
        mEvent.settings = settings;
        return this;
    }

    public TelephonyEventBuilder setServiceState(TelephonyServiceState state) {
        mEvent.setType(TelephonyEvent.Type.RIL_SERVICE_STATE_CHANGED);
        mEvent.serviceState = state;
        return this;
    }

    public TelephonyEventBuilder setImsConnectionState(ImsConnectionState state) {
        mEvent.setType(TelephonyEvent.Type.IMS_CONNECTION_STATE_CHANGED);
        mEvent.imsConnectionState = state;
        return this;
    }

    public TelephonyEventBuilder setImsCapabilities(ImsCapabilities capabilities) {
        mEvent.setType(TelephonyEvent.Type.IMS_CAPABILITIES_CHANGED);
        mEvent.imsCapabilities = capabilities;
        return this;
    }

    public TelephonyEventBuilder setDataStallRecoveryAction(int action) {
        mEvent.setType(TelephonyEvent.Type.DATA_STALL_ACTION);
        mEvent.setDataStallAction(action);
        return this;
    }

    public TelephonyEventBuilder setSetupDataCall(RilSetupDataCall request) {
        mEvent.setType(TelephonyEvent.Type.DATA_CALL_SETUP);
        mEvent.setupDataCall = request;
        return this;
    }

    public TelephonyEventBuilder setSetupDataCallResponse(RilSetupDataCallResponse rsp) {
        mEvent.setType(TelephonyEvent.Type.DATA_CALL_SETUP_RESPONSE);
        mEvent.setupDataCallResponse = rsp;
        return this;
    }

    public TelephonyEventBuilder setDeactivateDataCall(RilDeactivateDataCall request) {
        mEvent.setType(TelephonyEvent.Type.DATA_CALL_DEACTIVATE);
        mEvent.deactivateDataCall = request;
        return this;
    }

    public TelephonyEventBuilder setDeactivateDataCallResponse(int errno) {
        mEvent.setType(TelephonyEvent.Type.DATA_CALL_DEACTIVATE_RESPONSE);
        mEvent.setError(errno);
        return this;
    }

    public TelephonyEventBuilder setDataCalls(RilDataCall[] dataCalls) {
        mEvent.setType(TelephonyEvent.Type.DATA_CALL_LIST_CHANGED);
        mEvent.dataCalls = dataCalls;
        return this;
    }

    public TelephonyEventBuilder setNITZ(long timestamp) {
        mEvent.setType(TelephonyEvent.Type.NITZ_TIME);
        mEvent.setNitzTimestampMillis(timestamp);
        return this;
    }
}
