// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.snackbar;

import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.preferences.datareduction.DataReductionPromoUtils;
import org.chromium.chrome.browser.preferences.datareduction.DataReductionProxyUma;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;

/**
 * Tests the DataReductionPromoSnackbarController. Tests that the snackbar sizes are properly set
 * from a field trial param and that the correct uma is recorded.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class DataReductionPromoSnackbarControllerTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private static final int BYTES_IN_MB = 1024 * 1024;
    private static final int FIRST_SNACKBAR_SIZE_MB = 100;
    private static final int SECOND_SNACKBAR_SIZE_MB = 1024;
    private static final int COMMAND_LINE_FLAG_SNACKBAR_SIZE_MB = 1;
    private static final String FIRST_SNACKBAR_SIZE_STRING = "100 MB";
    private static final String SECOND_SNACKBAR_SIZE_STRING = "1 GB";
    private static final String COMMAND_LINE_FLAG_SNACKBAR_SIZE_STRING = "1 MB";

    private SnackbarManager mManager;
    private DataReductionPromoSnackbarController mController;

    @Before
    public void setUp() throws InterruptedException {
        ContextUtils.getAppSharedPreferences().edit().clear().apply();
        SnackbarManager.setDurationForTesting(1000);
        mActivityTestRule.startMainActivityOnBlankPage();
        mManager = mActivityTestRule.getActivity().getSnackbarManager();
        mController =
                new DataReductionPromoSnackbarController(mActivityTestRule.getActivity(), mManager);
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"force-fieldtrials="
                    + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME + "/Enabled",
            "force-fieldtrial-params=" + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME
                    + ".Enabled:" + DataReductionPromoSnackbarController.PROMO_PARAM_NAME + "/"
                    + FIRST_SNACKBAR_SIZE_MB + ";" + SECOND_SNACKBAR_SIZE_MB})
    public void testDataReductionPromoSnackbarController() throws Throwable {
        mActivityTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Assert.assertFalse(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());

                mController.maybeShowDataReductionPromoSnackbar(0);

                Assert.assertFalse(mManager.isShowing());
                Assert.assertTrue(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());
                Assert.assertEquals(
                        0, DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());

                mController.maybeShowDataReductionPromoSnackbar(
                        FIRST_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertTrue(mManager.isShowing());
                Assert.assertTrue(
                        mManager.getCurrentSnackbarForTesting().getText().toString().endsWith(
                                FIRST_SNACKBAR_SIZE_STRING));
                Assert.assertEquals(FIRST_SNACKBAR_SIZE_MB * BYTES_IN_MB,
                        DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());
                mManager.dismissSnackbars(mController);

                mController.maybeShowDataReductionPromoSnackbar(
                        SECOND_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertTrue(mManager.isShowing());

                Assert.assertTrue(
                        mManager.getCurrentSnackbarForTesting().getText().toString().endsWith(
                                SECOND_SNACKBAR_SIZE_STRING));
                Assert.assertEquals(SECOND_SNACKBAR_SIZE_MB * BYTES_IN_MB,
                        DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());
            }
        });
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"force-fieldtrials="
                    + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME
                    + "/SnackbarPromoOnly",
            "force-fieldtrial-params=" + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME
                    + ".SnackbarPromoOnly:" + DataReductionPromoSnackbarController.PROMO_PARAM_NAME
                    + "/" + FIRST_SNACKBAR_SIZE_MB + ";" + SECOND_SNACKBAR_SIZE_MB})
    public void testDataReductionPromoSnackbarControllerNoOtherPromos() throws Throwable {
        mActivityTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Assert.assertFalse(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());

                mController.maybeShowDataReductionPromoSnackbar(0);

                Assert.assertFalse(mManager.isShowing());
                Assert.assertTrue(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());
                Assert.assertEquals(
                        0, DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());

                mController.maybeShowDataReductionPromoSnackbar(
                        FIRST_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertTrue(mManager.isShowing());
                Assert.assertTrue(
                        mManager.getCurrentSnackbarForTesting().getText().toString().endsWith(
                                FIRST_SNACKBAR_SIZE_STRING));
                Assert.assertEquals(FIRST_SNACKBAR_SIZE_MB * BYTES_IN_MB,
                        DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());
                mManager.dismissSnackbars(mController);

                Assert.assertFalse(DataReductionProxySettings.getInstance()
                                           .isDataReductionProxyPromoAllowed());
            }
        });
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"force-fieldtrials="
                    + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME + "/Enabled",
            "force-fieldtrial-params=" + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME
                    + ".Enabled:" + DataReductionPromoSnackbarController.PROMO_PARAM_NAME + "/"
                    + FIRST_SNACKBAR_SIZE_MB + ";" + SECOND_SNACKBAR_SIZE_MB})
    public void testDataReductionPromoSnackbarControllerExistingUser() throws Throwable {
        mActivityTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Assert.assertFalse(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());

                mController.maybeShowDataReductionPromoSnackbar(
                        SECOND_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertFalse(mManager.isShowing());
                Assert.assertTrue(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());
                Assert.assertEquals(SECOND_SNACKBAR_SIZE_MB * BYTES_IN_MB,
                        DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());

                mController.maybeShowDataReductionPromoSnackbar(
                        SECOND_SNACKBAR_SIZE_MB * BYTES_IN_MB + 1);

                Assert.assertFalse(mManager.isShowing());
                Assert.assertEquals(SECOND_SNACKBAR_SIZE_MB * BYTES_IN_MB,
                        DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());
            }
        });
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"force-fieldtrials="
                    + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME + "/Enabled",
            "force-fieldtrial-params=" + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME
                    + ".Enabled:" + DataReductionPromoSnackbarController.PROMO_PARAM_NAME + "/"
                    + FIRST_SNACKBAR_SIZE_MB + ";" + SECOND_SNACKBAR_SIZE_MB})
    public void testDataReductionPromoSnackbarControllerHistograms() throws Throwable {
        mActivityTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Assert.assertFalse(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());

                mController.maybeShowDataReductionPromoSnackbar(0);

                Assert.assertTrue(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());
                Assert.assertEquals(0,
                        RecordHistogram.getHistogramValueCountForTesting(
                                DataReductionProxyUma.SNACKBAR_HISTOGRAM_NAME, 0));

                mController.maybeShowDataReductionPromoSnackbar(
                        FIRST_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertTrue(mManager.isShowing());
                Assert.assertEquals(1,
                        RecordHistogram.getHistogramValueCountForTesting(
                                DataReductionProxyUma.SNACKBAR_HISTOGRAM_NAME,
                                FIRST_SNACKBAR_SIZE_MB));
                mManager.getCurrentSnackbarForTesting().getController().onDismissNoAction(null);
                Assert.assertEquals(1,
                        RecordHistogram.getHistogramValueCountForTesting(
                                DataReductionProxyUma.UI_ACTION_HISTOGRAM_NAME,
                                DataReductionProxyUma.ACTION_SNACKBAR_DISMISSED));

                mController.maybeShowDataReductionPromoSnackbar(
                        SECOND_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertTrue(mManager.isShowing());
                Assert.assertEquals(1,
                        RecordHistogram.getHistogramValueCountForTesting(
                                DataReductionProxyUma.SNACKBAR_HISTOGRAM_NAME,
                                SECOND_SNACKBAR_SIZE_MB));
                mManager.getCurrentSnackbarForTesting().getController().onAction(null);
                // The dismissed histogram should not have been incremented.
                Assert.assertEquals(1,
                        RecordHistogram.getHistogramValueCountForTesting(
                                DataReductionProxyUma.UI_ACTION_HISTOGRAM_NAME,
                                DataReductionProxyUma.ACTION_SNACKBAR_DISMISSED));
            }
        });
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"enable-data-reduction-proxy-savings-promo",
            "disable-field-trial-config"})
    public void testDataReductionPromoSnackbarControllerCommandLineFlag() throws Throwable {
        mActivityTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Assert.assertFalse(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());
                mController.maybeShowDataReductionPromoSnackbar(0);

                mController.maybeShowDataReductionPromoSnackbar(
                        COMMAND_LINE_FLAG_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertTrue(mManager.isShowing());
                Assert.assertTrue(
                        mManager.getCurrentSnackbarForTesting().getText().toString().endsWith(
                                COMMAND_LINE_FLAG_SNACKBAR_SIZE_STRING));
                Assert.assertEquals(COMMAND_LINE_FLAG_SNACKBAR_SIZE_MB * BYTES_IN_MB,
                        DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());
                mManager.dismissSnackbars(mController);

                mController.maybeShowDataReductionPromoSnackbar(
                        FIRST_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertFalse(mManager.isShowing());
            }
        });
    }

    @Test
    @MediumTest
    @CommandLineFlags.Add({"enable-data-reduction-proxy-savings-promo",
            "force-fieldtrials=" + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME
                    + "/Enabled",
            "force-fieldtrial-params=" + DataReductionPromoSnackbarController.PROMO_FIELD_TRIAL_NAME
                    + ".Enabled:" + DataReductionPromoSnackbarController.PROMO_PARAM_NAME + "/"
                    + FIRST_SNACKBAR_SIZE_MB + ";" + SECOND_SNACKBAR_SIZE_MB})
    public void testDataReductionPromoSnackbarControllerCommandLineFlagWithFieldTrial()
            throws Throwable {
        mActivityTestRule.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Assert.assertFalse(
                        DataReductionPromoUtils.hasSnackbarPromoBeenInitWithStartingSavedBytes());
                mController.maybeShowDataReductionPromoSnackbar(0);

                mController.maybeShowDataReductionPromoSnackbar(
                        COMMAND_LINE_FLAG_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertTrue(mManager.isShowing());
                Assert.assertTrue(
                        mManager.getCurrentSnackbarForTesting().getText().toString().endsWith(
                                COMMAND_LINE_FLAG_SNACKBAR_SIZE_STRING));
                Assert.assertEquals(COMMAND_LINE_FLAG_SNACKBAR_SIZE_MB * BYTES_IN_MB,
                        DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());
                mManager.dismissSnackbars(mController);

                mController.maybeShowDataReductionPromoSnackbar(
                        FIRST_SNACKBAR_SIZE_MB * BYTES_IN_MB);

                Assert.assertTrue(mManager.isShowing());

                Assert.assertTrue(
                        mManager.getCurrentSnackbarForTesting().getText().toString().endsWith(
                                FIRST_SNACKBAR_SIZE_STRING));
                Assert.assertEquals(FIRST_SNACKBAR_SIZE_MB * BYTES_IN_MB,
                        DataReductionPromoUtils.getDisplayedSnackbarPromoSavedBytes());
            }
        });
    }
}
