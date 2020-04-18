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

package com.android.server.wifi;

import android.app.AppGlobals;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.TrafficStats;
import android.net.ip.IpManager;
import android.net.wifi.IWifiScanner;
import android.net.wifi.WifiScanner;
import android.os.Handler;
import android.os.IBinder;
import android.os.INetworkManagementService;
import android.os.Looper;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserManager;
import android.provider.Settings;
import android.security.KeyStore;
import android.telephony.CarrierConfigManager;

import java.util.ArrayList;

/**
 * This class allows overriding objects with mocks to write unit tests
 */
public class FrameworkFacade {
    public static final String TAG = "FrameworkFacade";

    public BaseWifiLogger makeBaseLogger() {
        return new BaseWifiLogger();
    }

    public BaseWifiLogger makeRealLogger(
            Context context, WifiStateMachine stateMachine, WifiNative wifiNative,
            BuildProperties buildProperties) {
        return new WifiLogger(context, stateMachine, wifiNative, buildProperties);
    }

    public boolean setIntegerSetting(Context context, String name, int def) {
        return Settings.Global.putInt(context.getContentResolver(), name, def);
    }

    public int getIntegerSetting(Context context, String name, int def) {
        return Settings.Global.getInt(context.getContentResolver(), name, def);
    }

    public long getLongSetting(Context context, String name, long def) {
        return Settings.Global.getLong(context.getContentResolver(), name, def);
    }

    public boolean setStringSetting(Context context, String name, String def) {
        return Settings.Global.putString(context.getContentResolver(), name, def);
    }

    public String getStringSetting(Context context, String name) {
        return Settings.Global.getString(context.getContentResolver(), name);
    }

    public IBinder getService(String serviceName) {
        return ServiceManager.getService(serviceName);
    }

    public WifiScanner makeWifiScanner(Context context, Looper looper) {
        return new WifiScanner(context, IWifiScanner.Stub.asInterface(
                        getService(Context.WIFI_SCANNING_SERVICE)), looper);
    }

    public PendingIntent getBroadcast(Context context, int requestCode, Intent intent, int flags) {
        return PendingIntent.getBroadcast(context, requestCode, intent, flags);
    }

    public SupplicantStateTracker makeSupplicantStateTracker(Context context,
            WifiConfigManager configManager, Handler handler) {
        return new SupplicantStateTracker(context, configManager, handler);
    }

    public boolean getConfigWiFiDisableInECBM(Context context) {
       CarrierConfigManager configManager = (CarrierConfigManager) context
               .getSystemService(Context.CARRIER_CONFIG_SERVICE);
       if (configManager != null) {
           return configManager.getConfig().getBoolean(
               CarrierConfigManager.KEY_CONFIG_WIFI_DISABLE_IN_ECBM);
       }
       /* Default to TRUE */
       return true;
    }

    /**
     * Create a new instance of WifiApConfigStore.
     * @param context reference to a Context
     * @param backupManagerProxy reference to a BackupManagerProxy
     * @return an instance of WifiApConfigStore
     */
    public WifiApConfigStore makeApConfigStore(Context context,
                                               BackupManagerProxy backupManagerProxy) {
        return new WifiApConfigStore(context, backupManagerProxy);
    }

    public long getTxPackets(String iface) {
        return TrafficStats.getTxPackets(iface);
    }

    public long getRxPackets(String iface) {
        return TrafficStats.getRxPackets(iface);
    }

    public IpManager makeIpManager(
            Context context, String iface, IpManager.Callback callback) {
        return new IpManager(context, iface, callback);
    }

    /**
     * Create a SoftApManager.
     * @param context current context
     * @param looper current thread looper
     * @param wifiNative reference to WifiNative
     * @param nmService reference to NetworkManagementService
     * @param cm reference to ConnectivityManager
     * @param countryCode Country code
     * @param allowed2GChannels list of allowed 2G channels
     * @param listener listener for SoftApManager
     * @return an instance of SoftApManager
     */
    public SoftApManager makeSoftApManager(
            Context context, Looper looper, WifiNative wifiNative,
            INetworkManagementService nmService, ConnectivityManager cm,
            String countryCode, ArrayList<Integer> allowed2GChannels,
            SoftApManager.Listener listener) {
        return new SoftApManager(
                looper, wifiNative, nmService, countryCode,
                allowed2GChannels, listener);
    }

    /**
     * Checks whether the given uid has been granted the given permission.
     * @param permName the permission to check
     * @param uid The uid to check
     * @return {@link PackageManager.PERMISSION_GRANTED} if the permission has been granted and
     *         {@link PackageManager.PERMISSION_DENIED} otherwise
     */
    public int checkUidPermission(String permName, int uid) throws RemoteException {
        return AppGlobals.getPackageManager().checkUidPermission(permName, uid);
    }

    public WifiConfigManager makeWifiConfigManager(Context context, WifiNative wifiNative,
            FrameworkFacade frameworkFacade, Clock clock, UserManager userManager,
            KeyStore keyStore) {
        return new WifiConfigManager(context, wifiNative, frameworkFacade, clock, userManager,
                keyStore);
    }
}
