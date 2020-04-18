// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.download.service;

import android.os.Bundle;

import org.chromium.base.ContextUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.components.background_task_scheduler.BackgroundTaskScheduler;
import org.chromium.components.background_task_scheduler.BackgroundTaskSchedulerFactory;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskInfo;
import org.chromium.components.download.DownloadTaskType;

import java.util.concurrent.TimeUnit;

/**
 * A background task scheduler that schedules various types of jobs with the system with certain
 * conditions as requested by the download service.
 */
@JNINamespace("download::android")
public class DownloadTaskScheduler {
    public static final String EXTRA_BATTERY_REQUIRES_CHARGING = "extra_battery_requires_charging";
    public static final String EXTRA_OPTIMAL_BATTERY_PERCENTAGE =
            "extra_optimal_battery_percentage";
    public static final String EXTRA_TASK_TYPE = "extra_task_type";
    static final long TWELVE_HOURS_IN_SECONDS = TimeUnit.HOURS.toSeconds(12);
    static final long FIVE_MINUTES_IN_SECONDS = TimeUnit.MINUTES.toSeconds(5);

    @CalledByNative
    private static void scheduleTask(@DownloadTaskType int taskType,
            boolean requiresUnmeteredNetwork, boolean requiresCharging,
            int optimalBatteryPercentage, long windowStartTimeSeconds, long windowEndTimeSeconds) {
        Bundle bundle = new Bundle();
        bundle.putInt(EXTRA_TASK_TYPE, taskType);
        bundle.putInt(EXTRA_OPTIMAL_BATTERY_PERCENTAGE, optimalBatteryPercentage);
        bundle.putBoolean(EXTRA_BATTERY_REQUIRES_CHARGING, requiresCharging);

        BackgroundTaskScheduler scheduler = BackgroundTaskSchedulerFactory.getScheduler();
        TaskInfo taskInfo =
                TaskInfo.createOneOffTask(getTaskId(taskType), DownloadBackgroundTask.class,
                                TimeUnit.SECONDS.toMillis(windowStartTimeSeconds),
                                TimeUnit.SECONDS.toMillis(windowEndTimeSeconds))
                        .setRequiredNetworkType(
                                getRequiredNetworkType(taskType, requiresUnmeteredNetwork))
                        .setRequiresCharging(requiresCharging)
                        .setUpdateCurrent(true)
                        .setIsPersisted(true)
                        .setExtras(bundle)
                        .build();
        scheduler.schedule(ContextUtils.getApplicationContext(), taskInfo);
    }

    @CalledByNative
    private static void cancelTask(@DownloadTaskType int taskType) {
        BackgroundTaskScheduler scheduler = BackgroundTaskSchedulerFactory.getScheduler();
        scheduler.cancel(ContextUtils.getApplicationContext(), getTaskId(taskType));
    }

    /**
     * Invoked when the system sends a reschedule() call, which might happen in case of OS or play
     * services upgrade etc. This will schedule the tasks in future with least restrictive criteria.
     */
    public static void rescheduleAllTasks() {
        scheduleTask(DownloadTaskType.DOWNLOAD_TASK, false, false, 0, FIVE_MINUTES_IN_SECONDS,
                2 * FIVE_MINUTES_IN_SECONDS);
        scheduleTask(DownloadTaskType.CLEANUP_TASK, false, false, 0, TWELVE_HOURS_IN_SECONDS,
                2 * TWELVE_HOURS_IN_SECONDS);
    }

    private static int getTaskId(@DownloadTaskType int taskType) {
        switch (taskType) {
            case DownloadTaskType.DOWNLOAD_TASK:
                return TaskIds.DOWNLOAD_SERVICE_JOB_ID;
            case DownloadTaskType.CLEANUP_TASK:
                return TaskIds.DOWNLOAD_CLEANUP_JOB_ID;
            default:
                assert false;
                return -1;
        }
    }

    private static int getRequiredNetworkType(
            @DownloadTaskType int taskType, boolean requiresUnmeteredNetwork) {
        if (taskType != DownloadTaskType.DOWNLOAD_TASK) return TaskInfo.NETWORK_TYPE_NONE;

        return requiresUnmeteredNetwork ? TaskInfo.NETWORK_TYPE_UNMETERED
                                        : TaskInfo.NETWORK_TYPE_ANY;
    }
}
