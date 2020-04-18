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

package com.android.internal.telephony.dataconnection;


import android.os.Handler;
import android.os.RegistrantList;
import android.util.Pair;

/**
 * The class to hold different data enabled/disabled settings. Also it allows clients to register
 * for overall data enabled setting changed event.
 * @hide
 */
public class DataEnabledSettings {

    public static final int REASON_REGISTERED = 0;

    public static final int REASON_INTERNAL_DATA_ENABLED = 1;

    public static final int REASON_USER_DATA_ENABLED = 2;

    public static final int REASON_POLICY_DATA_ENABLED = 3;

    public static final int REASON_DATA_ENABLED_BY_CARRIER = 4;

    /**
     * responds to the setInternalDataEnabled call - used internally to turn off data.
     * For example during emergency calls
     */
    private boolean mInternalDataEnabled = true;

    /**
     * responds to public (user) API to enable/disable data use independent of
     * mInternalDataEnabled and requests for APN access persisted
     */
    private boolean mUserDataEnabled = true;

    /**
     * Flag indicating data allowed by network policy manager or not.
     */
    private boolean mPolicyDataEnabled = true;

    /**
     * Indicate if metered APNs are enabled by the carrier. set false to block all the metered APNs
     * from continuously sending requests, which causes undesired network load.
     */
    private boolean mCarrierDataEnabled = true;

    private final RegistrantList mDataEnabledChangedRegistrants = new RegistrantList();

    public synchronized void setInternalDataEnabled(boolean enabled) {
        boolean prevDataEnabled = isDataEnabled(true);
        mInternalDataEnabled = enabled;
        if (prevDataEnabled != isDataEnabled(true)) {
            notifyDataEnabledChanged(!prevDataEnabled, REASON_INTERNAL_DATA_ENABLED);
        }
    }
    public synchronized boolean isInternalDataEnabled() {
        return mInternalDataEnabled;
    }

    public synchronized void setUserDataEnabled(boolean enabled) {
        boolean prevDataEnabled = isDataEnabled(true);
        mUserDataEnabled = enabled;
        if (prevDataEnabled != isDataEnabled(true)) {
            notifyDataEnabledChanged(!prevDataEnabled, REASON_USER_DATA_ENABLED);
        }
    }
    public synchronized boolean isUserDataEnabled() {
        return mUserDataEnabled;
    }

    public synchronized void setPolicyDataEnabled(boolean enabled) {
        boolean prevDataEnabled = isDataEnabled(true);
        mPolicyDataEnabled = enabled;
        if (prevDataEnabled != isDataEnabled(true)) {
            notifyDataEnabledChanged(!prevDataEnabled, REASON_POLICY_DATA_ENABLED);
        }
    }
    public synchronized boolean isPolicyDataEnabled() {
        return mPolicyDataEnabled;
    }

    public synchronized void setCarrierDataEnabled(boolean enabled) {
        boolean prevDataEnabled = isDataEnabled(true);
        mCarrierDataEnabled = enabled;
        if (prevDataEnabled != isDataEnabled(true)) {
            notifyDataEnabledChanged(!prevDataEnabled, REASON_DATA_ENABLED_BY_CARRIER);
        }
    }
    public synchronized boolean isCarrierDataEnabled() {
        return mCarrierDataEnabled;
    }

    public synchronized boolean isDataEnabled(boolean checkUserDataEnabled) {
        return (mInternalDataEnabled
                && (!checkUserDataEnabled || mUserDataEnabled)
                && (!checkUserDataEnabled || mPolicyDataEnabled)
                && (!checkUserDataEnabled || mCarrierDataEnabled));
    }

    private void notifyDataEnabledChanged(boolean enabled, int reason) {
        mDataEnabledChangedRegistrants.notifyResult(new Pair<>(enabled, reason));
    }

    public void registerForDataEnabledChanged(Handler h, int what, Object obj) {
        mDataEnabledChangedRegistrants.addUnique(h, what, obj);
        notifyDataEnabledChanged(isDataEnabled(true), REASON_REGISTERED);
    }

    public void unregisterForDataEnabledChanged(Handler h) {
        mDataEnabledChangedRegistrants.remove(h);
    }
}
