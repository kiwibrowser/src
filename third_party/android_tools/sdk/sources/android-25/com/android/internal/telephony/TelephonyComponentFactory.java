/*
 * Copyright (C) 2015 The Android Open Source Project
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

import android.content.Context;
import android.database.Cursor;
import android.os.Handler;
import android.os.IDeviceIdleController;
import android.os.PowerManager;
import android.os.ServiceManager;

import com.android.ims.ImsManager;
import com.android.internal.telephony.cdma.CdmaSubscriptionSourceManager;
import com.android.internal.telephony.cdma.EriManager;
import com.android.internal.telephony.dataconnection.DcTracker;
import com.android.internal.telephony.imsphone.ImsExternalCallTracker;
import com.android.internal.telephony.imsphone.ImsPhone;
import com.android.internal.telephony.imsphone.ImsPhoneCallTracker;
import com.android.internal.telephony.imsphone.ImsPullCall;
import com.android.internal.telephony.uicc.IccCardProxy;

/**
 * This class has one-line methods to instantiate objects only. The purpose is to make code
 * unit-test friendly and use this class as a way to do dependency injection. Instantiating objects
 * this way makes it easier to mock them in tests.
 */
public class TelephonyComponentFactory {
    private static TelephonyComponentFactory sInstance;

    public static TelephonyComponentFactory getInstance() {
        if (sInstance == null) {
            sInstance = new TelephonyComponentFactory();
        }
        return sInstance;
    }

    public GsmCdmaCallTracker makeGsmCdmaCallTracker(GsmCdmaPhone phone) {
        return new GsmCdmaCallTracker(phone);
    }

    public SmsStorageMonitor makeSmsStorageMonitor(Phone phone) {
        return new SmsStorageMonitor(phone);
    }

    public SmsUsageMonitor makeSmsUsageMonitor(Context context) {
        return new SmsUsageMonitor(context);
    }

    public ServiceStateTracker makeServiceStateTracker(GsmCdmaPhone phone, CommandsInterface ci) {
        return new ServiceStateTracker(phone, ci);
    }

    public DcTracker makeDcTracker(Phone phone) {
        return new DcTracker(phone);
    }

    public IccPhoneBookInterfaceManager makeIccPhoneBookInterfaceManager(Phone phone) {
        return new IccPhoneBookInterfaceManager(phone);
    }

    public IccSmsInterfaceManager makeIccSmsInterfaceManager(Phone phone) {
        return new IccSmsInterfaceManager(phone);
    }

    public IccCardProxy makeIccCardProxy(Context context, CommandsInterface ci, int phoneId) {
        return new IccCardProxy(context, ci, phoneId);
    }

    public EriManager makeEriManager(Phone phone, Context context, int eriFileSource) {
        return new EriManager(phone, context, eriFileSource);
    }

    public WspTypeDecoder makeWspTypeDecoder(byte[] pdu) {
        return new WspTypeDecoder(pdu);
    }

    /**
     * Create a tracker for a single-part SMS.
     */
    public InboundSmsTracker makeInboundSmsTracker(byte[] pdu, long timestamp, int destPort,
            boolean is3gpp2, boolean is3gpp2WapPdu, String address, String messageBody) {
        return new InboundSmsTracker(pdu, timestamp, destPort, is3gpp2, is3gpp2WapPdu, address,
                messageBody);
    }

    /**
     * Create a tracker for a multi-part SMS.
     */
    public InboundSmsTracker makeInboundSmsTracker(byte[] pdu, long timestamp, int destPort,
            boolean is3gpp2, String address, int referenceNumber, int sequenceNumber,
            int messageCount, boolean is3gpp2WapPdu, String messageBody) {
        return new InboundSmsTracker(pdu, timestamp, destPort, is3gpp2, address, referenceNumber,
                sequenceNumber, messageCount, is3gpp2WapPdu, messageBody);
    }

    /**
     * Create a tracker from a row of raw table
     */
    public InboundSmsTracker makeInboundSmsTracker(Cursor cursor, boolean isCurrentFormat3gpp2) {
        return new InboundSmsTracker(cursor, isCurrentFormat3gpp2);
    }

    public ImsPhoneCallTracker makeImsPhoneCallTracker(ImsPhone imsPhone) {
        return new ImsPhoneCallTracker(imsPhone);
    }

    public ImsExternalCallTracker makeImsExternalCallTracker(ImsPhone imsPhone) {

        return new ImsExternalCallTracker(imsPhone);
    }

    public CdmaSubscriptionSourceManager
    getCdmaSubscriptionSourceManagerInstance(Context context, CommandsInterface ci, Handler h,
                                             int what, Object obj) {
        return CdmaSubscriptionSourceManager.getInstance(context, ci, h, what, obj);
    }

    public IDeviceIdleController getIDeviceIdleController() {
        return IDeviceIdleController.Stub.asInterface(
                ServiceManager.getService(Context.DEVICE_IDLE_CONTROLLER));
    }
}
