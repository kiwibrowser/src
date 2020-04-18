/*
 * Copyright (C) 2007 The Android Open Source Project
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

import com.android.internal.telephony.RILConstants;

import android.telephony.Rlog;

/**
 * {@hide}
 */
public class CommandException extends RuntimeException {
    private Error mError;

    public enum Error {
        INVALID_RESPONSE,
        RADIO_NOT_AVAILABLE,
        GENERIC_FAILURE,
        PASSWORD_INCORRECT,
        SIM_PIN2,
        SIM_PUK2,
        REQUEST_NOT_SUPPORTED,
        OP_NOT_ALLOWED_DURING_VOICE_CALL,
        OP_NOT_ALLOWED_BEFORE_REG_NW,
        SMS_FAIL_RETRY,
        SIM_ABSENT,
        SUBSCRIPTION_NOT_AVAILABLE,
        MODE_NOT_SUPPORTED,
        FDN_CHECK_FAILURE,
        ILLEGAL_SIM_OR_ME,
        MISSING_RESOURCE,
        NO_SUCH_ELEMENT,
        SUBSCRIPTION_NOT_SUPPORTED,
        DIAL_MODIFIED_TO_USSD,
        DIAL_MODIFIED_TO_SS,
        DIAL_MODIFIED_TO_DIAL,
        USSD_MODIFIED_TO_DIAL,
        USSD_MODIFIED_TO_SS,
        USSD_MODIFIED_TO_USSD,
        SS_MODIFIED_TO_DIAL,
        SS_MODIFIED_TO_USSD,
        SS_MODIFIED_TO_SS,
        SIM_ALREADY_POWERED_OFF,
        SIM_ALREADY_POWERED_ON,
        SIM_DATA_NOT_AVAILABLE,
        SIM_SAP_CONNECT_FAILURE,
        SIM_SAP_MSG_SIZE_TOO_LARGE,
        SIM_SAP_MSG_SIZE_TOO_SMALL,
        SIM_SAP_CONNECT_OK_CALL_ONGOING,
        LCE_NOT_SUPPORTED,
        NO_MEMORY,
        INTERNAL_ERR,
        SYSTEM_ERR,
        MODEM_ERR,
        INVALID_STATE,
        NO_RESOURCES,
        SIM_ERR,
        INVALID_ARGUMENTS,
        INVALID_SIM_STATE,
        INVALID_MODEM_STATE,
        INVALID_CALL_ID,
        NO_SMS_TO_ACK,
        NETWORK_ERR,
        REQUEST_RATE_LIMITED,
        SIM_BUSY,
        SIM_FULL,
        NETWORK_REJECT,
        OPERATION_NOT_ALLOWED,
        EMPTY_RECORD,
        INVALID_SMS_FORMAT,
        ENCODING_ERR,
        INVALID_SMSC_ADDRESS,
        NO_SUCH_ENTRY,
        NETWORK_NOT_READY,
        NOT_PROVISIONED,
        NO_SUBSCRIPTION,
        NO_NETWORK_FOUND,
        DEVICE_IN_USE,
        ABORTED,
        OEM_ERROR_1,
        OEM_ERROR_2,
        OEM_ERROR_3,
        OEM_ERROR_4,
        OEM_ERROR_5,
        OEM_ERROR_6,
        OEM_ERROR_7,
        OEM_ERROR_8,
        OEM_ERROR_9,
        OEM_ERROR_10,
        OEM_ERROR_11,
        OEM_ERROR_12,
        OEM_ERROR_13,
        OEM_ERROR_14,
        OEM_ERROR_15,
        OEM_ERROR_16,
        OEM_ERROR_17,
        OEM_ERROR_18,
        OEM_ERROR_19,
        OEM_ERROR_20,
        OEM_ERROR_21,
        OEM_ERROR_22,
        OEM_ERROR_23,
        OEM_ERROR_24,
        OEM_ERROR_25,
    }

    public CommandException(Error e) {
        super(e.toString());
        mError = e;
    }

    public CommandException(Error e, String errString) {
        super(errString);
        mError = e;
    }

    public static CommandException
    fromRilErrno(int ril_errno) {
        switch(ril_errno) {
            case RILConstants.SUCCESS:                       return null;
            case RILConstants.RIL_ERRNO_INVALID_RESPONSE:
                return new CommandException(Error.INVALID_RESPONSE);
            case RILConstants.RADIO_NOT_AVAILABLE:
                return new CommandException(Error.RADIO_NOT_AVAILABLE);
            case RILConstants.GENERIC_FAILURE:
                return new CommandException(Error.GENERIC_FAILURE);
            case RILConstants.PASSWORD_INCORRECT:
                return new CommandException(Error.PASSWORD_INCORRECT);
            case RILConstants.SIM_PIN2:
                return new CommandException(Error.SIM_PIN2);
            case RILConstants.SIM_PUK2:
                return new CommandException(Error.SIM_PUK2);
            case RILConstants.REQUEST_NOT_SUPPORTED:
                return new CommandException(Error.REQUEST_NOT_SUPPORTED);
            case RILConstants.OP_NOT_ALLOWED_DURING_VOICE_CALL:
                return new CommandException(Error.OP_NOT_ALLOWED_DURING_VOICE_CALL);
            case RILConstants.OP_NOT_ALLOWED_BEFORE_REG_NW:
                return new CommandException(Error.OP_NOT_ALLOWED_BEFORE_REG_NW);
            case RILConstants.SMS_SEND_FAIL_RETRY:
                return new CommandException(Error.SMS_FAIL_RETRY);
            case RILConstants.SIM_ABSENT:
                return new CommandException(Error.SIM_ABSENT);
            case RILConstants.SUBSCRIPTION_NOT_AVAILABLE:
                return new CommandException(Error.SUBSCRIPTION_NOT_AVAILABLE);
            case RILConstants.MODE_NOT_SUPPORTED:
                return new CommandException(Error.MODE_NOT_SUPPORTED);
            case RILConstants.FDN_CHECK_FAILURE:
                return new CommandException(Error.FDN_CHECK_FAILURE);
            case RILConstants.ILLEGAL_SIM_OR_ME:
                return new CommandException(Error.ILLEGAL_SIM_OR_ME);
            case RILConstants.MISSING_RESOURCE:
                return new CommandException(Error.MISSING_RESOURCE);
            case RILConstants.NO_SUCH_ELEMENT:
                return new CommandException(Error.NO_SUCH_ELEMENT);
            case RILConstants.SUBSCRIPTION_NOT_SUPPORTED:
                return new CommandException(Error.SUBSCRIPTION_NOT_SUPPORTED);
            case RILConstants.DIAL_MODIFIED_TO_USSD:
                return new CommandException(Error.DIAL_MODIFIED_TO_USSD);
            case RILConstants.DIAL_MODIFIED_TO_SS:
                return new CommandException(Error.DIAL_MODIFIED_TO_SS);
            case RILConstants.DIAL_MODIFIED_TO_DIAL:
                return new CommandException(Error.DIAL_MODIFIED_TO_DIAL);
            case RILConstants.USSD_MODIFIED_TO_DIAL:
                return new CommandException(Error.USSD_MODIFIED_TO_DIAL);
            case RILConstants.USSD_MODIFIED_TO_SS:
                return new CommandException(Error.USSD_MODIFIED_TO_SS);
            case RILConstants.USSD_MODIFIED_TO_USSD:
                return new CommandException(Error.USSD_MODIFIED_TO_USSD);
            case RILConstants.SS_MODIFIED_TO_DIAL:
                return new CommandException(Error.SS_MODIFIED_TO_DIAL);
            case RILConstants.SS_MODIFIED_TO_USSD:
                return new CommandException(Error.SS_MODIFIED_TO_USSD);
            case RILConstants.SS_MODIFIED_TO_SS:
                return new CommandException(Error.SS_MODIFIED_TO_SS);
            case RILConstants.SIM_ALREADY_POWERED_OFF:
                return new CommandException(Error.SIM_ALREADY_POWERED_OFF);
            case RILConstants.SIM_ALREADY_POWERED_ON:
                return new CommandException(Error.SIM_ALREADY_POWERED_ON);
            case RILConstants.SIM_DATA_NOT_AVAILABLE:
                return new CommandException(Error.SIM_DATA_NOT_AVAILABLE);
            case RILConstants.SIM_SAP_CONNECT_FAILURE:
                return new CommandException(Error.SIM_SAP_CONNECT_FAILURE);
            case RILConstants.SIM_SAP_MSG_SIZE_TOO_LARGE:
                return new CommandException(Error.SIM_SAP_MSG_SIZE_TOO_LARGE);
            case RILConstants.SIM_SAP_MSG_SIZE_TOO_SMALL:
                return new CommandException(Error.SIM_SAP_MSG_SIZE_TOO_SMALL);
            case RILConstants.SIM_SAP_CONNECT_OK_CALL_ONGOING:
                return new CommandException(Error.SIM_SAP_CONNECT_OK_CALL_ONGOING);
            case RILConstants.LCE_NOT_SUPPORTED:
                return new CommandException(Error.LCE_NOT_SUPPORTED);
            case RILConstants.NO_MEMORY:
                return new CommandException(Error.NO_MEMORY);
            case RILConstants.INTERNAL_ERR:
                return new CommandException(Error.INTERNAL_ERR);
            case RILConstants.SYSTEM_ERR:
                return new CommandException(Error.SYSTEM_ERR);
            case RILConstants.MODEM_ERR:
                return new CommandException(Error.MODEM_ERR);
            case RILConstants.INVALID_STATE:
                return new CommandException(Error.INVALID_STATE);
            case RILConstants.NO_RESOURCES:
                return new CommandException(Error.NO_RESOURCES);
            case RILConstants.SIM_ERR:
                return new CommandException(Error.SIM_ERR);
            case RILConstants.INVALID_ARGUMENTS:
                return new CommandException(Error.INVALID_ARGUMENTS);
            case RILConstants.INVALID_SIM_STATE:
                return new CommandException(Error.INVALID_SIM_STATE);
            case RILConstants.INVALID_MODEM_STATE:
                return new CommandException(Error.INVALID_MODEM_STATE);
            case RILConstants.INVALID_CALL_ID:
                return new CommandException(Error.INVALID_CALL_ID);
            case RILConstants.NO_SMS_TO_ACK:
                return new CommandException(Error.NO_SMS_TO_ACK);
            case RILConstants.NETWORK_ERR:
                return new CommandException(Error.NETWORK_ERR);
            case RILConstants.REQUEST_RATE_LIMITED:
                return new CommandException(Error.REQUEST_RATE_LIMITED);
            case RILConstants.SIM_BUSY:
                return new CommandException(Error.SIM_BUSY);
            case RILConstants.SIM_FULL:
                return new CommandException(Error.SIM_FULL);
            case RILConstants.NETWORK_REJECT:
                return new CommandException(Error.NETWORK_REJECT);
            case RILConstants.OPERATION_NOT_ALLOWED:
                return new CommandException(Error.OPERATION_NOT_ALLOWED);
            case RILConstants.EMPTY_RECORD:
                return new CommandException(Error.EMPTY_RECORD);
            case RILConstants.INVALID_SMS_FORMAT:
                return new CommandException(Error.INVALID_SMS_FORMAT);
            case RILConstants.ENCODING_ERR:
                return new CommandException(Error.ENCODING_ERR);
            case RILConstants.INVALID_SMSC_ADDRESS:
                return new CommandException(Error.INVALID_SMSC_ADDRESS);
            case RILConstants.NO_SUCH_ENTRY:
                return new CommandException(Error.NO_SUCH_ENTRY);
            case RILConstants.NETWORK_NOT_READY:
                return new CommandException(Error.NETWORK_NOT_READY);
            case RILConstants.NOT_PROVISIONED:
                return new CommandException(Error.NOT_PROVISIONED);
            case RILConstants.NO_SUBSCRIPTION:
                return new CommandException(Error.NO_SUBSCRIPTION);
            case RILConstants.NO_NETWORK_FOUND:
                return new CommandException(Error.NO_NETWORK_FOUND);
            case RILConstants.DEVICE_IN_USE:
                return new CommandException(Error.DEVICE_IN_USE);
            case RILConstants.ABORTED:
                return new CommandException(Error.ABORTED);
            case RILConstants.OEM_ERROR_1:
                return new CommandException(Error.OEM_ERROR_1);
            case RILConstants.OEM_ERROR_2:
                return new CommandException(Error.OEM_ERROR_2);
            case RILConstants.OEM_ERROR_3:
                return new CommandException(Error.OEM_ERROR_3);
            case RILConstants.OEM_ERROR_4:
                return new CommandException(Error.OEM_ERROR_4);
            case RILConstants.OEM_ERROR_5:
                return new CommandException(Error.OEM_ERROR_5);
            case RILConstants.OEM_ERROR_6:
                return new CommandException(Error.OEM_ERROR_6);
            case RILConstants.OEM_ERROR_7:
                return new CommandException(Error.OEM_ERROR_7);
            case RILConstants.OEM_ERROR_8:
                return new CommandException(Error.OEM_ERROR_8);
            case RILConstants.OEM_ERROR_9:
                return new CommandException(Error.OEM_ERROR_9);
            case RILConstants.OEM_ERROR_10:
                return new CommandException(Error.OEM_ERROR_10);
            case RILConstants.OEM_ERROR_11:
                return new CommandException(Error.OEM_ERROR_11);
            case RILConstants.OEM_ERROR_12:
                return new CommandException(Error.OEM_ERROR_12);
            case RILConstants.OEM_ERROR_13:
                return new CommandException(Error.OEM_ERROR_13);
            case RILConstants.OEM_ERROR_14:
                return new CommandException(Error.OEM_ERROR_14);
            case RILConstants.OEM_ERROR_15:
                return new CommandException(Error.OEM_ERROR_15);
            case RILConstants.OEM_ERROR_16:
                return new CommandException(Error.OEM_ERROR_16);
            case RILConstants.OEM_ERROR_17:
                return new CommandException(Error.OEM_ERROR_17);
            case RILConstants.OEM_ERROR_18:
                return new CommandException(Error.OEM_ERROR_18);
            case RILConstants.OEM_ERROR_19:
                return new CommandException(Error.OEM_ERROR_19);
            case RILConstants.OEM_ERROR_20:
                return new CommandException(Error.OEM_ERROR_20);
            case RILConstants.OEM_ERROR_21:
                return new CommandException(Error.OEM_ERROR_21);
            case RILConstants.OEM_ERROR_22:
                return new CommandException(Error.OEM_ERROR_22);
            case RILConstants.OEM_ERROR_23:
                return new CommandException(Error.OEM_ERROR_23);
            case RILConstants.OEM_ERROR_24:
                return new CommandException(Error.OEM_ERROR_24);
            case RILConstants.OEM_ERROR_25:
                return new CommandException(Error.OEM_ERROR_25);

            default:
                Rlog.e("GSM", "Unrecognized RIL errno " + ril_errno);
                return new CommandException(Error.INVALID_RESPONSE);
        }
    }

    public Error getCommandError() {
        return mError;
    }



}
