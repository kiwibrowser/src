// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omaha;

import android.content.Context;
import android.support.test.filters.MediumTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.Restriction;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.chrome.test.util.OverviewModeBehaviorWatcher;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.ui.test.util.UiRestriction;

/**
 * Tests for the UpdateMenuItemHelper.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE, "enable_update_menu_item"})
public class UpdateMenuItemHelperTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    private static final long MS_TIMEOUT = 2000;
    private static final long MS_INTERVAL = 500;

    /** Reports versions that we want back to OmahaClient. */
    private static class MockVersionNumberGetter extends VersionNumberGetter {
        // Both of these strings must be of the format "#.#.#.#".
        private final String mCurrentVersion;
        private final String mLatestVersion;

        private boolean mAskedForCurrentVersion;
        private boolean mAskedForLatestVersion;

        public MockVersionNumberGetter(String currentVersion, String latestVersion) {
            mCurrentVersion = currentVersion;
            mLatestVersion = latestVersion;
        }

        @Override
        public String getCurrentlyUsedVersion(Context applicationContext) {
            Assert.assertNotNull("Never set the current version", mCurrentVersion);
            mAskedForCurrentVersion = true;
            return mCurrentVersion;
        }

        @Override
        public String getLatestKnownVersion(Context applicationContext) {
            Assert.assertNotNull("Never set the latest version", mLatestVersion);
            mAskedForLatestVersion = true;
            return mLatestVersion;
        }

        public boolean askedForCurrentVersion() {
            return mAskedForCurrentVersion;
        }

        public boolean askedForLatestVersion() {
            return mAskedForLatestVersion;
        }
    }

    /** Reports a dummy market URL back to OmahaClient. */
    private static class MockMarketURLGetter extends MarketURLGetter {
        private final String mURL;

        MockMarketURLGetter(String url) {
            mURL = url;
        }

        @Override
        protected String getMarketUrlInternal(Context context) {
            return mURL;
        }
    }

    private MockVersionNumberGetter mMockVersionNumberGetter;
    private MockMarketURLGetter mMockMarketURLGetter;

    @Before
    public void setUp() throws Exception {
        // This test explicitly tests for the menu item, so turn it on.
        VersionNumberGetter.setEnableUpdateDetection(true);
    }

    /**
     * Prepares Main before actually launching it.  This is required since we don't have all of the
     * info we need in setUp().
     * @param currentVersion Version to report as the current version of Chrome
     * @param latestVersion Version to report is available by Omaha
     */
    private void prepareAndStartMainActivity(String currentVersion, String latestVersion)
            throws Exception {
        // Report fake versions back to Main when it asks.
        mMockVersionNumberGetter = new MockVersionNumberGetter(currentVersion, latestVersion);
        VersionNumberGetter.setInstanceForTests(mMockVersionNumberGetter);

        // Report a dummy URL to Omaha.
        mMockMarketURLGetter = new MockMarketURLGetter(
                "https://play.google.com/store/apps/details?id=com.android.chrome");
        MarketURLGetter.setInstanceForTests(mMockMarketURLGetter);

        // Start up main.
        mActivityTestRule.startMainActivityWithURL(UrlConstants.NTP_URL);

        // Check to make sure that the version numbers get queried.
        versionNumbersQueried();
    }

    private void versionNumbersQueried() throws Exception {
        CriteriaHelper.pollInstrumentationThread(
                new Criteria() {
                    @Override
                    public boolean isSatisfied() {
                        return mMockVersionNumberGetter.askedForCurrentVersion()
                                && mMockVersionNumberGetter.askedForLatestVersion();
                    }
                },
                MS_TIMEOUT, MS_INTERVAL);
    }

    /**
     * Checks that the menu item is shown when a new version is available.
     */
    private void checkUpdateMenuItemIsShowing(String currentVersion, String latestVersion)
            throws Exception {
        prepareAndStartMainActivity(currentVersion, latestVersion);
        showAppMenuAndAssertMenuShown();
        Assert.assertTrue("Update menu item is not showing.",
                mActivityTestRule.getActivity()
                        .getAppMenuHandler()
                        .getAppMenu()
                        .getMenu()
                        .findItem(R.id.update_menu_id)
                        .isVisible());
    }

    /**
     * Checks that the menu item is not shown when a new version is not available.
     */
    private void checkUpdateMenuItemIsNotShowing(String currentVersion, String latestVersion)
            throws Exception {
        prepareAndStartMainActivity(currentVersion, latestVersion);
        showAppMenuAndAssertMenuShown();
        Assert.assertFalse("Update menu item is showing.",
                mActivityTestRule.getActivity()
                        .getAppMenuHandler()
                        .getAppMenu()
                        .getMenu()
                        .findItem(R.id.update_menu_id)
                        .isVisible());
    }

    @Test
    @MediumTest
    @Feature({"Omaha"})
    @RetryOnFailure
    public void testCurrentVersionIsOlder() throws Exception {
        checkUpdateMenuItemIsShowing("0.0.0.0", "1.2.3.4");
    }

    @Test
    @MediumTest
    @Feature({"Omaha"})
    @RetryOnFailure
    public void testCurrentVersionIsSame() throws Exception {
        checkUpdateMenuItemIsNotShowing("1.2.3.4", "1.2.3.4");
    }

    @Test
    @MediumTest
    @Feature({"Omaha"})
    public void testCurrentVersionIsNewer() throws Exception {
        checkUpdateMenuItemIsNotShowing("27.0.1453.42", "26.0.1410.49");
    }

    @Test
    @MediumTest
    @Feature({"Omaha"})
    @RetryOnFailure
    public void testNoVersionKnown() throws Exception {
        checkUpdateMenuItemIsNotShowing("1.2.3.4", "0");
    }

    @Test
    @MediumTest
    @Feature({"Omaha"})
    @Restriction(UiRestriction.RESTRICTION_TYPE_PHONE)
    @RetryOnFailure
    public void testMenuItemNotShownInOverview() throws Exception {
        checkUpdateMenuItemIsShowing("0.0.0.0", "1.2.3.4");

        // checkUpdateMenuItemIsShowing() opens the menu; hide it and assert it's dismissed.
        hideAppMenuAndAssertMenuShown();

        // Enter the tab switcher.
        OverviewModeBehaviorWatcher overviewModeWatcher = new OverviewModeBehaviorWatcher(
                mActivityTestRule.getActivity().getLayoutManager(), true, false);
        ThreadUtils.runOnUiThreadBlocking(
                () -> mActivityTestRule.getActivity().getLayoutManager().showOverview(false));
        overviewModeWatcher.waitForBehavior();

        // Make sure the item is not shown in tab switcher app menu.
        showAppMenuAndAssertMenuShown();
        Assert.assertFalse("Update menu item is showing.",
                mActivityTestRule.getActivity()
                        .getAppMenuHandler()
                        .getAppMenu()
                        .getMenu()
                        .findItem(R.id.update_menu_id)
                        .isVisible());
    }

    private void showAppMenuAndAssertMenuShown() {
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivityTestRule.getActivity().getAppMenuHandler().showAppMenu(null, false);
            }
        });
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mActivityTestRule.getActivity().getAppMenuHandler().isAppMenuShowing();
            }
        });
    }

    private void hideAppMenuAndAssertMenuShown() {
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivityTestRule.getActivity().getAppMenuHandler().hideAppMenu();
            }
        });
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return !mActivityTestRule.getActivity().getAppMenuHandler().isAppMenuShowing();
            }
        });
    }
}

