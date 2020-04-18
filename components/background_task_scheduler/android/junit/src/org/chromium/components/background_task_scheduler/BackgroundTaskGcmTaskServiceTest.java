// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;

import com.google.android.gms.gcm.GcmNetworkManager;
import com.google.android.gms.gcm.TaskParams;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;

import java.util.concurrent.TimeUnit;

/** Unit tests for {@link BackgroundTaskGcmTaskService}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BackgroundTaskGcmTaskServiceTest {
    static TestBackgroundTaskWithParams sLastTask;
    static boolean sReturnThroughCallback;
    static boolean sNeedsRescheduling;
    @Mock
    private BackgroundTaskSchedulerDelegate mDelegate;
    @Mock
    private BackgroundTaskSchedulerUma mBackgroundTaskSchedulerUma;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        BackgroundTaskSchedulerFactory.setSchedulerForTesting(
                new BackgroundTaskSchedulerImpl(mDelegate));
        BackgroundTaskSchedulerUma.setInstanceForTesting(mBackgroundTaskSchedulerUma);
        sReturnThroughCallback = false;
        sNeedsRescheduling = false;
        sLastTask = null;
        TestBackgroundTask.reset();
    }

    private static class TestBackgroundTaskWithParams extends TestBackgroundTask {
        private TaskParameters mTaskParameters;

        public TestBackgroundTaskWithParams() {}

        @Override
        public boolean onStartTask(
                Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
            mTaskParameters = taskParameters;
            callback.taskFinished(sNeedsRescheduling);
            sLastTask = this;
            return sReturnThroughCallback;
        }

        public TaskParameters getTaskParameters() {
            return mTaskParameters;
        }
    }

    private static class TestBackgroundTaskNoPublicConstructor extends TestBackgroundTask {}

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testOnRunTask() {
        Bundle taskExtras = new Bundle();
        taskExtras.putString("foo", "bar");
        TaskParams taskParams = buildTaskParams(TestBackgroundTaskWithParams.class, taskExtras);

        BackgroundTaskGcmTaskService taskService = new BackgroundTaskGcmTaskService();
        assertEquals(taskService.onRunTask(taskParams), GcmNetworkManager.RESULT_SUCCESS);

        assertNotNull(sLastTask);
        TaskParameters parameters = sLastTask.getTaskParameters();

        assertEquals(parameters.getTaskId(), TaskIds.TEST);
        assertEquals(parameters.getExtras().getString("foo"), "bar");

        verify(mBackgroundTaskSchedulerUma, times(1)).reportTaskStarted(eq(TaskIds.TEST));
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testOnRunTaskMissingConstructor() {
        TaskParams taskParams =
                buildTaskParams(TestBackgroundTaskNoPublicConstructor.class, new Bundle());

        BackgroundTaskGcmTaskService taskService = new BackgroundTaskGcmTaskService();
        assertEquals(taskService.onRunTask(taskParams), GcmNetworkManager.RESULT_FAILURE);
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testOnRuntaskNeedsReschedulingFromCallback() {
        sReturnThroughCallback = true;
        sNeedsRescheduling = true;
        TaskParams taskParams = buildTaskParams(TestBackgroundTaskWithParams.class, new Bundle());

        BackgroundTaskGcmTaskService taskService = new BackgroundTaskGcmTaskService();
        assertEquals(taskService.onRunTask(taskParams), GcmNetworkManager.RESULT_RESCHEDULE);
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testOnRuntaskDontRescheduleFromCallback() {
        sReturnThroughCallback = true;
        sNeedsRescheduling = false;
        TaskParams taskParams = buildTaskParams(TestBackgroundTaskWithParams.class, new Bundle());

        BackgroundTaskGcmTaskService taskService = new BackgroundTaskGcmTaskService();
        assertEquals(taskService.onRunTask(taskParams), GcmNetworkManager.RESULT_SUCCESS);
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testOnInitializeTasksOnPreM() {
        ReflectionHelpers.setStaticField(
                Build.VERSION.class, "SDK_INT", Build.VERSION_CODES.LOLLIPOP);
        TaskInfo task = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                        TimeUnit.DAYS.toMillis(1))
                                .build();
        BackgroundTaskSchedulerPrefs.addScheduledTask(task);
        assertEquals(0, TestBackgroundTask.getRescheduleCalls());

        new BackgroundTaskGcmTaskService().onInitializeTasks();
        assertEquals(1, TestBackgroundTask.getRescheduleCalls());
    }

    @Test
    @Feature({"BackgroundTaskScheduler"})
    public void testOnInitializeTasksOnMPlus() {
        ReflectionHelpers.setStaticField(Build.VERSION.class, "SDK_INT", Build.VERSION_CODES.M);
        TaskInfo task = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                        TimeUnit.DAYS.toMillis(1))
                                .build();
        BackgroundTaskSchedulerPrefs.addScheduledTask(task);
        assertEquals(0, TestBackgroundTask.getRescheduleCalls());

        new BackgroundTaskGcmTaskService().onInitializeTasks();
        assertEquals(0, TestBackgroundTask.getRescheduleCalls());
    }

    private TaskParams buildTaskParams(Class clazz, Bundle taskExtras) {
        Bundle extras = new Bundle();
        extras.putBundle(
                BackgroundTaskSchedulerGcmNetworkManager.BACKGROUND_TASK_EXTRAS_KEY, taskExtras);
        extras.putString(BackgroundTaskSchedulerGcmNetworkManager.BACKGROUND_TASK_CLASS_KEY,
                clazz.getName());

        return new TaskParams(Integer.toString(TaskIds.TEST), extras);
    }
}
