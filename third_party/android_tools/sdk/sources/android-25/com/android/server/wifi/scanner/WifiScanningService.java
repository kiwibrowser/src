/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.server.wifi.scanner;

import android.content.Context;
import android.os.HandlerThread;
import android.util.Log;

import com.android.server.SystemService;
import com.android.server.am.BatteryStatsService;
import com.android.server.wifi.WifiInjector;

public class WifiScanningService extends SystemService {

    static final String TAG = "WifiScanningService";
    private final WifiScanningServiceImpl mImpl;
    private final HandlerThread mHandlerThread;

    public WifiScanningService(Context context) {
        super(context);
        Log.i(TAG, "Creating " + Context.WIFI_SCANNING_SERVICE);
        mHandlerThread = new HandlerThread("WifiScanningService");
        mHandlerThread.start();
        mImpl = new WifiScanningServiceImpl(getContext(), mHandlerThread.getLooper(),
                WifiScannerImpl.DEFAULT_FACTORY, BatteryStatsService.getService(),
                WifiInjector.getInstance());
    }

    @Override
    public void onStart() {
        Log.i(TAG, "Publishing " + Context.WIFI_SCANNING_SERVICE);
        publishBinderService(Context.WIFI_SCANNING_SERVICE, mImpl);
    }

    @Override
    public void onBootPhase(int phase) {
        if (phase == SystemService.PHASE_SYSTEM_SERVICES_READY) {
            Log.i(TAG, "Starting " + Context.WIFI_SCANNING_SERVICE);
            mImpl.startService();
        }
    }
}
