/*
 * Copyright (c) 2011 The Android Open Source Project
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

package com.android.internal.telephony.cdma;

import java.util.concurrent.atomic.AtomicInteger;

import com.android.internal.telephony.CommandsInterface;
import android.content.Context;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.os.Registrant;
import android.os.RegistrantList;
import android.provider.Settings;
import android.telephony.Rlog;

/**
 * Class that handles the CDMA subscription source changed events from RIL
 */
public class CdmaSubscriptionSourceManager extends Handler {
    static final String LOG_TAG = "CdmaSSM";
    private static final int EVENT_CDMA_SUBSCRIPTION_SOURCE_CHANGED = 1;
    private static final int EVENT_GET_CDMA_SUBSCRIPTION_SOURCE     = 2;
    private static final int EVENT_RADIO_ON                         = 3;
    private static final int EVENT_SUBSCRIPTION_STATUS_CHANGED      = 4;

    // To know subscription is activated
    private static final int SUBSCRIPTION_ACTIVATED                 = 1;

    public static final int SUBSCRIPTION_SOURCE_UNKNOWN = -1;
    public static final int SUBSCRIPTION_FROM_RUIM      = 0; /* CDMA subscription from RUIM */
    public static final int SUBSCRIPTION_FROM_NV        = 1; /* CDMA subscription from NV */
    public static final int PREFERRED_CDMA_SUBSCRIPTION = SUBSCRIPTION_FROM_RUIM;

    private static CdmaSubscriptionSourceManager sInstance;
    private static final Object sReferenceCountMonitor = new Object();
    private static int sReferenceCount = 0;

    // ***** Instance Variables
    private CommandsInterface mCi;
    private RegistrantList mCdmaSubscriptionSourceChangedRegistrants = new RegistrantList();

    // Type of CDMA subscription source
    private AtomicInteger mCdmaSubscriptionSource = new AtomicInteger(SUBSCRIPTION_FROM_NV);

    // Constructor
    private CdmaSubscriptionSourceManager(Context context, CommandsInterface ci) {
        mCi = ci;
        mCi.registerForCdmaSubscriptionChanged(this, EVENT_CDMA_SUBSCRIPTION_SOURCE_CHANGED, null);
        mCi.registerForOn(this, EVENT_RADIO_ON, null);
        int subscriptionSource = getDefault(context);
        log("cdmaSSM constructor: " + subscriptionSource);
        mCdmaSubscriptionSource.set(subscriptionSource);
        mCi.registerForSubscriptionStatusChanged(this, EVENT_SUBSCRIPTION_STATUS_CHANGED, null);
    }

    /**
     * This function creates a single instance of this class
     *
     * @return object of type CdmaSubscriptionSourceManager
     */
    public static CdmaSubscriptionSourceManager getInstance(Context context,
            CommandsInterface ci, Handler h, int what, Object obj) {
        synchronized (sReferenceCountMonitor) {
            if (null == sInstance) {
                sInstance = new CdmaSubscriptionSourceManager(context, ci);
            }
            CdmaSubscriptionSourceManager.sReferenceCount++;
        }
        sInstance.registerForCdmaSubscriptionSourceChanged(h, what, obj);
        return sInstance;
    }

    /**
     * Unregisters for the registered event with RIL
     */
    public void dispose(Handler h) {
        mCdmaSubscriptionSourceChangedRegistrants.remove(h);
        synchronized (sReferenceCountMonitor) {
            sReferenceCount--;
            if (sReferenceCount <= 0) {
                mCi.unregisterForCdmaSubscriptionChanged(this);
                mCi.unregisterForOn(this);
                mCi.unregisterForSubscriptionStatusChanged(this);
                sInstance = null;
            }
        }
    }

    /*
     * (non-Javadoc)
     * @see android.os.Handler#handleMessage(android.os.Message)
     */
    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        switch (msg.what) {
            case EVENT_CDMA_SUBSCRIPTION_SOURCE_CHANGED:
            case EVENT_GET_CDMA_SUBSCRIPTION_SOURCE:
            {
                log("CDMA_SUBSCRIPTION_SOURCE event = " + msg.what);
                ar = (AsyncResult) msg.obj;
                handleGetCdmaSubscriptionSource(ar);
            }
            break;
            case EVENT_RADIO_ON: {
                mCi.getCdmaSubscriptionSource(obtainMessage(EVENT_GET_CDMA_SUBSCRIPTION_SOURCE));
            }
            break;
            case EVENT_SUBSCRIPTION_STATUS_CHANGED: {
                log("EVENT_SUBSCRIPTION_STATUS_CHANGED");
                ar = (AsyncResult)msg.obj;
                if (ar.exception == null) {
                    int actStatus = ((int[])ar.result)[0];
                    log("actStatus = " + actStatus);
                    if (actStatus == SUBSCRIPTION_ACTIVATED) { // Subscription Activated
                        // In case of multi-SIM, framework should wait for the subscription ready
                        // to send any request to RIL.  Otherwise it will return failure.
                        Rlog.v(LOG_TAG,"get Cdma Subscription Source");
                        mCi.getCdmaSubscriptionSource(
                                obtainMessage(EVENT_GET_CDMA_SUBSCRIPTION_SOURCE));
                    }
                } else {
                    logw("EVENT_SUBSCRIPTION_STATUS_CHANGED, Exception:" + ar.exception);
                }
            }
            break;
            default:
                super.handleMessage(msg);
        }
    }

    /**
     * Returns the current CDMA subscription source value
     * @return CDMA subscription source value
     */
    public int getCdmaSubscriptionSource() {
        log("getcdmasubscriptionSource: " + mCdmaSubscriptionSource.get());
        return mCdmaSubscriptionSource.get();
    }

    /**
     * Gets the default CDMA subscription source
     *
     * @return Default CDMA subscription source from Settings DB if present.
     */
    public static int getDefault(Context context) {
        // Get the default value from the Settings
        int subscriptionSource = Settings.Global.getInt(context.getContentResolver(),
                Settings.Global.CDMA_SUBSCRIPTION_MODE, PREFERRED_CDMA_SUBSCRIPTION);
        Rlog.d(LOG_TAG, "subscriptionSource from settings: " + subscriptionSource);
        return subscriptionSource;
    }

    /**
     * Clients automatically register for CDMA subscription source changed event
     * when they get an instance of this object.
     */
    private void registerForCdmaSubscriptionSourceChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant (h, what, obj);
        mCdmaSubscriptionSourceChangedRegistrants.add(r);
    }

    /**
     * Handles the call to get the subscription source
     *
     * @param ar AsyncResult object that contains the result of get CDMA
     *            subscription source call
     */
    private void handleGetCdmaSubscriptionSource(AsyncResult ar) {
        if ((ar.exception == null) && (ar.result != null)) {
            int newSubscriptionSource = ((int[]) ar.result)[0];

            if (newSubscriptionSource != mCdmaSubscriptionSource.get()) {
                log("Subscription Source Changed : " + mCdmaSubscriptionSource + " >> "
                        + newSubscriptionSource);
                mCdmaSubscriptionSource.set(newSubscriptionSource);

                // Notify registrants of the new CDMA subscription source
                mCdmaSubscriptionSourceChangedRegistrants.notifyRegistrants(new AsyncResult(null,
                        null, null));
            }
        } else {
            // GET_CDMA_SUBSCRIPTION is returning Failure. Probably because modem created GSM Phone.
            logw("Unable to get CDMA Subscription Source, Exception: " + ar.exception
                    + ", result: " + ar.result);
        }
    }

    private void log(String s) {
        Rlog.d(LOG_TAG, s);
    }

    private void logw(String s) {
        Rlog.w(LOG_TAG, s);
    }

}
