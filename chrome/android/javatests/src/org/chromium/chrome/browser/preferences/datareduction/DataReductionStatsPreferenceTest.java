// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.datareduction;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.SmallTest;
import android.support.test.rule.UiThreadTestRule;
import android.text.format.DateUtils;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.AdvancedMockContext;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;

/**
 * Unit test suite for DataReductionStatsPreference.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class DataReductionStatsPreferenceTest {
    @Rule
    public final RuleChain mChain =
            RuleChain.outerRule(new ChromeBrowserTestRule()).around(new UiThreadTestRule());

    /**
     * Key used to save the date that the site breakdown should be shown. If the user has historical
     * data saver stats, the site breakdown cannot be shown for DAYS_IN_CHART.
     */
    private static final String PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE =
            "data_reduction_site_breakdown_allowed_date";

    public static final int DAYS_IN_CHART = 30;

    private Context mContext;
    private TestDataReductionProxySettings mSettings;

    private static class TestDataReductionProxySettings extends DataReductionProxySettings {
        private long mLastUpdateInMillis;

        /**
         * Returns the time that the data reduction statistics were last updated.
         * @return The last update time in milliseconds since the epoch.
         */
        @Override
        public long getDataReductionLastUpdateTime() {
            return mLastUpdateInMillis;
        }

        /**
         * Sets the time that the data reduction statistics were last updated in milliseconds since
         * the epoch. This is only for testing and does not update the native pref values.
         */
        public void setDataReductionLastUpdateTime(long lastUpdateInMillis) {
            mLastUpdateInMillis = lastUpdateInMillis;
        }
    }

    @Before
    public void setUp() throws Exception, Throwable {
        // Using an AdvancedMockContext allows us to use a fresh in-memory SharedPreference.
        mContext = new AdvancedMockContext(InstrumentationRegistry.getInstrumentation()
                                                   .getTargetContext()
                                                   .getApplicationContext());
        ContextUtils.initApplicationContextForTests(mContext);
        mSettings = new TestDataReductionProxySettings();
        DataReductionProxySettings.setInstanceForTesting(mSettings);
    }

    /**
     * Tests that the site breakdown pref is initialized to now if there aren't historical stats.
     */
    @Test
    @SmallTest
    @UiThreadTest
    @Feature({"DataReduction"})
    public void testInitializeSiteBreakdownPrefNow() throws Throwable {
        long beforeTime = System.currentTimeMillis();
        DataReductionStatsPreference.initializeDataReductionSiteBreakdownPref();
        long afterTime = System.currentTimeMillis();

        Assert.assertTrue(ContextUtils.getAppSharedPreferences().getLong(
                                  PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE, -1)
                >= beforeTime);
        Assert.assertTrue(ContextUtils.getAppSharedPreferences().getLong(
                                  PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE, -1)
                <= afterTime);

        // Tests that the site breakdown pref isn't initialized again if the pref was
        // already set.
        DataReductionStatsPreference.initializeDataReductionSiteBreakdownPref();

        // Pref should still be the same value as before.
        Assert.assertTrue(ContextUtils.getAppSharedPreferences().getLong(
                                  PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE, -1)
                >= beforeTime);
        Assert.assertTrue(ContextUtils.getAppSharedPreferences().getLong(
                                  PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE, -1)
                <= afterTime);
    }

    /**
     * Tests that the site breakdown pref is initialized to 30 from Data Saver's last update time if
     * there are historical stats.
     */
    @Test
    @SmallTest
    @UiThreadTest
    @Feature({"DataReduction"})
    public void testInitializeSiteBreakdownPrefHistoricalStats() throws Throwable {
        // Make the last update one day ago.
        long lastUpdateInDays = 1;
        mSettings.setDataReductionLastUpdateTime(
                System.currentTimeMillis() - lastUpdateInDays * DateUtils.DAY_IN_MILLIS);
        long lastUpdateInMillis = mSettings.getDataReductionLastUpdateTime();
        DataReductionStatsPreference.initializeDataReductionSiteBreakdownPref();

        Assert.assertEquals(lastUpdateInMillis + DAYS_IN_CHART * DateUtils.DAY_IN_MILLIS,
                ContextUtils.getAppSharedPreferences().getLong(
                        PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE, -1));
    }

    /**
     * Tests that the site breakdown pref is initialized to now if there are historical stats, but
     * they are more than 30 days old.
     */
    @Test
    @SmallTest
    @UiThreadTest
    @Feature({"DataReduction"})
    public void testInitializeSiteBreakdownPrefOldHistoricalStats() throws Throwable {
        mSettings.setDataReductionLastUpdateTime(
                System.currentTimeMillis() - DAYS_IN_CHART * DateUtils.DAY_IN_MILLIS);
        long beforeTime = System.currentTimeMillis();
        DataReductionStatsPreference.initializeDataReductionSiteBreakdownPref();
        long afterTime = System.currentTimeMillis();

        Assert.assertTrue(ContextUtils.getAppSharedPreferences().getLong(
                                  PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE, -1)
                >= beforeTime);
        Assert.assertTrue(ContextUtils.getAppSharedPreferences().getLong(
                                  PREF_DATA_REDUCTION_SITE_BREAKDOWN_ALLOWED_DATE, -1)
                <= afterTime);
    }
}
