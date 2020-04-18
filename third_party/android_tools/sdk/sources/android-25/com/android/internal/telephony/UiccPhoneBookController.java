/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.os.ServiceManager;
import android.telephony.Rlog;

import com.android.internal.telephony.IIccPhoneBook;
import com.android.internal.telephony.uicc.AdnRecord;

import java.lang.ArrayIndexOutOfBoundsException;
import java.lang.NullPointerException;
import java.util.List;

public class UiccPhoneBookController extends IIccPhoneBook.Stub {
    private static final String TAG = "UiccPhoneBookController";
    private Phone[] mPhone;

    /* only one UiccPhoneBookController exists */
    public UiccPhoneBookController(Phone[] phone) {
        if (ServiceManager.getService("simphonebook") == null) {
               ServiceManager.addService("simphonebook", this);
        }
        mPhone = phone;
    }

    @Override
    public boolean
    updateAdnRecordsInEfBySearch (int efid, String oldTag, String oldPhoneNumber,
            String newTag, String newPhoneNumber, String pin2) throws android.os.RemoteException {
        return updateAdnRecordsInEfBySearchForSubscriber(getDefaultSubscription(), efid, oldTag,
                oldPhoneNumber, newTag, newPhoneNumber, pin2);
    }

    @Override
    public boolean
    updateAdnRecordsInEfBySearchForSubscriber(int subId, int efid, String oldTag,
            String oldPhoneNumber, String newTag, String newPhoneNumber,
            String pin2) throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                             getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.updateAdnRecordsInEfBySearch(efid, oldTag,
                    oldPhoneNumber, newTag, newPhoneNumber, pin2);
        } else {
            Rlog.e(TAG,"updateAdnRecordsInEfBySearch iccPbkIntMgr is" +
                      " null for Subscription:"+subId);
            return false;
        }
    }

    @Override
    public boolean
    updateAdnRecordsInEfByIndex(int efid, String newTag,
            String newPhoneNumber, int index, String pin2) throws android.os.RemoteException {
        return updateAdnRecordsInEfByIndexForSubscriber(getDefaultSubscription(), efid, newTag,
                newPhoneNumber, index, pin2);
    }

    @Override
    public boolean
    updateAdnRecordsInEfByIndexForSubscriber(int subId, int efid, String newTag,
            String newPhoneNumber, int index, String pin2) throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                             getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.updateAdnRecordsInEfByIndex(efid, newTag,
                    newPhoneNumber, index, pin2);
        } else {
            Rlog.e(TAG,"updateAdnRecordsInEfByIndex iccPbkIntMgr is" +
                      " null for Subscription:"+subId);
            return false;
        }
    }

    @Override
    public int[] getAdnRecordsSize(int efid) throws android.os.RemoteException {
        return getAdnRecordsSizeForSubscriber(getDefaultSubscription(), efid);
    }

    @Override
    public int[]
    getAdnRecordsSizeForSubscriber(int subId, int efid) throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                             getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getAdnRecordsSize(efid);
        } else {
            Rlog.e(TAG,"getAdnRecordsSize iccPbkIntMgr is" +
                      " null for Subscription:"+subId);
            return null;
        }
    }

    @Override
    public List<AdnRecord> getAdnRecordsInEf(int efid) throws android.os.RemoteException {
        return getAdnRecordsInEfForSubscriber(getDefaultSubscription(), efid);
    }

    @Override
    public List<AdnRecord> getAdnRecordsInEfForSubscriber(int subId, int efid)
           throws android.os.RemoteException {
        IccPhoneBookInterfaceManager iccPbkIntMgr =
                             getIccPhoneBookInterfaceManager(subId);
        if (iccPbkIntMgr != null) {
            return iccPbkIntMgr.getAdnRecordsInEf(efid);
        } else {
            Rlog.e(TAG,"getAdnRecordsInEf iccPbkIntMgr is" +
                      "null for Subscription:"+subId);
            return null;
        }
    }

    /**
     * get phone book interface manager object based on subscription.
     **/
    private IccPhoneBookInterfaceManager
            getIccPhoneBookInterfaceManager(int subId) {

        int phoneId = SubscriptionController.getInstance().getPhoneId(subId);
        try {
            return mPhone[phoneId].getIccPhoneBookInterfaceManager();
        } catch (NullPointerException e) {
            Rlog.e(TAG, "Exception is :"+e.toString()+" For subscription :"+subId );
            e.printStackTrace(); //To print stack trace
            return null;
        } catch (ArrayIndexOutOfBoundsException e) {
            Rlog.e(TAG, "Exception is :"+e.toString()+" For subscription :"+subId );
            e.printStackTrace();
            return null;
        }
    }

    private int getDefaultSubscription() {
        return PhoneFactory.getDefaultSubscription();
    }
}
