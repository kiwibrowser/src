/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.server.ethernet;

import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.IpConfiguration.ProxySettings;
import android.os.Environment;
import android.util.Log;
import android.util.SparseArray;

import com.android.server.net.IpConfigStore;


/**
 * This class provides an API to store and manage Ethernet network configuration.
 */
public class EthernetConfigStore extends IpConfigStore {
    private static final String TAG = "EthernetConfigStore";

    private static final String ipConfigFile = Environment.getDataDirectory() +
            "/misc/ethernet/ipconfig.txt";

    public EthernetConfigStore() {
    }

    public IpConfiguration readIpAndProxyConfigurations() {
        SparseArray<IpConfiguration> networks = readIpAndProxyConfigurations(ipConfigFile);

        if (networks.size() == 0) {
            Log.w(TAG, "No Ethernet configuration found. Using default.");
            return new IpConfiguration(IpAssignment.DHCP, ProxySettings.NONE, null, null);
        }

        if (networks.size() > 1) {
            // Currently we only support a single Ethernet interface.
            Log.w(TAG, "Multiple Ethernet configurations detected. Only reading first one.");
        }

        return networks.valueAt(0);
    }

    public void writeIpAndProxyConfigurations(IpConfiguration config) {
        SparseArray<IpConfiguration> networks = new SparseArray<IpConfiguration>();
        networks.put(0, config);
        writeIpAndProxyConfigurations(ipConfigFile, networks);
    }
}
