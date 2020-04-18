/*
 * Copyright (C) 2006 The Android Open Source Project
 * Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 * Not a Contribution.
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

import android.app.AppOpsManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.telephony.PhoneNumberUtils;
import android.telephony.SubscriptionManager;
import android.telephony.Rlog;

import com.android.internal.telephony.uicc.IsimRecords;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.internal.telephony.uicc.UiccCardApplication;

import static android.Manifest.permission.CALL_PRIVILEGED;
import static android.Manifest.permission.READ_PHONE_STATE;
import static android.Manifest.permission.READ_PRIVILEGED_PHONE_STATE;
import static android.Manifest.permission.READ_SMS;
import static android.telephony.TelephonyManager.CARRIER_PRIVILEGE_STATUS_HAS_ACCESS;

public class PhoneSubInfoController extends IPhoneSubInfo.Stub {
    private static final String TAG = "PhoneSubInfoController";
    private static final boolean DBG = true;
    private static final boolean VDBG = false; // STOPSHIP if true

    private final Phone[] mPhone;
    private final Context mContext;
    private final AppOpsManager mAppOps;

    public PhoneSubInfoController(Context context, Phone[] phone) {
        mPhone = phone;
        if (ServiceManager.getService("iphonesubinfo") == null) {
            ServiceManager.addService("iphonesubinfo", this);
        }
        mContext = context;
        mAppOps = (AppOpsManager) mContext.getSystemService(Context.APP_OPS_SERVICE);
    }

    public String getDeviceId(String callingPackage) {
        return getDeviceIdForPhone(SubscriptionManager.getPhoneId(getDefaultSubscription()),
                callingPackage);
    }

    public String getDeviceIdForPhone(int phoneId, String callingPackage) {
        if (!checkReadPhoneState(callingPackage, "getDeviceId")) {
            return null;
        }
        if (!SubscriptionManager.isValidPhoneId(phoneId)) {
            phoneId = 0;
        }
        final Phone phone = mPhone[phoneId];
        if (phone != null) {
            return phone.getDeviceId();
        } else {
            loge("getDeviceIdForPhone phone " + phoneId + " is null");
            return null;
        }
    }

    public String getNaiForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getNai")) {
                return null;
            }
            return phone.getNai();
        } else {
            loge("getNai phone is null for Subscription:" + subId);
            return null;
        }
    }

    public String getImeiForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getImei")) {
                return null;
            }
            return phone.getImei();
        } else {
            loge("getDeviceId phone is null for Subscription:" + subId);
            return null;
        }
    }

    public String getDeviceSvn(String callingPackage) {
        return getDeviceSvnUsingSubId(getDefaultSubscription(), callingPackage);
    }

    public String getDeviceSvnUsingSubId(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getDeviceSvn")) {
                return null;
            }
            return phone.getDeviceSvn();
        } else {
            loge("getDeviceSvn phone is null");
            return null;
        }
    }

    public String getSubscriberId(String callingPackage) {
        return getSubscriberIdForSubscriber(getDefaultSubscription(), callingPackage);
    }

    public String getSubscriberIdForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getSubscriberId")) {
                return null;
            }
            return phone.getSubscriberId();
        } else {
            loge("getSubscriberId phone is null for Subscription:" + subId);
            return null;
        }
    }

    /**
     * Retrieves the serial number of the ICC, if applicable.
     */
    public String getIccSerialNumber(String callingPackage) {
        return getIccSerialNumberForSubscriber(getDefaultSubscription(), callingPackage);
    }

    public String getIccSerialNumberForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getIccSerialNumber")) {
                return null;
            }
            return phone.getIccSerialNumber();
        } else {
            loge("getIccSerialNumber phone is null for Subscription:" + subId);
            return null;
        }
    }

    public String getLine1Number(String callingPackage) {
        return getLine1NumberForSubscriber(getDefaultSubscription(), callingPackage);
    }

    public String getLine1NumberForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            // This is open to apps with WRITE_SMS.
            if (!checkReadPhoneNumber(callingPackage, "getLine1Number")) {
                return null;
            }
            return phone.getLine1Number();
        } else {
            loge("getLine1Number phone is null for Subscription:" + subId);
            return null;
        }
    }

    public String getLine1AlphaTag(String callingPackage) {
        return getLine1AlphaTagForSubscriber(getDefaultSubscription(), callingPackage);
    }

    public String getLine1AlphaTagForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getLine1AlphaTag")) {
                return null;
            }
            return phone.getLine1AlphaTag();
        } else {
            loge("getLine1AlphaTag phone is null for Subscription:" + subId);
            return null;
        }
    }

    public String getMsisdn(String callingPackage) {
        return getMsisdnForSubscriber(getDefaultSubscription(), callingPackage);
    }

    public String getMsisdnForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getMsisdn")) {
                return null;
            }
            return phone.getMsisdn();
        } else {
            loge("getMsisdn phone is null for Subscription:" + subId);
            return null;
        }
    }

    public String getVoiceMailNumber(String callingPackage) {
        return getVoiceMailNumberForSubscriber(getDefaultSubscription(), callingPackage);
    }

    public String getVoiceMailNumberForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getVoiceMailNumber")) {
                return null;
            }
            String number = PhoneNumberUtils.extractNetworkPortion(phone.getVoiceMailNumber());
            if (VDBG) log("VM: getVoiceMailNUmber: " + number);
            return number;
        } else {
            loge("getVoiceMailNumber phone is null for Subscription:" + subId);
            return null;
        }
    }

    // TODO: change getCompleteVoiceMailNumber() to require READ_PRIVILEGED_PHONE_STATE
    public String getCompleteVoiceMailNumber() {
        return getCompleteVoiceMailNumberForSubscriber(getDefaultSubscription());
    }

    public String getCompleteVoiceMailNumberForSubscriber(int subId) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            mContext.enforceCallingOrSelfPermission(CALL_PRIVILEGED, "Requires CALL_PRIVILEGED");
            String number = phone.getVoiceMailNumber();
            if (VDBG) log("VM: getCompleteVoiceMailNUmber: " + number);
            return number;
        } else {
            loge("getCompleteVoiceMailNumber phone is null for Subscription:" + subId);
            return null;
        }
    }

    public String getVoiceMailAlphaTag(String callingPackage) {
        return getVoiceMailAlphaTagForSubscriber(getDefaultSubscription(), callingPackage);
    }

    public String getVoiceMailAlphaTagForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getVoiceMailAlphaTag")) {
                return null;
            }
            return phone.getVoiceMailAlphaTag();
        } else {
            loge("getVoiceMailAlphaTag phone is null for Subscription:" + subId);
            return null;
        }
    }

    /**
     * get Phone object based on subId.
     **/
    private Phone getPhone(int subId) {
        int phoneId = SubscriptionManager.getPhoneId(subId);
        if (!SubscriptionManager.isValidPhoneId(phoneId)) {
            phoneId = 0;
        }
        return mPhone[phoneId];
    }

    /**
     * Make sure caller has either read privileged phone permission or carrier privilege.
     *
     * @throws SecurityException if the caller does not have the required permission/privilege
     */
    private void enforcePrivilegedPermissionOrCarrierPrivilege(Phone phone) {
        int permissionResult = mContext.checkCallingOrSelfPermission(
                READ_PRIVILEGED_PHONE_STATE);
        if (permissionResult == PackageManager.PERMISSION_GRANTED) {
            return;
        }
        log("No read privileged phone permission, check carrier privilege next.");
        UiccCard uiccCard = phone.getUiccCard();
        if (uiccCard == null) {
            throw new SecurityException("No Carrier Privilege: No UICC");
        }
        if (uiccCard.getCarrierPrivilegeStatusForCurrentTransaction(
                mContext.getPackageManager()) != CARRIER_PRIVILEGE_STATUS_HAS_ACCESS) {
            throw new SecurityException("No Carrier Privilege.");
        }
    }

    private int getDefaultSubscription() {
        return  PhoneFactory.getDefaultSubscription();
    }


    public String getIsimImpi() {
        Phone phone = getPhone(getDefaultSubscription());
        mContext.enforceCallingOrSelfPermission(READ_PRIVILEGED_PHONE_STATE,
                "Requires READ_PRIVILEGED_PHONE_STATE");
        IsimRecords isim = phone.getIsimRecords();
        if (isim != null) {
            return isim.getIsimImpi();
        } else {
            return null;
        }
    }

    public String getIsimDomain() {
        Phone phone = getPhone(getDefaultSubscription());
        mContext.enforceCallingOrSelfPermission(READ_PRIVILEGED_PHONE_STATE,
                "Requires READ_PRIVILEGED_PHONE_STATE");
        IsimRecords isim = phone.getIsimRecords();
        if (isim != null) {
            return isim.getIsimDomain();
        } else {
            return null;
        }
    }

    public String[] getIsimImpu() {
        Phone phone = getPhone(getDefaultSubscription());
        mContext.enforceCallingOrSelfPermission(READ_PRIVILEGED_PHONE_STATE,
                "Requires READ_PRIVILEGED_PHONE_STATE");
        IsimRecords isim = phone.getIsimRecords();
        if (isim != null) {
            return isim.getIsimImpu();
        } else {
            return null;
        }
    }

    public String getIsimIst() throws RemoteException {
        Phone phone = getPhone(getDefaultSubscription());
        mContext.enforceCallingOrSelfPermission(READ_PRIVILEGED_PHONE_STATE,
                "Requires READ_PRIVILEGED_PHONE_STATE");
        IsimRecords isim = phone.getIsimRecords();
        if (isim != null) {
            return isim.getIsimIst();
        } else {
            return null;
        }
    }

    public String[] getIsimPcscf() throws RemoteException {
        Phone phone = getPhone(getDefaultSubscription());
        mContext.enforceCallingOrSelfPermission(READ_PRIVILEGED_PHONE_STATE,
                "Requires READ_PRIVILEGED_PHONE_STATE");
        IsimRecords isim = phone.getIsimRecords();
        if (isim != null) {
            return isim.getIsimPcscf();
        } else {
            return null;
        }
    }

    public String getIsimChallengeResponse(String nonce) throws RemoteException {
        Phone phone = getPhone(getDefaultSubscription());
        mContext.enforceCallingOrSelfPermission(READ_PRIVILEGED_PHONE_STATE,
                "Requires READ_PRIVILEGED_PHONE_STATE");
        IsimRecords isim = phone.getIsimRecords();
        if (isim != null) {
            return isim.getIsimChallengeResponse(nonce);
        } else {
            return null;
        }
    }

    public String getIccSimChallengeResponse(int subId, int appType, int authType, String data)
            throws RemoteException {
        Phone phone = getPhone(subId);
        enforcePrivilegedPermissionOrCarrierPrivilege(phone);
        UiccCard uiccCard = phone.getUiccCard();
        if (uiccCard == null) {
            loge("getIccSimChallengeResponse() UiccCard is null");
            return null;
        }

        UiccCardApplication uiccApp = uiccCard.getApplicationByType(appType);
        if (uiccApp == null) {
            loge("getIccSimChallengeResponse() no app with specified type -- " +
                    appType);
            return null;
        } else {
            loge("getIccSimChallengeResponse() found app " + uiccApp.getAid()
                    + " specified type -- " + appType);
        }

        if(authType != UiccCardApplication.AUTH_CONTEXT_EAP_SIM &&
                authType != UiccCardApplication.AUTH_CONTEXT_EAP_AKA) {
            loge("getIccSimChallengeResponse() unsupported authType: " + authType);
            return null;
        }

        return uiccApp.getIccRecords().getIccSimChallengeResponse(authType, data);
    }

    public String getGroupIdLevel1(String callingPackage) {
        return getGroupIdLevel1ForSubscriber(getDefaultSubscription(), callingPackage);
    }

    public String getGroupIdLevel1ForSubscriber(int subId, String callingPackage) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            if (!checkReadPhoneState(callingPackage, "getGroupIdLevel1")) {
                return null;
            }
            return phone.getGroupIdLevel1();
        } else {
            loge("getGroupIdLevel1 phone is null for Subscription:" + subId);
            return null;
        }
    }

    private boolean checkReadPhoneState(String callingPackage, String message) {
        try {
            mContext.enforceCallingOrSelfPermission(READ_PRIVILEGED_PHONE_STATE, message);

            // SKIP checking run-time OP_READ_PHONE_STATE since self or using PRIVILEGED
            return true;
        } catch (SecurityException e) {
            mContext.enforceCallingOrSelfPermission(READ_PHONE_STATE, message);
        }

        return mAppOps.noteOp(AppOpsManager.OP_READ_PHONE_STATE, Binder.getCallingUid(),
                callingPackage) == AppOpsManager.MODE_ALLOWED;
    }

    /**
     * Besides READ_PHONE_STATE, WRITE_SMS and READ_SMS also allow apps to get phone numbers.
     */
    private boolean checkReadPhoneNumber(String callingPackage, String message) {
        // Default SMS app can always read it.
        if (mAppOps.noteOp(AppOpsManager.OP_WRITE_SMS,
                Binder.getCallingUid(), callingPackage) == AppOpsManager.MODE_ALLOWED) {
            return true;
        }
        try {
            return checkReadPhoneState(callingPackage, message);
        } catch (SecurityException readPhoneStateSecurityException) {
            try {
                // Can be read with READ_SMS too.
                mContext.enforceCallingOrSelfPermission(READ_SMS, message);
                return mAppOps.noteOp(AppOpsManager.OP_READ_SMS,
                        Binder.getCallingUid(), callingPackage) == AppOpsManager.MODE_ALLOWED;
            } catch (SecurityException readSmsSecurityException) {
                // Throw exception with message including both READ_PHONE_STATE and READ_SMS
                // permissions
                throw new SecurityException(message + ": Neither user " + Binder.getCallingUid() +
                        " nor current process has " + READ_PHONE_STATE + " or " + READ_SMS + ".");
            }
        }
    }

    private void log(String s) {
        Rlog.d(TAG, s);
    }

    private void loge(String s) {
        Rlog.e(TAG, s);
    }
}
