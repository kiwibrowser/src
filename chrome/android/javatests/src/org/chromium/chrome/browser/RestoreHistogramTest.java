// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.MetricsUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

/**
 * This test tests the logic for writing the restore histogram at two different levels
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class RestoreHistogramTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "chrome";

    @Before
    public void setUp() throws Exception {
        // TODO(aberent): Find the correct place to put this.  Calling ensureInitialized() on the
        //                current pathway fails to set this variable with the modern linker.
        PathUtils.setPrivateDataDirectorySuffix(PRIVATE_DATA_DIRECTORY_SUFFIX);
    }

    private void clearPrefs() {
        ContextUtils.getAppSharedPreferences().edit().clear().apply();
    }

    /**
     * Test that the fundamental method for writing the histogram
     * {@link ChromeBackupAgent#recordRestoreHistogram()} works correctly
     *
     * @throws ProcessInitException
     * @Note This can't be tested in the ChromeBackupAgent Junit test, since the histograms are
     *       written in the C++ code, and because all the functions are static there is no easy way
     *       of mocking them in Mockito (one can disable them, but that would spoil the point of the
     *       test).
     */
    @Test
    @SmallTest
    public void testHistogramWriter() throws ProcessInitException {
        LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
        MetricsUtils.HistogramDelta noRestoreDelta = new MetricsUtils.HistogramDelta(
                ChromeBackupAgent.HISTOGRAM_ANDROID_RESTORE_RESULT, ChromeBackupAgent.NO_RESTORE);
        MetricsUtils.HistogramDelta restoreCompletedDelta =
                new MetricsUtils.HistogramDelta(ChromeBackupAgent.HISTOGRAM_ANDROID_RESTORE_RESULT,
                        ChromeBackupAgent.RESTORE_COMPLETED);
        MetricsUtils.HistogramDelta restoreStatusRecorded =
                new MetricsUtils.HistogramDelta(ChromeBackupAgent.HISTOGRAM_ANDROID_RESTORE_RESULT,
                        ChromeBackupAgent.RESTORE_STATUS_RECORDED);

        // Check behavior with no preference set
        clearPrefs();
        ChromeBackupAgent.recordRestoreHistogram();
        Assert.assertEquals(1, noRestoreDelta.getDelta());
        Assert.assertEquals(0, restoreCompletedDelta.getDelta());
        Assert.assertEquals(
                ChromeBackupAgent.RESTORE_STATUS_RECORDED, ChromeBackupAgent.getRestoreStatus());

        // Check behavior with a restore status
        ChromeBackupAgent.setRestoreStatus(ChromeBackupAgent.RESTORE_COMPLETED);
        ChromeBackupAgent.recordRestoreHistogram();
        Assert.assertEquals(1, noRestoreDelta.getDelta());
        Assert.assertEquals(1, restoreCompletedDelta.getDelta());
        Assert.assertEquals(
                ChromeBackupAgent.RESTORE_STATUS_RECORDED, ChromeBackupAgent.getRestoreStatus());

        // Second call should record nothing (note this assumes it doesn't record something totally
        // random)
        ChromeBackupAgent.recordRestoreHistogram();
        Assert.assertEquals(0, restoreStatusRecorded.getDelta());
    }

    /**
     * Test that the histogram is written during Chrome first run.
     *
     * @throws InterruptedException
     * @throws ProcessInitException
     */
    @Test
    @SmallTest
    public void testWritingHistogramAtStartup() throws InterruptedException, ProcessInitException {
        LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
        MetricsUtils.HistogramDelta noRestoreDelta = new MetricsUtils.HistogramDelta(
                ChromeBackupAgent.HISTOGRAM_ANDROID_RESTORE_RESULT, ChromeBackupAgent.NO_RESTORE);

        // Histogram should be written the first time the activity is started.
        mActivityTestRule.startMainActivityOnBlankPage();
        Assert.assertEquals(1, noRestoreDelta.getDelta());
    }
}
