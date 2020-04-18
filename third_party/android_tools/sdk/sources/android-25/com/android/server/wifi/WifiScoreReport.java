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

import android.net.NetworkAgent;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.util.Log;


/**
* Calculate scores for connected wifi networks.
*/
public class WifiScoreReport {
    // TODO: switch to WifiScoreReport if it doesn't break any tools
    private static final String TAG = "WifiStateMachine";

    // TODO: This score was hardcorded to 56.  Need to understand why after finishing code refactor
    private static final int STARTING_SCORE = 56;

    // TODO: Understand why these values are used
    private static final int MAX_BAD_LINKSPEED_COUNT = 6;
    private static final int SCAN_CACHE_VISIBILITY_MS = 12000;
    private static final int HOME_VISIBLE_NETWORK_MAX_COUNT = 6;
    private static final int SCAN_CACHE_COUNT_PENALTY = 2;
    private static final int AGGRESSIVE_HANDOVER_PENALTY = 6;
    private static final int MIN_SUCCESS_COUNT = 5;
    private static final int MAX_SUCCESS_COUNT_OF_STUCK_LINK = 3;
    private static final int MAX_STUCK_LINK_COUNT = 5;
    private static final int MIN_NUM_TICKS_AT_STATE = 1000;
    private static final int USER_DISCONNECT_PENALTY = 5;
    private static final int MAX_BAD_RSSI_COUNT = 7;
    private static final int BAD_RSSI_COUNT_PENALTY = 2;
    private static final int MAX_LOW_RSSI_COUNT = 1;
    private static final double MIN_TX_RATE_FOR_WORKING_LINK = 0.3;
    private static final int MIN_SUSTAINED_LINK_STUCK_COUNT = 1;
    private static final int LINK_STUCK_PENALTY = 2;
    private static final int BAD_LINKSPEED_PENALTY = 4;
    private static final int GOOD_LINKSPEED_BONUS = 4;


    private String mReport;
    private int mBadLinkspeedcount;

    WifiScoreReport(String report, int badLinkspeedcount) {
        mReport = report;
        mBadLinkspeedcount = badLinkspeedcount;
    }

    /**
     *  Method returning the String representation of the score report.
     *
     *  @return String score report
     */
    public String getReport() {
        return mReport;
    }

    /**
     *  Method returning the bad link speed count at the time of the current score report.
     *
     *  @return int bad linkspeed count
     */
    public int getBadLinkspeedcount() {
        return mBadLinkspeedcount;
    }

    /**
     * Calculate wifi network score based on updated link layer stats and return a new
     * WifiScoreReport object.
     *
     * If the score has changed from the previous value, update the WifiNetworkAgent.
     * @param wifiInfo WifiInfo information about current network connection
     * @param currentConfiguration WifiConfiguration current wifi config
     * @param wifiConfigManager WifiConfigManager Object holding current config state
     * @param networkAgent NetworkAgent to be notified of new score
     * @param lastReport String most recent score report
     * @param aggressiveHandover int current aggressiveHandover setting
     * @return WifiScoreReport Wifi Score report
     */
    public static WifiScoreReport calculateScore(WifiInfo wifiInfo,
                                                 WifiConfiguration currentConfiguration,
                                                 WifiConfigManager wifiConfigManager,
                                                 NetworkAgent networkAgent,
                                                 WifiScoreReport lastReport,
                                                 int aggressiveHandover,
                                                 WifiMetrics wifiMetrics) {
        boolean debugLogging = false;
        if (wifiConfigManager.mEnableVerboseLogging.get() > 0) {
            debugLogging = true;
        }

        StringBuilder sb = new StringBuilder();

        int score = STARTING_SCORE;
        boolean isBadLinkspeed = (wifiInfo.is24GHz()
                && wifiInfo.getLinkSpeed() < wifiConfigManager.mBadLinkSpeed24)
                || (wifiInfo.is5GHz() && wifiInfo.getLinkSpeed()
                < wifiConfigManager.mBadLinkSpeed5);
        boolean isGoodLinkspeed = (wifiInfo.is24GHz()
                && wifiInfo.getLinkSpeed() >= wifiConfigManager.mGoodLinkSpeed24)
                || (wifiInfo.is5GHz() && wifiInfo.getLinkSpeed()
                >= wifiConfigManager.mGoodLinkSpeed5);

        int badLinkspeedcount = 0;
        if (lastReport != null) {
            badLinkspeedcount = lastReport.getBadLinkspeedcount();
        }

        if (isBadLinkspeed) {
            if (badLinkspeedcount < MAX_BAD_LINKSPEED_COUNT) {
                badLinkspeedcount++;
            }
        } else {
            if (badLinkspeedcount > 0) {
                badLinkspeedcount--;
            }
        }

        if (isBadLinkspeed) sb.append(" bl(").append(badLinkspeedcount).append(")");
        if (isGoodLinkspeed) sb.append(" gl");

        /**
         * We want to make sure that we use the 24GHz RSSI thresholds if
         * there are 2.4GHz scan results
         * otherwise we end up lowering the score based on 5GHz values
         * which may cause a switch to LTE before roaming has a chance to try 2.4GHz
         * We also might unblacklist the configuation based on 2.4GHz
         * thresholds but joining 5GHz anyhow, and failing over to 2.4GHz because 5GHz is not good
         */
        boolean use24Thresholds = false;
        boolean homeNetworkBoost = false;
        ScanDetailCache scanDetailCache =
                wifiConfigManager.getScanDetailCache(currentConfiguration);
        if (currentConfiguration != null && scanDetailCache != null) {
            currentConfiguration.setVisibility(
                    scanDetailCache.getVisibility(SCAN_CACHE_VISIBILITY_MS));
            if (currentConfiguration.visibility != null) {
                if (currentConfiguration.visibility.rssi24 != WifiConfiguration.INVALID_RSSI
                        && currentConfiguration.visibility.rssi24
                        >= (currentConfiguration.visibility.rssi5 - SCAN_CACHE_COUNT_PENALTY)) {
                    use24Thresholds = true;
                }
            }
            if (scanDetailCache.size() <= HOME_VISIBLE_NETWORK_MAX_COUNT
                    && currentConfiguration.allowedKeyManagement.cardinality() == 1
                    && currentConfiguration.allowedKeyManagement
                            .get(WifiConfiguration.KeyMgmt.WPA_PSK)) {
                // A PSK network with less than 6 known BSSIDs
                // This is most likely a home network and thus we want to stick to wifi more
                homeNetworkBoost = true;
            }
        }
        if (homeNetworkBoost) sb.append(" hn");
        if (use24Thresholds) sb.append(" u24");

        int rssi = wifiInfo.getRssi() - AGGRESSIVE_HANDOVER_PENALTY * aggressiveHandover
                + (homeNetworkBoost ? WifiConfiguration.HOME_NETWORK_RSSI_BOOST : 0);
        sb.append(String.format(" rssi=%d ag=%d", rssi, aggressiveHandover));

        boolean is24GHz = use24Thresholds || wifiInfo.is24GHz();

        boolean isBadRSSI = (is24GHz && rssi < wifiConfigManager.mThresholdMinimumRssi24.get())
                || (!is24GHz && rssi < wifiConfigManager.mThresholdMinimumRssi5.get());
        boolean isLowRSSI = (is24GHz && rssi < wifiConfigManager.mThresholdQualifiedRssi24.get())
                || (!is24GHz
                        && wifiInfo.getRssi() < wifiConfigManager.mThresholdMinimumRssi5.get());
        boolean isHighRSSI = (is24GHz && rssi >= wifiConfigManager.mThresholdSaturatedRssi24.get())
                || (!is24GHz
                        && wifiInfo.getRssi() >= wifiConfigManager.mThresholdSaturatedRssi5.get());

        if (isBadRSSI) sb.append(" br");
        if (isLowRSSI) sb.append(" lr");
        if (isHighRSSI) sb.append(" hr");

        int penalizedDueToUserTriggeredDisconnect = 0;        // Not a user triggered disconnect
        if (currentConfiguration != null
                && (wifiInfo.txSuccessRate > MIN_SUCCESS_COUNT
                        || wifiInfo.rxSuccessRate > MIN_SUCCESS_COUNT)) {
            if (isBadRSSI) {
                currentConfiguration.numTicksAtBadRSSI++;
                if (currentConfiguration.numTicksAtBadRSSI > MIN_NUM_TICKS_AT_STATE) {
                    // We remained associated for a compound amount of time while passing
                    // traffic, hence loose the corresponding user triggered disabled stats
                    if (currentConfiguration.numUserTriggeredWifiDisableBadRSSI > 0) {
                        currentConfiguration.numUserTriggeredWifiDisableBadRSSI--;
                    }
                    if (currentConfiguration.numUserTriggeredWifiDisableLowRSSI > 0) {
                        currentConfiguration.numUserTriggeredWifiDisableLowRSSI--;
                    }
                    if (currentConfiguration.numUserTriggeredWifiDisableNotHighRSSI > 0) {
                        currentConfiguration.numUserTriggeredWifiDisableNotHighRSSI--;
                    }
                    currentConfiguration.numTicksAtBadRSSI = 0;
                }
                if (wifiConfigManager.mEnableWifiCellularHandoverUserTriggeredAdjustment
                        && (currentConfiguration.numUserTriggeredWifiDisableBadRSSI > 0
                                || currentConfiguration.numUserTriggeredWifiDisableLowRSSI > 0
                                || currentConfiguration
                                        .numUserTriggeredWifiDisableNotHighRSSI > 0)) {
                    score = score - USER_DISCONNECT_PENALTY;
                    penalizedDueToUserTriggeredDisconnect = 1;
                    sb.append(" p1");
                }
            } else if (isLowRSSI) {
                currentConfiguration.numTicksAtLowRSSI++;
                if (currentConfiguration.numTicksAtLowRSSI > MIN_NUM_TICKS_AT_STATE) {
                    // We remained associated for a compound amount of time while passing
                    // traffic, hence loose the corresponding user triggered disabled stats
                    if (currentConfiguration.numUserTriggeredWifiDisableLowRSSI > 0) {
                        currentConfiguration.numUserTriggeredWifiDisableLowRSSI--;
                    }
                    if (currentConfiguration.numUserTriggeredWifiDisableNotHighRSSI > 0) {
                        currentConfiguration.numUserTriggeredWifiDisableNotHighRSSI--;
                    }
                    currentConfiguration.numTicksAtLowRSSI = 0;
                }
                if (wifiConfigManager.mEnableWifiCellularHandoverUserTriggeredAdjustment
                        && (currentConfiguration.numUserTriggeredWifiDisableLowRSSI > 0
                                || currentConfiguration
                                        .numUserTriggeredWifiDisableNotHighRSSI > 0)) {
                    score = score - USER_DISCONNECT_PENALTY;
                    penalizedDueToUserTriggeredDisconnect = 2;
                    sb.append(" p2");
                }
            } else if (!isHighRSSI) {
                currentConfiguration.numTicksAtNotHighRSSI++;
                if (currentConfiguration.numTicksAtNotHighRSSI > MIN_NUM_TICKS_AT_STATE) {
                    // We remained associated for a compound amount of time while passing
                    // traffic, hence loose the corresponding user triggered disabled stats
                    if (currentConfiguration.numUserTriggeredWifiDisableNotHighRSSI > 0) {
                        currentConfiguration.numUserTriggeredWifiDisableNotHighRSSI--;
                    }
                    currentConfiguration.numTicksAtNotHighRSSI = 0;
                }
                if (wifiConfigManager.mEnableWifiCellularHandoverUserTriggeredAdjustment
                        && currentConfiguration.numUserTriggeredWifiDisableNotHighRSSI > 0) {
                    score = score - USER_DISCONNECT_PENALTY;
                    penalizedDueToUserTriggeredDisconnect = 3;
                    sb.append(" p3");
                }
            }
            sb.append(String.format(" ticks %d,%d,%d", currentConfiguration.numTicksAtBadRSSI,
                    currentConfiguration.numTicksAtLowRSSI,
                    currentConfiguration.numTicksAtNotHighRSSI));
        }

        if (debugLogging) {
            String rssiStatus = "";
            if (isBadRSSI) {
                rssiStatus += " badRSSI ";
            } else if (isHighRSSI) {
                rssiStatus += " highRSSI ";
            } else if (isLowRSSI) {
                rssiStatus += " lowRSSI ";
            }
            if (isBadLinkspeed) rssiStatus += " lowSpeed ";
            Log.d(TAG, "calculateWifiScore freq=" + Integer.toString(wifiInfo.getFrequency())
                    + " speed=" + Integer.toString(wifiInfo.getLinkSpeed())
                    + " score=" + Integer.toString(wifiInfo.score)
                    + rssiStatus
                    + " -> txbadrate=" + String.format("%.2f", wifiInfo.txBadRate)
                    + " txgoodrate=" + String.format("%.2f", wifiInfo.txSuccessRate)
                    + " txretriesrate=" + String.format("%.2f", wifiInfo.txRetriesRate)
                    + " rxrate=" + String.format("%.2f", wifiInfo.rxSuccessRate)
                    + " userTriggerdPenalty" + penalizedDueToUserTriggeredDisconnect);
        }

        if ((wifiInfo.txBadRate >= 1) && (wifiInfo.txSuccessRate < MAX_SUCCESS_COUNT_OF_STUCK_LINK)
                && (isBadRSSI || isLowRSSI)) {
            // Link is stuck
            if (wifiInfo.linkStuckCount < MAX_STUCK_LINK_COUNT) {
                wifiInfo.linkStuckCount += 1;
            }
            sb.append(String.format(" ls+=%d", wifiInfo.linkStuckCount));
            if (debugLogging) {
                Log.d(TAG, " bad link -> stuck count ="
                        + Integer.toString(wifiInfo.linkStuckCount));
            }
        } else if (wifiInfo.txBadRate < MIN_TX_RATE_FOR_WORKING_LINK) {
            if (wifiInfo.linkStuckCount > 0) {
                wifiInfo.linkStuckCount -= 1;
            }
            sb.append(String.format(" ls-=%d", wifiInfo.linkStuckCount));
            if (debugLogging) {
                Log.d(TAG, " good link -> stuck count ="
                        + Integer.toString(wifiInfo.linkStuckCount));
            }
        }

        sb.append(String.format(" [%d", score));

        if (wifiInfo.linkStuckCount > MIN_SUSTAINED_LINK_STUCK_COUNT) {
            // Once link gets stuck for more than 3 seconds, start reducing the score
            score = score - LINK_STUCK_PENALTY * (wifiInfo.linkStuckCount - 1);
        }
        sb.append(String.format(",%d", score));

        if (isBadLinkspeed) {
            score -= BAD_LINKSPEED_PENALTY;
            if (debugLogging) {
                Log.d(TAG, " isBadLinkspeed   ---> count=" + badLinkspeedcount
                        + " score=" + Integer.toString(score));
            }
        } else if ((isGoodLinkspeed) && (wifiInfo.txSuccessRate > 5)) {
            score += GOOD_LINKSPEED_BONUS; // So as bad rssi alone dont kill us
        }
        sb.append(String.format(",%d", score));

        if (isBadRSSI) {
            if (wifiInfo.badRssiCount < MAX_BAD_RSSI_COUNT) {
                wifiInfo.badRssiCount += 1;
            }
        } else if (isLowRSSI) {
            wifiInfo.lowRssiCount = MAX_LOW_RSSI_COUNT; // Dont increment the lowRssi count above 1
            if (wifiInfo.badRssiCount > 0) {
                // Decrement bad Rssi count
                wifiInfo.badRssiCount -= 1;
            }
        } else {
            wifiInfo.badRssiCount = 0;
            wifiInfo.lowRssiCount = 0;
        }

        score -= wifiInfo.badRssiCount * BAD_RSSI_COUNT_PENALTY + wifiInfo.lowRssiCount;
        sb.append(String.format(",%d", score));

        if (debugLogging) {
            Log.d(TAG, " badRSSI count" + Integer.toString(wifiInfo.badRssiCount)
                    + " lowRSSI count" + Integer.toString(wifiInfo.lowRssiCount)
                    + " --> score " + Integer.toString(score));
        }

        if (isHighRSSI) {
            score += 5;
            if (debugLogging) Log.d(TAG, " isHighRSSI       ---> score=" + Integer.toString(score));
        }
        sb.append(String.format(",%d]", score));

        sb.append(String.format(" brc=%d lrc=%d", wifiInfo.badRssiCount, wifiInfo.lowRssiCount));

        //sanitize boundaries
        if (score > NetworkAgent.WIFI_BASE_SCORE) {
            score = NetworkAgent.WIFI_BASE_SCORE;
        }
        if (score < 0) {
            score = 0;
        }

        //report score
        if (score != wifiInfo.score) {
            if (debugLogging) {
                Log.d(TAG, "calculateWifiScore() report new score " + Integer.toString(score));
            }
            wifiInfo.score = score;
            if (networkAgent != null) {
                networkAgent.sendNetworkScore(score);
            }
        }
        wifiMetrics.incrementWifiScoreCount(score);
        return new WifiScoreReport(sb.toString(), badLinkspeedcount);
    }
}
