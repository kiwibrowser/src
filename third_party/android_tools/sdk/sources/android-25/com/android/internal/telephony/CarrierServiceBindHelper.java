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

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.UserHandle;
import android.service.carrier.CarrierService;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.content.PackageMonitor;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.List;

/**
 * Manages long-lived bindings to carrier services
 * @hide
 */
public class CarrierServiceBindHelper {
    private static final String LOG_TAG = "CarrierSvcBindHelper";

    private Context mContext;
    private AppBinding[] mBindings;
    private String[] mLastSimState;
    private final PackageMonitor mPackageMonitor = new CarrierServicePackageMonitor();

    private BroadcastReceiver mUserUnlockedReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            log("Received " + action);

            if (Intent.ACTION_USER_UNLOCKED.equals(action)) {
                // On user unlock, new components might become available, so reevaluate all
                // bindings.
                for (int phoneId = 0; phoneId < mBindings.length; phoneId++) {
                    mBindings[phoneId].rebind();
                }
            }
        }
    };

    private static final int EVENT_REBIND = 0;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            AppBinding binding;
            log("mHandler: " + msg.what);

            switch (msg.what) {
                case EVENT_REBIND:
                    binding = (AppBinding) msg.obj;
                    log("Rebinding if necessary for phoneId: " + binding.getPhoneId());
                    binding.rebind();
                    break;
            }
        }
    };

    public CarrierServiceBindHelper(Context context) {
        mContext = context;

        int numPhones = TelephonyManager.from(context).getPhoneCount();
        mBindings = new AppBinding[numPhones];
        mLastSimState = new String[numPhones];

        for (int phoneId = 0; phoneId < numPhones; phoneId++) {
            mBindings[phoneId] = new AppBinding(phoneId);
        }

        mPackageMonitor.register(
                context, mHandler.getLooper(), UserHandle.ALL, false /* externalStorage */);
        mContext.registerReceiverAsUser(mUserUnlockedReceiver, UserHandle.SYSTEM,
                new IntentFilter(Intent.ACTION_USER_UNLOCKED), null /* broadcastPermission */,
                mHandler);
    }

    void updateForPhoneId(int phoneId, String simState) {
        log("update binding for phoneId: " + phoneId + " simState: " + simState);
        if (!SubscriptionManager.isValidPhoneId(phoneId)) {
            return;
        }
        if (TextUtils.isEmpty(simState)) return;
        if (simState.equals(mLastSimState[phoneId])) {
            // ignore consecutive duplicated events
            return;
        } else {
            mLastSimState[phoneId] = simState;
        }
        mHandler.sendMessage(mHandler.obtainMessage(EVENT_REBIND, mBindings[phoneId]));
    }

    private class AppBinding {
        private int phoneId;
        private CarrierServiceConnection connection;
        private int bindCount;
        private long lastBindStartMillis;
        private int unbindCount;
        private long lastUnbindMillis;
        private String carrierPackage;
        private String carrierServiceClass;

        public AppBinding(int phoneId) {
            this.phoneId = phoneId;
        }

        public int getPhoneId() {
            return phoneId;
        }

        /** Return the package that is currently being bound to, or null if there is no binding. */
        public String getPackage() {
            return carrierPackage;
        }

        /**
         * Update the bindings for the current carrier app for this phone.
         *
         * <p>Safe to call even if a binding already exists. If the current binding is invalid, it
         * will be dropped. If it is valid, it will be left untouched.
         */
        void rebind() {
            // Get the package name for the carrier app
            List<String> carrierPackageNames =
                TelephonyManager.from(mContext).getCarrierPackageNamesForIntentAndPhone(
                    new Intent(CarrierService.CARRIER_SERVICE_INTERFACE), phoneId
                );

            if (carrierPackageNames == null || carrierPackageNames.size() <= 0) {
                log("No carrier app for: " + phoneId);
                unbind();
                return;
            }

            log("Found carrier app: " + carrierPackageNames);
            String candidateCarrierPackage = carrierPackageNames.get(0);
            // If we are binding to a different package, unbind from the current one.
            if (!TextUtils.equals(carrierPackage, candidateCarrierPackage)) {
                unbind();
            }

            // Look up the carrier service
            Intent carrierService = new Intent(CarrierService.CARRIER_SERVICE_INTERFACE);
            carrierService.setPackage(candidateCarrierPackage);

            ResolveInfo carrierResolveInfo = mContext.getPackageManager().resolveService(
                carrierService, PackageManager.GET_META_DATA);
            Bundle metadata = null;
            String candidateServiceClass = null;
            if (carrierResolveInfo != null) {
                metadata = carrierResolveInfo.serviceInfo.metaData;
                candidateServiceClass =
                        carrierResolveInfo.getComponentInfo().getComponentName().getClassName();
            }

            // Only bind if the service wants it
            if (metadata == null ||
                !metadata.getBoolean("android.service.carrier.LONG_LIVED_BINDING", false)) {
                log("Carrier app does not want a long lived binding");
                unbind();
                return;
            }

            if (!TextUtils.equals(carrierServiceClass, candidateServiceClass)) {
                // Unbind if the carrier service component has changed.
                unbind();
            } else if (connection != null) {
                // Component is unchanged and connection is up - do nothing.
                return;
            }

            carrierPackage = candidateCarrierPackage;
            carrierServiceClass = candidateServiceClass;

            log("Binding to " + carrierPackage + " for phone " + phoneId);

            // Log debug information
            bindCount++;
            lastBindStartMillis = System.currentTimeMillis();

            connection = new CarrierServiceConnection();

            String error;
            try {
                if (mContext.bindService(carrierService, connection, Context.BIND_AUTO_CREATE |
                            Context.BIND_FOREGROUND_SERVICE)) {
                    return;
                }

                error = "bindService returned false";
            } catch (SecurityException ex) {
                error = ex.getMessage();
            }

            log("Unable to bind to " + carrierPackage + " for phone " + phoneId +
                ". Error: " + error);
            unbind();
        }

        void unbind() {
            if (connection == null) {
                return;
            }

            // Log debug information
            unbindCount++;
            lastUnbindMillis = System.currentTimeMillis();

            // Clear package state now that no binding is present.
            carrierPackage = null;
            carrierServiceClass = null;

            // Actually unbind
            log("Unbinding from carrier app");
            mContext.unbindService(connection);
            connection = null;
        }

        public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
            pw.println("Carrier app binding for phone " + phoneId);
            pw.println("  connection: " + connection);
            pw.println("  bindCount: " + bindCount);
            pw.println("  lastBindStartMillis: " + lastBindStartMillis);
            pw.println("  unbindCount: " + unbindCount);
            pw.println("  lastUnbindMillis: " + lastUnbindMillis);
            pw.println();
        }
    }

    private class CarrierServiceConnection implements ServiceConnection {
        private boolean connected;

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            log("Connected to carrier app: " + name.flattenToString());
            connected = true;
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            log("Disconnected from carrier app: " + name.flattenToString());
            connected = false;
        }

        @Override
        public String toString() {
            return "CarrierServiceConnection[connected=" + connected + "]";
        }
    }

    private class CarrierServicePackageMonitor extends PackageMonitor {
        @Override
        public void onPackageAdded(String packageName, int reason) {
            evaluateBinding(packageName, true /* forceUnbind */);
        }

        @Override
        public void onPackageRemoved(String packageName, int reason) {
            evaluateBinding(packageName, true /* forceUnbind */);
        }

        @Override
        public void onPackageUpdateFinished(String packageName, int uid) {
            evaluateBinding(packageName, true /* forceUnbind */);
        }

        @Override
        public void onPackageModified(String packageName) {
            evaluateBinding(packageName, false /* forceUnbind */);
        }

        @Override
        public boolean onHandleForceStop(Intent intent, String[] packages, int uid, boolean doit) {
            if (doit) {
                for (String packageName : packages) {
                    evaluateBinding(packageName, true /* forceUnbind */);
                }
            }
            return super.onHandleForceStop(intent, packages, uid, doit);
        }

        private void evaluateBinding(String carrierPackageName, boolean forceUnbind) {
            for (AppBinding appBinding : mBindings) {
                String appBindingPackage = appBinding.getPackage();
                boolean isBindingForPackage = carrierPackageName.equals(appBindingPackage);
                // Only log if this package was a carrier package to avoid log spam in the common
                // case that there are no carrier packages, but evaluate the binding if the package
                // is unset, in case this package change resulted in a new carrier package becoming
                // available for binding.
                if (isBindingForPackage) {
                    log(carrierPackageName + " changed and corresponds to a phone. Rebinding.");
                }
                if (appBindingPackage == null || isBindingForPackage) {
                    if (forceUnbind) {
                        appBinding.unbind();
                    }
                    appBinding.rebind();
                }
            }
        }
    }

    private static void log(String message) {
        Log.d(LOG_TAG, message);
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("CarrierServiceBindHelper:");
        for (AppBinding binding : mBindings) {
            binding.dump(fd, pw, args);
        }
    }
}
