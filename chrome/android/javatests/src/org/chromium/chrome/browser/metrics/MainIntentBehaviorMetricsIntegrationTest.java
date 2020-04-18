// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.metrics;

import static org.junit.Assert.assertThat;

import android.annotation.SuppressLint;
import android.content.ComponentName;
import android.content.Intent;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;

import org.hamcrest.Matchers;
import org.junit.After;
import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.UserActionTester;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.omnibox.UrlBar;
import org.chromium.chrome.browser.tabmodel.TabModel.TabLaunchType;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.OmniboxTestUtils;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.common.ContentUrlConstants;

import java.util.concurrent.Callable;

/**
 * Tests the metrics recording for main intent behaviours.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@SuppressLint({"ApplySharedPref", "CommitPrefEdits"})
public class MainIntentBehaviorMetricsIntegrationTest {
    private static final long HOURS_IN_MS = 60 * 60 * 1000L;

    @Rule
    public ChromeActivityTestRule<ChromeTabbedActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeTabbedActivity.class);

    private UserActionTester mActionTester;

    @After
    public void tearDown() {
        if (mActionTester != null) mActionTester.tearDown();
    }

    @MediumTest
    @Test
    public void testFocusOmnibox() {
        startActivity(true);
        assertMainIntentBehavior(null);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                UrlBar urlBar = (UrlBar) mActivityTestRule.getActivity().findViewById(R.id.url_bar);
                OmniboxTestUtils.toggleUrlBarFocus(urlBar, true);
            }
        });
        assertMainIntentBehavior(MainIntentBehaviorMetrics.FOCUS_OMNIBOX);
    }

    @MediumTest
    @Test
    public void testSwitchTabs() {
        startActivity(true);
        assertMainIntentBehavior(null);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mActivityTestRule.getActivity().getTabCreator(false).createNewTab(
                        new LoadUrlParams(ContentUrlConstants.ABOUT_BLANK_URL),
                        TabLaunchType.FROM_RESTORE, null);
            }
        });
        CriteriaHelper.pollUiThread(Criteria.equals(2, new Callable<Integer>() {
            @Override
            public Integer call() throws Exception {
                return mActivityTestRule.getActivity().getTabModelSelector().getTotalTabCount();
            }
        }));
        assertMainIntentBehavior(null);

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                TabModelUtils.setIndex(mActivityTestRule.getActivity().getCurrentTabModel(), 1);
            }
        });
        assertMainIntentBehavior(MainIntentBehaviorMetrics.SWITCH_TABS);
    }

    @MediumTest
    @Test
    public void testBackgrounded() {
        startActivity(true);
        assertMainIntentBehavior(null);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mActivityTestRule.getActivity().finish();
            }
        });
        assertMainIntentBehavior(MainIntentBehaviorMetrics.BACKGROUNDED);
    }

    @MediumTest
    @Test
    public void testCreateNtp() {
        startActivity(true);
        assertMainIntentBehavior(null);
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                mActivityTestRule.getActivity().getTabCreator(false).launchNTP();
            }
        });
        assertMainIntentBehavior(MainIntentBehaviorMetrics.NTP_CREATED);
    }

    @MediumTest
    @Test
    public void testContinuation() {
        try {
            MainIntentBehaviorMetrics.setTimeoutDurationMsForTesting(500);
            startActivity(true);
            assertMainIntentBehavior(MainIntentBehaviorMetrics.CONTINUATION);
        } finally {
            MainIntentBehaviorMetrics.setTimeoutDurationMsForTesting(
                    MainIntentBehaviorMetrics.TIMEOUT_DURATION_MS);
        }
    }

    @MediumTest
    @Test
    public void testMainIntentWithoutLauncherCategory() {
        startActivity(false);
        assertMainIntentBehavior(null);
        Assert.assertFalse(mActivityTestRule.getActivity().getMainIntentBehaviorMetricsForTesting()
                .getPendingActionRecordForMainIntent());
    }

    @MediumTest
    @Test
    public void testBackgroundDuration_24hrs() {
        assertBackgroundDurationLogged(
                24 * HOURS_IN_MS, "MobileStartup.MainIntentReceived.After24Hours");
    }

    @MediumTest
    @Test
    public void testBackgroundDuration_12hrs() {
        assertBackgroundDurationLogged(
                12 * HOURS_IN_MS, "MobileStartup.MainIntentReceived.After12Hours");
    }

    @MediumTest
    @Test
    public void testBackgroundDuration_6hrs() {
        assertBackgroundDurationLogged(
                6 * HOURS_IN_MS, "MobileStartup.MainIntentReceived.After6Hours");
    }

    @MediumTest
    @Test
    public void testBackgroundDuration_1hr() {
        assertBackgroundDurationLogged(HOURS_IN_MS, "MobileStartup.MainIntentReceived.After1Hour");
    }

    @MediumTest
    @Test
    public void testBackgroundDuration_0hr() {
        assertBackgroundDurationLogged(0, null);
        for (String action : mActionTester.getActions()) {
            if (action.startsWith("MobileStartup.MainIntentReceived.After")) {
                Assert.fail("Unexpected background duration logged: " + action);
            }
        }
    }

    private void assertBackgroundDurationLogged(long duration, String expectedMetric) {
        startActivity(false);
        mActionTester = new UserActionTester();
        ContextUtils.getAppSharedPreferences()
                .edit()
                .putLong(ChromeTabbedActivity.LAST_BACKGROUNDED_TIME_MS_PREF,
                        System.currentTimeMillis() - duration)
                .commit();

        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        ThreadUtils.runOnUiThreadBlocking(
                () -> { mActivityTestRule.getActivity().onNewIntent(intent); });

        assertThat(mActionTester.toString(), mActionTester.getActions(),
                Matchers.hasItem("MobileStartup.MainIntentReceived"));
        if (expectedMetric != null) {
            assertThat(mActionTester.toString(), mActionTester.getActions(),
                    Matchers.hasItem(expectedMetric));
        }
    }

    private void startActivity(boolean addLauncherCategory) {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        if (addLauncherCategory) intent.addCategory(Intent.CATEGORY_LAUNCHER);
        intent.setComponent(new ComponentName(
                InstrumentationRegistry.getTargetContext(), ChromeTabbedActivity.class));

        mActivityTestRule.startActivityCompletely(intent);
        mActivityTestRule.waitForActivityNativeInitializationComplete();
    }

    private void assertMainIntentBehavior(Integer expected) {
        CriteriaHelper.pollUiThread(Criteria.equals(expected, new Callable<Integer>() {
            @Override
            public Integer call() throws Exception {
                return mActivityTestRule.getActivity()
                        .getMainIntentBehaviorMetricsForTesting()
                        .getLastMainIntentBehaviorForTesting();
            }
        }));
    }
}
