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

import android.app.PendingIntent;
import android.content.res.Resources;
import android.net.ConnectivityManager;
import android.net.NetworkCapabilities;
import android.net.NetworkConfig;
import android.net.NetworkRequest;
import android.telephony.Rlog;
import android.text.TextUtils;
import android.util.LocalLog;
import android.util.SparseIntArray;

import com.android.internal.R;
import com.android.internal.telephony.DctConstants;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.RetryManager;
import com.android.internal.util.IndentingPrintWriter;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Maintain the Apn context
 */
public class ApnContext {

    public final String LOG_TAG;
    private final static String SLOG_TAG = "ApnContext";

    protected static final boolean DBG = false;

    private final Phone mPhone;

    private final String mApnType;

    private DctConstants.State mState;

    public final int priority;

    private ApnSetting mApnSetting;

    DcAsyncChannel mDcAc;

    String mReason;

    PendingIntent mReconnectAlarmIntent;

    /**
     * user/app requested connection on this APN
     */
    AtomicBoolean mDataEnabled;

    private final Object mRefCountLock = new Object();
    private int mRefCount = 0;

    /**
     * carrier requirements met
     */
    AtomicBoolean mDependencyMet;

    private final DcTracker mDcTracker;

    /**
     * Remember this as a change in this value to a more permissive state
     * should cause us to retry even permanent failures
     */
    private boolean mConcurrentVoiceAndDataAllowed;

    /**
     * used to track a single connection request so disconnects can get ignored if
     * obsolete.
     */
    private final AtomicInteger mConnectionGeneration = new AtomicInteger(0);

    /**
     * Retry manager that handles the APN retry and delays.
     */
    private final RetryManager mRetryManager;

    /**
     * AonContext constructor
     * @param phone phone object
     * @param apnType APN type (e.g. default, supl, mms, etc...)
     * @param logTag Tag for logging
     * @param config Network configuration
     * @param tracker Data call tracker
     */
    public ApnContext(Phone phone, String apnType, String logTag, NetworkConfig config,
            DcTracker tracker) {
        mPhone = phone;
        mApnType = apnType;
        mState = DctConstants.State.IDLE;
        setReason(Phone.REASON_DATA_ENABLED);
        mDataEnabled = new AtomicBoolean(false);
        mDependencyMet = new AtomicBoolean(config.dependencyMet);
        priority = config.priority;
        LOG_TAG = logTag;
        mDcTracker = tracker;
        mRetryManager = new RetryManager(phone, apnType);
    }

    /**
     * Get the APN type
     * @return The APN type
     */
    public String getApnType() {
        return mApnType;
    }

    /**
     * Get the data call async channel.
     * @return The data call async channel
     */
    public synchronized DcAsyncChannel getDcAc() {
        return mDcAc;
    }

    /**
     * Set the data call async channel.
     * @param dcac The data call async channel
     */
    public synchronized void setDataConnectionAc(DcAsyncChannel dcac) {
        if (DBG) {
            log("setDataConnectionAc: old dcac=" + mDcAc + " new dcac=" + dcac
                    + " this=" + this);
        }
        mDcAc = dcac;
    }

    /**
     * Release data connection.
     * @param reason The reason of releasing data connection
     */
    public synchronized void releaseDataConnection(String reason) {
        if (mDcAc != null) {
            mDcAc.tearDown(this, reason, null);
            mDcAc = null;
        }
        setState(DctConstants.State.IDLE);
    }

    /**
     * Get the reconnect intent.
     * @return The reconnect intent
     */
    public synchronized PendingIntent getReconnectIntent() {
        return mReconnectAlarmIntent;
    }

    /**
     * Save the reconnect intent which can be used for cancelling later.
     * @param intent The reconnect intent
     */
    public synchronized void setReconnectIntent(PendingIntent intent) {
        mReconnectAlarmIntent = intent;
    }

    /**
     * Get the current APN setting.
     * @return APN setting
     */
    public synchronized ApnSetting getApnSetting() {
        if (DBG) log("getApnSetting: apnSetting=" + mApnSetting);
        return mApnSetting;
    }

    /**
     * Set the APN setting.
     * @param apnSetting APN setting
     */
    public synchronized void setApnSetting(ApnSetting apnSetting) {
        if (DBG) log("setApnSetting: apnSetting=" + apnSetting);
        mApnSetting = apnSetting;
    }

    /**
     * Set the list of APN candidates which will be used for data call setup later.
     * @param waitingApns List of APN candidates
     */
    public synchronized void setWaitingApns(ArrayList<ApnSetting> waitingApns) {
        mRetryManager.setWaitingApns(waitingApns);
    }

    /**
     * Get the next available APN to try.
     * @return APN setting which will be used for data call setup. Return null if there is no
     * APN can be retried.
     */
    public ApnSetting getNextApnSetting() {
        return mRetryManager.getNextApnSetting();
    }

    /**
     * Save the modem suggested delay for retrying the current APN.
     * This method is called when we get the suggested delay from RIL.
     * @param delay The delay in milliseconds
     */
    public void setModemSuggestedDelay(long delay) {
        mRetryManager.setModemSuggestedDelay(delay);
    }

    /**
     * Get the delay for trying the next APN setting if the current one failed.
     * @param failFastEnabled True if fail fast mode enabled. In this case we'll use a shorter
     *                        delay.
     * @return The delay in milliseconds
     */
    public long getDelayForNextApn(boolean failFastEnabled) {
        return mRetryManager.getDelayForNextApn(failFastEnabled || isFastRetryReason());
    }

    /**
     * Mark the current APN setting permanently failed, which means it will not be retried anymore.
     * @param apn APN setting
     */
    public void markApnPermanentFailed(ApnSetting apn) {
        mRetryManager.markApnPermanentFailed(apn);
    }

    /**
     * Get the list of waiting APNs.
     * @return the list of waiting APNs
     */
    public ArrayList<ApnSetting> getWaitingApns() {
        return mRetryManager.getWaitingApns();
    }

    /**
     * Save the state indicating concurrent voice/data allowed.
     * @param allowed True if concurrent voice/data is allowed
     */
    public synchronized void setConcurrentVoiceAndDataAllowed(boolean allowed) {
        mConcurrentVoiceAndDataAllowed = allowed;
    }

    /**
     * Get the state indicating concurrent voice/data allowed.
     * @return True if concurrent voice/data is allowed
     */
    public synchronized boolean isConcurrentVoiceAndDataAllowed() {
        return mConcurrentVoiceAndDataAllowed;
    }

    /**
     * Set the current data call state.
     * @param s Current data call state
     */
    public synchronized void setState(DctConstants.State s) {
        if (DBG) {
            log("setState: " + s + ", previous state:" + mState);
        }

        mState = s;

        if (mState == DctConstants.State.FAILED) {
            if (mRetryManager.getWaitingApns() != null) {
                mRetryManager.getWaitingApns().clear(); // when teardown the connection and set to IDLE
            }
        }
    }

    /**
     * Get the current data call state.
     * @return The current data call state
     */
    public synchronized DctConstants.State getState() {
        return mState;
    }

    /**
     * Check whether the data call is disconnected or not.
     * @return True if the data call is disconnected
     */
    public boolean isDisconnected() {
        DctConstants.State currentState = getState();
        return ((currentState == DctConstants.State.IDLE) ||
                    currentState == DctConstants.State.FAILED);
    }

    /**
     * Set the reason for data call connection.
     * @param reason Reason for data call connection
     */
    public synchronized void setReason(String reason) {
        if (DBG) {
            log("set reason as " + reason + ",current state " + mState);
        }
        mReason = reason;
    }

    /**
     * Get the reason for data call connection.
     * @return The reason for data call connection
     */
    public synchronized String getReason() {
        return mReason;
    }

    /**
     * Check if ready for data call connection
     * @return True if ready, otherwise false.
     */
    public boolean isReady() {
        return mDataEnabled.get() && mDependencyMet.get();
    }

    /**
     * Check if the data call is in the state which allow connecting.
     * @return True if allowed, otherwise false.
     */
    public boolean isConnectable() {
        return isReady() && ((mState == DctConstants.State.IDLE)
                                || (mState == DctConstants.State.SCANNING)
                                || (mState == DctConstants.State.RETRYING)
                                || (mState == DctConstants.State.FAILED));
    }

    /**
     * Check if apn reason is fast retry reason which should apply shorter delay between apn re-try.
     * @return True if it is fast retry reason, otherwise false.
     */
    private boolean isFastRetryReason() {
        return Phone.REASON_NW_TYPE_CHANGED.equals(mReason) ||
                Phone.REASON_APN_CHANGED.equals(mReason);
    }

    /** Check if the data call is in connected or connecting state.
     * @return True if the data call is in connected or connecting state
     */
    public boolean isConnectedOrConnecting() {
        return isReady() && ((mState == DctConstants.State.CONNECTED)
                                || (mState == DctConstants.State.CONNECTING)
                                || (mState == DctConstants.State.SCANNING)
                                || (mState == DctConstants.State.RETRYING));
    }

    /**
     * Set data call enabled/disabled state.
     * @param enabled True if data call is enabled
     */
    public void setEnabled(boolean enabled) {
        if (DBG) {
            log("set enabled as " + enabled + ", current state is " + mDataEnabled.get());
        }
        mDataEnabled.set(enabled);
    }

    /**
     * Check if the data call is enabled or not.
     * @return True if enabled
     */
    public boolean isEnabled() {
        return mDataEnabled.get();
    }

    public void setDependencyMet(boolean met) {
        if (DBG) {
            log("set mDependencyMet as " + met + " current state is " + mDependencyMet.get());
        }
        mDependencyMet.set(met);
    }

    public boolean getDependencyMet() {
       return mDependencyMet.get();
    }

    public boolean isProvisioningApn() {
        String provisioningApn = mPhone.getContext().getResources()
                .getString(R.string.mobile_provisioning_apn);
        if (!TextUtils.isEmpty(provisioningApn) &&
                (mApnSetting != null) && (mApnSetting.apn != null)) {
            return (mApnSetting.apn.equals(provisioningApn));
        } else {
            return false;
        }
    }

    private final ArrayList<LocalLog> mLocalLogs = new ArrayList<>();
    private final ArrayList<NetworkRequest> mNetworkRequests = new ArrayList<>();
    private final ArrayDeque<LocalLog> mHistoryLogs = new ArrayDeque<>();
    private final static int MAX_HISTORY_LOG_COUNT = 4;

    public void requestLog(String str) {
        synchronized (mRefCountLock) {
            for (LocalLog l : mLocalLogs) {
                l.log(str);
            }
        }
    }

    public void requestNetwork(NetworkRequest networkRequest, LocalLog log) {
        synchronized (mRefCountLock) {
            if (mLocalLogs.contains(log) || mNetworkRequests.contains(networkRequest)) {
                log.log("ApnContext.requestNetwork has duplicate add - " + mNetworkRequests.size());
            } else {
                mLocalLogs.add(log);
                mNetworkRequests.add(networkRequest);
                mDcTracker.setEnabled(apnIdForApnName(mApnType), true);
            }
        }
    }

    public void releaseNetwork(NetworkRequest networkRequest, LocalLog log) {
        synchronized (mRefCountLock) {
            if (mLocalLogs.contains(log) == false) {
                log.log("ApnContext.releaseNetwork can't find this log");
            } else {
                mLocalLogs.remove(log);
            }
            if (mNetworkRequests.contains(networkRequest) == false) {
                log.log("ApnContext.releaseNetwork can't find this request ("
                        + networkRequest + ")");
            } else {
                mNetworkRequests.remove(networkRequest);
                log.log("ApnContext.releaseNetwork left with " + mNetworkRequests.size() +
                        " requests.");
                if (mNetworkRequests.size() == 0) {
                    mDcTracker.setEnabled(apnIdForApnName(mApnType), false);
                }
            }
        }
    }

    public List<NetworkRequest> getNetworkRequests() {
        synchronized (mRefCountLock) {
            return new ArrayList<NetworkRequest>(mNetworkRequests);
        }
    }

    public boolean hasNoRestrictedRequests(boolean excludeDun) {
        synchronized (mRefCountLock) {
            for (NetworkRequest nr : mNetworkRequests) {
                if (excludeDun &&
                        nr.networkCapabilities.hasCapability(
                        NetworkCapabilities.NET_CAPABILITY_DUN)) {
                    continue;
                }
                if (nr.networkCapabilities.hasCapability(
                        NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED) == false) {
                    return false;
                }
            }
        }
        return true;
    }

    private final SparseIntArray mRetriesLeftPerErrorCode = new SparseIntArray();

    public void resetErrorCodeRetries() {
        requestLog("ApnContext.resetErrorCodeRetries");
        if (DBG) log("ApnContext.resetErrorCodeRetries");

        String[] config = mPhone.getContext().getResources().getStringArray(
                com.android.internal.R.array.config_cell_retries_per_error_code);
        synchronized (mRetriesLeftPerErrorCode) {
            mRetriesLeftPerErrorCode.clear();

            for (String c : config) {
                String errorValue[] = c.split(",");
                if (errorValue != null && errorValue.length == 2) {
                    int count = 0;
                    int errorCode = 0;
                    try {
                        errorCode = Integer.parseInt(errorValue[0]);
                        count = Integer.parseInt(errorValue[1]);
                    } catch (NumberFormatException e) {
                        log("Exception parsing config_retries_per_error_code: " + e);
                        continue;
                    }
                    if (count > 0 && errorCode > 0) {
                        mRetriesLeftPerErrorCode.put(errorCode, count);
                    }
                } else {
                    log("Exception parsing config_retries_per_error_code: " + c);
                }
            }
        }
    }

    public boolean restartOnError(int errorCode) {
        boolean result = false;
        int retriesLeft = 0;
        synchronized(mRetriesLeftPerErrorCode) {
            retriesLeft = mRetriesLeftPerErrorCode.get(errorCode);
            switch (retriesLeft) {
                case 0: {
                    // not set, never restart modem
                    break;
                }
                case 1: {
                    resetErrorCodeRetries();
                    result = true;
                    break;
                }
                default: {
                    mRetriesLeftPerErrorCode.put(errorCode, retriesLeft - 1);
                    result = false;
                }
            }
        }
        String str = "ApnContext.restartOnError(" + errorCode + ") found " + retriesLeft +
                " and returned " + result;
        if (DBG) log(str);
        requestLog(str);
        return result;
    }

    public int incAndGetConnectionGeneration() {
        return mConnectionGeneration.incrementAndGet();
    }

    public int getConnectionGeneration() {
        return mConnectionGeneration.get();
    }

    public long getInterApnDelay(boolean failFastEnabled) {
        return mRetryManager.getInterApnDelay(failFastEnabled || isFastRetryReason());
    }

    public static int apnIdForType(int networkType) {
        switch (networkType) {
        case ConnectivityManager.TYPE_MOBILE:
            return DctConstants.APN_DEFAULT_ID;
        case ConnectivityManager.TYPE_MOBILE_MMS:
            return DctConstants.APN_MMS_ID;
        case ConnectivityManager.TYPE_MOBILE_SUPL:
            return DctConstants.APN_SUPL_ID;
        case ConnectivityManager.TYPE_MOBILE_DUN:
            return DctConstants.APN_DUN_ID;
        case ConnectivityManager.TYPE_MOBILE_FOTA:
            return DctConstants.APN_FOTA_ID;
        case ConnectivityManager.TYPE_MOBILE_IMS:
            return DctConstants.APN_IMS_ID;
        case ConnectivityManager.TYPE_MOBILE_CBS:
            return DctConstants.APN_CBS_ID;
        case ConnectivityManager.TYPE_MOBILE_IA:
            return DctConstants.APN_IA_ID;
        case ConnectivityManager.TYPE_MOBILE_EMERGENCY:
            return DctConstants.APN_EMERGENCY_ID;
        default:
            return DctConstants.APN_INVALID_ID;
        }
    }

    public static int apnIdForNetworkRequest(NetworkRequest nr) {
        NetworkCapabilities nc = nr.networkCapabilities;
        // For now, ignore the bandwidth stuff
        if (nc.getTransportTypes().length > 0 &&
                nc.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) == false) {
            return DctConstants.APN_INVALID_ID;
        }

        // in the near term just do 1-1 matches.
        // TODO - actually try to match the set of capabilities
        int apnId = DctConstants.APN_INVALID_ID;
        boolean error = false;

        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)) {
            apnId = DctConstants.APN_DEFAULT_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_MMS)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;
            apnId = DctConstants.APN_MMS_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_SUPL)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;
            apnId = DctConstants.APN_SUPL_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_DUN)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;
            apnId = DctConstants.APN_DUN_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_FOTA)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;
            apnId = DctConstants.APN_FOTA_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_IMS)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;
            apnId = DctConstants.APN_IMS_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_CBS)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;
            apnId = DctConstants.APN_CBS_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_IA)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;
            apnId = DctConstants.APN_IA_ID;
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_RCS)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;

            Rlog.d(SLOG_TAG, "RCS APN type not yet supported");
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_XCAP)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;

            Rlog.d(SLOG_TAG, "XCAP APN type not yet supported");
        }
        if (nc.hasCapability(NetworkCapabilities.NET_CAPABILITY_EIMS)) {
            if (apnId != DctConstants.APN_INVALID_ID) error = true;
            apnId = DctConstants.APN_EMERGENCY_ID;
        }
        if (error) {
            // TODO: If this error condition is removed, the framework's handling of
            // NET_CAPABILITY_NOT_RESTRICTED will need to be updated so requests for
            // say FOTA and INTERNET are marked as restricted.  This is not how
            // NetworkCapabilities.maybeMarkCapabilitiesRestricted currently works.
            Rlog.d(SLOG_TAG, "Multiple apn types specified in request - result is unspecified!");
        }
        if (apnId == DctConstants.APN_INVALID_ID) {
            Rlog.d(SLOG_TAG, "Unsupported NetworkRequest in Telephony: nr=" + nr);
        }
        return apnId;
    }

    // TODO - kill The use of these strings
    public static int apnIdForApnName(String type) {
        switch (type) {
            case PhoneConstants.APN_TYPE_DEFAULT:
                return DctConstants.APN_DEFAULT_ID;
            case PhoneConstants.APN_TYPE_MMS:
                return DctConstants.APN_MMS_ID;
            case PhoneConstants.APN_TYPE_SUPL:
                return DctConstants.APN_SUPL_ID;
            case PhoneConstants.APN_TYPE_DUN:
                return DctConstants.APN_DUN_ID;
            case PhoneConstants.APN_TYPE_HIPRI:
                return DctConstants.APN_HIPRI_ID;
            case PhoneConstants.APN_TYPE_IMS:
                return DctConstants.APN_IMS_ID;
            case PhoneConstants.APN_TYPE_FOTA:
                return DctConstants.APN_FOTA_ID;
            case PhoneConstants.APN_TYPE_CBS:
                return DctConstants.APN_CBS_ID;
            case PhoneConstants.APN_TYPE_IA:
                return DctConstants.APN_IA_ID;
            case PhoneConstants.APN_TYPE_EMERGENCY:
                return DctConstants.APN_EMERGENCY_ID;
            default:
                return DctConstants.APN_INVALID_ID;
        }
    }

    private static String apnNameForApnId(int id) {
        switch (id) {
            case DctConstants.APN_DEFAULT_ID:
                return PhoneConstants.APN_TYPE_DEFAULT;
            case DctConstants.APN_MMS_ID:
                return PhoneConstants.APN_TYPE_MMS;
            case DctConstants.APN_SUPL_ID:
                return PhoneConstants.APN_TYPE_SUPL;
            case DctConstants.APN_DUN_ID:
                return PhoneConstants.APN_TYPE_DUN;
            case DctConstants.APN_HIPRI_ID:
                return PhoneConstants.APN_TYPE_HIPRI;
            case DctConstants.APN_IMS_ID:
                return PhoneConstants.APN_TYPE_IMS;
            case DctConstants.APN_FOTA_ID:
                return PhoneConstants.APN_TYPE_FOTA;
            case DctConstants.APN_CBS_ID:
                return PhoneConstants.APN_TYPE_CBS;
            case DctConstants.APN_IA_ID:
                return PhoneConstants.APN_TYPE_IA;
            case DctConstants.APN_EMERGENCY_ID:
                return PhoneConstants.APN_TYPE_EMERGENCY;
            default:
                Rlog.d(SLOG_TAG, "Unknown id (" + id + ") in apnIdToType");
                return PhoneConstants.APN_TYPE_DEFAULT;
        }
    }

    @Override
    public synchronized String toString() {
        // We don't print mDataConnection because its recursive.
        return "{mApnType=" + mApnType + " mState=" + getState() + " mWaitingApns={" +
                mRetryManager.getWaitingApns() + "}" + " mApnSetting={" + mApnSetting +
                "} mReason=" + mReason + " mDataEnabled=" + mDataEnabled + " mDependencyMet=" +
                mDependencyMet + "}";
    }

    private void log(String s) {
        Rlog.d(LOG_TAG, "[ApnContext:" + mApnType + "] " + s);
    }

    public void dump(FileDescriptor fd, PrintWriter printWriter, String[] args) {
        final IndentingPrintWriter pw = new IndentingPrintWriter(printWriter, "  ");
        synchronized (mRefCountLock) {
            pw.println(toString());
            if (mNetworkRequests.size() > 0) {
                pw.println("NetworkRequests:");
                pw.increaseIndent();
                for (NetworkRequest nr : mNetworkRequests) {
                    pw.println(nr);
                }
                pw.decreaseIndent();
            }
            pw.increaseIndent();
            for (LocalLog l : mLocalLogs) {
                l.dump(fd, pw, args);
            }
            if (mHistoryLogs.size() > 0) pw.println("Historical Logs:");
            for (LocalLog l : mHistoryLogs) {
                l.dump(fd, pw, args);
            }
            pw.decreaseIndent();
            pw.println("mRetryManager={" + mRetryManager.toString() + "}");
        }
    }
}
