// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages.prefetch;

import android.content.Context;

import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.background_task_scheduler.NativeBackgroundTask;
import org.chromium.chrome.browser.offlinepages.DeviceConditions;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.background_task_scheduler.BackgroundTask.TaskFinishedCallback;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskParameters;
import org.chromium.net.ConnectionType;

/**
 * Handles servicing background offlining requests.
 */
@JNINamespace("offline_pages::prefetch")
public class PrefetchBackgroundTask extends NativeBackgroundTask {
    /** Key used in the extra data {@link Bundle} when the limitless flag is enabled. */
    public static final String LIMITLESS_BUNDLE_KEY = "limitlessPrefetching";

    private static final int MINIMUM_BATTERY_PERCENTAGE_FOR_PREFETCHING = 50;

    private static boolean sSkipConditionCheckingForTesting = false;

    private long mNativeTask = 0;
    private TaskFinishedCallback mTaskFinishedCallback = null;
    private Profile mProfile = null;
    // We update this when we call TaskFinishedCallback, so that subsequent calls to
    // onStopTask* can respond the same way.  This is possible due to races with the JobScheduler.
    // Defaults to true so that we are rescheduled automatically if somehow we were unable to start
    // up native.
    private boolean mCachedRescheduleResult = true;
    private boolean mLimitlessPrefetchingEnabled = false;

    public PrefetchBackgroundTask() {}

    protected Profile getProfile() {
        if (mProfile == null) mProfile = Profile.getLastUsedProfile();
        return mProfile;
    }

    @Override
    public int onStartTaskBeforeNativeLoaded(
            Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
        // Ensure that the conditions are right to do work.  If the maximum time to
        // wait is reached, it is possible the task will fire even if network conditions are
        // incorrect.  We want:
        // * Unmetered WiFi connection
        // * >50% battery
        // TODO(dewittj): * Preferences enabled.

        mTaskFinishedCallback = callback;
        mLimitlessPrefetchingEnabled = taskParameters.getExtras().getBoolean(LIMITLESS_BUNDLE_KEY);

        // Check current device conditions. They might be set to null when testing and for some
        // specific Android devices.
        final DeviceConditions deviceConditions;
        if (!sSkipConditionCheckingForTesting) {
            deviceConditions = DeviceConditions.getCurrent(context);
        } else {
            deviceConditions = null;
        }

        // Note: when |deviceConditions| is null native is always loaded because with the evidence
        // we have so far only specific devices that do not run on batteries actually return null.
        if (deviceConditions == null
                || (areBatteryConditionsMet(deviceConditions)
                           && areNetworkConditionsMet(deviceConditions))) {
            return NativeBackgroundTask.LOAD_NATIVE;
        }

        return NativeBackgroundTask.RESCHEDULE;
    }

    /**
     * For integration tests, skip checking the condition since the testing conditions
     * won't be what we expect for the scenario.
     */
    @VisibleForTesting
    static void skipConditionCheckingForTesting() {
        sSkipConditionCheckingForTesting = true;
    }

    @Override
    protected void onStartTaskWithNative(
            Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
        assert taskParameters.getTaskId() == TaskIds.OFFLINE_PAGES_PREFETCH_JOB_ID;
        if (mNativeTask != 0) return;

        nativeStartPrefetchTask(getProfile());
    }

    @Override
    protected boolean onStopTaskBeforeNativeLoaded(Context context, TaskParameters taskParameters) {
        return mCachedRescheduleResult;
    }

    @Override
    protected boolean onStopTaskWithNative(Context context, TaskParameters taskParameters) {
        assert taskParameters.getTaskId() == TaskIds.OFFLINE_PAGES_PREFETCH_JOB_ID;
        // Sometimes we can race with JobScheduler and receive a stop call after we have called the
        // TaskFinishedCallback, so we need to save the reschedule result.
        if (mNativeTask == 0) return mCachedRescheduleResult;

        return nativeOnStopTask(mNativeTask);
    }

    @Override
    public void reschedule(Context context) {
        // TODO(dewittj): Set the backoff time appropriately.
        if (mLimitlessPrefetchingEnabled) {
            PrefetchBackgroundTaskScheduler.scheduleTaskLimitless(0);
        } else {
            PrefetchBackgroundTaskScheduler.scheduleTask(0);
        }
    }

    /**
     * Called during construction of the native task.
     *
     * PrefetchBackgroundTask#onStartTask constructs the native task.
     */
    @VisibleForTesting
    @CalledByNative
    void setNativeTask(long nativeTask) {
        mNativeTask = nativeTask;
    }

    /**
     * Invoked by the native task when it is destroyed.
     */
    @VisibleForTesting
    @CalledByNative
    void doneProcessing(boolean needsReschedule) {
        assert mTaskFinishedCallback != null;
        mCachedRescheduleResult = needsReschedule;
        mTaskFinishedCallback.taskFinished(needsReschedule);
        setNativeTask(0);
    }

    /** Whether battery conditions (on power or enough battery percentage) are met. */
    private boolean areBatteryConditionsMet(DeviceConditions deviceConditions) {
        return (!deviceConditions.isInPowerSaveMode()
                       && (deviceConditions.isPowerConnected()
                                  || (deviceConditions.getBatteryPercentage()
                                             >= MINIMUM_BATTERY_PERCENTAGE_FOR_PREFETCHING)))
                || mLimitlessPrefetchingEnabled;
    }

    /** Whether network conditions are met. */
    private boolean areNetworkConditionsMet(DeviceConditions deviceConditions) {
        if (mLimitlessPrefetchingEnabled) {
            return deviceConditions.getNetConnectionType() != ConnectionType.CONNECTION_NONE;
        }
        return !deviceConditions.isActiveNetworkMetered()
                && deviceConditions.getNetConnectionType() == ConnectionType.CONNECTION_WIFI;
    }

    @VisibleForTesting
    void setTaskReschedulingForTesting(int rescheduleType) {
        if (mNativeTask == 0) return;
        nativeSetTaskReschedulingForTesting(mNativeTask, rescheduleType);
    }

    @VisibleForTesting
    void signalTaskFinishedForTesting() {
        if (mNativeTask == 0) return;
        nativeSignalTaskFinishedForTesting(mNativeTask);
    }

    @VisibleForTesting
    native boolean nativeStartPrefetchTask(Profile profile);
    @VisibleForTesting
    native boolean nativeOnStopTask(long nativePrefetchBackgroundTaskAndroid);
    native void nativeSetTaskReschedulingForTesting(
            long nativePrefetchBackgroundTaskAndroid, int rescheduleType);
    native void nativeSignalTaskFinishedForTesting(long nativePrefetchBackgroundTaskAndroid);
}
