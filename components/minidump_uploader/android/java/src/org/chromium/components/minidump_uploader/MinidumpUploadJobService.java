// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.components.minidump_uploader;

import android.annotation.TargetApi;
import android.app.job.JobInfo;
import android.app.job.JobParameters;
import android.app.job.JobScheduler;
import android.app.job.JobService;
import android.content.Context;
import android.os.Build;
import android.os.PersistableBundle;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;

/**
 * Class that interacts with the Android JobScheduler to upload Minidumps at appropriate times.
 */
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public abstract class MinidumpUploadJobService extends JobService {
    private static final String TAG = "MinidumpJobService";

    // Initial back-off time for upload-job, i.e. the minimum delay when a job is retried. A retry
    // will happen when there are minidumps left after trying to upload all minidumps. This could
    // happen if an upload attempt fails, or if more minidumps are added at the same time as
    // uploading old ones. The initial backoff is set to a fairly high number (30 minutes) to
    // increase the chance of performing uploads in batches if the initial upload fails.
    private static final int JOB_INITIAL_BACKOFF_TIME_IN_MS = 1000 * 60 * 30;

    // Back-off policy for upload-job.
    private static final int JOB_BACKOFF_POLICY = JobInfo.BACKOFF_POLICY_EXPONENTIAL;

    private MinidumpUploader mMinidumpUploader;

    // Used in Debug builds to assert that this job service never attempts to run more than one job
    // at a time:
    private final Object mRunningLock = new Object();
    private boolean mRunningJob = false;

    /**
     * Schedules uploading of all pending minidumps.
     * @param jobInfoBuilder A job info builder that has been initialized with any embedder-specific
     *     requriements. This builder will be extended to include shared requirements, and then used
     *     to build an upload job for scheduling.
     */
    public static void scheduleUpload(JobInfo.Builder jobInfoBuilder) {
        Log.i(TAG, "Scheduling upload of all pending minidumps.");
        JobScheduler scheduler =
                (JobScheduler) ContextUtils.getApplicationContext().getSystemService(
                        Context.JOB_SCHEDULER_SERVICE);
        JobInfo uploadJob =
                jobInfoBuilder.setRequiredNetworkType(JobInfo.NETWORK_TYPE_UNMETERED)
                        .setBackoffCriteria(JOB_INITIAL_BACKOFF_TIME_IN_MS, JOB_BACKOFF_POLICY)
                        .build();
        int result = scheduler.schedule(uploadJob);
        assert result == JobScheduler.RESULT_SUCCESS;
    }

    @Override
    public boolean onStartJob(JobParameters params) {
        // Ensure we only run one job at a time.
        synchronized (mRunningLock) {
            assert !mRunningJob;
            mRunningJob = true;
        }
        mMinidumpUploader = createMinidumpUploader(params.getExtras());
        mMinidumpUploader.uploadAllMinidumps(createJobFinishedCallback(params));
        return true; // true = processing work on a separate thread, false = done already.
    }

    @Override
    public boolean onStopJob(JobParameters params) {
        Log.i(TAG, "Canceling pending uploads due to change in networking status.");
        boolean reschedule = mMinidumpUploader.cancelUploads();
        synchronized (mRunningLock) {
            mRunningJob = false;
        }
        return reschedule;
    }

    @Override
    public void onDestroy() {
        mMinidumpUploader = null;
        super.onDestroy();
    }

    private MinidumpUploader.UploadsFinishedCallback createJobFinishedCallback(
            final JobParameters params) {
        return new MinidumpUploader.UploadsFinishedCallback() {
            @Override
            public void uploadsFinished(boolean reschedule) {
                if (reschedule) {
                    Log.i(TAG, "Some minidumps remain un-uploaded; rescheduling.");
                }
                synchronized (mRunningLock) {
                    mRunningJob = false;
                }
                MinidumpUploadJobService.this.jobFinished(params, reschedule);
            }
        };
    }

    /**
     * @param extras Any extra data persisted for this job.
     * @return The minidump uploader that jobs should use.
     */
    protected abstract MinidumpUploader createMinidumpUploader(PersistableBundle extras);
}
