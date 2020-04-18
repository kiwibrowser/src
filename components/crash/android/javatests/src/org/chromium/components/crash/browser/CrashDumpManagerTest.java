// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.crash.browser;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.TestFileUtil;

import java.io.File;
import java.io.IOException;

/**
 * Unittests for {@link CrashDumpManager}.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class CrashDumpManagerTest {
    File mTempDir;

    @Before
    public void setUp() throws Exception {
        ContextUtils.initApplicationContextForTests(InstrumentationRegistry.getInstrumentation()
                                                            .getTargetContext()
                                                            .getApplicationContext());
        mTempDir = ContextUtils.getApplicationContext().getCacheDir();
        assert mTempDir.exists();
    }

    @After
    public void tearDown() throws Exception {
        File[] files = mTempDir.listFiles();
        if (files == null) return;

        for (File file : files) {
            TestFileUtil.deleteFile(file);
        }
    }

    @Test
    @SmallTest
    @Feature({"Android-AppBase"})
    @DisabledTest // Flaky, crbug.com/725379.
    public void testUploadMinidump_NoCallback() throws IOException {
        File minidump = new File(mTempDir, "mini.dmp");
        Assert.assertTrue(minidump.createNewFile());

        CrashDumpManager.tryToUploadMinidump(minidump.getAbsolutePath());
    }

    @Test
    @SmallTest
    @Feature({"Android-AppBase"})
    @DisabledTest // Flaky, crbug.com/725379.
    public void testUploadMinidump_NullMinidumpPath() {
        registerUploadCallback(new CrashDumpManager.UploadMinidumpCallback() {
            @Override
            public void tryToUploadMinidump(File minidump) {
                Assert.fail("The callback should not be called when the minidump path is null.");
            }
        });

        CrashDumpManager.tryToUploadMinidump(null);
    }

    // @SmallTest
    // @Feature({"Android-AppBase"})
    @Test
    @DisabledTest // Flaky, crbug.com/725379.
    public void testUploadMinidump_FileDoesntExist() {
        registerUploadCallback(new CrashDumpManager.UploadMinidumpCallback() {
            @Override
            public void tryToUploadMinidump(File minidump) {
                Assert.fail(
                        "The callback should not be called when the minidump file doesn't exist.");
            }
        });

        CrashDumpManager.tryToUploadMinidump(
                mTempDir.getAbsolutePath() + "/some/file/that/doesnt/exist");
    }

    @Test
    @SmallTest
    @Feature({"Android-AppBase"})
    @DisabledTest // Flaky, crbug.com/725379.
    public void testUploadMinidump_MinidumpPathIsDirectory() throws IOException {
        File directory = new File(mTempDir, "a_directory");
        Assert.assertTrue(directory.mkdir());

        registerUploadCallback(new CrashDumpManager.UploadMinidumpCallback() {
            @Override
            public void tryToUploadMinidump(File minidump) {
                Assert.fail(
                        "The callback should not be called when the minidump path is not a file.");
            }
        });

        CrashDumpManager.tryToUploadMinidump(directory.getAbsolutePath());
    }

    @Test
    @SmallTest
    @Feature({"Android-AppBase"})
    @DisabledTest // Flaky, crbug.com/725379.
    public void testUploadMinidump_MinidumpPathIsValid() throws IOException {
        final File minidump = new File(mTempDir, "mini.dmp");
        Assert.assertTrue(minidump.createNewFile());

        registerUploadCallback(new CrashDumpManager.UploadMinidumpCallback() {
            @Override
            public void tryToUploadMinidump(File actualMinidump) {
                Assert.assertEquals("Should call the callback with the correct filename.", minidump,
                        actualMinidump);
            }
        });

        CrashDumpManager.tryToUploadMinidump(minidump.getAbsolutePath());
    }

    /**
     * A convenience wrapper that registers the upload {@param callback}, running the registration
     * on the UI thread, as expected by the CrashDumpManager code.
     * @param callback The callback for uploading minidumps.
     */
    private static void registerUploadCallback(
            final CrashDumpManager.UploadMinidumpCallback callback) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                CrashDumpManager.registerUploadCallback(callback);
            }
        });
    }
}
