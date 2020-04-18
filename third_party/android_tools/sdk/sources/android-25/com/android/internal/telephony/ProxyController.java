/*
 * Copyright (C) 2013 The Android Open Source Project
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

import java.util.ArrayList;
import java.util.Random;

import android.content.Context;
import android.content.Intent;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.telephony.RadioAccessFamily;
import android.telephony.Rlog;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.telephony.PhoneSwitcher;
import com.android.internal.telephony.uicc.UiccController;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.HashSet;

public class ProxyController {
    static final String LOG_TAG = "ProxyController";

    private static final int EVENT_NOTIFICATION_RC_CHANGED        = 1;
    private static final int EVENT_START_RC_RESPONSE        = 2;
    private static final int EVENT_APPLY_RC_RESPONSE        = 3;
    private static final int EVENT_FINISH_RC_RESPONSE       = 4;
    private static final int EVENT_TIMEOUT                  = 5;

    private static final int SET_RC_STATUS_IDLE             = 0;
    private static final int SET_RC_STATUS_STARTING         = 1;
    private static final int SET_RC_STATUS_STARTED          = 2;
    private static final int SET_RC_STATUS_APPLYING         = 3;
    private static final int SET_RC_STATUS_SUCCESS          = 4;
    private static final int SET_RC_STATUS_FAIL             = 5;

    // The entire transaction must complete within this amount of time
    // or a FINISH will be issued to each Logical Modem with the old
    // Radio Access Family.
    private static final int SET_RC_TIMEOUT_WAITING_MSEC    = (45 * 1000);

    //***** Class Variables
    private static ProxyController sProxyController;

    private Phone[] mPhones;

    private UiccController mUiccController;

    private CommandsInterface[] mCi;

    private Context mContext;

    private PhoneSwitcher mPhoneSwitcher;

    //UiccPhoneBookController to use proper IccPhoneBookInterfaceManagerProxy object
    private UiccPhoneBookController mUiccPhoneBookController;

    //PhoneSubInfoController to use proper PhoneSubInfoProxy object
    private PhoneSubInfoController mPhoneSubInfoController;

    //UiccSmsController to use proper IccSmsInterfaceManager object
    private UiccSmsController mUiccSmsController;

    WakeLock mWakeLock;

    // record each phone's set radio capability status
    private int[] mSetRadioAccessFamilyStatus;
    private int mRadioAccessFamilyStatusCounter;
    private boolean mTransactionFailed = false;

    private String[] mCurrentLogicalModemIds;
    private String[] mNewLogicalModemIds;

    // Allows the generation of unique Id's for radio capability request session  id
    private AtomicInteger mUniqueIdGenerator = new AtomicInteger(new Random().nextInt());

    // on-going radio capability request session id
    private int mRadioCapabilitySessionId;

    // Record new and old Radio Access Family (raf) configuration.
    // The old raf configuration is used to restore each logical modem raf when FINISH is
    // issued if any requests fail.
    private int[] mNewRadioAccessFamily;
    private int[] mOldRadioAccessFamily;


    //***** Class Methods
    public static ProxyController getInstance(Context context, Phone[] phone,
            UiccController uiccController, CommandsInterface[] ci, PhoneSwitcher ps) {
        if (sProxyController == null) {
            sProxyController = new ProxyController(context, phone, uiccController, ci, ps);
        }
        return sProxyController;
    }

    public static ProxyController getInstance() {
        return sProxyController;
    }

    private ProxyController(Context context, Phone[] phone, UiccController uiccController,
            CommandsInterface[] ci, PhoneSwitcher phoneSwitcher) {
        logd("Constructor - Enter");

        mContext = context;
        mPhones = phone;
        mUiccController = uiccController;
        mCi = ci;
        mPhoneSwitcher = phoneSwitcher;

        mUiccPhoneBookController = new UiccPhoneBookController(mPhones);
        mPhoneSubInfoController = new PhoneSubInfoController(mContext, mPhones);
        mUiccSmsController = new UiccSmsController(mPhones);
        mSetRadioAccessFamilyStatus = new int[mPhones.length];
        mNewRadioAccessFamily = new int[mPhones.length];
        mOldRadioAccessFamily = new int[mPhones.length];
        mCurrentLogicalModemIds = new String[mPhones.length];
        mNewLogicalModemIds = new String[mPhones.length];

        // wake lock for set radio capability
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, LOG_TAG);
        mWakeLock.setReferenceCounted(false);

        // Clear to be sure we're in the initial state
        clearTransaction();
        for (int i = 0; i < mPhones.length; i++) {
            mPhones[i].registerForRadioCapabilityChanged(
                    mHandler, EVENT_NOTIFICATION_RC_CHANGED, null);
        }
        logd("Constructor - Exit");
    }

    public void updateDataConnectionTracker(int sub) {
        mPhones[sub].updateDataConnectionTracker();
    }

    public void enableDataConnectivity(int sub) {
        mPhones[sub].setInternalDataEnabled(true, null);
    }

    public void disableDataConnectivity(int sub,
            Message dataCleanedUpMsg) {
        mPhones[sub].setInternalDataEnabled(false, dataCleanedUpMsg);
    }

    public void updateCurrentCarrierInProvider(int sub) {
        mPhones[sub].updateCurrentCarrierInProvider();
    }

    public void registerForAllDataDisconnected(int subId, Handler h, int what, Object obj) {
        int phoneId = SubscriptionController.getInstance().getPhoneId(subId);

        if (phoneId >= 0 && phoneId < TelephonyManager.getDefault().getPhoneCount()) {
            mPhones[phoneId].registerForAllDataDisconnected(h, what, obj);
        }
    }

    public void unregisterForAllDataDisconnected(int subId, Handler h) {
        int phoneId = SubscriptionController.getInstance().getPhoneId(subId);

        if (phoneId >= 0 && phoneId < TelephonyManager.getDefault().getPhoneCount()) {
            mPhones[phoneId].unregisterForAllDataDisconnected(h);
        }
    }

    public boolean isDataDisconnected(int subId) {
        int phoneId = SubscriptionController.getInstance().getPhoneId(subId);

        if (phoneId >= 0 && phoneId < TelephonyManager.getDefault().getPhoneCount()) {
            return mPhones[phoneId].mDcTracker.isDisconnected();
        } else {
            // if we can't find a phone for the given subId, it is disconnected.
            return true;
        }
    }

    /**
     * Get phone radio type and access technology.
     *
     * @param phoneId which phone you want to get
     * @return phone radio type and access technology for input phone ID
     */
    public int getRadioAccessFamily(int phoneId) {
        if (phoneId >= mPhones.length) {
            return RadioAccessFamily.RAF_UNKNOWN;
        } else {
            return mPhones[phoneId].getRadioAccessFamily();
        }
    }

    /**
     * Set phone radio type and access technology for each phone.
     *
     * @param rafs an RadioAccessFamily array to indicate all phone's
     *        new radio access family. The length of RadioAccessFamily
     *        must equal to phone count.
     * @return false if another session is already active and the request is rejected.
     */
    public boolean setRadioCapability(RadioAccessFamily[] rafs) {
        if (rafs.length != mPhones.length) {
            throw new RuntimeException("Length of input rafs must equal to total phone count");
        }
        // Check if there is any ongoing transaction and throw an exception if there
        // is one as this is a programming error.
        synchronized (mSetRadioAccessFamilyStatus) {
            for (int i = 0; i < mPhones.length; i++) {
                if (mSetRadioAccessFamilyStatus[i] != SET_RC_STATUS_IDLE) {
                    // TODO: The right behaviour is to cancel previous request and send this.
                    loge("setRadioCapability: Phone[" + i + "] is not idle. Rejecting request.");
                    return false;
                }
            }
        }

        // Check we actually need to do anything
        boolean same = true;
        for (int i = 0; i < mPhones.length; i++) {
            if (mPhones[i].getRadioAccessFamily() != rafs[i].getRadioAccessFamily()) {
                same = false;
            }
        }
        if (same) {
            // All phones are already set to the requested raf
            logd("setRadioCapability: Already in requested configuration, nothing to do.");
            // It isn't really an error, so return true - everything is OK.
            return true;
        }

        // Clear to be sure we're in the initial state
        clearTransaction();

        // Keep a wake lock until we finish radio capability changed
        mWakeLock.acquire();

        return doSetRadioCapabilities(rafs);
    }

    private boolean doSetRadioCapabilities(RadioAccessFamily[] rafs) {
        // A new sessionId for this transaction
        mRadioCapabilitySessionId = mUniqueIdGenerator.getAndIncrement();

        // Start timer to make sure all phones respond within a specific time interval.
        // Will send FINISH if a timeout occurs.
        Message msg = mHandler.obtainMessage(EVENT_TIMEOUT, mRadioCapabilitySessionId, 0);
        mHandler.sendMessageDelayed(msg, SET_RC_TIMEOUT_WAITING_MSEC);

        synchronized (mSetRadioAccessFamilyStatus) {
            logd("setRadioCapability: new request session id=" + mRadioCapabilitySessionId);
            resetRadioAccessFamilyStatusCounter();
            for (int i = 0; i < rafs.length; i++) {
                int phoneId = rafs[i].getPhoneId();
                logd("setRadioCapability: phoneId=" + phoneId + " status=STARTING");
                mSetRadioAccessFamilyStatus[phoneId] = SET_RC_STATUS_STARTING;
                mOldRadioAccessFamily[phoneId] = mPhones[phoneId].getRadioAccessFamily();
                int requestedRaf = rafs[i].getRadioAccessFamily();
                // TODO Set the new radio access family to the maximum of the requested & supported
                // int supportedRaf = mPhones[i].getRadioAccessFamily();
                // mNewRadioAccessFamily[phoneId] = requestedRaf & supportedRaf;
                mNewRadioAccessFamily[phoneId] = requestedRaf;

                mCurrentLogicalModemIds[phoneId] = mPhones[phoneId].getModemUuId();
                // get the logical mode corresponds to new raf requested and pass the
                // same as part of SET_RADIO_CAP APPLY phase
                mNewLogicalModemIds[phoneId] = getLogicalModemIdFromRaf(requestedRaf);
                logd("setRadioCapability: mOldRadioAccessFamily[" + phoneId + "]="
                        + mOldRadioAccessFamily[phoneId]);
                logd("setRadioCapability: mNewRadioAccessFamily[" + phoneId + "]="
                        + mNewRadioAccessFamily[phoneId]);
                sendRadioCapabilityRequest(
                        phoneId,
                        mRadioCapabilitySessionId,
                        RadioCapability.RC_PHASE_START,
                        mOldRadioAccessFamily[phoneId],
                        mCurrentLogicalModemIds[phoneId],
                        RadioCapability.RC_STATUS_NONE,
                        EVENT_START_RC_RESPONSE);
            }
        }

        return true;
    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            logd("handleMessage msg.what=" + msg.what);
            switch (msg.what) {
                case EVENT_START_RC_RESPONSE:
                    onStartRadioCapabilityResponse(msg);
                    break;

                case EVENT_APPLY_RC_RESPONSE:
                    onApplyRadioCapabilityResponse(msg);
                    break;

                case EVENT_NOTIFICATION_RC_CHANGED:
                    onNotificationRadioCapabilityChanged(msg);
                    break;

                case EVENT_FINISH_RC_RESPONSE:
                    onFinishRadioCapabilityResponse(msg);
                    break;

                case EVENT_TIMEOUT:
                    onTimeoutRadioCapability(msg);
                    break;

                default:
                    break;
            }
        }
    };

    /**
     * Handle START response
     * @param msg obj field isa RadioCapability
     */
    private void onStartRadioCapabilityResponse(Message msg) {
        synchronized (mSetRadioAccessFamilyStatus) {
            AsyncResult ar = (AsyncResult)msg.obj;
            if (ar.exception != null) {
                // just abort now.  They didn't take our start so we don't have to revert
                logd("onStartRadioCapabilityResponse got exception=" + ar.exception);
                mRadioCapabilitySessionId = mUniqueIdGenerator.getAndIncrement();
                Intent intent = new Intent(TelephonyIntents.ACTION_SET_RADIO_CAPABILITY_FAILED);
                mContext.sendBroadcast(intent);
                clearTransaction();
                return;
            }
            RadioCapability rc = (RadioCapability) ((AsyncResult) msg.obj).result;
            if ((rc == null) || (rc.getSession() != mRadioCapabilitySessionId)) {
                logd("onStartRadioCapabilityResponse: Ignore session=" + mRadioCapabilitySessionId
                        + " rc=" + rc);
                return;
            }
            mRadioAccessFamilyStatusCounter--;
            int id = rc.getPhoneId();
            if (((AsyncResult) msg.obj).exception != null) {
                logd("onStartRadioCapabilityResponse: Error response session=" + rc.getSession());
                logd("onStartRadioCapabilityResponse: phoneId=" + id + " status=FAIL");
                mSetRadioAccessFamilyStatus[id] = SET_RC_STATUS_FAIL;
                mTransactionFailed = true;
            } else {
                logd("onStartRadioCapabilityResponse: phoneId=" + id + " status=STARTED");
                mSetRadioAccessFamilyStatus[id] = SET_RC_STATUS_STARTED;
            }

            if (mRadioAccessFamilyStatusCounter == 0) {
                HashSet<String> modemsInUse = new HashSet<String>(mNewLogicalModemIds.length);
                for (String modemId : mNewLogicalModemIds) {
                    if (!modemsInUse.add(modemId)) {
                        mTransactionFailed = true;
                        Log.wtf(LOG_TAG, "ERROR: sending down the same id for different phones");
                    }
                }
                logd("onStartRadioCapabilityResponse: success=" + !mTransactionFailed);
                if (mTransactionFailed) {
                    // Sends a variable number of requests, so don't resetRadioAccessFamilyCounter
                    // here.
                    issueFinish(mRadioCapabilitySessionId);
                } else {
                    // All logical modem accepted the new radio access family, issue the APPLY
                    resetRadioAccessFamilyStatusCounter();
                    for (int i = 0; i < mPhones.length; i++) {
                        sendRadioCapabilityRequest(
                            i,
                            mRadioCapabilitySessionId,
                            RadioCapability.RC_PHASE_APPLY,
                            mNewRadioAccessFamily[i],
                            mNewLogicalModemIds[i],
                            RadioCapability.RC_STATUS_NONE,
                            EVENT_APPLY_RC_RESPONSE);

                        logd("onStartRadioCapabilityResponse: phoneId=" + i + " status=APPLYING");
                        mSetRadioAccessFamilyStatus[i] = SET_RC_STATUS_APPLYING;
                    }
                }
            }
        }
    }

    /**
     * Handle APPLY response
     * @param msg obj field isa RadioCapability
     */
    private void onApplyRadioCapabilityResponse(Message msg) {
        RadioCapability rc = (RadioCapability) ((AsyncResult) msg.obj).result;
        if ((rc == null) || (rc.getSession() != mRadioCapabilitySessionId)) {
            logd("onApplyRadioCapabilityResponse: Ignore session=" + mRadioCapabilitySessionId
                    + " rc=" + rc);
            return;
        }
        logd("onApplyRadioCapabilityResponse: rc=" + rc);
        if (((AsyncResult) msg.obj).exception != null) {
            synchronized (mSetRadioAccessFamilyStatus) {
                logd("onApplyRadioCapabilityResponse: Error response session=" + rc.getSession());
                int id = rc.getPhoneId();
                logd("onApplyRadioCapabilityResponse: phoneId=" + id + " status=FAIL");
                mSetRadioAccessFamilyStatus[id] = SET_RC_STATUS_FAIL;
                mTransactionFailed = true;
            }
        } else {
            logd("onApplyRadioCapabilityResponse: Valid start expecting notification rc=" + rc);
        }
    }

    /**
     * Handle the notification unsolicited response associated with the APPLY
     * @param msg obj field isa RadioCapability
     */
    private void onNotificationRadioCapabilityChanged(Message msg) {
        RadioCapability rc = (RadioCapability) ((AsyncResult) msg.obj).result;
        if ((rc == null) || (rc.getSession() != mRadioCapabilitySessionId)) {
            logd("onNotificationRadioCapabilityChanged: Ignore session=" + mRadioCapabilitySessionId
                    + " rc=" + rc);
            return;
        }
        synchronized (mSetRadioAccessFamilyStatus) {
            logd("onNotificationRadioCapabilityChanged: rc=" + rc);
            // skip the overdue response by checking sessionId
            if (rc.getSession() != mRadioCapabilitySessionId) {
                logd("onNotificationRadioCapabilityChanged: Ignore session="
                        + mRadioCapabilitySessionId + " rc=" + rc);
                return;
            }

            int id = rc.getPhoneId();
            if ((((AsyncResult) msg.obj).exception != null) ||
                    (rc.getStatus() == RadioCapability.RC_STATUS_FAIL)) {
                logd("onNotificationRadioCapabilityChanged: phoneId=" + id + " status=FAIL");
                mSetRadioAccessFamilyStatus[id] = SET_RC_STATUS_FAIL;
                mTransactionFailed = true;
            } else {
                logd("onNotificationRadioCapabilityChanged: phoneId=" + id + " status=SUCCESS");
                mSetRadioAccessFamilyStatus[id] = SET_RC_STATUS_SUCCESS;
                // The modems may have been restarted and forgotten this
                mPhoneSwitcher.resendDataAllowed(id);
                mPhones[id].radioCapabilityUpdated(rc);
            }

            mRadioAccessFamilyStatusCounter--;
            if (mRadioAccessFamilyStatusCounter == 0) {
                logd("onNotificationRadioCapabilityChanged: APPLY URC success=" +
                        mTransactionFailed);
                issueFinish(mRadioCapabilitySessionId);
            }
        }
    }

    /**
     * Handle the FINISH Phase response
     * @param msg obj field isa RadioCapability
     */
    void onFinishRadioCapabilityResponse(Message msg) {
        RadioCapability rc = (RadioCapability) ((AsyncResult) msg.obj).result;
        if ((rc == null) || (rc.getSession() != mRadioCapabilitySessionId)) {
            logd("onFinishRadioCapabilityResponse: Ignore session=" + mRadioCapabilitySessionId
                    + " rc=" + rc);
            return;
        }
        synchronized (mSetRadioAccessFamilyStatus) {
            logd(" onFinishRadioCapabilityResponse mRadioAccessFamilyStatusCounter="
                    + mRadioAccessFamilyStatusCounter);
            mRadioAccessFamilyStatusCounter--;
            if (mRadioAccessFamilyStatusCounter == 0) {
                completeRadioCapabilityTransaction();
            }
        }
    }

    private void onTimeoutRadioCapability(Message msg) {
        if (msg.arg1 != mRadioCapabilitySessionId) {
           logd("RadioCapability timeout: Ignore msg.arg1=" + msg.arg1 +
                   "!= mRadioCapabilitySessionId=" + mRadioCapabilitySessionId);
            return;
        }

        synchronized(mSetRadioAccessFamilyStatus) {
            // timed-out.  Clean up as best we can
            for (int i = 0; i < mPhones.length; i++) {
                logd("RadioCapability timeout: mSetRadioAccessFamilyStatus[" + i + "]=" +
                        mSetRadioAccessFamilyStatus[i]);
            }

            // Increment the sessionId as we are completing the transaction below
            // so we don't want it completed when the FINISH phase is done.
            int uniqueDifferentId = mUniqueIdGenerator.getAndIncrement();
            // send FINISH request with fail status and then uniqueDifferentId
            mTransactionFailed = true;
            issueFinish(uniqueDifferentId);
        }
    }

    private void issueFinish(int sessionId) {
        // Issue FINISH
        synchronized(mSetRadioAccessFamilyStatus) {
            for (int i = 0; i < mPhones.length; i++) {
                logd("issueFinish: phoneId=" + i + " sessionId=" + sessionId
                        + " mTransactionFailed=" + mTransactionFailed);
                mRadioAccessFamilyStatusCounter++;
                sendRadioCapabilityRequest(
                        i,
                        sessionId,
                        RadioCapability.RC_PHASE_FINISH,
                        mOldRadioAccessFamily[i],
                        mCurrentLogicalModemIds[i],
                        (mTransactionFailed ? RadioCapability.RC_STATUS_FAIL :
                        RadioCapability.RC_STATUS_SUCCESS),
                        EVENT_FINISH_RC_RESPONSE);
                if (mTransactionFailed) {
                    logd("issueFinish: phoneId: " + i + " status: FAIL");
                    // At least one failed, mark them all failed.
                    mSetRadioAccessFamilyStatus[i] = SET_RC_STATUS_FAIL;
                }
            }
        }
    }

    private void completeRadioCapabilityTransaction() {
        // Create the intent to broadcast
        Intent intent;
        logd("onFinishRadioCapabilityResponse: success=" + !mTransactionFailed);
        if (!mTransactionFailed) {
            ArrayList<RadioAccessFamily> phoneRAFList = new ArrayList<RadioAccessFamily>();
            for (int i = 0; i < mPhones.length; i++) {
                int raf = mPhones[i].getRadioAccessFamily();
                logd("radioAccessFamily[" + i + "]=" + raf);
                RadioAccessFamily phoneRC = new RadioAccessFamily(i, raf);
                phoneRAFList.add(phoneRC);
            }
            intent = new Intent(TelephonyIntents.ACTION_SET_RADIO_CAPABILITY_DONE);
            intent.putParcelableArrayListExtra(TelephonyIntents.EXTRA_RADIO_ACCESS_FAMILY,
                    phoneRAFList);

            // make messages about the old transaction obsolete (specifically the timeout)
            mRadioCapabilitySessionId = mUniqueIdGenerator.getAndIncrement();

            // Reinitialize
            clearTransaction();
        } else {
            intent = new Intent(TelephonyIntents.ACTION_SET_RADIO_CAPABILITY_FAILED);

            // now revert.
            mTransactionFailed = false;
            RadioAccessFamily[] rafs = new RadioAccessFamily[mPhones.length];
            for (int phoneId = 0; phoneId < mPhones.length; phoneId++) {
                rafs[phoneId] = new RadioAccessFamily(phoneId, mOldRadioAccessFamily[phoneId]);
            }
            doSetRadioCapabilities(rafs);
        }

        // Broadcast that we're done
        mContext.sendBroadcast(intent, android.Manifest.permission.READ_PHONE_STATE);
    }

    // Clear this transaction
    private void clearTransaction() {
        logd("clearTransaction");
        synchronized(mSetRadioAccessFamilyStatus) {
            for (int i = 0; i < mPhones.length; i++) {
                logd("clearTransaction: phoneId=" + i + " status=IDLE");
                mSetRadioAccessFamilyStatus[i] = SET_RC_STATUS_IDLE;
                mOldRadioAccessFamily[i] = 0;
                mNewRadioAccessFamily[i] = 0;
                mTransactionFailed = false;
            }

            if (mWakeLock.isHeld()) {
                mWakeLock.release();
            }
        }
    }

    private void resetRadioAccessFamilyStatusCounter() {
        mRadioAccessFamilyStatusCounter = mPhones.length;
    }

    private void sendRadioCapabilityRequest(int phoneId, int sessionId, int rcPhase,
            int radioFamily, String logicalModemId, int status, int eventId) {
        RadioCapability requestRC = new RadioCapability(
                phoneId, sessionId, rcPhase, radioFamily, logicalModemId, status);
        mPhones[phoneId].setRadioCapability(
                requestRC, mHandler.obtainMessage(eventId));
    }

    // This method will return max number of raf bits supported from the raf
    // values currently stored in all phone objects
    public int getMaxRafSupported() {
        int[] numRafSupported = new int[mPhones.length];
        int maxNumRafBit = 0;
        int maxRaf = RadioAccessFamily.RAF_UNKNOWN;

        for (int len = 0; len < mPhones.length; len++) {
            numRafSupported[len] = Integer.bitCount(mPhones[len].getRadioAccessFamily());
            if (maxNumRafBit < numRafSupported[len]) {
                maxNumRafBit = numRafSupported[len];
                maxRaf = mPhones[len].getRadioAccessFamily();
            }
        }

        return maxRaf;
    }

    // This method will return minimum number of raf bits supported from the raf
    // values currently stored in all phone objects
    public int getMinRafSupported() {
        int[] numRafSupported = new int[mPhones.length];
        int minNumRafBit = 0;
        int minRaf = RadioAccessFamily.RAF_UNKNOWN;

        for (int len = 0; len < mPhones.length; len++) {
            numRafSupported[len] = Integer.bitCount(mPhones[len].getRadioAccessFamily());
            if ((minNumRafBit == 0) || (minNumRafBit > numRafSupported[len])) {
                minNumRafBit = numRafSupported[len];
                minRaf = mPhones[len].getRadioAccessFamily();
            }
        }
        return minRaf;
    }

    // This method checks current raf values stored in all phones and
    // whicheve phone raf matches with input raf, returns modemId from that phone
    private String getLogicalModemIdFromRaf(int raf) {
        String modemUuid = null;

        for (int phoneId = 0; phoneId < mPhones.length; phoneId++) {
            if (mPhones[phoneId].getRadioAccessFamily() == raf) {
                modemUuid = mPhones[phoneId].getModemUuId();
                break;
            }
        }
        return modemUuid;
    }

    private void logd(String string) {
        Rlog.d(LOG_TAG, string);
    }

    private void loge(String string) {
        Rlog.e(LOG_TAG, string);
    }
}
