// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.offlinepages;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.components.background_task_scheduler.BackgroundTaskScheduler;
import org.chromium.components.background_task_scheduler.BackgroundTaskSchedulerFactory;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskInfo;

/**
 * Unit tests for BackgroundScheduler.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BackgroundSchedulerTest {
    private TriggerConditions mConditions1 = new TriggerConditions(
            true /* power */, 10 /* battery percentage */, true /* requires unmetered */);
    private TriggerConditions mConditions2 = new TriggerConditions(
            false /* power */, 0 /* battery percentage */, false /* does not require unmetered */);

    @Mock
    private BackgroundTaskScheduler mTaskScheduler;
    @Captor
    ArgumentCaptor<TaskInfo> mTaskInfo;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        BackgroundTaskSchedulerFactory.setSchedulerForTesting(mTaskScheduler);
        doReturn(true)
                .when(mTaskScheduler)
                .schedule(eq(RuntimeEnvironment.application), mTaskInfo.capture());
    }

    private void verifyFixedTaskInfoValues(TaskInfo info) {
        assertEquals(TaskIds.OFFLINE_PAGES_BACKGROUND_JOB_ID, info.getTaskId());
        assertEquals(OfflineBackgroundTask.class, info.getBackgroundTaskClass());
        assertTrue(info.isPersisted());
        assertFalse(info.isPeriodic());
        assertEquals(BackgroundScheduler.ONE_WEEK_IN_MILLISECONDS,
                info.getOneOffInfo().getWindowEndTimeMs());
        assertTrue(info.getOneOffInfo().hasWindowStartTimeConstraint());

        long scheduledTimeMillis = TaskExtrasPacker.unpackTimeFromBundle(info.getExtras());
        assertTrue(scheduledTimeMillis > 0L);
    }

    @Test
    @Feature({"OfflinePages"})
    public void testScheduleUnmeteredAndCharging() {
        BackgroundScheduler.getInstance().schedule(mConditions1);
        verify(mTaskScheduler, times(1))
                .schedule(eq(RuntimeEnvironment.application), eq(mTaskInfo.getValue()));

        TaskInfo info = mTaskInfo.getValue();
        verifyFixedTaskInfoValues(info);

        assertEquals(TaskInfo.NETWORK_TYPE_UNMETERED, info.getRequiredNetworkType());
        assertTrue(info.requiresCharging());

        assertTrue(info.shouldUpdateCurrent());
        assertEquals(BackgroundScheduler.NO_DELAY, info.getOneOffInfo().getWindowStartTimeMs());

        assertEquals(
                mConditions1, TaskExtrasPacker.unpackTriggerConditionsFromBundle(info.getExtras()));
    }

    @Test
    @Feature({"OfflinePages"})
    public void testScheduleMeteredAndNotCharging() {
        BackgroundScheduler.getInstance().schedule(mConditions2);
        verify(mTaskScheduler, times(1))
                .schedule(eq(RuntimeEnvironment.application), eq(mTaskInfo.getValue()));

        TaskInfo info = mTaskInfo.getValue();
        verifyFixedTaskInfoValues(info);

        assertEquals(TaskInfo.NETWORK_TYPE_ANY, info.getRequiredNetworkType());
        assertFalse(info.requiresCharging());

        assertTrue(info.shouldUpdateCurrent());
        assertEquals(BackgroundScheduler.NO_DELAY, info.getOneOffInfo().getWindowStartTimeMs());

        assertEquals(
                mConditions2, TaskExtrasPacker.unpackTriggerConditionsFromBundle(info.getExtras()));
    }

    @Test
    @Feature({"OfflinePages"})
    public void testScheduleBackup() {
        BackgroundScheduler.getInstance().scheduleBackup(
                mConditions1, BackgroundScheduler.FIVE_MINUTES_IN_MILLISECONDS);
        verify(mTaskScheduler, times(1))
                .schedule(eq(RuntimeEnvironment.application), eq(mTaskInfo.getValue()));

        TaskInfo info = mTaskInfo.getValue();
        verifyFixedTaskInfoValues(info);

        assertEquals(TaskInfo.NETWORK_TYPE_UNMETERED, info.getRequiredNetworkType());
        assertTrue(info.requiresCharging());

        assertFalse(info.shouldUpdateCurrent());
        assertEquals(BackgroundScheduler.FIVE_MINUTES_IN_MILLISECONDS,
                info.getOneOffInfo().getWindowStartTimeMs());

        assertEquals(
                mConditions1, TaskExtrasPacker.unpackTriggerConditionsFromBundle(info.getExtras()));
    }

    @Test
    @Feature({"OfflinePages"})
    public void testCancel() {
        BackgroundScheduler.getInstance().schedule(mConditions1);
        verify(mTaskScheduler, times(1))
                .schedule(eq(RuntimeEnvironment.application), eq(mTaskInfo.getValue()));

        doNothing()
                .when(mTaskScheduler)
                .cancel(eq(RuntimeEnvironment.application),
                        eq(TaskIds.OFFLINE_PAGES_BACKGROUND_JOB_ID));
        BackgroundScheduler.getInstance().cancel();
        verify(mTaskScheduler, times(1))
                .cancel(eq(RuntimeEnvironment.application),
                        eq(TaskIds.OFFLINE_PAGES_BACKGROUND_JOB_ID));
    }
}
