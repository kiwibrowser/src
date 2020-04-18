// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.metrics;

import android.app.Activity;
import android.os.Handler;
import android.support.annotation.IntDef;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModel.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabModelObserver;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

/**
 * Records the behavior metrics after an ACTION_MAIN intent is received.
 */
public class MainIntentBehaviorMetrics implements ApplicationStatus.ActivityStateListener {

    private static final long BACKGROUND_TIME_24_HOUR_MS = 86400000;
    private static final long BACKGROUND_TIME_12_HOUR_MS = 43200000;
    private static final long BACKGROUND_TIME_6_HOUR_MS = 21600000;
    private static final long BACKGROUND_TIME_1_HOUR_MS = 3600000;

    static final long TIMEOUT_DURATION_MS = 10000;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({CONTINUATION, FOCUS_OMNIBOX, SWITCH_TABS, NTP_CREATED, BACKGROUNDED})
    @interface MainIntentActionType {}
    static final int CONTINUATION = 0;
    static final int FOCUS_OMNIBOX = 1;
    static final int SWITCH_TABS = 2;
    static final int NTP_CREATED = 3;
    static final int BACKGROUNDED = 4;

    // Min and max values (in minutes) for the buckets in the duration histograms.
    private static final int DURATION_HISTOGRAM_MIN = 5;
    private static final int DURATION_HISTOGRAM_MAX = 48 * 60;
    private static final int DURATION_HISTOGRAM_BUCKET_COUNT = 50;

    private static long sTimeoutDurationMs = TIMEOUT_DURATION_MS;

    private final ChromeActivity mActivity;
    private final Handler mHandler;
    private final Runnable mTimeoutRunnable;

    private boolean mPendingActionRecordForMainIntent;
    private long mBackgroundDurationMs;
    private TabModelSelectorTabModelObserver mTabModelObserver;
    private boolean mIgnoreEvents;

    @MainIntentActionType
    private Integer mLastMainIntentBehavior;

    /**
     * Constructs a metrics handler for ACTION_MAIN intents received for the specified activity.
     */
    public MainIntentBehaviorMetrics(ChromeActivity activity) {
        mActivity = activity;
        mHandler = new Handler();
        mTimeoutRunnable = new Runnable() {
            @Override
            public void run() {
                recordUserBehavior(CONTINUATION);
            }
        };
    }

    /**
     * Signal that an intent with ACTION_MAIN was received.
     *
     * This must only be called after the native libraries have been initialized.
     */
    public void onMainIntentWithNative(long backgroundDurationMs) {
        mLastMainIntentBehavior = null;

        RecordUserAction.record("MobileStartup.MainIntentReceived");

        if (backgroundDurationMs >= BACKGROUND_TIME_24_HOUR_MS) {
            RecordUserAction.record("MobileStartup.MainIntentReceived.After24Hours");
        } else if (backgroundDurationMs >= BACKGROUND_TIME_12_HOUR_MS) {
            RecordUserAction.record("MobileStartup.MainIntentReceived.After12Hours");
        } else if (backgroundDurationMs >= BACKGROUND_TIME_6_HOUR_MS) {
            RecordUserAction.record("MobileStartup.MainIntentReceived.After6Hours");
        } else if (backgroundDurationMs >= BACKGROUND_TIME_1_HOUR_MS) {
            RecordUserAction.record("MobileStartup.MainIntentReceived.After1Hour");
        }

        if (mPendingActionRecordForMainIntent) return;
        mBackgroundDurationMs = backgroundDurationMs;

        ApplicationStatus.registerStateListenerForActivity(this, mActivity);
        mPendingActionRecordForMainIntent = true;

        mHandler.postDelayed(mTimeoutRunnable, sTimeoutDurationMs);

        mTabModelObserver = new TabModelSelectorTabModelObserver(
                mActivity.getTabModelSelector()) {
            @Override
            public void didAddTab(Tab tab, TabLaunchType type) {
                if (TabLaunchType.FROM_RESTORE.equals(type)) return;
                if (NewTabPage.isNTPUrl(tab.getUrl())) recordUserBehavior(NTP_CREATED);
            }

            @Override
            public void didSelectTab(Tab tab, TabSelectionType type, int lastId) {
                recordUserBehavior(SWITCH_TABS);
            }
        };
    }

    /**
     * Signal that any events should be ignored as a signal of MAIN intent behavior.
     *
     * @param shouldIgnore Whether events should be ignored.
     */
    public void setIgnoreEvents(boolean shouldIgnore) {
        mIgnoreEvents = shouldIgnore;
    }

    /**
     * Signal that the omnibox received focus.
     */
    public void onOmniboxFocused() {
        recordUserBehavior(FOCUS_OMNIBOX);
    }

    @Override
    public void onActivityStateChange(Activity activity, int newState) {
        if (newState == ActivityState.STOPPED || newState == ActivityState.DESTROYED) {
            recordUserBehavior(BACKGROUNDED);
        }
    }

    /**
     * @return The last main intent behavior recorded, which can be null if no MAIN intent has been
     *         received or if the event has not yet occurred.
     */
    @MainIntentActionType
    public Integer getLastMainIntentBehaviorForTesting() {
        return mLastMainIntentBehavior;
    }

    /**
     * Allows test to override the timeout duration.
     */
    public static void setTimeoutDurationMsForTesting(long duration) {
        sTimeoutDurationMs = duration;
    }

    /**
     * @return Whether we are pending action for a received main intent.
     */
    public boolean getPendingActionRecordForMainIntent() {
        return mPendingActionRecordForMainIntent;
    }

    private String getHistogramNameForBehavior(@MainIntentActionType int behavior) {
        switch (behavior) {
            case CONTINUATION:
                return "FirstUserAction.BackgroundTime.MainIntent.Continuation";
            case FOCUS_OMNIBOX:
                return "FirstUserAction.BackgroundTime.MainIntent.Omnibox";
            case SWITCH_TABS:
                return "FirstUserAction.BackgroundTime.MainIntent.SwitchTabs";
            case NTP_CREATED:
                return "FirstUserAction.BackgroundTime.MainIntent.NtpCreated";
            case BACKGROUNDED:
                return "FirstUserAction.BackgroundTime.MainIntent.Backgrounded";
            default:
                return null;
        }
    }

    private void recordUserBehavior(@MainIntentActionType int behavior) {
        if (!mPendingActionRecordForMainIntent || mIgnoreEvents) return;
        mPendingActionRecordForMainIntent = false;

        mLastMainIntentBehavior = behavior;
        String histogramName = getHistogramNameForBehavior(behavior);
        if (histogramName != null) {
            RecordHistogram.recordCustomCountHistogram(
                    histogramName,
                    (int) TimeUnit.MINUTES.convert(mBackgroundDurationMs, TimeUnit.MILLISECONDS),
                    DURATION_HISTOGRAM_MIN,
                    DURATION_HISTOGRAM_MAX,
                    DURATION_HISTOGRAM_BUCKET_COUNT);
        } else {
            assert false : String.format(Locale.getDefault(), "Invalid behavior: %d", behavior);
        }

        ApplicationStatus.unregisterActivityStateListener(this);

        mHandler.removeCallbacksAndMessages(null);

        mTabModelObserver.destroy();
        mTabModelObserver = null;
    }
}