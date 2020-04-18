// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.appmenu;

import android.app.Activity;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.net.spdyproxy.DataReductionProxySettings;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;

/**
 * Tests the Data Saver AppMenu footer
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@RetryOnFailure
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class DataSaverAppMenuTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private AppMenuHandlerForTest mAppMenuHandler;

    /**
     * AppMenuHandler that will be used to intercept the delegate for testing.
     */
    public static class AppMenuHandlerForTest extends AppMenuHandler {
        AppMenuPropertiesDelegate mDelegate;

        /**
         * AppMenuHandler for intercepting options item selections.
         */
        public AppMenuHandlerForTest(
                Activity activity, AppMenuPropertiesDelegate delegate, int menuResourceId) {
            super(activity, delegate, menuResourceId);
            mDelegate = delegate;
        }

        public AppMenuPropertiesDelegate getDelegate() {
            return mDelegate;
        }
    }

    @Before
    public void setUp() throws Exception {
        ChromeTabbedActivity.setAppMenuHandlerFactoryForTesting(
                (activity, delegate, menuResourceId) -> {
                    mAppMenuHandler =
                            new AppMenuHandlerForTest(activity, delegate, menuResourceId);
                    return mAppMenuHandler;
                });

        mActivityTestRule.startMainActivityOnBlankPage();
    }

    /**
     * Verify the Data Saver item does not show when the feature isn't on, and the proxy is enabled.
     */
    @Test
    @SmallTest
    @CommandLineFlags.Add("disable-field-trial-config")
    @Feature({"Browser", "Main"})
    public void testMenuDataSaverNoFeature() throws Throwable {
        mActivityTestRule.runOnUiThread((Runnable) () -> {
            ContextUtils.getAppSharedPreferences().edit().clear().apply();
            Assert.assertEquals(0, mAppMenuHandler.getDelegate().getFooterResourceId());
            DataReductionProxySettings.getInstance().setDataReductionProxyEnabled(
                    mActivityTestRule.getActivity().getApplicationContext(), true);
            Assert.assertEquals(0, mAppMenuHandler.getDelegate().getFooterResourceId());
        });
    }

    /**
     * Verify the Data Saver footer shows with the flag when the proxy is on.
     */
    @Test
    @SmallTest
    @CommandLineFlags.Add({"enable-features=DataReductionProxyMainMenu",
            "disable-field-trial-config"})
    @Feature({"Browser", "Main"})
    public void testMenuDataSaver() throws Throwable {
        mActivityTestRule.runOnUiThread((Runnable) () -> {
            ContextUtils.getAppSharedPreferences().edit().clear().apply();
            // Data Saver hasn't been turned on, the footer shouldn't show.
            Assert.assertEquals(0, mAppMenuHandler.getDelegate().getFooterResourceId());

            // Turn Data Saver on, the footer should show.
            DataReductionProxySettings.getInstance().setDataReductionProxyEnabled(
                    mActivityTestRule.getActivity().getApplicationContext(), true);
            Assert.assertEquals(R.layout.data_reduction_main_menu_item,
                    mAppMenuHandler.getDelegate().getFooterResourceId());

            // Ensure the footer is removed if the proxy is turned off.
            DataReductionProxySettings.getInstance().setDataReductionProxyEnabled(
                    mActivityTestRule.getActivity().getApplicationContext(), false);
            Assert.assertEquals(0, mAppMenuHandler.getDelegate().getFooterResourceId());
        });
    }

    /**
     * Verify the Data Saver footer shows with the flag when the proxy turns on and remains in the
     * main menu.
     */
    @Test
    @SmallTest
    @CommandLineFlags.Add({"enable-features=DataReductionProxyMainMenu<DataReductionProxyMainMenu",
            "force-fieldtrials=DataReductionProxyMainMenu/Enabled",
            "force-fieldtrial-params=DataReductionProxyMainMenu.Enabled:"
                    + "persistent_menu_item_enabled/true",
            "disable-field-trial-config"})
    @Feature({"Browser", "Main"})
    public void testMenuDataSaverPersistent() throws Throwable {
        mActivityTestRule.runOnUiThread((Runnable) () -> {
            ContextUtils.getAppSharedPreferences().edit().clear().apply();
            // Data Saver hasn't been turned on, the footer shouldn't show.
            Assert.assertEquals(0, mAppMenuHandler.getDelegate().getFooterResourceId());

            // Turn Data Saver on, the footer should show.
            DataReductionProxySettings.getInstance().setDataReductionProxyEnabled(
                    mActivityTestRule.getActivity().getApplicationContext(), true);
            Assert.assertEquals(R.layout.data_reduction_main_menu_item,
                    mAppMenuHandler.getDelegate().getFooterResourceId());

            // Ensure the footer remains if the proxy is turned off.
            DataReductionProxySettings.getInstance().setDataReductionProxyEnabled(
                    mActivityTestRule.getActivity().getApplicationContext(), false);
            Assert.assertEquals(R.layout.data_reduction_main_menu_item,
                    mAppMenuHandler.getDelegate().getFooterResourceId());
        });
    }
}
