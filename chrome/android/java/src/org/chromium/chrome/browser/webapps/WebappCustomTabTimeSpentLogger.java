// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.os.SystemClock;

import org.chromium.base.metrics.RecordHistogram;

import java.util.concurrent.TimeUnit;

/** Logs to UMA the amount of time user spends in a CCT for CCTs launched from webapps. */
public class WebappCustomTabTimeSpentLogger {
    private long mStartTime;
    private @WebappActivity.ActivityType int mActivityType;

    private WebappCustomTabTimeSpentLogger(@WebappActivity.ActivityType int activityType) {
        mActivityType = activityType;
        mStartTime = SystemClock.elapsedRealtime();
    }

    /**
     * Create {@link WebappCustomTabTimeSpentLogger} instance and starts timer.
     * @param type of the activity that opens the CCT.
     * @return {@link WebappCustomTabTimeSpentLogger} instance.
     */
    public static WebappCustomTabTimeSpentLogger createInstanceAndStartTimer(
            @WebappActivity.ActivityType int activityType) {
        return new WebappCustomTabTimeSpentLogger(activityType);
    }

    /**
     * Stop timer and log UMA.
     */
    public void onPause() {
        long timeSpent = SystemClock.elapsedRealtime() - mStartTime;
        String umaSuffix;
        switch (mActivityType) {
            case WebappActivity.ACTIVITY_TYPE_WEBAPP:
                umaSuffix = ".Webapp";
                break;
            case WebappActivity.ACTIVITY_TYPE_WEBAPK:
                umaSuffix = ".WebApk";
                break;
            case WebappActivity.ACTIVITY_TYPE_TWA:
                umaSuffix = ".TWA";
                break;
            default:
                umaSuffix = ".Other";
                break;
        }
        RecordHistogram.recordLongTimesHistogram(
                "CustomTab.SessionDuration" + umaSuffix, timeSpent, TimeUnit.MILLISECONDS);
    }
}
