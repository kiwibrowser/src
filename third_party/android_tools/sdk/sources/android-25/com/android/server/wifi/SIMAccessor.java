package com.android.server.wifi;

import android.content.Context;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import java.util.ArrayList;
import java.util.List;

public class SIMAccessor {
    private final TelephonyManager mTelephonyManager;
    private final SubscriptionManager mSubscriptionManager;

    public SIMAccessor(Context context) {
        mTelephonyManager = TelephonyManager.from(context);
        mSubscriptionManager = SubscriptionManager.from(context);
    }

    public List<String> getMatchingImsis(IMSIParameter mccMnc) {
        if (mccMnc == null) {
            return null;
        }
        List<String> imsis = new ArrayList<>();
        for (int subId : mSubscriptionManager.getActiveSubscriptionIdList()) {
            String imsi = mTelephonyManager.getSubscriberId(subId);
            if (mccMnc.matches(imsi)) {
                imsis.add(imsi);
            }
        }
        return imsis.isEmpty() ? null : imsis;
    }
}
