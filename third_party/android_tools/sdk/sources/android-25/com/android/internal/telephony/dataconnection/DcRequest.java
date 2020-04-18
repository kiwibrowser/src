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
package com.android.internal.telephony.dataconnection;

import static com.android.internal.telephony.DctConstants.APN_CBS_ID;
import static com.android.internal.telephony.DctConstants.APN_DEFAULT_ID;
import static com.android.internal.telephony.DctConstants.APN_DUN_ID;
import static com.android.internal.telephony.DctConstants.APN_EMERGENCY_ID;
import static com.android.internal.telephony.DctConstants.APN_FOTA_ID;
import static com.android.internal.telephony.DctConstants.APN_IA_ID;
import static com.android.internal.telephony.DctConstants.APN_IMS_ID;
import static com.android.internal.telephony.DctConstants.APN_INVALID_ID;
import static com.android.internal.telephony.DctConstants.APN_MMS_ID;
import static com.android.internal.telephony.DctConstants.APN_SUPL_ID;

import android.content.Context;
import android.net.NetworkCapabilities;
import android.net.NetworkConfig;
import android.net.NetworkRequest;
import android.telephony.Rlog;

import java.util.HashMap;

public class DcRequest implements Comparable<DcRequest> {
    private static final String LOG_TAG = "DcRequest";

    public final NetworkRequest networkRequest;
    public final int priority;
    public final int apnId;

    public DcRequest(NetworkRequest nr, Context context) {
        initApnPriorities(context);
        networkRequest = nr;
        apnId = apnIdForNetworkRequest(networkRequest);
        priority = priorityForApnId(apnId);
    }

    public String toString() {
        return networkRequest.toString() + ", priority=" + priority + ", apnId=" + apnId;
    }

    public int hashCode() {
        return networkRequest.hashCode();
    }

    public boolean equals(Object o) {
        if (o instanceof DcRequest) {
            return networkRequest.equals(((DcRequest)o).networkRequest);
        }
        return false;
    }

    public int compareTo(DcRequest o) {
        return o.priority - priority;
    }

    private int apnIdForNetworkRequest(NetworkRequest nr) {
        NetworkCapabilities nc = nr.networkCapabilities;
        // For now, ignore the bandwidth stuff
        if (nc.getTransportTypes().length > 0 &&
                nc.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) == false) {
            return APN_INVALID_ID;
        }

        // in the near term just do 1-1 matches.
        // TODO - actually try to match the set of capabilities
        int apnId = APN_INVALID_ID;

        boolean error = false;
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_DEFAULT_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_MMS)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_MMS_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_SUPL)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_SUPL_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_DUN)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_DUN_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_FOTA)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_FOTA_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_IMS)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_IMS_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_CBS)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_CBS_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_IA)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_IA_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_RCS)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_INVALID_ID;
            loge("RCS APN type not yet supported");
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_XCAP)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_INVALID_ID;
            loge("XCAP APN type not yet supported");
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_EIMS)) {
            if (apnId != APN_INVALID_ID) error = true;
            apnId = APN_EMERGENCY_ID;
        }
        if (error) {
            // TODO: If this error condition is removed, the framework's handling of
            // NET_CAPABILITY_NOT_RESTRICTED will need to be updated so requests for
            // say FOTA and INTERNET are marked as restricted.  This is not how
            // NetworkCapabilities.maybeMarkCapabilitiesRestricted currently works.
            loge("Multiple apn types specified in request - result is unspecified!");
        }
        if (apnId == APN_INVALID_ID) {
            loge("Unsupported NetworkRequest in Telephony: nr=" + nr);
        }
        return apnId;
    }

    private static final HashMap<Integer, Integer> sApnPriorityMap =
            new HashMap<Integer, Integer>();

    private void initApnPriorities(Context context) {
        synchronized (sApnPriorityMap) {
            if (sApnPriorityMap.isEmpty()) {
                String[] networkConfigStrings = context.getResources().getStringArray(
                        com.android.internal.R.array.networkAttributes);
                for (String networkConfigString : networkConfigStrings) {
                    NetworkConfig networkConfig = new NetworkConfig(networkConfigString);
                    final int apnId = ApnContext.apnIdForType(networkConfig.type);
                    sApnPriorityMap.put(apnId, networkConfig.priority);
                }
            }
        }
    }

    private int priorityForApnId(int apnId) {
        Integer priority = sApnPriorityMap.get(apnId);
        return (priority != null ? priority.intValue() : 0);
    }

    private void loge(String s) {
        Rlog.e(LOG_TAG, s);
    }
}
