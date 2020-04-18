// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feature_engagement;

import android.os.FileObserver;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

import java.io.File;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
/**
 * Tests ScreenshotMonitor.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
public class ScreenshotMonitorTest {
    private static final String FILENAME = "image.jpeg";
    private static final String TAG = "ScreenshotTest";

    private ScreenshotMonitor mTestScreenshotMonitor;
    private TestScreenshotMonitorDelegate mTestScreenshotMonitorDelegate;
    private TestScreenshotMonitorFileObserver mTestFileObserver;

    /**
     * This class acts as the inner FileObserver used by TestScreenshotMonitor to test that
     * ScreenshotMonitor calls the FileObserver.
     */
    static class TestScreenshotMonitorFileObserver
            extends ScreenshotMonitor.ScreenshotMonitorFileObserver {
        // The number of times concurrent watching occurs. It corresponds to the number of times
        // startWatching() and stopWatching() are called. It is modified on the UI thread and
        // accessed on the test thread.
        public final AtomicInteger watchingCount = new AtomicInteger();

        // Note: FileObserver's startWatching will have no effect if monitoring started already.
        @Override
        public void startWatching() {
            watchingCount.getAndIncrement();
        }

        // Note: FileObserver's stopWatching will have no effect if monitoring stopped already.
        @Override
        public void stopWatching() {
            watchingCount.getAndDecrement();
        }
    }

    static class TestScreenshotMonitorDelegate implements ScreenshotMonitorDelegate {
        // This is modified on the UI thread and accessed on the test thread.
        public final AtomicInteger screenshotShowUiCount = new AtomicInteger();

        @Override
        public void onScreenshotTaken() {
            Assert.assertTrue(ThreadUtils.runningOnUiThread());
            screenshotShowUiCount.getAndIncrement();
        }
    }

    static String getTestFilePath() {
        return ScreenshotMonitor.ScreenshotMonitorFileObserver.getDirPath() + File.separator
                + FILENAME;
    }

    @Before
    public void setUp() throws Exception {
        mTestScreenshotMonitorDelegate = new TestScreenshotMonitorDelegate();

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor = new ScreenshotMonitor(
                        mTestScreenshotMonitorDelegate, new TestScreenshotMonitorFileObserver());
            }
        });
        Assert.assertTrue(
                mTestScreenshotMonitor.mFileObserver instanceof TestScreenshotMonitorFileObserver);
        mTestFileObserver =
                (TestScreenshotMonitorFileObserver) mTestScreenshotMonitor.mFileObserver;
    }

    /**
     * Verify that if monitoring starts, the delegate should be called. Also verify that the
     * inner TestFileObserver monitors as expected.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagement", "Screenshot"})
    public void testStartMonitoringShouldTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());

        startMonitoringOnUiThreadBlocking();
        Assert.assertEquals(1, mTestFileObserver.watchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.screenshotShowUiCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(1);

        stopMonitoringOnUiThreadBlocking();
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());
    }

    /**
     * Verify that if monitoring starts multiple times, the delegate should still be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagement", "Screenshot"})
    public void testMultipleStartMonitoringShouldTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());

        startMonitoringOnUiThreadBlocking();
        startMonitoringOnUiThreadBlocking();
        Assert.assertEquals(2, mTestFileObserver.watchingCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(1);
    }

    /**
     * Verify that if monitoring stops multiple times, the delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagement", "Screenshot"})
    public void testMultipleStopMonitoringShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());

        stopMonitoringOnUiThreadBlocking();
        stopMonitoringOnUiThreadBlocking();
        Assert.assertEquals(-2, mTestFileObserver.watchingCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(0);
    }

    /**
     * Verify that even if startMonitoring is called multiple times before stopMonitoring, the
     * delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagement", "Screenshot"})
    public void testMultipleStartMonitoringThenStopShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.screenshotShowUiCount.get());

        multipleStartMonitoringBeforeStopMonitoringOnUiThreadBlocking(2);
        Assert.assertEquals(1, mTestFileObserver.watchingCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(0);
    }

    /**
     * Verify that if stopMonitoring is called multiple times before startMonitoring, the delegate
     * should still be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagement", "Screenshot"})
    public void testMultipleStopMonitoringThenStartShouldTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());

        multipleStopMonitoringBeforeStartMonitoringOnUiThreadBlocking(2);
        Assert.assertEquals(-1, mTestFileObserver.watchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.screenshotShowUiCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(1);
    }

    /**
     * Verify that the delegate is called after a restart.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagement", "Screenshot"})
    public void testRestartShouldTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());

        startMonitoringOnUiThreadBlocking();
        Assert.assertEquals(1, mTestFileObserver.watchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.screenshotShowUiCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(1);

        stopMonitoringOnUiThreadBlocking();
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());

        // Restart and call onEvent a second time
        startMonitoringOnUiThreadBlocking();
        Assert.assertEquals(1, mTestFileObserver.watchingCount.get());
        Assert.assertEquals(1, mTestScreenshotMonitorDelegate.screenshotShowUiCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(2);
    }

    /**
     * Verify that if monitoring stops, the delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagement", "Screenshot"})
    public void testStopMonitoringShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());

        startMonitoringOnUiThreadBlocking();
        Assert.assertEquals(1, mTestFileObserver.watchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.screenshotShowUiCount.get());

        stopMonitoringOnUiThreadBlocking();
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(0);
    }

    /**
     * Verify that if monitoring is never started, the delegate should not be called.
     */
    @Test
    @SmallTest
    @Feature({"FeatureEngagement", "Screenshot"})
    public void testNoMonitoringShouldNotTriggerDelegate() throws Throwable {
        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());
        Assert.assertEquals(0, mTestScreenshotMonitorDelegate.screenshotShowUiCount.get());

        mTestScreenshotMonitor.mFileObserver.onEvent(FileObserver.CREATE, getTestFilePath());
        assertScreenshotShowUiCountOnUiThreadBlocking(0);

        Assert.assertEquals(0, mTestFileObserver.watchingCount.get());
    }

    // This ensures that the UI thread finishes executing startMonitoring.
    private void startMonitoringOnUiThreadBlocking() {
        final Semaphore semaphore = new Semaphore(0);

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.startMonitoring();
                semaphore.release();
            }
        });
        try {
            Assert.assertTrue(semaphore.tryAcquire(10, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
    }

    // This ensures that the UI thread finishes executing stopMonitoring.
    private void stopMonitoringOnUiThreadBlocking() {
        final Semaphore semaphore = new Semaphore(0);

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                mTestScreenshotMonitor.stopMonitoring();
                semaphore.release();
            }
        });
        try {
            Assert.assertTrue(semaphore.tryAcquire(10, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
    }

    // This ensures that the UI thread finishes executing startCallCount calls to startMonitoring
    // before calling stopMonitoring.
    private void multipleStartMonitoringBeforeStopMonitoringOnUiThreadBlocking(
            final int startCallCount) {
        final Semaphore semaphore = new Semaphore(0);

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < startCallCount; i++) {
                    mTestScreenshotMonitor.startMonitoring();
                }
                mTestScreenshotMonitor.stopMonitoring();
                semaphore.release();
            }
        });
        try {
            Assert.assertTrue(semaphore.tryAcquire(10, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
    }

    // This ensures that the UI thread finishes executing stopCallCount calls to stopMonitoring
    // before calling startMonitoring.
    private void multipleStopMonitoringBeforeStartMonitoringOnUiThreadBlocking(
            final int stopCallCount) {
        final Semaphore semaphore = new Semaphore(0);

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                for (int i = 0; i < stopCallCount; i++) {
                    mTestScreenshotMonitor.stopMonitoring();
                }
                mTestScreenshotMonitor.startMonitoring();
                semaphore.release();
            }
        });
        try {
            Assert.assertTrue(semaphore.tryAcquire(10, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
    }

    // This ensures that after UI thread finishes all tasks, screenshotShowUiCount equals
    // expectedCount.
    private void assertScreenshotShowUiCountOnUiThreadBlocking(int expectedCount) {
        final Semaphore semaphore = new Semaphore(0);

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                semaphore.release();
            }
        });
        try {
            Assert.assertTrue(semaphore.tryAcquire(10, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            Log.e(TAG, "Cannot acquire semaphore");
        }
        Assert.assertEquals(
                expectedCount, mTestScreenshotMonitorDelegate.screenshotShowUiCount.get());
    }
}