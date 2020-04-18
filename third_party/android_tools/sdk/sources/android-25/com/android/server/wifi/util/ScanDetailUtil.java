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

package com.android.server.wifi.util;

import android.net.wifi.ScanResult;

import com.android.server.wifi.ScanDetail;
import com.android.server.wifi.hotspot2.NetworkDetail;

/**
 * Utility for converting a ScanResult to a ScanDetail.
 * Only fields that are supported in ScanResult are copied.
 */
public class ScanDetailUtil {
    private ScanDetailUtil() { /* not constructable */ }

    /**
     * This method should only be used when the informationElements field in the provided scan
     * result is filled in with the IEs from the beacon.
     */
    public static ScanDetail toScanDetail(ScanResult scanResult) {
        NetworkDetail networkDetail = new NetworkDetail(scanResult.BSSID,
                scanResult.informationElements, scanResult.anqpLines, scanResult.frequency);
        return new ScanDetail(scanResult, networkDetail, null);
    }
}
