// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.partnercustomizations;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.v7.widget.SwitchCompat;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.preferences.HomepageEditor;
import org.chromium.chrome.browser.preferences.HomepagePreferences;
import org.chromium.chrome.browser.preferences.Preferences;
import org.chromium.chrome.browser.tabmodel.EmptyTabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabList;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.partnercustomizations.TestPartnerBrowserCustomizationsProvider;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TouchCommon;
import org.chromium.content.browser.test.util.UiUtils;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.concurrent.Callable;
import java.util.concurrent.TimeoutException;

/**
 * Integration test suite for partner homepage.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class PartnerHomepageIntegrationTest {
    @Rule
    public BasePartnerBrowserCustomizationIntegrationTestRule mActivityTestRule =
            new BasePartnerBrowserCustomizationIntegrationTestRule();

    private static final String TEST_PAGE = "/chrome/test/data/android/about.html";

    @Before
    public void setUp() throws InterruptedException {
        ThreadUtils.runOnUiThreadBlocking(new Runnable(){
            @Override
            public void run() {
                // TODO(newt): Remove this once SharedPreferences is cleared automatically at the
                // beginning of every test. http://crbug.com/441859
                SharedPreferences sp = ContextUtils.getAppSharedPreferences();
                sp.edit().clear().apply();
            }
        });

        mActivityTestRule.startMainActivityFromLauncher();
    }

    /**
     * Homepage is loaded on startup.
     */
    @Test
    @MediumTest
    @Feature({"Homepage"})
    @RetryOnFailure
    public void testHomepageInitialLoading() {
        Assert.assertEquals(Uri.parse(TestPartnerBrowserCustomizationsProvider.HOMEPAGE_URI),
                Uri.parse(mActivityTestRule.getActivity().getActivityTab().getUrl()));
    }

    /**
     * Clicking the homepage button should load homepage in the current tab.
     */
    @Test
    @MediumTest
    @Feature({"Homepage"})
    public void testHomepageButtonClick() throws InterruptedException {
        EmbeddedTestServer testServer =
                EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
        try {
            // Load non-homepage URL.
            mActivityTestRule.loadUrl(testServer.getURL(TEST_PAGE));
            UiUtils.settleDownUI(InstrumentationRegistry.getInstrumentation());
            Assert.assertNotSame(Uri.parse(TestPartnerBrowserCustomizationsProvider.HOMEPAGE_URI),
                    Uri.parse(mActivityTestRule.getActivity().getActivityTab().getUrl()));

            // Click homepage button.
            ChromeTabUtils.waitForTabPageLoaded(
                    mActivityTestRule.getActivity().getActivityTab(), new Runnable() {
                        @Override
                        public void run() {
                            View homeButton =
                                    mActivityTestRule.getActivity().findViewById(R.id.home_button);
                            Assert.assertEquals("Homepage button is not shown", View.VISIBLE,
                                    homeButton.getVisibility());
                            TouchCommon.singleClickView(homeButton);
                        }
                    });
            Assert.assertEquals(Uri.parse(TestPartnerBrowserCustomizationsProvider.HOMEPAGE_URI),
                    Uri.parse(mActivityTestRule.getActivity().getActivityTab().getUrl()));
        } finally {
            testServer.stopAndDestroyServer();
        }
    }

    /**
     * Homepage button visibility should be updated by enabling and disabling homepage in settings.
     */
    @Test
    @MediumTest
    @Feature({"Homepage"})
    @RetryOnFailure
    public void testHomepageButtonEnableDisable() {
        // Disable homepage.
        Preferences homepagePreferenceActivity =
                mActivityTestRule.startPreferences(HomepagePreferences.class.getName());
        SwitchCompat homepageSwitch =
                (SwitchCompat) homepagePreferenceActivity.findViewById(R.id.switch_widget);
        Assert.assertNotNull(homepageSwitch);
        TouchCommon.singleClickView(homepageSwitch);
        waitForCheckedState(homepagePreferenceActivity, false);
        homepagePreferenceActivity.finish();

        // Assert no homepage button.
        Assert.assertFalse(HomepageManager.isHomepageEnabled());
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Assert.assertEquals("Homepage button is shown", View.GONE,
                        mActivityTestRule.getActivity()
                                .findViewById(R.id.home_button)
                                .getVisibility());
            }
        });

        // Enable homepage.
        homepagePreferenceActivity =
                mActivityTestRule.startPreferences(HomepagePreferences.class.getName());
        homepageSwitch = (SwitchCompat) homepagePreferenceActivity.findViewById(R.id.switch_widget);
        Assert.assertNotNull(homepageSwitch);
        TouchCommon.singleClickView(homepageSwitch);
        waitForCheckedState(homepagePreferenceActivity, true);
        homepagePreferenceActivity.finish();

        // Assert homepage button.
        Assert.assertTrue(HomepageManager.isHomepageEnabled());
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Assert.assertEquals("Homepage button is shown", View.VISIBLE,
                        mActivityTestRule.getActivity()
                                .findViewById(R.id.home_button)
                                .getVisibility());
            }
        });
    }

    private void waitForCheckedState(final Preferences preferenceActivity, boolean isChecked) {
        CriteriaHelper.pollUiThread(Criteria.equals(isChecked, new Callable<Boolean>() {
            @Override
            public Boolean call() {
                // The underlying switch view in the preference can change, so we need to fetch
                // it each time to ensure we are checking the activity view.
                SwitchCompat homepageSwitch =
                        (SwitchCompat) preferenceActivity.findViewById(R.id.switch_widget);
                return homepageSwitch.isChecked();
            }
        }));
    }

    /**
     * Custom homepage URI should be fixed (e.g., "chrome.com" -> "http://chrome.com/")
     * when the URI is saved from the home page edit screen.
     */
    @Test
    @MediumTest
    @Feature({"Homepage"})
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    @RetryOnFailure
    public void testPreferenceCustomUriFixup() throws InterruptedException {
        // Change home page custom URI on hompage edit screen.
        final Preferences editHomepagePreferenceActivity =
                mActivityTestRule.startPreferences(HomepageEditor.class.getName());
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            // TODO(crbug.com/635567): Fix this properly.
            @SuppressLint("SetTextI18n")
            public void run() {
                ((EditText) editHomepagePreferenceActivity.findViewById(R.id.homepage_url_edit))
                        .setText("chrome.com");
            }
        });
        Button saveButton =
                (Button) editHomepagePreferenceActivity.findViewById(R.id.homepage_save);
        TouchCommon.singleClickView(saveButton);

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return editHomepagePreferenceActivity.isDestroyed();
            }
        });

        Assert.assertEquals("http://chrome.com/", HomepageManager.getHomepageUri());
    }

    /**
     * Closing the last tab should also close Chrome on Tabbed mode.
     */
    @Test
    @MediumTest
    @Feature({"Homepage"})
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    @RetryOnFailure
    public void testLastTabClosed() throws InterruptedException {
        ChromeTabUtils.closeCurrentTab(InstrumentationRegistry.getInstrumentation(),
                (ChromeTabbedActivity) mActivityTestRule.getActivity());
        Assert.assertTrue("Activity was not closed.",
                mActivityTestRule.getActivity().isFinishing()
                        || mActivityTestRule.getActivity().isDestroyed());
    }

    /**
     * Closing all tabs should finalize all tab closures and close Chrome on Tabbed mode.
     */
    @Test
    @MediumTest
    @Feature({"Homepage"})
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    public void testCloseAllTabs() throws InterruptedException {
        final CallbackHelper tabClosed = new CallbackHelper();
        final TabModel tabModel = mActivityTestRule.getActivity().getCurrentTabModel();
        mActivityTestRule.getActivity().getCurrentTabModel().addObserver(
                new EmptyTabModelObserver() {
                    @Override
                    public void didCloseTab(int tabId, boolean incognito) {
                        if (tabModel.getCount() == 0) tabClosed.notifyCalled();
                    }
                });
        InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                mActivityTestRule.getActivity().getTabModelSelector().closeAllTabs();
            }
        });

        try {
            tabClosed.waitForCallback(0);
        } catch (TimeoutException e) {
            Assert.fail("Never closed all of the tabs");
        }
        Assert.assertEquals("Expected no tabs to be present", 0,
                mActivityTestRule.getActivity().getCurrentTabModel().getCount());
        TabList fullModel =
                mActivityTestRule.getActivity().getCurrentTabModel().getComprehensiveModel();
        // By the time TAB_CLOSED event is received, all tab closures should be finalized
        Assert.assertEquals("Expected no tabs to be present in the comprehensive model", 0,
                fullModel.getCount());

        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        Assert.assertTrue("Activity was not closed.",
                mActivityTestRule.getActivity().isFinishing()
                        || mActivityTestRule.getActivity().isDestroyed());
    }
}
