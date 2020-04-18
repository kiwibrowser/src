// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.annotation.SuppressLint;
import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobWorkItem;
import android.content.ComponentName;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.VariationsUtils;
import org.chromium.android_webview.services.AwVariationsSeedFetcher;
import org.chromium.android_webview.services.ServiceInit;
import org.chromium.android_webview.test.util.VariationsTestUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.parameter.SkipCommandLineParameterization;
import org.chromium.components.background_task_scheduler.TaskIds;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher;
import org.chromium.components.variations.firstrun.VariationsSeedFetcher.SeedInfo;

import java.io.File;
import java.io.IOException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * Test AwVariationsSeedFetcher.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@SkipCommandLineParameterization
public class AwVariationsSeedFetcherTest {
    private static final int JOB_ID = TaskIds.WEBVIEW_VARIATIONS_SEED_FETCH_JOB_ID;

    // A mock JobScheduler which only holds one job, and never does anything with it.
    private class MockJobScheduler extends JobScheduler {
        public JobInfo mJob;

        public void clear() {
            mJob = null;
        }

        public void assertScheduled() {
            Assert.assertNotNull("No job scheduled", mJob);
        }

        public void assertNotScheduled() {
            Assert.assertNull("Job should not have been scheduled", mJob);
        }

        @Override
        public void cancel(int jobId) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void cancelAll() {
            throw new UnsupportedOperationException();
        }

        @Override
        public int enqueue(JobInfo job, JobWorkItem work) {
            throw new UnsupportedOperationException();
        }

        @Override
        public List<JobInfo> getAllPendingJobs() {
            ArrayList<JobInfo> list = new ArrayList<>();
            if (mJob != null) list.add(mJob);
            return list;
        }

        @Override
        public JobInfo getPendingJob(int jobId) {
            if (mJob != null && mJob.getId() == jobId) return mJob;
            return null;
        }

        @Override
        public int schedule(JobInfo job) {
            Assert.assertEquals("Job scheduled with wrong ID", JOB_ID, job.getId());
            Assert.assertEquals("Job scheduled with wrong network type",
                    JobInfo.NETWORK_TYPE_ANY, job.getNetworkType());
            Assert.assertTrue("Job scheduled without charging requirement",
                    job.isRequireCharging());
            mJob = job;
            return JobScheduler.RESULT_SUCCESS;
        }
    }

    // A mock VariationsSeedFetcher which doesn't actually download seeds, but verifies the request
    // parameters.
    private class MockFetcher extends VariationsSeedFetcher {
        public CallbackHelper helper = new CallbackHelper();

        @Override
        public SeedInfo downloadContent(VariationsSeedFetcher.VariationsPlatform platform,
                String restrictMode, String milestone, String channel)
                throws SocketTimeoutException, UnknownHostException, IOException {
            Assert.assertEquals(VariationsSeedFetcher.VariationsPlatform.ANDROID_WEBVIEW, platform);
            Assert.assertTrue(Integer.parseInt(milestone) > 0);
            helper.notifyCalled();
            return null;
        }
    }

    private MockJobScheduler mScheduler = new MockJobScheduler();
    private MockFetcher mDownloader = new MockFetcher();

    @Before
    public void setUp() throws IOException {
        ServiceInit.setPrivateDataDirectorySuffix();
        AwVariationsSeedFetcher.setMocks(mScheduler, mDownloader);
        VariationsTestUtils.deleteSeeds();
    }

    @After
    public void tearDown() throws IOException {
        AwVariationsSeedFetcher.setMocks(null, null);
        VariationsTestUtils.deleteSeeds();
    }

    // Test scheduleIfNeeded(), which should schedule a job.
    @Test
    @SmallTest
    public void testScheduleWithNoStamp() {
        try {
            AwVariationsSeedFetcher.scheduleIfNeeded();
            mScheduler.assertScheduled();
        } finally {
            mScheduler.clear();
        }
    }

    // Create a stamp file with time = epoch, indicating the download job hasn't run in a long time.
    // Then test scheduleIfNeeded(), which should schedule a job.
    @Test
    @MediumTest
    public void testScheduleWithExpiredStamp() throws IOException {
        File stamp = VariationsUtils.getStampFile();
        try {
            Assert.assertFalse("Stamp file already exists", stamp.exists());
            Assert.assertTrue("Failed to create stamp file", stamp.createNewFile());
            Assert.assertTrue("Failed to set stamp time", stamp.setLastModified(0));
            AwVariationsSeedFetcher.scheduleIfNeeded();
            mScheduler.assertScheduled();
        } finally {
            mScheduler.clear();
            VariationsTestUtils.deleteSeeds(); // Remove the stamp file.
        }
    }

    // Create a stamp file with time = now, indicating the download job ran recently. Then test
    // scheduleIfNeeded(), which should not schedule a job.
    @Test
    @MediumTest
    public void testScheduleWithFreshStamp() throws IOException {
        File stamp = VariationsUtils.getStampFile();
        try {
            Assert.assertFalse("Stamp file already exists", stamp.exists());
            Assert.assertTrue("Failed to create stamp file", stamp.createNewFile());
            AwVariationsSeedFetcher.scheduleIfNeeded();
            mScheduler.assertNotScheduled();
        } finally {
            mScheduler.clear();
            VariationsTestUtils.deleteSeeds(); // Remove the stamp file.
        }
    }

    // Pretend that a job is already scheduled. Then test scheduleIfNeeded(), which should not
    // schedule a job.
    @Test
    @SmallTest
    public void testScheduleAlreadyScheduled() {
        File stamp = VariationsUtils.getStampFile();
        try {
            @SuppressLint("JobSchedulerService")
            ComponentName component = new ComponentName(
                    ContextUtils.getApplicationContext(), AwVariationsSeedFetcher.class);
            JobInfo job = new JobInfo.Builder(JOB_ID, component)
                    .setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)
                    .setRequiresCharging(true)
                    .build();
            mScheduler.schedule(job);
            AwVariationsSeedFetcher.scheduleIfNeeded();
            // Check that our job object hasn't been replaced (meaning that scheduleIfNeeded didn't
            // schedule a job).
            Assert.assertSame(job, mScheduler.getPendingJob(JOB_ID));
        } finally {
            mScheduler.clear();
        }
    }

    @Test
    @SmallTest
    public void testFetch() throws IOException, InterruptedException, TimeoutException {
        try {
            AwVariationsSeedFetcher fetcher = new AwVariationsSeedFetcher() {
                // p is null in this test. Don't actually call JobService.jobFinished.
                @Override
                protected void jobFinished(JobParameters p) {}
            };
            int downloads = mDownloader.helper.getCallCount();
            fetcher.onStartJob(null);
            mDownloader.helper.waitForCallback(
                    "Timeout out waiting for AwVariationsSeedFetcher to call downloadContent",
                    downloads);
            File stamp = VariationsUtils.getStampFile();
            Assert.assertTrue("AwVariationsSeedFetcher should have updated stamp file " + stamp,
                    stamp.exists());
        } finally {
            VariationsTestUtils.deleteSeeds(); // Remove the stamp file.
        }
    }
}
