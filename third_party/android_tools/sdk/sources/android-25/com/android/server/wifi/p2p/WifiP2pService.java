/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.server.wifi.p2p;

import android.content.Context;
import android.util.Log;
import com.android.server.SystemService;

public final class WifiP2pService extends SystemService {

    private static final String TAG = "WifiP2pService";
    final WifiP2pServiceImpl mImpl;

    public WifiP2pService(Context context) {
        super(context);
        mImpl = new WifiP2pServiceImpl(context);
    }

    @Override
    public void onStart() {
        Log.i(TAG, "Registering " + Context.WIFI_P2P_SERVICE);
        publishBinderService(Context.WIFI_P2P_SERVICE, mImpl);
    }

    @Override
    public void onBootPhase(int phase) {
        if (phase == SystemService.PHASE_SYSTEM_SERVICES_READY) {
            mImpl.connectivityServiceReady();
        }
    }
}
