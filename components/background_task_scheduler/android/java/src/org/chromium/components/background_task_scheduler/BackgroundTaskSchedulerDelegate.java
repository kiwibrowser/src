// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

import android.content.Context;

/**
 * The internal representation of a {@link BackgroundTaskScheduler} to make it possible to use
 * system APIs ({@link android.app.job.JobScheduler} on newer platforms and GCM
 * ({@link com.google.android.gms.gcm.GcmNetworkManager}) on older platforms.
 */
interface BackgroundTaskSchedulerDelegate {
    /**
     * Schedules a background task. See {@link TaskInfo} for information on what types of tasks that
     * can be scheduled.
     *
     * @param context the current context.
     * @param taskInfo the information about the task to be scheduled.
     * @return true if the schedule operation succeeded, and false otherwise.
     * @see TaskInfo
     */
    boolean schedule(Context context, TaskInfo taskInfo);

    /**
     * Cancels the task specified by the task ID.
     *
     * @param context the current context.
     * @param taskId the ID of the task to cancel. See {@link TaskIds} for a list.
     */
    void cancel(Context context, int taskId);
}
