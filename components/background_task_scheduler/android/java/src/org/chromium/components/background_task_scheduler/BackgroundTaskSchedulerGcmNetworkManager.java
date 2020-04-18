// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.gcm.GcmNetworkManager;
import com.google.android.gms.gcm.OneoffTask;
import com.google.android.gms.gcm.PeriodicTask;
import com.google.android.gms.gcm.Task;
import com.google.android.gms.gcm.TaskParams;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;

import java.util.concurrent.TimeUnit;

/**
 * An implementation of {@link BackgroundTaskSchedulerDelegate} that uses the Play Services
 * {@link GcmNetworkManager} to schedule jobs.
 */
class BackgroundTaskSchedulerGcmNetworkManager implements BackgroundTaskSchedulerDelegate {
    private static final String TAG = "BkgrdTaskSchedGcmNM";

    @VisibleForTesting
    static final String BACKGROUND_TASK_CLASS_KEY = "_background_task_class";
    @VisibleForTesting
    static final String BACKGROUND_TASK_EXTRAS_KEY = "_background_task_extras";

    static BackgroundTask getBackgroundTaskFromTaskParams(@NonNull TaskParams taskParams) {
        String backgroundTaskClassName = getBackgroundTaskClassFromTaskParams(taskParams);
        return BackgroundTaskReflection.getBackgroundTaskFromClassName(backgroundTaskClassName);
    }

    private static String getBackgroundTaskClassFromTaskParams(@NonNull TaskParams taskParams) {
        Bundle extras = taskParams.getExtras();
        if (extras == null) return null;
        return extras.getString(BACKGROUND_TASK_CLASS_KEY);
    }

    /**
     * Retrieves the {@link TaskParameters} from the {@link TaskParams}, which are passed as
     * one of the keys. Only values valid for {@link android.os.BaseBundle} are supported, and other
     * values are stripped at the time when the task is scheduled.
     *
     * @param taskParams the {@link TaskParams} to extract the {@link TaskParameters} from.
     * @return the {@link TaskParameters} for the current job.
     */
    static TaskParameters getTaskParametersFromTaskParams(@NonNull TaskParams taskParams) {
        int taskId;
        try {
            taskId = Integer.parseInt(taskParams.getTag());
        } catch (NumberFormatException e) {
            Log.e(TAG, "Cound not parse task ID from task tag: " + taskParams.getTag());
            return null;
        }

        TaskParameters.Builder builder = TaskParameters.create(taskId);

        Bundle extras = taskParams.getExtras();
        Bundle taskExtras = extras.getBundle(BACKGROUND_TASK_EXTRAS_KEY);
        builder.addExtras(taskExtras);

        return builder.build();
    }

    @VisibleForTesting
    static Task createTaskFromTaskInfo(@NonNull TaskInfo taskInfo) {
        Bundle taskExtras = new Bundle();
        taskExtras.putString(
                BACKGROUND_TASK_CLASS_KEY, taskInfo.getBackgroundTaskClass().getName());
        taskExtras.putBundle(BACKGROUND_TASK_EXTRAS_KEY, taskInfo.getExtras());

        Task.Builder builder;
        if (taskInfo.isPeriodic()) {
            builder = getPeriodicTaskBuilder(taskInfo.getPeriodicInfo());
        } else {
            builder = getOneOffTaskBuilder(taskInfo.getOneOffInfo());
        }

        builder.setExtras(taskExtras)
                .setPersisted(taskInfo.isPersisted())
                .setRequiredNetwork(getGcmNetworkManagerNetworkTypeFromTypeFromTaskNetworkType(
                        taskInfo.getRequiredNetworkType()))
                .setRequiresCharging(taskInfo.requiresCharging())
                .setService(BackgroundTaskGcmTaskService.class)
                .setTag(taskIdToTaskTag(taskInfo.getTaskId()))
                .setUpdateCurrent(taskInfo.shouldUpdateCurrent());

        return builder.build();
    }

    private static Task.Builder getPeriodicTaskBuilder(TaskInfo.PeriodicInfo periodicInfo) {
        PeriodicTask.Builder builder = new PeriodicTask.Builder();
        builder.setPeriod(TimeUnit.MILLISECONDS.toSeconds(periodicInfo.getIntervalMs()));
        if (periodicInfo.hasFlex()) {
            builder.setFlex(TimeUnit.MILLISECONDS.toSeconds(periodicInfo.getFlexMs()));
        }
        return builder;
    }

    private static Task.Builder getOneOffTaskBuilder(TaskInfo.OneOffInfo oneOffInfo) {
        OneoffTask.Builder builder = new OneoffTask.Builder();
        long windowStartSeconds = oneOffInfo.hasWindowStartTimeConstraint()
                ? TimeUnit.MILLISECONDS.toSeconds(oneOffInfo.getWindowStartTimeMs())
                : 0;
        builder.setExecutionWindow(windowStartSeconds,
                TimeUnit.MILLISECONDS.toSeconds(oneOffInfo.getWindowEndTimeMs()));
        return builder;
    }

    private static int getGcmNetworkManagerNetworkTypeFromTypeFromTaskNetworkType(
            @TaskInfo.NetworkType int networkType) {
        switch (networkType) {
            // This is correct: GcmNM ANY means no network is guaranteed.
            case TaskInfo.NETWORK_TYPE_NONE:
                return Task.NETWORK_STATE_ANY;
            case TaskInfo.NETWORK_TYPE_ANY:
                return Task.NETWORK_STATE_CONNECTED;
            case TaskInfo.NETWORK_TYPE_UNMETERED:
                return Task.NETWORK_STATE_UNMETERED;
            default:
                assert false;
        }
        return Task.NETWORK_STATE_ANY;
    }

    @Override
    public boolean schedule(Context context, @NonNull TaskInfo taskInfo) {
        ThreadUtils.assertOnUiThread();
        if (!BackgroundTaskReflection.hasParameterlessPublicConstructor(
                    taskInfo.getBackgroundTaskClass())) {
            Log.e(TAG,
                    "BackgroundTask " + taskInfo.getBackgroundTaskClass()
                            + " has no parameterless public constructor.");
            return false;
        }

        GcmNetworkManager gcmNetworkManager = getGcmNetworkManager(context);
        if (gcmNetworkManager == null) {
            Log.e(TAG, "GcmNetworkManager is not available.");
            return false;
        }

        try {
            Task task = createTaskFromTaskInfo(taskInfo);
            gcmNetworkManager.schedule(task);
        } catch (IllegalArgumentException e) {
            String gcmErrorMessage = e.getMessage() == null ? "null." : e.getMessage();
            Log.e(TAG,
                    "GcmNetworkManager failed to schedule task, gcm message: " + gcmErrorMessage);
            return false;
        }

        return true;
    }

    @Override
    public void cancel(Context context, int taskId) {
        ThreadUtils.assertOnUiThread();

        GcmNetworkManager gcmNetworkManager = getGcmNetworkManager(context);
        if (gcmNetworkManager == null) {
            Log.e(TAG, "GcmNetworkManager is not available.");
            return;
        }

        try {
            gcmNetworkManager.cancelTask(
                    taskIdToTaskTag(taskId), BackgroundTaskGcmTaskService.class);
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "GcmNetworkManager failed to cancel task.");
        }
    }

    private GcmNetworkManager getGcmNetworkManager(Context context) {
        if (GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context)
                == ConnectionResult.SUCCESS) {
            return GcmNetworkManager.getInstance(context);
        }
        return null;
    }

    private static String taskIdToTaskTag(int taskId) {
        return Integer.toString(taskId);
    }
}
