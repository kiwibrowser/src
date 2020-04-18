/*
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.internal.telephony.gsm;

import android.telephony.Rlog;
import com.android.internal.telephony.CallForwardInfo;
import com.android.internal.telephony.GsmCdmaPhone;

import java.util.ArrayList;

/**
 * See also RIL_StkCcUnsolSsResponse in include/telephony/ril.h
 *
 * {@hide}
 */
public class SsData {
    public enum ServiceType {
        SS_CFU,
        SS_CF_BUSY,
        SS_CF_NO_REPLY,
        SS_CF_NOT_REACHABLE,
        SS_CF_ALL,
        SS_CF_ALL_CONDITIONAL,
        SS_CLIP,
        SS_CLIR,
        SS_COLP,
        SS_COLR,
        SS_WAIT,
        SS_BAOC,
        SS_BAOIC,
        SS_BAOIC_EXC_HOME,
        SS_BAIC,
        SS_BAIC_ROAMING,
        SS_ALL_BARRING,
        SS_OUTGOING_BARRING,
        SS_INCOMING_BARRING;

        public boolean isTypeCF() {
            return (this == SS_CFU || this == SS_CF_BUSY || this == SS_CF_NO_REPLY ||
                  this == SS_CF_NOT_REACHABLE || this == SS_CF_ALL ||
                  this == SS_CF_ALL_CONDITIONAL);
        }

        public boolean isTypeUnConditional() {
            return (this == SS_CFU || this == SS_CF_ALL);
        }

        public boolean isTypeCW() {
            return (this == SS_WAIT);
        }

        public boolean isTypeClip() {
            return (this == SS_CLIP);
        }

        public boolean isTypeClir() {
            return (this == SS_CLIR);
        }

        public boolean isTypeBarring() {
            return (this == SS_BAOC || this == SS_BAOIC || this == SS_BAOIC_EXC_HOME ||
                  this == SS_BAIC || this == SS_BAIC_ROAMING || this == SS_ALL_BARRING ||
                  this == SS_OUTGOING_BARRING || this == SS_INCOMING_BARRING);
        }
    };

    public enum RequestType {
        SS_ACTIVATION,
        SS_DEACTIVATION,
        SS_INTERROGATION,
        SS_REGISTRATION,
        SS_ERASURE;

        public boolean isTypeInterrogation() {
            return (this == SS_INTERROGATION);
        }
    };

    public enum TeleserviceType {
        SS_ALL_TELE_AND_BEARER_SERVICES,
        SS_ALL_TELESEVICES,
        SS_TELEPHONY,
        SS_ALL_DATA_TELESERVICES,
        SS_SMS_SERVICES,
        SS_ALL_TELESERVICES_EXCEPT_SMS;
    };

    public ServiceType serviceType;
    public RequestType requestType;
    public TeleserviceType teleserviceType;
    public int serviceClass;
    public int result;

    public int[] ssInfo; /* This is the response data for most of the SS GET/SET
                            RIL requests. E.g. RIL_REQUSET_GET_CLIR returns
                            two ints, so first two values of ssInfo[] will be
                            used for respone if serviceType is SS_CLIR and
                            requestType is SS_INTERROGATION */

    public CallForwardInfo[] cfInfo; /* This is the response data for SS request
                                        to query call forward status. see
                                        RIL_REQUEST_QUERY_CALL_FORWARD_STATUS */

    public ServiceType ServiceTypeFromRILInt(int type) {
        try {
            return ServiceType.values()[type];
        } catch (IndexOutOfBoundsException e) {
            Rlog.e(GsmCdmaPhone.LOG_TAG, "Invalid Service type");
            return null;
        }
    }

    public RequestType RequestTypeFromRILInt(int type) {
        try {
            return RequestType.values()[type];
        } catch (IndexOutOfBoundsException e) {
            Rlog.e(GsmCdmaPhone.LOG_TAG, "Invalid Request type");
            return null;
        }
    }

    public TeleserviceType TeleserviceTypeFromRILInt(int type) {
        try {
            return TeleserviceType.values()[type];
        } catch (IndexOutOfBoundsException e) {
            Rlog.e(GsmCdmaPhone.LOG_TAG, "Invalid Teleservice type");
            return null;
        }
    }

    public String toString() {
        return "[SsData] " + "ServiceType: " + serviceType
            + " RequestType: " + requestType
            + " TeleserviceType: " + teleserviceType
            + " ServiceClass: " + serviceClass
            + " Result: " + result
            + " Is Service Type CF: " + serviceType.isTypeCF();
    }
}
