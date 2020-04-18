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

import android.security.KeyStore;

/**
 *  WiFi dependency injector using thread-safe lazy singleton pattern. To be used for accessing
 *  various wifi class instances and as a handle for mock injection.
 */
public class WifiInjector {
    // see: https://en.wikipedia.org/wiki/Initialization-on-demand_holder_idiom
    private static class LazyHolder {
        public static final WifiInjector sInstance = new WifiInjector();
    }

    public static WifiInjector getInstance() {
        return LazyHolder.sInstance;
    }

    private final Clock mClock = new Clock();
    private final WifiMetrics mWifiMetrics = new WifiMetrics(mClock);
    private final WifiLastResortWatchdog mWifiLastResortWatchdog =
            new WifiLastResortWatchdog(mWifiMetrics);
    private final PropertyService mPropertyService = new SystemPropertyService();
    private final BuildProperties mBuildProperties = new SystemBuildProperties();
    private final KeyStore mKeyStore = KeyStore.getInstance();

    public WifiMetrics getWifiMetrics() {
        return mWifiMetrics;
    }

    public WifiLastResortWatchdog getWifiLastResortWatchdog() {
        return mWifiLastResortWatchdog;
    }

    public Clock getClock() {
        return mClock;
    }

    public PropertyService getPropertyService() {
        return mPropertyService;
    }

    public BuildProperties getBuildProperties() { return mBuildProperties; }

    public KeyStore getKeyStore() {
        return mKeyStore;
    }
}
