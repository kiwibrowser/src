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

public class WifiLoggerHal {
    // Must match wifi_logger.h
    public static final int MAX_FATE_LOG_LEN = 32;

    public static final byte FRAME_TYPE_UNKNOWN = 0;
    public static final byte FRAME_TYPE_ETHERNET_II = 1;
    public static final byte FRAME_TYPE_80211_MGMT = 2;

    public static final byte TX_PKT_FATE_ACKED = 0;
    public static final byte TX_PKT_FATE_SENT = 1;
    public static final byte TX_PKT_FATE_FW_QUEUED = 2;
    public static final byte TX_PKT_FATE_FW_DROP_INVALID = 3;
    public static final byte TX_PKT_FATE_FW_DROP_NOBUFS = 4;
    public static final byte TX_PKT_FATE_FW_DROP_OTHER = 5;
    public static final byte TX_PKT_FATE_DRV_QUEUED = 6;
    public static final byte TX_PKT_FATE_DRV_DROP_INVALID = 7;
    public static final byte TX_PKT_FATE_DRV_DROP_NOBUFS = 8;
    public static final byte TX_PKT_FATE_DRV_DROP_OTHER = 9;

    public static final byte RX_PKT_FATE_SUCCESS = 0;
    public static final byte RX_PKT_FATE_FW_QUEUED = 1;
    public static final byte RX_PKT_FATE_FW_DROP_FILTER = 2;
    public static final byte RX_PKT_FATE_FW_DROP_INVALID = 3;
    public static final byte RX_PKT_FATE_FW_DROP_NOBUFS = 4;
    public static final byte RX_PKT_FATE_FW_DROP_OTHER = 5;
    public static final byte RX_PKT_FATE_DRV_QUEUED = 6;
    public static final byte RX_PKT_FATE_DRV_DROP_FILTER = 7;
    public static final byte RX_PKT_FATE_DRV_DROP_INVALID = 8;
    public static final byte RX_PKT_FATE_DRV_DROP_NOBUFS = 9;
    public static final byte RX_PKT_FATE_DRV_DROP_OTHER = 10;

    /** These aren't formally part of the HAL. But they probably should be, eventually. */
    public static final byte WIFI_ALERT_REASON_RESERVED = 0;
    public static final byte WIFI_ALERT_REASON_MIN = 0;
    public static final byte WIFI_ALERT_REASON_MAX = 64;
}
