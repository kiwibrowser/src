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

package com.android.server.wifi.util;

import android.content.Context;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiEnterpriseConfig;
import android.telephony.TelephonyManager;

/**
 * Utilities for the Wifi Service to interact with telephony.
 */
public class TelephonyUtil {

    /**
     * Get the identity for the current SIM or null if the sim is not available
     */
    public static String getSimIdentity(Context context, int eapMethod) {
        TelephonyManager tm = TelephonyManager.from(context);
        if (tm != null) {
            String imsi = tm.getSubscriberId();
            String mccMnc = "";

            if (tm.getSimState() == TelephonyManager.SIM_STATE_READY) {
                mccMnc = tm.getSimOperator();
            }

            return buildIdentity(eapMethod, imsi, mccMnc);
        } else {
            return null;
        }
    }

    /**
     * create Permanent Identity base on IMSI,
     *
     * rfc4186 & rfc4187:
     * identity = usernam@realm
     * with username = prefix | IMSI
     * and realm is derived MMC/MNC tuple according 3GGP spec(TS23.003)
     */
    private static String buildIdentity(int eapMethod, String imsi, String mccMnc) {
        if (imsi == null || imsi.isEmpty()) {
            return null;
        }

        String prefix;
        if (eapMethod == WifiEnterpriseConfig.Eap.SIM) {
            prefix = "1";
        } else if (eapMethod == WifiEnterpriseConfig.Eap.AKA) {
            prefix = "0";
        } else if (eapMethod == WifiEnterpriseConfig.Eap.AKA_PRIME) {
            prefix = "6";
        } else {  // not a valide EapMethod
            return null;
        }

        /* extract mcc & mnc from mccMnc */
        String mcc;
        String mnc;
        if (mccMnc != null && !mccMnc.isEmpty()) {
            mcc = mccMnc.substring(0, 3);
            mnc = mccMnc.substring(3);
            if (mnc.length() == 2) {
                mnc = "0" + mnc;
            }
        } else {
            // extract mcc & mnc from IMSI, assume mnc size is 3
            mcc = imsi.substring(0, 3);
            mnc = imsi.substring(3, 6);
        }

        return prefix + imsi + "@wlan.mnc" + mnc + ".mcc" + mcc + ".3gppnetwork.org";
    }

    /**
     * Checks if the network is a sim config.
     *
     * @param config Config corresponding to the network.
     * @return true if it is a sim config, false otherwise.
     */
    public static boolean isSimConfig(WifiConfiguration config) {
        if (config == null || config.enterpriseConfig == null) {
            return false;
        }

        return isSimEapMethod(config.enterpriseConfig.getEapMethod());
    }

    /**
     * Checks if the network is a sim config.
     *
     * @param method
     * @return true if it is a sim config, false otherwise.
     */
    public static boolean isSimEapMethod(int eapMethod) {
        return eapMethod == WifiEnterpriseConfig.Eap.SIM
                || eapMethod == WifiEnterpriseConfig.Eap.AKA
                || eapMethod == WifiEnterpriseConfig.Eap.AKA_PRIME;
    }
}
