/*
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

package com.android.internal.telephony;

/**
 * Call fail causes from TS 24.008 .
 * These are mostly the cause codes we need to distinguish for the UI.
 * See 22.001 Annex F.4 for mapping of cause codes to local tones.
 *
 * CDMA call failure reasons are derived from the possible call failure scenarios described
 * in "CDMA IS2000 - Release A (C.S0005-A v6.0)" standard.
 *
 * {@hide}
 *
 */
public interface CallFailCause {
    // Unassigned/Unobtainable number
    int UNOBTAINABLE_NUMBER = 1;

    int NORMAL_CLEARING     = 16;
    // Busy Tone
    int USER_BUSY           = 17;

    // No Tone
    int NUMBER_CHANGED      = 22;
    int STATUS_ENQUIRY      = 30;
    int NORMAL_UNSPECIFIED  = 31;

    // Congestion Tone
    int NO_CIRCUIT_AVAIL    = 34;
    int TEMPORARY_FAILURE   = 41;
    int SWITCHING_CONGESTION    = 42;
    int CHANNEL_NOT_AVAIL   = 44;
    int QOS_NOT_AVAIL       = 49;
    int BEARER_NOT_AVAIL    = 58;

    // others
    int ACM_LIMIT_EXCEEDED = 68;
    int CALL_BARRED        = 240;
    int FDN_BLOCKED        = 241;

    // Stk Call Control
    int DIAL_MODIFIED_TO_USSD = 244;
    int DIAL_MODIFIED_TO_SS   = 245;
    int DIAL_MODIFIED_TO_DIAL = 246;

    int CDMA_LOCKED_UNTIL_POWER_CYCLE  = 1000;
    int CDMA_DROP                      = 1001;
    int CDMA_INTERCEPT                 = 1002;
    int CDMA_REORDER                   = 1003;
    int CDMA_SO_REJECT                 = 1004;
    int CDMA_RETRY_ORDER               = 1005;
    int CDMA_ACCESS_FAILURE            = 1006;
    int CDMA_PREEMPTED                 = 1007;

    // For non-emergency number dialed while in emergency callback mode.
    int CDMA_NOT_EMERGENCY             = 1008;

    // Access Blocked by CDMA Network.
    int CDMA_ACCESS_BLOCKED            = 1009;

    int ERROR_UNSPECIFIED = 0xffff;

}
