// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.background_task_scheduler;

import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.os.PersistableBundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.MinAndroidSdkLevel;

import java.util.concurrent.TimeUnit;

/**
 * Tests for {@link BackgroundTaskSchedulerJobService}.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@TargetApi(Build.VERSION_CODES.LOLLIPOP_MR1)
@MinAndroidSdkLevel(Build.VERSION_CODES.LOLLIPOP_MR1)
public class BackgroundTaskSchedulerJobServiceTest {
    private static class TestBackgroundTask implements BackgroundTask {
        @Override
        public boolean onStartTask(
                Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
            return false;
        }

        @Override
        public boolean onStopTask(Context context, TaskParameters taskParameters) {
            return false;
        }

        @Override
        public void reschedule(Context context) {}
    }

    @Test
    @SmallTest
    public void testOneOffTaskInfoWithDeadlineConversion() {
        TaskInfo oneOffTask = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                              TimeUnit.MINUTES.toMillis(200))
                                      .build();
        JobInfo jobInfo = BackgroundTaskSchedulerJobService.createJobInfoFromTaskInfo(
                InstrumentationRegistry.getTargetContext(), oneOffTask);
        Assert.assertEquals(oneOffTask.getTaskId(), jobInfo.getId());
        Assert.assertFalse(jobInfo.isPeriodic());
        Assert.assertEquals(oneOffTask.getOneOffInfo().getWindowEndTimeMs(),
                jobInfo.getMaxExecutionDelayMillis());
    }

    @Test
    @SmallTest
    public void testOneOffTaskInfoWithWindowConversion() {
        TaskInfo oneOffTask =
                TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                TimeUnit.MINUTES.toMillis(100), TimeUnit.MINUTES.toMillis(200))
                        .build();
        JobInfo jobInfo = BackgroundTaskSchedulerJobService.createJobInfoFromTaskInfo(
                InstrumentationRegistry.getTargetContext(), oneOffTask);
        Assert.assertEquals(oneOffTask.getTaskId(), jobInfo.getId());
        Assert.assertFalse(jobInfo.isPeriodic());
        Assert.assertEquals(
                oneOffTask.getOneOffInfo().getWindowStartTimeMs(), jobInfo.getMinLatencyMillis());
        Assert.assertEquals(oneOffTask.getOneOffInfo().getWindowEndTimeMs(),
                jobInfo.getMaxExecutionDelayMillis());
    }

    @Test
    @SmallTest
    public void testPeriodicTaskInfoWithoutFlexConversion() {
        TaskInfo periodicTask = TaskInfo.createPeriodicTask(TaskIds.TEST, TestBackgroundTask.class,
                                                TimeUnit.MINUTES.toMillis(200))
                                        .build();
        JobInfo jobInfo = BackgroundTaskSchedulerJobService.createJobInfoFromTaskInfo(
                InstrumentationRegistry.getTargetContext(), periodicTask);
        Assert.assertEquals(periodicTask.getTaskId(), jobInfo.getId());
        Assert.assertTrue(jobInfo.isPeriodic());
        Assert.assertEquals(
                periodicTask.getPeriodicInfo().getIntervalMs(), jobInfo.getIntervalMillis());
    }

    @Test
    @SmallTest
    public void testPeriodicTaskInfoWithFlexConversion() {
        TaskInfo periodicTask =
                TaskInfo.createPeriodicTask(TaskIds.TEST, TestBackgroundTask.class,
                                TimeUnit.MINUTES.toMillis(200), TimeUnit.MINUTES.toMillis(50))
                        .build();
        JobInfo jobInfo = BackgroundTaskSchedulerJobService.createJobInfoFromTaskInfo(
                InstrumentationRegistry.getTargetContext(), periodicTask);
        Assert.assertEquals(periodicTask.getTaskId(), jobInfo.getId());
        Assert.assertTrue(jobInfo.isPeriodic());
        Assert.assertEquals(
                periodicTask.getPeriodicInfo().getIntervalMs(), jobInfo.getIntervalMillis());
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            Assert.assertEquals(
                    periodicTask.getPeriodicInfo().getFlexMs(), jobInfo.getFlexMillis());
        }
    }

    @Test
    @SmallTest
    public void testTaskInfoWithExtras() {
        Bundle taskExtras = new Bundle();
        taskExtras.putString("foo", "bar");
        taskExtras.putBoolean("bools", true);
        taskExtras.putLong("longs", 1342543L);
        TaskInfo oneOffTask = TaskInfo.createOneOffTask(TaskIds.TEST, TestBackgroundTask.class,
                                              TimeUnit.MINUTES.toMillis(200))
                                      .setExtras(taskExtras)
                                      .build();
        JobInfo jobInfo = BackgroundTaskSchedulerJobService.createJobInfoFromTaskInfo(
                InstrumentationRegistry.getTargetContext(), oneOffTask);
        Assert.assertEquals(oneOffTask.getTaskId(), jobInfo.getId());
        PersistableBundle jobExtras = jobInfo.getExtras();
        PersistableBundle persistableBundle = jobExtras.getPersistableBundle(
                BackgroundTaskSchedulerJobService.BACKGROUND_TASK_EXTRAS_KEY);
        Assert.assertEquals(taskExtras.keySet().size(), persistableBundle.keySet().size());
        Assert.assertEquals(taskExtras.getString("foo"), persistableBundle.getString("foo"));
        Assert.assertEquals(taskExtras.getBoolean("bools"), persistableBundle.getBoolean("bools"));
        Assert.assertEquals(taskExtras.getLong("longs"), persistableBundle.getLong("longs"));
    }

    @Test
    @SmallTest
    public void testTaskInfoWithManyConstraints() {
        TaskInfo.Builder taskBuilder = TaskInfo.createOneOffTask(
                TaskIds.TEST, TestBackgroundTask.class, TimeUnit.MINUTES.toMillis(200));

        JobInfo jobInfo = BackgroundTaskSchedulerJobService.createJobInfoFromTaskInfo(
                InstrumentationRegistry.getTargetContext(),
                taskBuilder.setIsPersisted(true).build());
        Assert.assertTrue(jobInfo.isPersisted());

        jobInfo = BackgroundTaskSchedulerJobService.createJobInfoFromTaskInfo(
                InstrumentationRegistry.getTargetContext(),
                taskBuilder.setRequiredNetworkType(TaskInfo.NETWORK_TYPE_UNMETERED).build());
        Assert.assertEquals(JobInfo.NETWORK_TYPE_UNMETERED, jobInfo.getNetworkType());

        jobInfo = BackgroundTaskSchedulerJobService.createJobInfoFromTaskInfo(
                InstrumentationRegistry.getTargetContext(),
                taskBuilder.setRequiresCharging(true).build());
        Assert.assertTrue(jobInfo.isRequireCharging());
    }
}
