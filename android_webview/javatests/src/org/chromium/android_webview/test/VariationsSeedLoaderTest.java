// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.VariationsSeedLoader;
import org.chromium.android_webview.VariationsUtils;
import org.chromium.android_webview.services.ServiceInit;
import org.chromium.android_webview.test.services.MockVariationsSeedServer;
import org.chromium.android_webview.test.util.VariationsTestUtils;
import org.chromium.base.ContextUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.parameter.SkipCommandLineParameterization;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Test VariationsSeedLoader.
 */
@RunWith(BaseJUnit4ClassRunner.class)
@SkipCommandLineParameterization
public class VariationsSeedLoaderTest {
    private static final long TIMEOUT_MILLIS = 10000;

    private static class TestLoaderResult extends CallbackHelper {
        private boolean mSeedRequested;

        public boolean wasSeedRequested() {
            assert getCallCount() > 0;
            return mSeedRequested;
        }

        public void notifyCalled(boolean seedRequested) {
            mSeedRequested = seedRequested;
            super.notifyCalled();
        }
    }

    private static class TestLoader extends VariationsSeedLoader {
        private boolean mSeedRequested;
        private TestLoaderResult mResult;

        public TestLoader(TestLoaderResult result) {
            mResult = result;
        }

        @Override
        protected boolean isEnabledByCmd() {
            return true;
        }

        // Bind to the MockVariationsSeedServer built in to the instrumentation test app, rather
        // than the real server in the WebView provider.
        @Override
        protected Intent getServerIntent() {
            return new Intent(ContextUtils.getApplicationContext(), MockVariationsSeedServer.class);
        }

        @Override
        protected void requestSeedFromService(long oldSeedDate) {
            super.requestSeedFromService(oldSeedDate);
            mSeedRequested = true;
        }

        @Override
        protected void onBackgroundWorkFinished() {
            mResult.notifyCalled(mSeedRequested);
        }
    }

    private Handler mMainHandler;

    // Create a TestLoader, run it on the UI thread, and block until it's finished. The return value
    // indicates whether the loader decided to request a new seed.
    private boolean runTestLoaderBlocking() throws InterruptedException, TimeoutException {
        final TestLoaderResult result = new TestLoaderResult();
        Runnable run = () -> {
            TestLoader loader = new TestLoader(result);
            loader.startVariationsInit();
            loader.finishVariationsInit();
        };

        CallbackHelper onRequestReceived = MockVariationsSeedServer.getRequestHelper();
        int requestsReceived = onRequestReceived.getCallCount();
        Assert.assertTrue("Failed to post seed loader Runnable", mMainHandler.post(run));
        result.waitForCallback("Timed out waiting for loader to finish background work.", 0);
        if (result.wasSeedRequested()) {
            onRequestReceived.waitForCallback("Seed requested, but timed out waiting for request" +
                    " to arrive in MockVariationsSeedServer", requestsReceived);
            return true;
        }
        return false;
    }

    @Before
    public void setUp() throws IOException {
        mMainHandler = new Handler(Looper.getMainLooper());
        ContextUtils.initApplicationContextForTests(
                InstrumentationRegistry.getInstrumentation()
                        .getTargetContext().getApplicationContext());
        ServiceInit.setPrivateDataDirectorySuffix();
        RecordHistogram.setDisabledForTests(true);
        VariationsTestUtils.deleteSeeds();
    }

    @After
    public void tearDown() throws IOException {
        RecordHistogram.setDisabledForTests(false);
        VariationsTestUtils.deleteSeeds();
    }

    // Test the case that:
    // VariationsUtils.getSeedFile() - doesn't exist
    // VariationsUtils.getNewSeedFile() - doesn't exist
    @Test
    @MediumTest
    public void testHaveNoSeed() throws Exception {
        try {
            boolean seedRequested = runTestLoaderBlocking();

            // Since there was no seed, another seed should be requested.
            Assert.assertTrue("No seed requested", seedRequested);
        } finally {
            VariationsTestUtils.deleteSeeds();
        }
    }

    // Test the case that:
    // VariationsUtils.getSeedFile() - exists, timestamp = now
    // VariationsUtils.getNewSeedFile() - doesn't exist
    @Test
    @MediumTest
    public void testHaveFreshSeed() throws Exception {
        try {
            File oldFile = VariationsUtils.getSeedFile();
            Assert.assertTrue("Seed file already exists", oldFile.createNewFile());
            VariationsTestUtils.writeMockSeed(oldFile);

            boolean seedRequested = runTestLoaderBlocking();

            // Since there was a fresh seed, we should not request another seed.
            Assert.assertFalse("New seed was requested when it should not have been",
                    seedRequested);
        } finally {
            VariationsTestUtils.deleteSeeds();
        }
    }

    // Test the case that:
    // VariationsUtils.getSeedFile() - exists, timestamp = epoch
    // VariationsUtils.getNewSeedFile() - doesn't exist
    @Test
    @MediumTest
    public void testHaveExpiredSeed() throws Exception {
        try {
            File oldFile = VariationsUtils.getSeedFile();
            Assert.assertTrue("Seed file already exists", oldFile.createNewFile());
            VariationsTestUtils.writeMockSeed(oldFile);
            oldFile.setLastModified(0);

            boolean seedRequested = runTestLoaderBlocking();

            // Since the seed was expired, another seed should be requested.
            Assert.assertTrue("No seed requested", seedRequested);
        } finally {
            VariationsTestUtils.deleteSeeds();
        }
    }

    // Test the case that:
    // VariationsUtils.getSeedFile() - doesn't exist
    // VariationsUtils.getNewSeedFile() - exists, timestamp = now
    @Test
    @MediumTest
    public void testHaveFreshNewSeed() throws Exception {
        try {
            File oldFile = VariationsUtils.getSeedFile();
            File newFile = VariationsUtils.getNewSeedFile();
            Assert.assertTrue("New seed file already exists", newFile.createNewFile());
            VariationsTestUtils.writeMockSeed(newFile);

            boolean seedRequested = runTestLoaderBlocking();

            // The "new" seed should have been renamed to the "old" seed.
            Assert.assertTrue("Old seed not found", oldFile.exists());
            Assert.assertFalse("New seed still exists", newFile.exists());

            // Since the "new" seed was fresh, we should not request another seed.
            Assert.assertFalse("New seed was requested when it should not have been",
                    seedRequested);
        } finally {
            VariationsTestUtils.deleteSeeds();
        }
    }

    // Test the case that:
    // VariationsUtils.getSeedFile() - doesn't exist
    // VariationsUtils.getNewSeedFile() - exists, timestamp = epoch
    @Test
    @MediumTest
    public void testHaveExpiredNewSeed() throws Exception {
        try {
            File oldFile = VariationsUtils.getSeedFile();
            File newFile = VariationsUtils.getNewSeedFile();
            Assert.assertTrue("Seed file already exists", newFile.createNewFile());
            VariationsTestUtils.writeMockSeed(newFile);
            newFile.setLastModified(0);

            boolean seedRequested = runTestLoaderBlocking();

            // The "new" seed should have been renamed to the "old" seed. Another empty "new" seed
            // should have been created as a destination for the request.
            Assert.assertTrue("Old seed not found", oldFile.exists());
            Assert.assertTrue("New seed not found", newFile.exists());
            Assert.assertTrue("New seed is not empty", newFile.length() == 0L);

            // Since the "new" seed was expired, another seed should be requested.
            Assert.assertTrue("No seed requested", seedRequested);
        } finally {
            VariationsTestUtils.deleteSeeds();
        }
    }

    // Test the case that:
    // VariationsUtils.getSeedFile() - exists, timestamp = epoch
    // VariationsUtils.getNewSeedFile() - exists, timestamp = epoch + 1 day
    @Test
    @MediumTest
    public void testHaveBothExpiredSeeds() throws Exception {
        try {
            File oldFile = VariationsUtils.getSeedFile();
            Assert.assertTrue("Old seed file already exists", oldFile.createNewFile());
            VariationsTestUtils.writeMockSeed(oldFile);
            oldFile.setLastModified(0);

            File newFile = VariationsUtils.getNewSeedFile();
            Assert.assertTrue("New seed file already exists", newFile.createNewFile());
            VariationsTestUtils.writeMockSeed(newFile);
            newFile.setLastModified(TimeUnit.DAYS.toMillis(1));

            boolean seedRequested = runTestLoaderBlocking();

            // The "new" seed should have been renamed to the "old" seed. Another empty "new" seed
            // should have been created as a destination for the request.
            Assert.assertTrue("Old seed not found", oldFile.exists());
            Assert.assertTrue("New seed not found", newFile.exists());
            Assert.assertTrue("New seed is not empty", newFile.length() == 0L);

            // Since the "new" seed was expired, another seed should be requested.
            Assert.assertTrue("No seed requested", seedRequested);
        } finally {
            VariationsTestUtils.deleteSeeds();
        }
    }

    // Test loading twice. The first load should trigger a request, but the second should not,
    // because requests should be rate-limited.
    // VariationsUtils.getSeedFile() - doesn't exist
    // VariationsUtils.getNewSeedFile() - doesn't exist
    @Test
    @MediumTest
    public void testDoubleLoad() throws Exception {
        try {
            boolean seedRequested = runTestLoaderBlocking();
            Assert.assertTrue("No seed requested", seedRequested);

            seedRequested = runTestLoaderBlocking();
            Assert.assertFalse("New seed was requested when it should not have been",
                    seedRequested);
        } finally {
            VariationsTestUtils.deleteSeeds();
        }
    }
}
