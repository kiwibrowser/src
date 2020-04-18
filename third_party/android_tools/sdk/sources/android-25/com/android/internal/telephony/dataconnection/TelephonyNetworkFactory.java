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

import static android.telephony.SubscriptionManager.INVALID_SUBSCRIPTION_ID;

import android.content.Context;
import android.net.NetworkCapabilities;
import android.net.NetworkFactory;
import android.net.NetworkRequest;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.telephony.Rlog;
import android.text.TextUtils;
import android.util.LocalLog;

import com.android.internal.telephony.PhoneSwitcher;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.SubscriptionMonitor;
import com.android.internal.util.IndentingPrintWriter;
import com.android.internal.util.Protocol;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.HashMap;

public class TelephonyNetworkFactory extends NetworkFactory {
    public final String LOG_TAG;
    protected static final boolean DBG = true;

    private final PhoneSwitcher mPhoneSwitcher;
    private final SubscriptionController mSubscriptionController;
    private final SubscriptionMonitor mSubscriptionMonitor;
    private final DcTracker mDcTracker;

    private final HashMap<NetworkRequest, LocalLog> mDefaultRequests =
            new HashMap<NetworkRequest, LocalLog>();
    private final HashMap<NetworkRequest, LocalLog> mSpecificRequests =
            new HashMap<NetworkRequest, LocalLog>();

    private int mPhoneId;
    private boolean mIsActive;
    private boolean mIsDefault;
    private int mSubscriptionId;

    private final static int TELEPHONY_NETWORK_SCORE = 50;

    private final Handler mInternalHandler;
    private static final int EVENT_ACTIVE_PHONE_SWITCH          = 1;
    private static final int EVENT_SUBSCRIPTION_CHANGED         = 2;
    private static final int EVENT_DEFAULT_SUBSCRIPTION_CHANGED = 3;
    private static final int EVENT_NETWORK_REQUEST              = 4;
    private static final int EVENT_NETWORK_RELEASE              = 5;

    public TelephonyNetworkFactory(PhoneSwitcher phoneSwitcher,
            SubscriptionController subscriptionController, SubscriptionMonitor subscriptionMonitor,
            Looper looper, Context context, int phoneId, DcTracker dcTracker) {
        super(looper, context, "TelephonyNetworkFactory[" + phoneId + "]", null);
        mInternalHandler = new InternalHandler(looper);

        setCapabilityFilter(makeNetworkFilter(subscriptionController, phoneId));
        setScoreFilter(TELEPHONY_NETWORK_SCORE);

        mPhoneSwitcher = phoneSwitcher;
        mSubscriptionController = subscriptionController;
        mSubscriptionMonitor = subscriptionMonitor;
        mPhoneId = phoneId;
        LOG_TAG = "TelephonyNetworkFactory[" + phoneId + "]";
        mDcTracker = dcTracker;

        mIsActive = false;
        mPhoneSwitcher.registerForActivePhoneSwitch(mPhoneId, mInternalHandler,
                EVENT_ACTIVE_PHONE_SWITCH, null);

        mSubscriptionId = INVALID_SUBSCRIPTION_ID;
        mSubscriptionMonitor.registerForSubscriptionChanged(mPhoneId, mInternalHandler,
                EVENT_SUBSCRIPTION_CHANGED, null);

        mIsDefault = false;
        mSubscriptionMonitor.registerForDefaultDataSubscriptionChanged(mPhoneId, mInternalHandler,
                EVENT_DEFAULT_SUBSCRIPTION_CHANGED, null);

        register();
    }

    private NetworkCapabilities makeNetworkFilter(SubscriptionController subscriptionController,
            int phoneId) {
        final int subscriptionId = subscriptionController.getSubIdUsingPhoneId(phoneId);
        return makeNetworkFilter(subscriptionId);
    }

    private NetworkCapabilities makeNetworkFilter(int subscriptionId) {
        NetworkCapabilities nc = new NetworkCapabilities();
        nc.addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_MMS);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_SUPL);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_DUN);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_FOTA);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_IMS);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_CBS);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_IA);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_RCS);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_XCAP);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_EIMS);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED);
        nc.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
        nc.setNetworkSpecifier(String.valueOf(subscriptionId));
        return nc;
    }

    private class InternalHandler extends Handler {
        public InternalHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case EVENT_ACTIVE_PHONE_SWITCH: {
                    onActivePhoneSwitch();
                    break;
                }
                case EVENT_SUBSCRIPTION_CHANGED: {
                    onSubIdChange();
                    break;
                }
                case EVENT_DEFAULT_SUBSCRIPTION_CHANGED: {
                    onDefaultChange();
                    break;
                }
                case EVENT_NETWORK_REQUEST: {
                    onNeedNetworkFor(msg);
                    break;
                }
                case EVENT_NETWORK_RELEASE: {
                    onReleaseNetworkFor(msg);
                    break;
                }
            }
        }
    }

    private static final int REQUEST_LOG_SIZE = 40;
    private static final boolean REQUEST = true;
    private static final boolean RELEASE = false;

    private void applyRequests(HashMap<NetworkRequest, LocalLog> requestMap, boolean action,
            String logStr) {
        for (NetworkRequest networkRequest : requestMap.keySet()) {
            LocalLog localLog = requestMap.get(networkRequest);
            localLog.log(logStr);
            if (action == REQUEST) {
                mDcTracker.requestNetwork(networkRequest, localLog);
            } else {
                mDcTracker.releaseNetwork(networkRequest, localLog);
            }
        }
    }

    // apply or revoke requests if our active-ness changes
    private void onActivePhoneSwitch() {
        final boolean newIsActive = mPhoneSwitcher.isPhoneActive(mPhoneId);
        if (mIsActive != newIsActive) {
            mIsActive = newIsActive;
            String logString = "onActivePhoneSwitch(" + mIsActive + ", " + mIsDefault + ")";
            if (DBG) log(logString);
            if (mIsDefault) {
                applyRequests(mDefaultRequests, (mIsActive ? REQUEST : RELEASE), logString);
            }
            applyRequests(mSpecificRequests, (mIsActive ? REQUEST : RELEASE), logString);
        }
    }

    // watch for phone->subId changes, reapply new filter and let
    // that flow through to apply/revoke of requests
    private void onSubIdChange() {
        final int newSubscriptionId = mSubscriptionController.getSubIdUsingPhoneId(mPhoneId);
        if (mSubscriptionId != newSubscriptionId) {
            if (DBG) log("onSubIdChange " + mSubscriptionId + "->" + newSubscriptionId);
            mSubscriptionId = newSubscriptionId;
            setCapabilityFilter(makeNetworkFilter(mSubscriptionId));
        }
    }

    // watch for default-data changes (could be side effect of
    // phoneId->subId map change or direct change of default subId)
    // and apply/revoke default-only requests.
    private void onDefaultChange() {
        final int newDefaultSubscriptionId = mSubscriptionController.getDefaultDataSubId();
        final boolean newIsDefault = (newDefaultSubscriptionId == mSubscriptionId);
        if (newIsDefault != mIsDefault) {
            mIsDefault = newIsDefault;
            String logString = "onDefaultChange(" + mIsActive + "," + mIsDefault + ")";
            if (DBG) log(logString);
            if (mIsActive == false) return;
            applyRequests(mDefaultRequests, (mIsDefault ? REQUEST : RELEASE), logString);
        }
    }

    @Override
    public void needNetworkFor(NetworkRequest networkRequest, int score) {
        Message msg = mInternalHandler.obtainMessage(EVENT_NETWORK_REQUEST);
        msg.obj = networkRequest;
        msg.sendToTarget();
    }

    private void onNeedNetworkFor(Message msg) {
        NetworkRequest networkRequest = (NetworkRequest)msg.obj;
        boolean isApplicable = false;
        LocalLog localLog = null;
        if (TextUtils.isEmpty(networkRequest.networkCapabilities.getNetworkSpecifier())) {
            // request only for the default network
            localLog = mDefaultRequests.get(networkRequest);
            if (localLog == null) {
                localLog = new LocalLog(REQUEST_LOG_SIZE);
                localLog.log("created for " + networkRequest);
                mDefaultRequests.put(networkRequest, localLog);
                isApplicable = mIsDefault;
            }
        } else {
            localLog = mSpecificRequests.get(networkRequest);
            if (localLog == null) {
                localLog = new LocalLog(REQUEST_LOG_SIZE);
                mSpecificRequests.put(networkRequest, localLog);
                isApplicable = true;
            }
        }
        if (mIsActive && isApplicable) {
            String s = "onNeedNetworkFor";
            localLog.log(s);
            log(s + " " + networkRequest);
            mDcTracker.requestNetwork(networkRequest, localLog);
        } else {
            String s = "not acting - isApp=" + isApplicable + ", isAct=" + mIsActive;
            localLog.log(s);
            log(s + " " + networkRequest);
        }
    }

    @Override
    public void releaseNetworkFor(NetworkRequest networkRequest) {
        Message msg = mInternalHandler.obtainMessage(EVENT_NETWORK_RELEASE);
        msg.obj = networkRequest;
        msg.sendToTarget();
    }

    private void onReleaseNetworkFor(Message msg) {
        NetworkRequest networkRequest = (NetworkRequest)msg.obj;
        LocalLog localLog = null;
        boolean isApplicable = false;
        if (TextUtils.isEmpty(networkRequest.networkCapabilities.getNetworkSpecifier())) {
            // request only for the default network
            localLog = mDefaultRequests.remove(networkRequest);
            isApplicable = (localLog != null) && mIsDefault;
        } else {
            localLog = mSpecificRequests.remove(networkRequest);
            isApplicable = (localLog != null);
        }
        if (mIsActive && isApplicable) {
            String s = "onReleaseNetworkFor";
            localLog.log(s);
            log(s + " " + networkRequest);
            mDcTracker.releaseNetwork(networkRequest, localLog);
        } else {
            String s = "not releasing - isApp=" + isApplicable + ", isAct=" + mIsActive;
            localLog.log(s);
            log(s + " " + networkRequest);
        }
    }

    protected void log(String s) {
        Rlog.d(LOG_TAG, s);
    }

    public void dump(FileDescriptor fd, PrintWriter writer, String[] args) {
        final IndentingPrintWriter pw = new IndentingPrintWriter(writer, "  ");
        pw.println(LOG_TAG + " mSubId=" + mSubscriptionId + " mIsActive=" +
                mIsActive + " mIsDefault=" + mIsDefault);
        pw.println("Default Requests:");
        pw.increaseIndent();
        for (NetworkRequest nr : mDefaultRequests.keySet()) {
            pw.println(nr);
            pw.increaseIndent();
            mDefaultRequests.get(nr).dump(fd, pw, args);
            pw.decreaseIndent();
        }
        pw.decreaseIndent();
    }
}
