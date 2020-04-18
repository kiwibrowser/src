// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.multiwindow;

import static org.chromium.chrome.browser.multiwindow.MultiWindowTestHelper.createSecondChromeTabbedActivity;

import android.annotation.TargetApi;
import android.content.Intent;
import android.os.Build;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ActivityState;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.ChromeTabbedActivity2;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.ChromeTabbedActivityTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

import java.util.concurrent.Callable;

/**
 * Class for testing MultiWindowUtils.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
@MinAndroidSdkLevel(Build.VERSION_CODES.N)
public class MultiWindowUtilsTest {
    @Rule
    public ChromeTabbedActivityTestRule mActivityTestRule = new ChromeTabbedActivityTestRule();

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityOnBlankPage();
    }

    /**
     * Tests that ChromeTabbedActivity2 is used for intents when EXTRA_WINDOW_ID is set to 2.
     */
    @Test
    @SmallTest
    @Feature("MultiWindow")
    public void testTabbedActivityForIntentWithExtraWindowId() {
        ChromeTabbedActivity activity1 = mActivityTestRule.getActivity();
        createSecondChromeTabbedActivity(activity1);

        Intent intent = activity1.getIntent();
        intent.putExtra(IntentHandler.EXTRA_WINDOW_ID, 2);

        Assert.assertEquals(
                "ChromeTabbedActivity2 should be used when EXTRA_WINDOW_ID is set to 2.",
                ChromeTabbedActivity2.class,
                MultiWindowUtils.getInstance().getTabbedActivityForIntent(intent, activity1));
    }

    /**
     * Tests that if two ChromeTabbedActivities are running the one that was resumed most recently
     * is used as the class name for new intents.
     */
    @Test
    @SmallTest
    @Feature("MultiWindow")
    public void testTabbedActivityForIntentLastResumedActivity() {
        ChromeTabbedActivity activity1 = mActivityTestRule.getActivity();
        final ChromeTabbedActivity2 activity2 = createSecondChromeTabbedActivity(activity1);

        Assert.assertFalse("ChromeTabbedActivity should not be resumed",
                ApplicationStatus.getStateForActivity(activity1) == ActivityState.RESUMED);
        Assert.assertTrue("ChromeTabbedActivity2 should be resumed",
                ApplicationStatus.getStateForActivity(activity2) == ActivityState.RESUMED);

        // Open settings and wait for ChromeTabbedActivity2 to pause.
        activity2.onMenuOrKeyboardAction(R.id.preferences_id, true);
        int expected = ActivityState.PAUSED;
        CriteriaHelper.pollUiThread(Criteria.equals(expected, new Callable<Integer>() {
            @Override
            public Integer call() {
                return ApplicationStatus.getStateForActivity(activity2);
            }
        }));

        Assert.assertEquals(
                "The most recently resumed ChromeTabbedActivity should be used for intents.",
                ChromeTabbedActivity2.class,
                MultiWindowUtils.getInstance().getTabbedActivityForIntent(
                        activity1.getIntent(), activity1));
    }

    /**
     * Tests that if only ChromeTabbedActivity is running it is used as the class name for intents.
     */
    @Test
    @SmallTest
    @Feature("MultiWindow")
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void testTabbedActivityForIntentOnlyActivity1IsRunning() {
        ChromeTabbedActivity activity1 = mActivityTestRule.getActivity();
        ChromeTabbedActivity2 activity2 = createSecondChromeTabbedActivity(activity1);
        activity2.finishAndRemoveTask();

        Assert.assertEquals(
                "ChromeTabbedActivity should be used for intents if ChromeTabbedActivity2 is "
                        + "not running.",
                ChromeTabbedActivity.class,
                MultiWindowUtils.getInstance().getTabbedActivityForIntent(
                        activity1.getIntent(), activity1));
    }

    /**
     * Tests that if only ChromeTabbedActivity2 is running it is used as the class name for intents.
     */
    @Test
    @SmallTest
    @Feature("MultiWindow")
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void testTabbedActivityForIntentOnlyActivity2IsRunning() {
        ChromeTabbedActivity activity1 = mActivityTestRule.getActivity();
        createSecondChromeTabbedActivity(activity1);
        activity1.finishAndRemoveTask();

        Assert.assertEquals(
                "ChromeTabbedActivity2 should be used for intents if ChromeTabbedActivity is "
                        + "not running.",
                ChromeTabbedActivity2.class,
                MultiWindowUtils.getInstance().getTabbedActivityForIntent(
                        activity1.getIntent(), activity1));
    }

    /**
     * Tests that if no ChromeTabbedActivities are running ChromeTabbedActivity is used as the
     * default for intents.
     */
    @Test
    @SmallTest
    @Feature("MultiWindow")
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void testTabbedActivityForIntentNoActivitiesAlive() {
        ChromeTabbedActivity activity1 = mActivityTestRule.getActivity();
        activity1.finishAndRemoveTask();

        Assert.assertEquals(
                "ChromeTabbedActivity should be used as the default for external intents.",
                ChromeTabbedActivity.class,
                MultiWindowUtils.getInstance().getTabbedActivityForIntent(
                        activity1.getIntent(), activity1));
    }

    /**
     * Tests that MultiWindowUtils properly tracks whether ChromeTabbedActivity2 is running.
     */
    @Test
    @SmallTest
    @Feature("MultiWindow")
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public void testTabbedActivity2TaskRunning() {
        ChromeTabbedActivity activity2 =
                createSecondChromeTabbedActivity(mActivityTestRule.getActivity());
        Assert.assertTrue(MultiWindowUtils.getInstance().getTabbedActivity2TaskRunning());

        activity2.finishAndRemoveTask();
        MultiWindowUtils.getInstance().getTabbedActivityForIntent(
                mActivityTestRule.getActivity().getIntent(), mActivityTestRule.getActivity());
        Assert.assertFalse(MultiWindowUtils.getInstance().getTabbedActivity2TaskRunning());
    }
}
