// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test.services;

import android.os.ParcelFileDescriptor;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.android_webview.services.CrashReceiverService;
import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;

import java.io.File;
import java.io.IOException;

/**
 * Instrumentation tests for CrashReceiverService.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class CrashReceiverServiceTest {
    @Before
    public void setUp() throws Exception {
        ContextUtils.initApplicationContextForTests(InstrumentationRegistry.getInstrumentation()
                                                            .getTargetContext()
                                                            .getApplicationContext());
    }

    /**
     * Ensure that the minidump copying doesn't trigger when we pass it invalid file descriptors.
     */
    @Test
    @MediumTest
    public void testCopyingAbortsForInvalidFds() throws IOException {
        Assert.assertFalse(CrashReceiverService.copyMinidumps(0 /* uid */, null));
        Assert.assertFalse(CrashReceiverService.copyMinidumps(
                0 /* uid */, new ParcelFileDescriptor[] {null, null}));
        Assert.assertFalse(
                CrashReceiverService.copyMinidumps(0 /* uid */, new ParcelFileDescriptor[0]));
    }

    /**
     * Ensure deleting temporary files used when copying minidumps works correctly.
     */
    @Test
    @MediumTest
    public void testDeleteFilesInDir() throws IOException {
        File webviewTmpDir = CrashReceiverService.getWebViewTmpCrashDir();
        if (!webviewTmpDir.isDirectory()) {
            Assert.assertTrue(webviewTmpDir.mkdir());
        }
        File testFile1 = new File(webviewTmpDir, "testFile1");
        File testFile2 = new File(webviewTmpDir, "testFile2");
        Assert.assertTrue(testFile1.createNewFile());
        Assert.assertTrue(testFile2.createNewFile());
        Assert.assertTrue(testFile1.exists());
        Assert.assertTrue(testFile2.exists());
        CrashReceiverService.deleteFilesInWebViewTmpDirIfExists();
        Assert.assertFalse(testFile1.exists());
        Assert.assertFalse(testFile2.exists());
    }
}
