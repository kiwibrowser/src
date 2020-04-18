package com.android.server.wifi.hotspot2.pps;

import com.android.server.wifi.hotspot2.Utils;
import com.android.server.wifi.hotspot2.omadm.OMAException;
import com.android.server.wifi.hotspot2.omadm.OMANode;
import com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager;

import java.util.ArrayList;
import java.util.List;

import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_CreationDate;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_DataLimit;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_ExpirationDate;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_StartDate;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_TimeLimit;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_TypeOfSubscription;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_UsageLimits;
import static com.android.server.wifi.hotspot2.omadm.PasspointManagementObjectManager.TAG_UsageTimePeriod;

public class SubscriptionParameters {
    private final long mCDate;
    private final long mXDate;
    private final String mType;
    private final List<Limit> mLimits;

    public SubscriptionParameters(OMANode node) throws OMAException {
        mCDate = PasspointManagementObjectManager.getTime(node.getChild(TAG_CreationDate));
        mXDate = PasspointManagementObjectManager.getTime(node.getChild(TAG_ExpirationDate));
        mType = PasspointManagementObjectManager.getString(node.getChild(TAG_TypeOfSubscription));

        OMANode ulNode = node.getChild(TAG_UsageLimits);
        if (ulNode == null) {
            mLimits = null;
        }
        else {
            mLimits = new ArrayList<>(ulNode.getChildren().size());
            for (OMANode instance : ulNode.getChildren()) {
                if (instance.isLeaf()) {
                    throw new OMAException("Not expecting leaf node in " +
                            TAG_UsageLimits);
                }
                mLimits.add(new Limit(instance));
            }
        }

    }

    private static class Limit {
        private final long mDataLimit;
        private final long mStartDate;
        private final long mTimeLimit;
        private final long mUsageTimePeriod;

        private Limit(OMANode node) throws OMAException {
            mDataLimit = PasspointManagementObjectManager
                    .getLong(node, TAG_DataLimit, Long.MAX_VALUE);
            mStartDate = PasspointManagementObjectManager
                    .getTime(node.getChild(TAG_StartDate));
            mTimeLimit = PasspointManagementObjectManager
                    .getLong(node, TAG_TimeLimit, Long.MAX_VALUE)
                    * PasspointManagementObjectManager.IntervalFactor;
            mUsageTimePeriod = PasspointManagementObjectManager
                    .getLong(node, TAG_UsageTimePeriod, null);
        }

        @Override
        public String toString() {
            return "Limit{" +
                    "dataLimit=" + mDataLimit +
                    ", startDate=" + Utils.toUTCString(mStartDate) +
                    ", timeLimit=" + mTimeLimit +
                    ", usageTimePeriod=" + mUsageTimePeriod +
                    '}';
        }
    }

    @Override
    public String toString() {
        return "SubscriptionParameters{" +
                "cDate=" + Utils.toUTCString(mCDate) +
                ", xDate=" + Utils.toUTCString(mXDate) +
                ", type='" + mType + '\'' +
                ", limits=" + mLimits +
                '}';
    }
}
