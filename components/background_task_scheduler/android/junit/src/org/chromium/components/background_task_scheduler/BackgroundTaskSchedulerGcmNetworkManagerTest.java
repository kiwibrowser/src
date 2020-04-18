// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.os.Bundle;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.gcm.GcmNetworkManager;
import com.google.android.gms.gcm.OneoffTask;
import com.google.android.gms.gcm.PeriodicTask;
import com.google.android.gms.gcm.Task;
import com.google.android.gms.gcm.TaskParams;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;
import org.robolectric.shadow.api.Shadow;
import org.robolectric.shadows.gms.Shadows;
import org.robolectric.shadows.gms.common.ShadowGoogleApiAvailability;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;

import java.util.concurrent.TimeUnit;

/** Unit tests for {@link BackgroundTaskSchedulerGcmNetworkManager}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE,
        shadows = {ShadowGcmNetworkManager.class, ShadowGoogleApiAvailability.class})
public class BackgroundTaskSchedulerGcmNetworkManagerTest {
    private static class TestBackgroundTaskNoPublicConstructor extends TestBackgroundTask {
        protected TestBackgroundTaskNoPublicConstructor() {}
    }

    ShadowGcmNetworkManager mGcmNetworkManager;

    @Before
    public void setUp() {

        Shadows.shadowOf(GoogleApiAvailability.getInstance())
                .setIsGooglePlayServicesAvailable(ConnectionResult.SUCCESS);
        mGcmNetworkManager = (ShadowGcmNetworkManager) Shadow.extract(
                GcmNetworkManager.getInstance(ContextUtils.getApplicationContext()));
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testOneOffTaskInfoWithDeadlineConversion() {
        TaskInfo oneOffTask = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                              TimeUnit.MINUTES.toMillis(200))
                                      .build();
        Task task = BackgroundTaskSchedulerGcmNetworkManager.createTaskFromTaskInfo(oneOffTask);
        assertTrue(task instanceof OneoffTask);
        assertEquals(Integer.toString(oneOffTask.getTaskId()), task.getTag());
        assertEquals(
                TimeUnit.MILLISECONDS.toSeconds(oneOffTask.getOneOffInfo().getWindowEndTimeMs()),
                ((OneoffTask) task).getWindowEnd());
        assertEquals(0, ((OneoffTask) task).getWindowStart());
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testOneOffTaskInfoWithWindowConversion() {
        TaskInfo oneOffTask =
                TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                TimeUnit.MINUTES.toMillis(100), TimeUnit.MINUTES.toMillis(200))
                        .build();
        Task task = BackgroundTaskSchedulerGcmNetworkManager.createTaskFromTaskInfo(oneOffTask);
        assertTrue(task instanceof OneoffTask);
        assertEquals(Integer.toString(oneOffTask.getTaskId()), task.getTag());
        assertEquals(
                TimeUnit.MILLISECONDS.toSeconds(oneOffTask.getOneOffInfo().getWindowEndTimeMs()),
                ((OneoffTask) task).getWindowEnd());
        assertEquals(
                TimeUnit.MILLISECONDS.toSeconds(oneOffTask.getOneOffInfo().getWindowStartTimeMs()),
                ((OneoffTask) task).getWindowStart());
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testPeriodicTaskInfoWithoutFlexConversion() {
        TaskInfo periodicTask = TaskInfo.createPeriodicTask(TaskIds.TEST, TestBackgroundTask.class,
                                                TimeUnit.MINUTES.toMillis(200))
                                        .build();
        Task task = BackgroundTaskSchedulerGcmNetworkManager.createTaskFromTaskInfo(periodicTask);
        assertEquals(Integer.toString(periodicTask.getTaskId()), task.getTag());
        assertTrue(task instanceof PeriodicTask);
        assertEquals(
                TimeUnit.MILLISECONDS.toSeconds(periodicTask.getPeriodicInfo().getIntervalMs()),
                ((PeriodicTask) task).getPeriod());
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testPeriodicTaskInfoWithFlexConversion() {
        TaskInfo periodicTask =
                TaskInfo.createPeriodicTask(TaskIds.TEST, TestBackgroundTask.class,
                                TimeUnit.MINUTES.toMillis(200), TimeUnit.MINUTES.toMillis(50))
                        .build();
        Task task = BackgroundTaskSchedulerGcmNetworkManager.createTaskFromTaskInfo(periodicTask);
        assertEquals(Integer.toString(periodicTask.getTaskId()), task.getTag());
        assertTrue(task instanceof PeriodicTask);
        assertEquals(
                TimeUnit.MILLISECONDS.toSeconds(periodicTask.getPeriodicInfo().getIntervalMs()),
                ((PeriodicTask) task).getPeriod());
        assertEquals(TimeUnit.MILLISECONDS.toSeconds(periodicTask.getPeriodicInfo().getFlexMs()),
                ((PeriodicTask) task).getFlex());
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testTaskInfoWithExtras() {
        Bundle userExtras = new Bundle();
        userExtras.putString("foo", "bar");
        userExtras.putBoolean("bools", true);
        userExtras.putLong("longs", 1342543L);
        TaskInfo oneOffTask = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                              TimeUnit.MINUTES.toMillis(200))
                                      .setExtras(userExtras)
                                      .build();
        Task task = BackgroundTaskSchedulerGcmNetworkManager.createTaskFromTaskInfo(oneOffTask);
        assertEquals(Integer.toString(oneOffTask.getTaskId()), task.getTag());
        assertTrue(task instanceof OneoffTask);

        Bundle taskExtras = task.getExtras();
        Bundle bundle = taskExtras.getBundle(
                BackgroundTaskSchedulerGcmNetworkManager.BACKGROUND_TASK_EXTRAS_KEY);
        assertEquals(userExtras.keySet().size(), bundle.keySet().size());
        assertEquals(userExtras.getString("foo"), bundle.getString("foo"));
        assertEquals(userExtras.getBoolean("bools"), bundle.getBoolean("bools"));
        assertEquals(userExtras.getLong("longs"), bundle.getLong("longs"));
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testTaskInfoWithManyConstraints() {
        TaskInfo.Builder taskBuilder = TaskInfo.createOneOffTask(
                TaskIds.TEST, TestBackgroundTask.class, TimeUnit.MINUTES.toMillis(200));

        Task task = BackgroundTaskSchedulerGcmNetworkManager.createTaskFromTaskInfo(
                taskBuilder.setIsPersisted(true).build());
        assertTrue(task.isPersisted());

        task = BackgroundTaskSchedulerGcmNetworkManager.createTaskFromTaskInfo(
                taskBuilder.setRequiredNetworkType(TaskInfo.NETWORK_TYPE_UNMETERED).build());
        assertEquals(Task.NETWORK_STATE_UNMETERED, task.getRequiredNetwork());

        task = BackgroundTaskSchedulerGcmNetworkManager.createTaskFromTaskInfo(
                taskBuilder.setRequiresCharging(true).build());
        assertTrue(task.getRequiresCharging());
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testGetTaskParametersFromTaskParams() {
        Bundle extras = new Bundle();
        Bundle taskExtras = new Bundle();
        taskExtras.putString("foo", "bar");
        extras.putBundle(
                BackgroundTaskSchedulerGcmNetworkManager.BACKGROUND_TASK_EXTRAS_KEY, taskExtras);

        TaskParams params = new TaskParams(Integer.toString(TaskIds.TEST), extras);

        TaskParameters parameters =
                BackgroundTaskSchedulerGcmNetworkManager.getTaskParametersFromTaskParams(params);
        assertEquals(parameters.getTaskId(), TaskIds.TEST);
        assertEquals(parameters.getExtras().getString("foo"), "bar");
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testGetBackgroundTaskFromTaskParams() {
        Bundle extras = new Bundle();
        extras.putString(BackgroundTaskSchedulerGcmNetworkManager.BACKGROUND_TASK_CLASS_KEY,
                TestBackgroundTask.class.getName());

        TaskParams params = new TaskParams(Integer.toString(TaskIds.TEST), extras);
        BackgroundTask backgroundTask =
                BackgroundTaskSchedulerGcmNetworkManager.getBackgroundTaskFromTaskParams(params);

        assertNotNull(backgroundTask);
        assertTrue(backgroundTask instanceof TestBackgroundTask);
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testSchedule() {
        TaskInfo oneOffTask = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                              TimeUnit.DAYS.toMillis(1))
                                      .build();

        assertNull(mGcmNetworkManager.getScheduledTask());

        BackgroundTaskSchedulerDelegate delegate = new BackgroundTaskSchedulerGcmNetworkManager();

        assertTrue(delegate.schedule(ContextUtils.getApplicationContext(), oneOffTask));

        // Check that a task was scheduled using GCM Network Manager.
        assertNotNull(mGcmNetworkManager.getScheduledTask());

        // Verify details of the scheduled task.
        Task scheduledTask = mGcmNetworkManager.getScheduledTask();

        assertEquals(Integer.toString(oneOffTask.getTaskId()), scheduledTask.getTag());
        assertEquals(
                TimeUnit.MILLISECONDS.toSeconds(oneOffTask.getOneOffInfo().getWindowEndTimeMs()),
                ((OneoffTask) scheduledTask).getWindowEnd());
        assertEquals(
                TimeUnit.MILLISECONDS.toSeconds(oneOffTask.getOneOffInfo().getWindowStartTimeMs()),
                ((OneoffTask) scheduledTask).getWindowStart());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testScheduleNoPublicConstructor() {
        TaskInfo oneOffTask =
                TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTaskNoPublicConstructor.class,
                                TimeUnit.DAYS.toMillis(1))
                        .build();

        assertFalse(new BackgroundTaskSchedulerGcmNetworkManager().schedule(
                ContextUtils.getApplicationContext(), oneOffTask));
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testScheduleNoGooglePlayServices() {
        Shadows.shadowOf(GoogleApiAvailability.getInstance())
                .setIsGooglePlayServicesAvailable(ConnectionResult.SERVICE_MISSING);

        TaskInfo oneOffTask = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                              TimeUnit.DAYS.toMillis(1))
                                      .build();

        assertFalse(new BackgroundTaskSchedulerGcmNetworkManager().schedule(
                ContextUtils.getApplicationContext(), oneOffTask));
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testCancel() {
        TaskInfo oneOffTask = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                              TimeUnit.DAYS.toMillis(1))
                                      .build();

        BackgroundTaskSchedulerDelegate delegate = new BackgroundTaskSchedulerGcmNetworkManager();

        assertTrue(delegate.schedule(ContextUtils.getApplicationContext(), oneOffTask));
        delegate.cancel(ContextUtils.getApplicationContext(), oneOffTask.getTaskId());

        Task canceledTask = mGcmNetworkManager.getCanceledTask();
        assertEquals(Integer.toString(oneOffTask.getTaskId()), canceledTask.getTag());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testCancelNoGooglePlayServices() {
        // This simulates situation where Google Play Services is uninstalled.
        Shadows.shadowOf(GoogleApiAvailability.getInstance())
                .setIsGooglePlayServicesAvailable(ConnectionResult.SERVICE_MISSING);

        TaskInfo oneOffTask = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                              TimeUnit.DAYS.toMillis(1))
                                      .build();

        // Ensure there was a previously scheduled task.
        mGcmNetworkManager.schedule(
                BackgroundTaskSchedulerGcmNetworkManager.createTaskFromTaskInfo(oneOffTask));

        BackgroundTaskSchedulerDelegate delegate = new BackgroundTaskSchedulerGcmNetworkManager();

        // This call is expected to have no effect on GCM Network Manager, because Play Services are
        // not available.
        delegate.cancel(ContextUtils.getApplicationContext(), oneOffTask.getTaskId());
        assertNull(mGcmNetworkManager.getCanceledTask());
        assertNotNull(mGcmNetworkManager.getScheduledTask());
    }
}
