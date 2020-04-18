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

package com.android.internal.telephony;

import static com.android.internal.telephony.TelephonyProperties.PROPERTY_DEFAULT_SUBSCRIPTION;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.LocalServerSocket;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.telephony.Rlog;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.LocalLog;

import com.android.internal.telephony.cdma.CdmaSubscriptionSourceManager;
import com.android.internal.telephony.dataconnection.TelephonyNetworkFactory;
import com.android.internal.telephony.imsphone.ImsPhone;
import com.android.internal.telephony.imsphone.ImsPhoneFactory;
import com.android.internal.telephony.sip.SipPhone;
import com.android.internal.telephony.sip.SipPhoneFactory;
import com.android.internal.telephony.uicc.IccCardProxy;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.util.IndentingPrintWriter;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.HashMap;

/**
 * {@hide}
 */
public class PhoneFactory {
    static final String LOG_TAG = "PhoneFactory";
    static final int SOCKET_OPEN_RETRY_MILLIS = 2 * 1000;
    static final int SOCKET_OPEN_MAX_RETRY = 3;
    static final boolean DBG = false;

    //***** Class Variables

    // lock sLockProxyPhones protects both sPhones and sPhone
    final static Object sLockProxyPhones = new Object();
    static private Phone[] sPhones = null;
    static private Phone sPhone = null;

    static private CommandsInterface[] sCommandsInterfaces = null;

    static private ProxyController sProxyController;
    static private UiccController sUiccController;

    static private CommandsInterface sCommandsInterface = null;
    static private SubscriptionInfoUpdater sSubInfoRecordUpdater = null;

    static private boolean sMadeDefaults = false;
    static private PhoneNotifier sPhoneNotifier;
    static private Context sContext;
    static private PhoneSwitcher sPhoneSwitcher;
    static private SubscriptionMonitor sSubscriptionMonitor;
    static private TelephonyNetworkFactory[] sTelephonyNetworkFactories;

    static private final HashMap<String, LocalLog>sLocalLogs = new HashMap<String, LocalLog>();

    // TODO - make this a dynamic property read from the modem
    public static final int MAX_ACTIVE_PHONES = 1;

    //***** Class Methods

    public static void makeDefaultPhones(Context context) {
        makeDefaultPhone(context);
    }

    /**
     * FIXME replace this with some other way of making these
     * instances
     */
    public static void makeDefaultPhone(Context context) {
        synchronized (sLockProxyPhones) {
            if (!sMadeDefaults) {
                sContext = context;

                // create the telephony device controller.
                TelephonyDevController.create();

                int retryCount = 0;
                for(;;) {
                    boolean hasException = false;
                    retryCount ++;

                    try {
                        // use UNIX domain socket to
                        // prevent subsequent initialization
                        new LocalServerSocket("com.android.internal.telephony");
                    } catch (java.io.IOException ex) {
                        hasException = true;
                    }

                    if ( !hasException ) {
                        break;
                    } else if (retryCount > SOCKET_OPEN_MAX_RETRY) {
                        throw new RuntimeException("PhoneFactory probably already running");
                    } else {
                        try {
                            Thread.sleep(SOCKET_OPEN_RETRY_MILLIS);
                        } catch (InterruptedException er) {
                        }
                    }
                }

                sPhoneNotifier = new DefaultPhoneNotifier();

                int cdmaSubscription = CdmaSubscriptionSourceManager.getDefault(context);
                Rlog.i(LOG_TAG, "Cdma Subscription set to " + cdmaSubscription);

                /* In case of multi SIM mode two instances of Phone, RIL are created,
                   where as in single SIM mode only instance. isMultiSimEnabled() function checks
                   whether it is single SIM or multi SIM mode */
                int numPhones = TelephonyManager.getDefault().getPhoneCount();
                int[] networkModes = new int[numPhones];
                sPhones = new Phone[numPhones];
                sCommandsInterfaces = new RIL[numPhones];
                sTelephonyNetworkFactories = new TelephonyNetworkFactory[numPhones];

                for (int i = 0; i < numPhones; i++) {
                    // reads the system properties and makes commandsinterface
                    // Get preferred network type.
                    networkModes[i] = RILConstants.PREFERRED_NETWORK_MODE;

                    Rlog.i(LOG_TAG, "Network Mode set to " + Integer.toString(networkModes[i]));
                    sCommandsInterfaces[i] = new RIL(context, networkModes[i],
                            cdmaSubscription, i);
                }
                Rlog.i(LOG_TAG, "Creating SubscriptionController");
                SubscriptionController.init(context, sCommandsInterfaces);

                // Instantiate UiccController so that all other classes can just
                // call getInstance()
                sUiccController = UiccController.make(context, sCommandsInterfaces);

                for (int i = 0; i < numPhones; i++) {
                    Phone phone = null;
                    int phoneType = TelephonyManager.getPhoneType(networkModes[i]);
                    if (phoneType == PhoneConstants.PHONE_TYPE_GSM) {
                        phone = new GsmCdmaPhone(context,
                                sCommandsInterfaces[i], sPhoneNotifier, i,
                                PhoneConstants.PHONE_TYPE_GSM,
                                TelephonyComponentFactory.getInstance());
                    } else if (phoneType == PhoneConstants.PHONE_TYPE_CDMA) {
                        phone = new GsmCdmaPhone(context,
                                sCommandsInterfaces[i], sPhoneNotifier, i,
                                PhoneConstants.PHONE_TYPE_CDMA_LTE,
                                TelephonyComponentFactory.getInstance());
                    }
                    Rlog.i(LOG_TAG, "Creating Phone with type = " + phoneType + " sub = " + i);

                    sPhones[i] = phone;
                }

                // Set the default phone in base class.
                // FIXME: This is a first best guess at what the defaults will be. It
                // FIXME: needs to be done in a more controlled manner in the future.
                sPhone = sPhones[0];
                sCommandsInterface = sCommandsInterfaces[0];

                // Ensure that we have a default SMS app. Requesting the app with
                // updateIfNeeded set to true is enough to configure a default SMS app.
                ComponentName componentName =
                        SmsApplication.getDefaultSmsApplication(context, true /* updateIfNeeded */);
                String packageName = "NONE";
                if (componentName != null) {
                    packageName = componentName.getPackageName();
                }
                Rlog.i(LOG_TAG, "defaultSmsApplication: " + packageName);

                // Set up monitor to watch for changes to SMS packages
                SmsApplication.initSmsPackageMonitor(context);

                sMadeDefaults = true;

                Rlog.i(LOG_TAG, "Creating SubInfoRecordUpdater ");
                sSubInfoRecordUpdater = new SubscriptionInfoUpdater(context,
                        sPhones, sCommandsInterfaces);
                SubscriptionController.getInstance().updatePhonesAvailability(sPhones);

                // Start monitoring after defaults have been made.
                // Default phone must be ready before ImsPhone is created
                // because ImsService might need it when it is being opened.
                for (int i = 0; i < numPhones; i++) {
                    sPhones[i].startMonitoringImsService();
                }

                ITelephonyRegistry tr = ITelephonyRegistry.Stub.asInterface(
                        ServiceManager.getService("telephony.registry"));
                SubscriptionController sc = SubscriptionController.getInstance();

                sSubscriptionMonitor = new SubscriptionMonitor(tr, sContext, sc, numPhones);

                sPhoneSwitcher = new PhoneSwitcher(MAX_ACTIVE_PHONES, numPhones,
                        sContext, sc, Looper.myLooper(), tr, sCommandsInterfaces,
                        sPhones);

                sProxyController = ProxyController.getInstance(context, sPhones,
                        sUiccController, sCommandsInterfaces, sPhoneSwitcher);

                sTelephonyNetworkFactories = new TelephonyNetworkFactory[numPhones];
                for (int i = 0; i < numPhones; i++) {
                    sTelephonyNetworkFactories[i] = new TelephonyNetworkFactory(
                            sPhoneSwitcher, sc, sSubscriptionMonitor, Looper.myLooper(),
                            sContext, i, sPhones[i].mDcTracker);
                }
            }
        }
    }

    public static Phone getDefaultPhone() {
        synchronized (sLockProxyPhones) {
            if (!sMadeDefaults) {
                throw new IllegalStateException("Default phones haven't been made yet!");
            }
            return sPhone;
        }
    }

    public static Phone getPhone(int phoneId) {
        Phone phone;
        String dbgInfo = "";

        synchronized (sLockProxyPhones) {
            if (!sMadeDefaults) {
                throw new IllegalStateException("Default phones haven't been made yet!");
                // CAF_MSIM FIXME need to introduce default phone id ?
            } else if (phoneId == SubscriptionManager.DEFAULT_PHONE_INDEX) {
                if (DBG) dbgInfo = "phoneId == DEFAULT_PHONE_ID return sPhone";
                phone = sPhone;
            } else {
                if (DBG) dbgInfo = "phoneId != DEFAULT_PHONE_ID return sPhones[phoneId]";
                phone = (((phoneId >= 0)
                                && (phoneId < TelephonyManager.getDefault().getPhoneCount()))
                        ? sPhones[phoneId] : null);
            }
            if (DBG) {
                Rlog.d(LOG_TAG, "getPhone:- " + dbgInfo + " phoneId=" + phoneId +
                        " phone=" + phone);
            }
            return phone;
        }
    }

    public static Phone[] getPhones() {
        synchronized (sLockProxyPhones) {
            if (!sMadeDefaults) {
                throw new IllegalStateException("Default phones haven't been made yet!");
            }
            return sPhones;
        }
    }

    /**
     * Makes a {@link SipPhone} object.
     * @param sipUri the local SIP URI the phone runs on
     * @return the {@code SipPhone} object or null if the SIP URI is not valid
     */
    public static SipPhone makeSipPhone(String sipUri) {
        return SipPhoneFactory.makePhone(sipUri, sContext, sPhoneNotifier);
    }

    /**
     * Returns the preferred network type that should be set in the modem.
     *
     * @param context The current {@link Context}.
     * @return the preferred network mode that should be set.
     */
    // TODO: Fix when we "properly" have TelephonyDevController/SubscriptionController ..
    public static int calculatePreferredNetworkType(Context context, int phoneSubId) {
        int networkType = android.provider.Settings.Global.getInt(context.getContentResolver(),
                android.provider.Settings.Global.PREFERRED_NETWORK_MODE + phoneSubId,
                RILConstants.PREFERRED_NETWORK_MODE);
        Rlog.d(LOG_TAG, "calculatePreferredNetworkType: phoneSubId = " + phoneSubId +
                " networkType = " + networkType);
        return networkType;
    }

    /* Gets the default subscription */
    public static int getDefaultSubscription() {
        return SubscriptionController.getInstance().getDefaultSubId();
    }

    /* Returns User SMS Prompt property,  enabled or not */
    public static boolean isSMSPromptEnabled() {
        boolean prompt = false;
        int value = 0;
        try {
            value = Settings.Global.getInt(sContext.getContentResolver(),
                    Settings.Global.MULTI_SIM_SMS_PROMPT);
        } catch (SettingNotFoundException snfe) {
            Rlog.e(LOG_TAG, "Settings Exception Reading Dual Sim SMS Prompt Values");
        }
        prompt = (value == 0) ? false : true ;
        Rlog.d(LOG_TAG, "SMS Prompt option:" + prompt);

       return prompt;
    }

    /**
     * Makes a {@link ImsPhone} object.
     * @return the {@code ImsPhone} object or null if the exception occured
     */
    public static Phone makeImsPhone(PhoneNotifier phoneNotifier, Phone defaultPhone) {
        return ImsPhoneFactory.makePhone(sContext, phoneNotifier, defaultPhone);
    }

    /**
     * Adds a local log category.
     *
     * Only used within the telephony process.  Use localLog to add log entries.
     *
     * TODO - is there a better way to do this?  Think about design when we have a minute.
     *
     * @param key the name of the category - will be the header in the service dump.
     * @param size the number of lines to maintain in this category
     */
    public static void addLocalLog(String key, int size) {
        synchronized(sLocalLogs) {
            if (sLocalLogs.containsKey(key)) {
                throw new IllegalArgumentException("key " + key + " already present");
            }
            sLocalLogs.put(key, new LocalLog(size));
        }
    }

    /**
     * Add a line to the named Local Log.
     *
     * This will appear in the TelephonyDebugService dump.
     *
     * @param key the name of the log category to put this in.  Must be created
     *            via addLocalLog.
     * @param log the string to add to the log.
     */
    public static void localLog(String key, String log) {
        synchronized(sLocalLogs) {
            if (sLocalLogs.containsKey(key) == false) {
                throw new IllegalArgumentException("key " + key + " not found");
            }
            sLocalLogs.get(key).log(log);
        }
    }

    public static void dump(FileDescriptor fd, PrintWriter printwriter, String[] args) {
        IndentingPrintWriter pw = new IndentingPrintWriter(printwriter, "  ");
        pw.println("PhoneFactory:");
        pw.println(" sMadeDefaults=" + sMadeDefaults);

        sPhoneSwitcher.dump(fd, pw, args);
        pw.println();

        Phone[] phones = (Phone[])PhoneFactory.getPhones();
        for (int i = 0; i < phones.length; i++) {
            pw.increaseIndent();
            Phone phone = phones[i];

            try {
                phone.dump(fd, pw, args);
            } catch (Exception e) {
                pw.println("Telephony DebugService: Could not get Phone[" + i + "] e=" + e);
                continue;
            }

            pw.flush();
            pw.println("++++++++++++++++++++++++++++++++");

            sTelephonyNetworkFactories[i].dump(fd, pw, args);

            pw.flush();
            pw.println("++++++++++++++++++++++++++++++++");

            try {
                ((IccCardProxy)phone.getIccCard()).dump(fd, pw, args);
            } catch (Exception e) {
                e.printStackTrace();
            }
            pw.flush();
            pw.decreaseIndent();
            pw.println("++++++++++++++++++++++++++++++++");
        }

        pw.println("SubscriptionMonitor:");
        pw.increaseIndent();
        try {
            sSubscriptionMonitor.dump(fd, pw, args);
        } catch (Exception e) {
            e.printStackTrace();
        }
        pw.decreaseIndent();
        pw.println("++++++++++++++++++++++++++++++++");

        pw.println("UiccController:");
        pw.increaseIndent();
        try {
            sUiccController.dump(fd, pw, args);
        } catch (Exception e) {
            e.printStackTrace();
        }
        pw.flush();
        pw.decreaseIndent();
        pw.println("++++++++++++++++++++++++++++++++");

        pw.println("SubscriptionController:");
        pw.increaseIndent();
        try {
            SubscriptionController.getInstance().dump(fd, pw, args);
        } catch (Exception e) {
            e.printStackTrace();
        }
        pw.flush();
        pw.decreaseIndent();
        pw.println("++++++++++++++++++++++++++++++++");

        pw.println("SubInfoRecordUpdater:");
        pw.increaseIndent();
        try {
            sSubInfoRecordUpdater.dump(fd, pw, args);
        } catch (Exception e) {
            e.printStackTrace();
        }
        pw.flush();
        pw.decreaseIndent();
        pw.println("++++++++++++++++++++++++++++++++");

        pw.println("LocalLogs:");
        pw.increaseIndent();
        synchronized (sLocalLogs) {
            for (String key : sLocalLogs.keySet()) {
                pw.println(key);
                pw.increaseIndent();
                sLocalLogs.get(key).dump(fd, pw, args);
                pw.decreaseIndent();
            }
            pw.flush();
        }
        pw.decreaseIndent();
    }
}
