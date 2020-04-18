// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.background_task_scheduler;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.content.Context;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.LoaderErrors;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.init.BrowserParts;
import org.chromium.chrome.browser.init.ChromeBrowserInitializer;
import org.chromium.components.background_task_scheduler.BackgroundTask;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.background_task_scheduler.TaskParameters;
import org.chromium.content.browser.BrowserStartupController;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/** Unit tests for {@link BackgroundTaskScheduler}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class NativeBackgroundTaskTest {
    private enum InitializerSetup {
        SUCCESS,
        FAILURE,
        EXCEPTION,
    }

    private static class LazyTaskParameters {
        static final TaskParameters INSTANCE = TaskParameters.create(TaskIds.TEST).build();
    }

    private static TaskParameters getTaskParameters() {
        return LazyTaskParameters.INSTANCE;
    }

    @Mock
    private BrowserStartupController mBrowserStartupController;
    @Mock
    private ChromeBrowserInitializer mChromeBrowserInitializer;
    @Captor
    ArgumentCaptor<BrowserParts> mBrowserParts;

    private static class TaskFinishedCallback implements BackgroundTask.TaskFinishedCallback {
        private boolean mWasCalled;
        private boolean mNeedsReschedule;
        private CountDownLatch mCallbackLatch;

        TaskFinishedCallback() {
            mCallbackLatch = new CountDownLatch(1);
        }

        @Override
        public void taskFinished(boolean needsReschedule) {
            mNeedsReschedule = needsReschedule;
            mWasCalled = true;
            mCallbackLatch.countDown();
        }

        boolean wasCalled() {
            return mWasCalled;
        }

        boolean needsRescheduling() {
            return mNeedsReschedule;
        }

        boolean waitOnCallback() {
            return waitOnLatch(mCallbackLatch);
        }
    }

    private static class TestNativeBackgroundTask extends NativeBackgroundTask {
        @StartBeforeNativeResult
        private int mStartBeforeNativeResult;
        private boolean mWasOnStartTaskWithNativeCalled;
        private boolean mNeedsReschedulingAfterStop;
        private CountDownLatch mStartWithNativeLatch;
        private boolean mWasOnStopTaskWithNativeCalled;
        private boolean mWasOnStopTaskBeforeNativeLoadedCalled;

        public TestNativeBackgroundTask() {
            mWasOnStartTaskWithNativeCalled = false;
            mStartBeforeNativeResult = LOAD_NATIVE;
            mNeedsReschedulingAfterStop = false;
            mStartWithNativeLatch = new CountDownLatch(1);
        }

        @Override
        protected int onStartTaskBeforeNativeLoaded(
                Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
            return mStartBeforeNativeResult;
        }

        @Override
        protected void onStartTaskWithNative(
                Context context, TaskParameters taskParameters, TaskFinishedCallback callback) {
            assertEquals(RuntimeEnvironment.application, context);
            assertEquals(getTaskParameters(), taskParameters);
            mWasOnStartTaskWithNativeCalled = true;
            mStartWithNativeLatch.countDown();
        }

        @Override
        protected boolean onStopTaskBeforeNativeLoaded(
                Context context, TaskParameters taskParameters) {
            mWasOnStopTaskBeforeNativeLoadedCalled = true;
            return mNeedsReschedulingAfterStop;
        }

        @Override
        protected boolean onStopTaskWithNative(Context context, TaskParameters taskParameters) {
            mWasOnStopTaskWithNativeCalled = true;
            return mNeedsReschedulingAfterStop;
        }

        @Override
        public void reschedule(Context context) {}

        boolean waitOnStartWithNativeCallback() {
            return waitOnLatch(mStartWithNativeLatch);
        }

        boolean wasOnStartTaskWithNativeCalled() {
            return mWasOnStartTaskWithNativeCalled;
        }

        boolean wasOnStopTaskWithNativeCalled() {
            return mWasOnStopTaskWithNativeCalled;
        }

        boolean wasOnStopTaskBeforeNativeLoadedCalled() {
            return mWasOnStopTaskBeforeNativeLoadedCalled;
        }

        void setStartTaskBeforeNativeResult(@StartBeforeNativeResult int result) {
            mStartBeforeNativeResult = result;
        }

        void setNeedsReschedulingAfterStop(boolean needsReschedulingAfterStop) {
            mNeedsReschedulingAfterStop = needsReschedulingAfterStop;
        }
    }

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        ReflectionHelpers.setField(mBrowserStartupController, "mLibraryProcessType",
                LibraryProcessType.PROCESS_BROWSER);
        BrowserStartupController.overrideInstanceForTest(mBrowserStartupController);
        ChromeBrowserInitializer.setForTesting(mChromeBrowserInitializer);
    }

    private void setUpChromeBrowserInitializer(InitializerSetup setup) {
        doNothing().when(mChromeBrowserInitializer).handlePreNativeStartup(any(BrowserParts.class));
        try {
            switch (setup) {
                case SUCCESS:
                    doAnswer(new Answer<Void>() {
                        @Override
                        public Void answer(InvocationOnMock invocation) {
                            mBrowserParts.getValue().finishNativeInitialization();
                            return null;
                        }
                    })
                            .when(mChromeBrowserInitializer)
                            .handlePostNativeStartup(eq(true), mBrowserParts.capture());
                    break;
                case FAILURE:
                    doAnswer(new Answer<Void>() {
                        @Override
                        public Void answer(InvocationOnMock invocation) {
                            mBrowserParts.getValue().onStartupFailure();
                            return null;
                        }
                    })
                            .when(mChromeBrowserInitializer)
                            .handlePostNativeStartup(eq(true), mBrowserParts.capture());
                    break;
                case EXCEPTION:
                    doThrow(new ProcessInitException(
                                    LoaderErrors.LOADER_ERROR_NATIVE_LIBRARY_LOAD_FAILED))
                            .when(mChromeBrowserInitializer)
                            .handlePostNativeStartup(eq(true), any(BrowserParts.class));
                    break;
                default:
                    assert false;
            }
        } catch (ProcessInitException e) {
            // Exception ignored, as try-catch is required by language.
        }
    }

    private void verifyStartupCalls(int expectedPreNativeCalls, int expectedPostNativeCalls) {
        try {
            verify(mChromeBrowserInitializer, times(expectedPreNativeCalls))
                    .handlePreNativeStartup(any(BrowserParts.class));
            verify(mChromeBrowserInitializer, times(expectedPostNativeCalls))
                    .handlePostNativeStartup(eq(true), any(BrowserParts.class));
        } catch (ProcessInitException e) {
            // Exception ignored, as try-catch is required by language.
        }
    }

    private static boolean waitOnLatch(CountDownLatch latch) {
        try {
            // All tests are expected to get it done much faster
            return latch.await(5, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            return false;
        }
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStartTask_Done_BeforeNativeLoaded() {
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.setStartTaskBeforeNativeResult(NativeBackgroundTask.DONE);
        assertFalse(
                task.onStartTask(RuntimeEnvironment.application, getTaskParameters(), callback));

        verify(mBrowserStartupController, times(0)).isStartupSuccessfullyCompleted();
        verifyStartupCalls(0, 0);
        assertFalse(task.wasOnStartTaskWithNativeCalled());
        assertFalse(callback.wasCalled());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStartTask_Reschedule_BeforeNativeLoaded() {
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.setStartTaskBeforeNativeResult(NativeBackgroundTask.RESCHEDULE);
        assertTrue(task.onStartTask(RuntimeEnvironment.application, getTaskParameters(), callback));

        assertTrue(callback.waitOnCallback());
        verify(mBrowserStartupController, times(0)).isStartupSuccessfullyCompleted();
        verifyStartupCalls(0, 0);
        assertFalse(task.wasOnStartTaskWithNativeCalled());
        assertTrue(callback.wasCalled());
        assertTrue(callback.needsRescheduling());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStartTask_NativeAlreadyLoaded() {
        doReturn(true).when(mBrowserStartupController).isStartupSuccessfullyCompleted();
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.onStartTask(RuntimeEnvironment.application, getTaskParameters(), callback);

        assertTrue(task.waitOnStartWithNativeCallback());
        verify(mBrowserStartupController, times(1)).isStartupSuccessfullyCompleted();
        verifyStartupCalls(0, 0);
        assertTrue(task.wasOnStartTaskWithNativeCalled());
        assertFalse(callback.wasCalled());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStartTask_NativeInitialization_Success() {
        doReturn(false).when(mBrowserStartupController).isStartupSuccessfullyCompleted();
        setUpChromeBrowserInitializer(InitializerSetup.SUCCESS);
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.onStartTask(RuntimeEnvironment.application, getTaskParameters(), callback);

        assertTrue(task.waitOnStartWithNativeCallback());
        verify(mBrowserStartupController, times(1)).isStartupSuccessfullyCompleted();
        verifyStartupCalls(1, 1);
        assertTrue(task.wasOnStartTaskWithNativeCalled());
        assertFalse(callback.wasCalled());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStartTask_NativeInitialization_Failure() {
        doReturn(false).when(mBrowserStartupController).isStartupSuccessfullyCompleted();
        setUpChromeBrowserInitializer(InitializerSetup.FAILURE);
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.onStartTask(RuntimeEnvironment.application, getTaskParameters(), callback);

        assertTrue(callback.waitOnCallback());
        verify(mBrowserStartupController, times(1)).isStartupSuccessfullyCompleted();
        verifyStartupCalls(1, 1);
        assertFalse(task.wasOnStartTaskWithNativeCalled());
        assertTrue(callback.wasCalled());
        assertTrue(callback.needsRescheduling());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStartTask_NativeInitialization_Throws() {
        doReturn(false).when(mBrowserStartupController).isStartupSuccessfullyCompleted();
        setUpChromeBrowserInitializer(InitializerSetup.EXCEPTION);
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.onStartTask(RuntimeEnvironment.application, getTaskParameters(), callback);

        assertTrue(callback.waitOnCallback());
        verify(mBrowserStartupController, times(1)).isStartupSuccessfullyCompleted();
        verifyStartupCalls(1, 1);
        assertFalse(task.wasOnStartTaskWithNativeCalled());
        assertTrue(callback.wasCalled());
        assertTrue(callback.needsRescheduling());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStopTask_BeforeNativeLoaded_NeedsRescheduling() {
        doReturn(false).when(mBrowserStartupController).isStartupSuccessfullyCompleted();
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.setNeedsReschedulingAfterStop(true);

        assertTrue(task.onStopTask(RuntimeEnvironment.application, getTaskParameters()));
        assertTrue(task.wasOnStopTaskBeforeNativeLoadedCalled());
        assertFalse(task.wasOnStopTaskWithNativeCalled());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStopTask_BeforeNativeLoaded_DoesntNeedRescheduling() {
        doReturn(false).when(mBrowserStartupController).isStartupSuccessfullyCompleted();
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.setNeedsReschedulingAfterStop(false);

        assertFalse(task.onStopTask(RuntimeEnvironment.application, getTaskParameters()));
        assertTrue(task.wasOnStopTaskBeforeNativeLoadedCalled());
        assertFalse(task.wasOnStopTaskWithNativeCalled());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStopTask_NativeLoaded_NeedsRescheduling() {
        doReturn(true).when(mBrowserStartupController).isStartupSuccessfullyCompleted();
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.setNeedsReschedulingAfterStop(true);

        assertTrue(task.onStopTask(RuntimeEnvironment.application, getTaskParameters()));
        assertFalse(task.wasOnStopTaskBeforeNativeLoadedCalled());
        assertTrue(task.wasOnStopTaskWithNativeCalled());
    }

    @Test
    @Feature("BackgroundTaskScheduler")
    public void testOnStopTask_NativeLoaded_DoesntNeedRescheduling() {
        doReturn(true).when(mBrowserStartupController).isStartupSuccessfullyCompleted();
        TaskFinishedCallback callback = new TaskFinishedCallback();
        TestNativeBackgroundTask task = new TestNativeBackgroundTask();
        task.setNeedsReschedulingAfterStop(false);

        assertFalse(task.onStopTask(RuntimeEnvironment.application, getTaskParameters()));
        assertFalse(task.wasOnStopTaskBeforeNativeLoadedCalled());
        assertTrue(task.wasOnStopTaskWithNativeCalled());
    }
}
