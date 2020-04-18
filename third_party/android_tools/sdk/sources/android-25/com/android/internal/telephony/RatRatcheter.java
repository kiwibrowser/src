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
package com.android.internal.telephony;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.PersistableBundle;
import android.os.UserHandle;
import android.telephony.CarrierConfigManager;
import android.telephony.ServiceState;
import android.telephony.Rlog;
import android.util.SparseArray;
import android.util.SparseIntArray;

import java.util.ArrayList;

/**
 * This class loads configuration from CarrierConfig and uses it to determine
 * what RATs are within a ratcheting family.  For example all the HSPA/HSDPA/HSUPA RATs.
 * Then, until reset the class will only ratchet upwards within the family (order
 * determined by the CarrierConfig data).  The ServiceStateTracker will reset this
 * on cell-change.
 */
public class RatRatcheter {
    private final static String LOG_TAG = "RilRatcheter";

    /**
     * This is a map of RAT types -> RAT families for rapid lookup.
     * The RAT families are defined by RAT type -> RAT Rank SparseIntArrays, so
     * we can compare the priorities of two RAT types by comparing the values
     * stored in the SparseIntArrays, higher values are higher priority.
     */
    private final SparseArray<SparseIntArray> mRatFamilyMap = new SparseArray<>();

    private final Phone mPhone;

    /** Constructor */
    public RatRatcheter(Phone phone) {
        mPhone = phone;

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        phone.getContext().registerReceiverAsUser(mConfigChangedReceiver, UserHandle.ALL,
                intentFilter, null, null);
        resetRatFamilyMap();
    }

    public int ratchetRat(int oldRat, int newRat) {
        synchronized (mRatFamilyMap) {
            final SparseIntArray oldFamily = mRatFamilyMap.get(oldRat);
            if (oldFamily == null) return newRat;

            final SparseIntArray newFamily = mRatFamilyMap.get(newRat);
            if (newFamily != oldFamily) return newRat;

            // now go with the higher of the two
            final int oldRatRank = newFamily.get(oldRat, -1);
            final int newRatRank = newFamily.get(newRat, -1);
            return (oldRatRank > newRatRank ? oldRat : newRat);
        }
    }

    public void ratchetRat(ServiceState oldSS, ServiceState newSS) {
        int newVoiceRat = ratchetRat(oldSS.getRilVoiceRadioTechnology(),
                newSS.getRilVoiceRadioTechnology());
        int newDataRat = ratchetRat(oldSS.getRilDataRadioTechnology(),
                newSS.getRilDataRadioTechnology());
        boolean newUsingCA = oldSS.isUsingCarrierAggregation() ||
                newSS.isUsingCarrierAggregation();

        newSS.setRilVoiceRadioTechnology(newVoiceRat);
        newSS.setRilDataRadioTechnology(newDataRat);
        newSS.setIsUsingCarrierAggregation(newUsingCA);
    }

    private BroadcastReceiver mConfigChangedReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED.equals(action)) {
                resetRatFamilyMap();
            }
        }
    };

    private void resetRatFamilyMap() {
        synchronized(mRatFamilyMap) {
            mRatFamilyMap.clear();

            final CarrierConfigManager configManager = (CarrierConfigManager)
                    mPhone.getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
            if (configManager == null) return;
            PersistableBundle b = configManager.getConfig();
            if (b == null) return;

            // Reads an array of strings, eg:
            // ["GPRS, EDGE", "EVDO, EVDO_A, EVDO_B", "HSPA, HSDPA, HSUPA, HSPAP"]
            // Each string defines a family and the order of rats within the string express
            // the priority of the RAT within the family (ie, we'd move up to later-listed RATs, but
            // not down).
            String[] ratFamilies = b.getStringArray(CarrierConfigManager.KEY_RATCHET_RAT_FAMILIES);
            if (ratFamilies == null) return;
            for (String ratFamily : ratFamilies) {
                String[] rats = ratFamily.split(",");
                if (rats.length < 2) continue;
                SparseIntArray currentFamily = new SparseIntArray(rats.length);
                int pos = 0;
                for (String ratString : rats) {
                    int ratInt;
                    try {
                        ratInt = Integer.parseInt(ratString.trim());
                    } catch (NumberFormatException e) {
                        Rlog.e(LOG_TAG, "NumberFormatException on " + ratString);
                        break;
                    }
                    if (mRatFamilyMap.get(ratInt) != null) {
                        Rlog.e(LOG_TAG, "RAT listed twice: " + ratString);
                        break;
                    }
                    currentFamily.put(ratInt, pos++);
                    mRatFamilyMap.put(ratInt, currentFamily);
                }
            }
        }
    }
}
