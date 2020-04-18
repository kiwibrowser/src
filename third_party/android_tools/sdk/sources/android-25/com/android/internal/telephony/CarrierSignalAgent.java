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

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.Rlog;

import com.android.internal.util.ArrayUtils;

import java.util.HashMap;
import java.util.Map;

/**
 * This class act as an CarrierSignalling Agent.
 * it load registered carrier signalling receivers from Carrier Config and cache the result to avoid
 * repeated polling and send the intent to the interested receivers.
 * each CarrierSignalAgent is associated with a phone object.
 */
public class CarrierSignalAgent {

    private static final String LOG_TAG = "CarrierSignalAgent";
    private static final boolean DBG = true;

    /** Member variables */
    private final Phone mPhone;
    /**
     * This is a map of intent action -> string array of carrier signal receiver names which are
     * interested in this intent action
     */
    private final HashMap<String, String[]> mCachedCarrierSignalReceiverNames =
            new HashMap<>();
    /**
     * This is a map of intent action -> carrier config key of signal receiver names which are
     * interested in this intent action
     */
    private final Map<String, String> mIntentToCarrierConfigKeyMap =
            new HashMap<String, String>() {{
                put(TelephonyIntents.ACTION_CARRIER_SIGNAL_REDIRECTED,
                        CarrierConfigManager.KEY_SIGNAL_REDIRECTION_RECEIVER_STRING_ARRAY);
                put(TelephonyIntents.ACTION_CARRIER_SIGNAL_PCO_VALUE,
                        CarrierConfigManager.KEY_SIGNAL_PCO_RECEIVER_STRING_ARRAY);
                put(TelephonyIntents.ACTION_CARRIER_SIGNAL_REQUEST_NETWORK_FAILED,
                        CarrierConfigManager.KEY_SIGNAL_DCFAILURE_RECEIVER_STRING_ARRAY);
            }};

    /** Constructor */
    public CarrierSignalAgent(Phone phone) {
        mPhone = phone;
    }

    /**
     * Read carrier signalling receiver name from CarrierConfig based on the intent type
     * @return array of receiver Name: the package (a String) name / the class (a String) name
     */
    private String[] getCarrierSignalReceiverName(String intentAction) {
        String receiverType = mIntentToCarrierConfigKeyMap.get(intentAction);
        if(receiverType == null) {
            return null;
        }
        String[] receiverNames = mCachedCarrierSignalReceiverNames.get(intentAction);
        // In case of cache miss, we need to look up/load from carrier config.
        if (!mCachedCarrierSignalReceiverNames.containsKey(intentAction)) {
            CarrierConfigManager configManager = (CarrierConfigManager) mPhone.getContext()
                    .getSystemService(Context.CARRIER_CONFIG_SERVICE);
            PersistableBundle b = null;
            if (configManager != null) {
                b = configManager.getConfig();
            }
            if (b != null) {
                receiverNames = b.getStringArray(receiverType);
                if(receiverNames!=null) {
                    for(String name: receiverNames) {
                        Rlog.d("loadCarrierSignalReceiverNames: ", name);
                    }
                }
            }
            mCachedCarrierSignalReceiverNames.put(intentAction, receiverNames);
        }
        return receiverNames;
    }

    /**
     * Check if there are registered carrier broadcast receivers to handle any registered intents.
     */
    public boolean hasRegisteredCarrierSignalReceivers() {
        for(String intent : mIntentToCarrierConfigKeyMap.keySet()) {
            if(!ArrayUtils.isEmpty(getCarrierSignalReceiverName(intent))) {
                return true;
            }
        }
        return false;
    }

    public boolean notifyCarrierSignalReceivers(Intent intent) {
        // Read a list of broadcast receivers from carrier config manager
        // which are interested on certain intent type
        String[] receiverName = getCarrierSignalReceiverName(intent.getAction());
        if (receiverName == null) {
            loge("Carrier receiver name is null");
            return false;
        }
        final PackageManager packageManager = mPhone.getContext().getPackageManager();
        boolean ret = false;

        for(String name : receiverName) {
            ComponentName componentName = ComponentName.unflattenFromString(name);
            if (componentName == null) {
                loge("Carrier receiver name could not be parsed");
                return false;
            }
            intent.setComponent(componentName);
            // Check if broadcast receiver is available
            if (packageManager.queryBroadcastReceivers(intent,
                    PackageManager.MATCH_DEFAULT_ONLY).isEmpty()) {
                loge("Carrier signal receiver is configured, but not available: " + name);
                break;
            }

            intent.putExtra(PhoneConstants.SUBSCRIPTION_KEY, mPhone.getSubId());
            intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);

            try {
                mPhone.getContext().sendBroadcast(intent);
                if (DBG) log("send Intent to carrier signal receiver with action: " +
                        intent.getAction());
                ret = true;
            } catch (ActivityNotFoundException e) {
                loge("sendBroadcast failed: " + e);
            }
        }

        return ret;
    }

    /* Clear cached receiver names */
    public void reset() {
        mCachedCarrierSignalReceiverNames.clear();
    }

    private void log(String s) {
        Rlog.d(LOG_TAG, "[" + mPhone.getPhoneId() + "]" + s);
    }

    private void loge(String s) {
        Rlog.e(LOG_TAG, "[" + mPhone.getPhoneId() + "]" + s);
    }
}
