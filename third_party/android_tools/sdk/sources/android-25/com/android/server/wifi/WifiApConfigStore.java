/*
 * Copyright (C) 2010 The Android Open Source Project
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

import android.content.Context;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.os.Environment;
import android.util.Log;

import com.android.internal.R;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.UUID;

/**
 * Provides API for reading/writing soft access point configuration.
 */
public class WifiApConfigStore {

    private static final String TAG = "WifiApConfigStore";

    private static final String DEFAULT_AP_CONFIG_FILE =
            Environment.getDataDirectory() + "/misc/wifi/softap.conf";

    private static final int AP_CONFIG_FILE_VERSION = 2;

    private WifiConfiguration mWifiApConfig = null;

    private ArrayList<Integer> mAllowed2GChannel = null;

    private final Context mContext;
    private final String mApConfigFile;
    private final BackupManagerProxy mBackupManagerProxy;

    WifiApConfigStore(Context context, BackupManagerProxy backupManagerProxy) {
        this(context, backupManagerProxy, DEFAULT_AP_CONFIG_FILE);
    }

    WifiApConfigStore(Context context,
                      BackupManagerProxy backupManagerProxy,
                      String apConfigFile) {
        mContext = context;
        mBackupManagerProxy = backupManagerProxy;
        mApConfigFile = apConfigFile;

        String ap2GChannelListStr = mContext.getResources().getString(
                R.string.config_wifi_framework_sap_2G_channel_list);
        Log.d(TAG, "2G band allowed channels are:" + ap2GChannelListStr);

        if (ap2GChannelListStr != null) {
            mAllowed2GChannel = new ArrayList<Integer>();
            String channelList[] = ap2GChannelListStr.split(",");
            for (String tmp : channelList) {
                mAllowed2GChannel.add(Integer.parseInt(tmp));
            }
        }

        /* Load AP configuration from persistent storage. */
        mWifiApConfig = loadApConfiguration(mApConfigFile);
        if (mWifiApConfig == null) {
            /* Use default configuration. */
            Log.d(TAG, "Fallback to use default AP configuration");
            mWifiApConfig = getDefaultApConfiguration();

            /* Save the default configuration to persistent storage. */
            writeApConfiguration(mApConfigFile, mWifiApConfig);
        }
    }

    /**
     * Return the current soft access point configuration.
     */
    public synchronized WifiConfiguration getApConfiguration() {
        return mWifiApConfig;
    }

    /**
     * Update the current soft access point configuration.
     * Restore to default AP configuration if null is provided.
     * This can be invoked under context of binder threads (WifiManager.setWifiApConfiguration)
     * and WifiStateMachine thread (CMD_START_AP).
     */
    public synchronized void setApConfiguration(WifiConfiguration config) {
        if (config == null) {
            mWifiApConfig = getDefaultApConfiguration();
        } else {
            mWifiApConfig = config;
        }
        writeApConfiguration(mApConfigFile, mWifiApConfig);

        // Stage the backup of the SettingsProvider package which backs this up
        mBackupManagerProxy.notifyDataChanged();
    }

    public ArrayList<Integer> getAllowed2GChannel() {
        return mAllowed2GChannel;
    }

    /**
     * Load AP configuration from persistent storage.
     */
    private static WifiConfiguration loadApConfiguration(final String filename) {
        WifiConfiguration config = null;
        DataInputStream in = null;
        try {
            config = new WifiConfiguration();
            in = new DataInputStream(
                    new BufferedInputStream(new FileInputStream(filename)));

            int version = in.readInt();
            if ((version != 1) && (version != 2)) {
                Log.e(TAG, "Bad version on hotspot configuration file");
                return null;
            }
            config.SSID = in.readUTF();

            if (version >= 2) {
                config.apBand = in.readInt();
                config.apChannel = in.readInt();
            }

            int authType = in.readInt();
            config.allowedKeyManagement.set(authType);
            if (authType != KeyMgmt.NONE) {
                config.preSharedKey = in.readUTF();
            }
        } catch (IOException e) {
            Log.e(TAG, "Error reading hotspot configuration " + e);
            config = null;
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                    Log.e(TAG, "Error closing hotspot configuration during read" + e);
                }
            }
        }
        return config;
    }

    /**
     * Write AP configuration to persistent storage.
     */
    private static void writeApConfiguration(final String filename,
                                             final WifiConfiguration config) {
        try (DataOutputStream out = new DataOutputStream(new BufferedOutputStream(
                        new FileOutputStream(filename)))) {
            out.writeInt(AP_CONFIG_FILE_VERSION);
            out.writeUTF(config.SSID);
            out.writeInt(config.apBand);
            out.writeInt(config.apChannel);
            int authType = config.getAuthType();
            out.writeInt(authType);
            if (authType != KeyMgmt.NONE) {
                out.writeUTF(config.preSharedKey);
            }
        } catch (IOException e) {
            Log.e(TAG, "Error writing hotspot configuration" + e);
        }
    }

    /**
     * Generate a default WPA2 based configuration with a random password.
     * We are changing the Wifi Ap configuration storage from secure settings to a
     * flat file accessible only by the system. A WPA2 based default configuration
     * will keep the device secure after the update.
     */
    private WifiConfiguration getDefaultApConfiguration() {
        WifiConfiguration config = new WifiConfiguration();
        config.SSID = mContext.getResources().getString(
                R.string.wifi_tether_configure_ssid_default);
        config.allowedKeyManagement.set(KeyMgmt.WPA2_PSK);
        String randomUUID = UUID.randomUUID().toString();
        //first 12 chars from xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
        config.preSharedKey = randomUUID.substring(0, 8) + randomUUID.substring(9, 13);
        return config;
    }
}
