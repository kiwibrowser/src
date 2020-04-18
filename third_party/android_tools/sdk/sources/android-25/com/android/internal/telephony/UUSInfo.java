/*
 * Copyright (C) 2010 The Android Open Source Project
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

public class UUSInfo {

    /*
     * User-to-User signaling Info activation types derived from 3GPP 23.087
     * v8.0
     */

    public static final int UUS_TYPE1_IMPLICIT = 0;

    public static final int UUS_TYPE1_REQUIRED = 1;

    public static final int UUS_TYPE1_NOT_REQUIRED = 2;

    public static final int UUS_TYPE2_REQUIRED = 3;

    public static final int UUS_TYPE2_NOT_REQUIRED = 4;

    public static final int UUS_TYPE3_REQUIRED = 5;

    public static final int UUS_TYPE3_NOT_REQUIRED = 6;

    /*
     * User-to-User Signaling Information data coding schemes. Possible values
     * for Octet 3 (Protocol Discriminator field) in the UUIE. The values have
     * been specified in section 10.5.4.25 of 3GPP TS 24.008
     */

    public static final int UUS_DCS_USP = 0; /* User specified protocol */

    public static final int UUS_DCS_OSIHLP = 1; /* OSI higher layer protocol */

    public static final int UUS_DCS_X244 = 2; /* X.244 */

    public static final int UUS_DCS_RMCF = 3; /*
                                               * Reserved for system management
                                               * convergence function
                                               */

    public static final int UUS_DCS_IA5c = 4; /* IA5 characters */

    private int mUusType;

    private int mUusDcs;

    private byte[] mUusData;

    public UUSInfo() {
        mUusType = UUS_TYPE1_IMPLICIT;
        mUusDcs = UUS_DCS_IA5c;
        mUusData = null;
    }

    public UUSInfo(int uusType, int uusDcs, byte[] uusData) {
        mUusType = uusType;
        mUusDcs = uusDcs;
        mUusData = uusData;
    }

    public int getDcs() {
        return mUusDcs;
    }

    public void setDcs(int uusDcs) {
        mUusDcs = uusDcs;
    }

    public int getType() {
        return mUusType;
    }

    public void setType(int uusType) {
        mUusType = uusType;
    }

    public byte[] getUserData() {
        return mUusData;
    }

    public void setUserData(byte[] uusData) {
        mUusData = uusData;
    }
}
