/*
* Copyright (C) 2014 The Android Open Source Project
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
 * Object to indicate the phone radio capability.
 *
 * @hide
 */
public class RadioCapability {

    /*
     * The RC_PHASE constants are the set of valid values for the mPhase field.
     */

    /**
     *  LM is configured is initial value and value after FINISH completes.
     */
    public static final int RC_PHASE_CONFIGURED = 0;

    /**
     * START is sent before Apply and indicates that an APPLY will be
     * forthcoming with these same parameters.
     */
    public static final int RC_PHASE_START = 1;

    /**
     * APPLY is sent after all LM's receive START and returned
     * RIL_RadioCapability. status = 0, if any START's fail no APPLY will
     * be sent.
     */
    public static final int RC_PHASE_APPLY = 2;

    /**
     *  UNSOL_RSP is sent with RIL_UNSOL_RADIO_CAPABILITY.
     */
    public static final int RC_PHASE_UNSOL_RSP = 3;

    /**
     * RC_PHASE_FINISH is sent after all previous phases have completed.
     * If an error occurs in any previous commands the RIL_RadioAccessesFamily
     * and LogicalModemId fields will be the prior configuration thus
     * restoring the configuration to the previous value. An error returned
     * by this command will generally be ignored or may cause that logical
     * modem to be removed from service
     */
    public static final int RC_PHASE_FINISH = 4;

    /*
     * The RC_STATUS_xxx constants are returned in the mStatus field.
     */

     /**
      *  this parameter is no meaning with RC_Phase_START, RC_Phase_APPLY
      */
    public static final int RC_STATUS_NONE = 0;

    /**
     * Tell modem  the action transaction of set radio capability is
     * success with RC_Phase_FINISH.
     */
    public static final int RC_STATUS_SUCCESS = 1;

    /**
     * tell modem the action transaction of set radio capability is fail
     * with RC_Phase_FINISH
     */
    public static final int RC_STATUS_FAIL = 2;

    /** Version of structure, RIL_RadioCapability_Version */
    private static final int RADIO_CAPABILITY_VERSION = 1;

    /** Unique session value defined by framework returned in all "responses/unsol" */
    private int mSession;

    /** CONFIGURED, START, APPLY, FINISH */
    private int mPhase;

    /**
     * RadioAccessFamily is a bit field of radio access technologies the
     * for the modem is currently supporting. The initial value returned
     * my the modem must the the set of bits that the modem currently supports.
     * see RadioAccessFamily#RADIO_TECHNOLOGY_XXXX
     */
    private int mRadioAccessFamily;

    /**
     * Logical modem this radio is be connected to.
     * This must be Globally unique on convention is
     * to use a registered name such as com.google.android.lm0
     */
    private String mLogicalModemUuid;

    /** Return status and an input parameter for RC_Phase_FINISH */
    private int mStatus;

    /** Phone ID of phone */
    private int mPhoneId;

    /**
     * Constructor.
     *
     * @param phoneId the phone ID
     * @param session the request transaction id
     * @param phase the request phase id
     * @param radioAccessFamily the phone radio access family defined in
     *        RadioAccessFamily. It's a bit mask value to represent
     *        the support type.
     * @param logicalModemUuid the logicalModem UUID which phone connected to
     * @param status tell modem the action transaction of
     *        set radio capability is success or fail with RC_Phase_FINISH
     */
    public RadioCapability(int phoneId, int session, int phase,
            int radioAccessFamily, String logicalModemUuid, int status) {
        mPhoneId = phoneId;
        mSession = session;
        mPhase = phase;
        mRadioAccessFamily = radioAccessFamily;
        mLogicalModemUuid = logicalModemUuid;
        mStatus = status;
    }

    /**
     * Get phone ID.
     *
     * @return phone ID
     */
    public int getPhoneId() {
        return mPhoneId;
    }

    /**
     * Get radio capability version.
     *
     * @return radio capability version
     */
    public int getVersion() {
        return RADIO_CAPABILITY_VERSION;
    }

    /**
     * Get unique session id.
     *
     * @return unique session id
     */
    public int getSession() {
        return mSession;
    }


    /**
     * get radio capability phase.
     *
     * @return RadioCapabilityPhase, including CONFIGURED, START, APPLY, FINISH
     */
    public int getPhase() {
        return mPhase;
    }

    /**
     * get radio access family.
     *
     * @return radio access family
     */
    public int getRadioAccessFamily() {
        return mRadioAccessFamily;
    }

    /**
     * get logical modem Universally Unique ID.
     *
     * @return logical modem uuid
     */
    public String getLogicalModemUuid() {
        return mLogicalModemUuid;
    }

    /**
     * get request status.
     *
     * @return status and an input parameter for RC_PHASE_FINISH
     */
    public int getStatus() {
        return mStatus;
    }

    @Override
    public String toString() {
        return "{mPhoneId = " + mPhoneId
                + " mVersion=" + getVersion()
                + " mSession=" + getSession()
                + " mPhase=" + getPhase()
                + " mRadioAccessFamily=" + getRadioAccessFamily()
                + " mLogicModemId=" + getLogicalModemUuid()
                + " mStatus=" + getStatus()
                + "}";
    }
}

