// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.minidump_uploader;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.components.minidump_uploader.CrashTestRule.MockCrashReportingPermissionManager;
import org.chromium.components.minidump_uploader.util.CrashReportingPermissionManager;
import org.chromium.components.minidump_uploader.util.HttpURLConnectionFactory;

import java.io.File;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;

/**
 * Instrumentation tests for the common MinidumpUploader implementation within the minidump_uploader
 * component.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class MinidumpUploaderImplTest {
    @Rule
    public CrashTestRule mTestRule = new CrashTestRule();

    private static final String BOUNDARY = "TESTBOUNDARY";

    /**
     * Test to ensure the minidump uploading mechanism behaves as expected when we fail to upload
     * minidumps.
     */
    @Test
    @MediumTest
    public void testFailUploadingMinidumps() throws IOException {
        final CrashReportingPermissionManager permManager =
                new MockCrashReportingPermissionManager() {
                    {
                        mIsInSample = true;
                        mIsUserPermitted = true;
                        mIsNetworkAvailable = false; // Will cause us to fail uploads
                        mIsEnabledForTests = false;
                    }
                };
        MinidumpUploader minidumpUploader =
                new TestMinidumpUploaderImpl(mTestRule.getExistingCacheDir(), permManager);

        File firstFile = createMinidumpFileInCrashDir("1_abc.dmp0.try0");
        File secondFile = createMinidumpFileInCrashDir("12_abc.dmp0.try0");
        String triesBelowMaxString = ".try" + (MinidumpUploaderImpl.MAX_UPLOAD_TRIES_ALLOWED - 1);
        String maxTriesString = ".try" + MinidumpUploaderImpl.MAX_UPLOAD_TRIES_ALLOWED;
        File justBelowMaxTriesFile =
                createMinidumpFileInCrashDir("belowmaxtries.dmp0" + triesBelowMaxString);
        File maxTriesFile = createMinidumpFileInCrashDir("maxtries.dmp0" + maxTriesString);

        File expectedFirstFile = new File(mTestRule.getCrashDir(), "1_abc.dmp0.try1");
        File expectedSecondFile = new File(mTestRule.getCrashDir(), "12_abc.dmp0.try1");
        File expectedJustBelowMaxTriesFile = new File(mTestRule.getCrashDir(),
                justBelowMaxTriesFile.getName().replace(triesBelowMaxString, maxTriesString));

        MinidumpUploadTestUtility.uploadMinidumpsSync(
                minidumpUploader, true /* expectReschedule */);
        Assert.assertFalse(firstFile.exists());
        Assert.assertFalse(secondFile.exists());
        Assert.assertFalse(justBelowMaxTriesFile.exists());
        Assert.assertTrue(expectedFirstFile.exists());
        Assert.assertTrue(expectedSecondFile.exists());
        Assert.assertTrue(expectedJustBelowMaxTriesFile.exists());
        // This file should have been left untouched.
        Assert.assertTrue(maxTriesFile.exists());
    }

    @Test
    @MediumTest
    public void testFailingThenPassingUpload() throws IOException {
        final CrashReportingPermissionManager permManager =
                new MockCrashReportingPermissionManager() {
                    { mIsEnabledForTests = true; }
                };
        List<MinidumpUploadCallableCreator> callables = new ArrayList<>();
        callables.add(new MinidumpUploadCallableCreator() {
            @Override
            public MinidumpUploadCallable createCallable(File minidumpFile, File logfile) {
                return new MinidumpUploadCallable(
                        minidumpFile, logfile, new FailingHttpUrlConnectionFactory(), permManager);
            }
        });
        callables.add(new MinidumpUploadCallableCreator() {
            @Override
            public MinidumpUploadCallable createCallable(File minidumpFile, File logfile) {
                return new MinidumpUploadCallable(minidumpFile, logfile,
                        new MinidumpUploadCallableTest.TestHttpURLConnectionFactory(), permManager);
            }
        });
        MinidumpUploader minidumpUploader = createCallableListMinidumpUploader(
                callables, permManager.isUsageAndCrashReportingPermittedByUser());

        File firstFile = createMinidumpFileInCrashDir("firstFile.dmp0.try0");
        File secondFile = createMinidumpFileInCrashDir("secondFile.dmp0.try0");

        MinidumpUploadTestUtility.uploadMinidumpsSync(
                minidumpUploader, true /* expectReschedule */);
        Assert.assertFalse(firstFile.exists());
        Assert.assertFalse(secondFile.exists());
        File expectedSecondFile;
        // Not sure which minidump will fail and which will succeed, so just ensure one was uploaded
        // and the other one failed.
        if (new File(mTestRule.getCrashDir(), "firstFile.dmp0.try1").exists()) {
            expectedSecondFile = new File(mTestRule.getCrashDir(), "secondFile.up0.try0");
        } else {
            File uploadedFirstFile = new File(mTestRule.getCrashDir(), "firstFile.up0.try0");
            Assert.assertTrue(uploadedFirstFile.exists());
            expectedSecondFile = new File(mTestRule.getCrashDir(), "secondFile.dmp0.try1");
        }
        Assert.assertTrue(expectedSecondFile.exists());
    }

    /**
     * Prior to M60, the ".try0" suffix was optional; however now it is not. This test verifies that
     * the code rejects minidumps that lack this suffix.
     */
    @Test
    @MediumTest
    public void testInvalidMinidumpNameGeneratesNoUploads() throws IOException {
        MinidumpUploader minidumpUploader =
                new ExpectNoUploadsMinidumpUploaderImpl(mTestRule.getExistingCacheDir());

        // Note the omitted ".try0" suffix.
        File fileUsingLegacyNamingScheme = createMinidumpFileInCrashDir("1_abc.dmp0");

        MinidumpUploadTestUtility.uploadMinidumpsSync(
                minidumpUploader, false /* expectReschedule */);

        // The file should not have been touched, nor should any successful upload files have
        // appeared.
        Assert.assertTrue(fileUsingLegacyNamingScheme.exists());
        Assert.assertFalse(new File(mTestRule.getCrashDir(), "1_abc.up0").exists());
        Assert.assertFalse(new File(mTestRule.getCrashDir(), "1_abc.up0.try0").exists());
    }

    /**
     * Test that ensures we can interrupt the MinidumpUploader when uploading minidumps.
     */
    @Test
    @MediumTest
    public void testCancelMinidumpUploadsFailedUpload() throws IOException {
        testCancellation(false /* successfulUpload */);
    }

    /**
     * Test that ensures interrupting our upload-job will not interrupt the first upload.
     */
    @Test
    @MediumTest
    public void testCancelingWontCancelFirstUpload() throws IOException {
        testCancellation(true /* successfulUpload */);
    }

    private void testCancellation(final boolean successfulUpload) throws IOException {
        final CrashReportingPermissionManager permManager =
                new MockCrashReportingPermissionManager() {
                    { mIsEnabledForTests = true; }
                };
        final CountDownLatch stopStallingLatch = new CountDownLatch(1);
        MinidumpUploaderImpl minidumpUploader = new StallingMinidumpUploaderImpl(
                mTestRule.getExistingCacheDir(), permManager, stopStallingLatch, successfulUpload);

        File firstFile = createMinidumpFileInCrashDir("123_abc.dmp0.try0");

        // This is run on the UI thread to avoid failing any assertOnUiThread assertions.
        MinidumpUploadTestUtility.uploadAllMinidumpsOnUiThread(minidumpUploader,
                new MinidumpUploader.UploadsFinishedCallback() {
                    @Override
                    public void uploadsFinished(boolean reschedule) {
                        if (successfulUpload) {
                            Assert.assertFalse(reschedule);
                        } else {
                            Assert.fail("This shouldn't be called when a canceled upload fails.");
                        }
                    }
                },
                // Block until job posted - otherwise the worker thread might not have been created
                // before we try to join it.
                true /* blockUntilJobPosted */);
        minidumpUploader.cancelUploads();
        stopStallingLatch.countDown();
        // Wait until our job finished.
        try {
            minidumpUploader.joinWorkerThreadForTesting();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }

        File expectedFirstUploadFile = new File(mTestRule.getCrashDir(), "123_abc.up0.try0");
        File expectedFirstRetryFile = new File(mTestRule.getCrashDir(), "123_abc.dmp0.try1");
        if (successfulUpload) {
            // When the upload succeeds we expect the file to be renamed.
            Assert.assertFalse(firstFile.exists());
            Assert.assertTrue(expectedFirstUploadFile.exists());
            Assert.assertFalse(expectedFirstRetryFile.exists());
        } else {
            // When the upload fails we won't change the minidump at all.
            Assert.assertTrue(firstFile.exists());
            Assert.assertFalse(expectedFirstUploadFile.exists());
            Assert.assertFalse(expectedFirstRetryFile.exists());
        }
    }

    /**
     * Ensure that canceling an upload that fails causes a reschedule.
     */
    @Test
    @MediumTest
    public void testCancelFailedUploadCausesReschedule() throws IOException {
        final CrashReportingPermissionManager permManager =
                new MockCrashReportingPermissionManager() {
                    { mIsEnabledForTests = true; }
                };
        final CountDownLatch stopStallingLatch = new CountDownLatch(1);
        MinidumpUploaderImpl minidumpUploader =
                new StallingMinidumpUploaderImpl(mTestRule.getExistingCacheDir(), permManager,
                        stopStallingLatch, false /* successfulUpload */);

        createMinidumpFileInCrashDir("123_abc.dmp0.try0");

        MinidumpUploader.UploadsFinishedCallback crashingCallback =
                new MinidumpUploader.UploadsFinishedCallback() {
                    @Override
                    public void uploadsFinished(boolean reschedule) {
                        // We don't guarantee whether uploadsFinished is called after a job has been
                        // cancelled, but if it is, it should indicate that we want to reschedule
                        // the job.
                        Assert.assertTrue(reschedule);
                    }
                };

        // This is run on the UI thread to avoid failing any assertOnUiThread assertions.
        MinidumpUploadTestUtility.uploadAllMinidumpsOnUiThread(minidumpUploader, crashingCallback);
        // Ensure we tell JobScheduler to reschedule the job.
        Assert.assertTrue(minidumpUploader.cancelUploads());
        stopStallingLatch.countDown();
        // Wait for the MinidumpUploader worker thread to finish before ending the test. This is to
        // ensure the worker thread doesn't continue running after the test finishes - trying to
        // access directories or minidumps set up and deleted by the test framework.
        ThreadUtils.runOnUiThreadBlocking(() -> {
            try {
                minidumpUploader.joinWorkerThreadForTesting();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        });
    }

    private interface MinidumpUploadCallableCreator {
        MinidumpUploadCallable createCallable(File minidumpFile, File logfile);
    }

    private MinidumpUploaderImpl createCallableListMinidumpUploader(
            final List<MinidumpUploadCallableCreator> callables, final boolean userPermitted) {
        return new TestMinidumpUploaderImpl(mTestRule.getExistingCacheDir(), null) {
            private int mIndex = 0;

            @Override
            public MinidumpUploadCallable createMinidumpUploadCallable(
                    File minidumpFile, File logfile) {
                if (mIndex >= callables.size()) {
                    Assert.fail("Should not create callable number " + mIndex);
                }
                return callables.get(mIndex++).createCallable(minidumpFile, logfile);
            }
        };
    }

    private static class ExpectNoUploadsMinidumpUploaderImpl extends MinidumpUploaderImpl {
        public ExpectNoUploadsMinidumpUploaderImpl(File cacheDir) {
            super(new TestMinidumpUploaderDelegate(
                    cacheDir, new MockCrashReportingPermissionManager() {
                        { mIsEnabledForTests = true; }
                    }));
        }

        @Override
        public CrashFileManager createCrashFileManager(File crashDir) {
            return new CrashFileManager(crashDir) {
                @Override
                public void cleanOutAllNonFreshMinidumpFiles() {}
            };
        }

        @Override
        public MinidumpUploadCallable createMinidumpUploadCallable(
                File minidumpFile, File logfile) {
            Assert.fail("No minidumps upload attempts should be initiated by this uploader.");
            return null;
        }
    }

    /**
     * Minidump uploader implementation that stalls minidump-uploading until a given CountDownLatch
     * counts down.
     */
    private static class StallingMinidumpUploaderImpl extends TestMinidumpUploaderImpl {
        CountDownLatch mStopStallingLatch;
        boolean mSuccessfulUpload;

        public StallingMinidumpUploaderImpl(File cacheDir,
                CrashReportingPermissionManager permissionManager, CountDownLatch stopStallingLatch,
                boolean successfulUpload) {
            super(cacheDir, permissionManager);
            mStopStallingLatch = stopStallingLatch;
            mSuccessfulUpload = successfulUpload;
        }

        @Override
        public MinidumpUploadCallable createMinidumpUploadCallable(
                File minidumpFile, File logfile) {
            return new MinidumpUploadCallable(minidumpFile, logfile,
                    new StallingHttpUrlConnectionFactory(mStopStallingLatch, mSuccessfulUpload),
                    mDelegate.createCrashReportingPermissionManager());
        }
    }

    private static class StallingHttpUrlConnectionFactory implements HttpURLConnectionFactory {
        private final CountDownLatch mStopStallingLatch;
        private final boolean mSucceed;

        private class StallingOutputStream extends OutputStream {
            @Override
            public void write(int b) throws IOException {
                try {
                    mStopStallingLatch.await();
                } catch (InterruptedException e) {
                    throw new InterruptedIOException(e.toString());
                }
                if (!mSucceed) {
                    throw new IOException();
                }
            }
        }

        public StallingHttpUrlConnectionFactory(CountDownLatch stopStallingLatch, boolean succeed) {
            mStopStallingLatch = stopStallingLatch;
            mSucceed = succeed;
        }

        @Override
        public HttpURLConnection createHttpURLConnection(String url) {
            try {
                return new MinidumpUploadCallableTest.TestHttpURLConnection(new URL(url)) {
                    @Override
                    public OutputStream getOutputStream() {
                        return new StallingOutputStream();
                    }
                };
            } catch (MalformedURLException e) {
                return null;
            }
        }
    }

    private static class FailingHttpUrlConnectionFactory implements HttpURLConnectionFactory {
        @Override
        public HttpURLConnection createHttpURLConnection(String url) {
            return null;
        }
    }

    private File createMinidumpFileInCrashDir(String name) throws IOException {
        File minidumpFile = new File(mTestRule.getCrashDir(), name);
        CrashTestRule.setUpMinidumpFile(minidumpFile, BOUNDARY);
        return minidumpFile;
    }
}
