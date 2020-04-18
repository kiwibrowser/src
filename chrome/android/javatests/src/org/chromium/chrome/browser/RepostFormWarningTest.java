// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;
import android.support.v7.app.AlertDialog;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.concurrent.TimeoutException;

/**
 * Integration tests verifying that form resubmission dialogs are correctly displayed and handled.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@RetryOnFailure
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class RepostFormWarningTest {
    // Active tab.

    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private Tab mTab;
    // Callback helper that manages waiting for pageloads to finish.
    private TestCallbackHelperContainer mCallbackHelper;

    private EmbeddedTestServer mTestServer;

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();

        mTab = mActivityTestRule.getActivity().getActivityTab();
        mCallbackHelper = new TestCallbackHelperContainer(mTab.getWebContents());
        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
    }

    @After
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
    }

    /** Verifies that the form resubmission warning is not displayed upon first POST navigation. */
    @Test
    @MediumTest
    @Feature({"Navigation"})
    public void testFormFirstNavigation() throws Throwable {
        // Load the url posting data for the first time.
        postNavigation();
        mCallbackHelper.getOnPageFinishedHelper().waitForCallback(0);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        // Verify that the form resubmission warning was not shown.
        Assert.assertNull("Form resubmission warning shown upon first load.",
                RepostFormWarningDialog.getCurrentDialogForTesting());
    }

    /** Verifies that confirming the form reload performs the reload. */
    @Test
    @MediumTest
    @Feature({"Navigation"})
    public void testFormResubmissionContinue() throws Throwable {
        // Load the url posting data for the first time.
        postNavigation();
        mCallbackHelper.getOnPageFinishedHelper().waitForCallback(0);

        // Trigger a reload and wait for the warning to be displayed.
        reload();
        AlertDialog dialog = waitForRepostFormWarningDialog();

        // Click "Continue" and verify that the page is reloaded.
        clickButton(dialog, AlertDialog.BUTTON_POSITIVE);
        mCallbackHelper.getOnPageFinishedHelper().waitForCallback(1);

        // Verify that the reference to the dialog in RepostFormWarningDialog was cleared.
        Assert.assertNull("Form resubmission warning dialog was not dismissed correctly.",
                RepostFormWarningDialog.getCurrentDialogForTesting());
    }

    /**
     * Verifies that cancelling the form reload prevents it from happening. Currently the test waits
     * after the "Cancel" button is clicked to verify that the load was not triggered, which blocks
     * for CallbackHelper's default timeout upon each execution.
     */
    @Test
    @SmallTest
    @Feature({"Navigation"})
    public void testFormResubmissionCancel() throws Throwable {
        // Load the url posting data for the first time.
        postNavigation();
        mCallbackHelper.getOnPageFinishedHelper().waitForCallback(0);

        // Trigger a reload and wait for the warning to be displayed.
        reload();
        AlertDialog dialog = waitForRepostFormWarningDialog();

        // Click "Cancel" and verify that the page is not reloaded.
        clickButton(dialog, AlertDialog.BUTTON_NEGATIVE);
        boolean timedOut = false;
        try {
            mCallbackHelper.getOnPageFinishedHelper().waitForCallback(1);
        } catch (TimeoutException ex) {
            timedOut = true;
        }
        Assert.assertTrue("Page was reloaded despite selecting Cancel.", timedOut);

        // Verify that the reference to the dialog in RepostFormWarningDialog was cleared.
        Assert.assertNull("Form resubmission warning dialog was not dismissed correctly.",
                RepostFormWarningDialog.getCurrentDialogForTesting());
    }

    /**
     * Verifies that destroying the Tab dismisses the form resubmission dialog.
     */
    @Test
    @SmallTest
    @Feature({"Navigation"})
    public void testFormResubmissionTabDestroyed() throws Throwable {
        // Load the url posting data for the first time.
        postNavigation();
        mCallbackHelper.getOnPageFinishedHelper().waitForCallback(0);

        // Trigger a reload and wait for the warning to be displayed.
        reload();
        waitForRepostFormWarningDialog();

        ThreadUtils.runOnUiThreadBlocking(
                (Runnable) () -> mActivityTestRule.getActivity().getCurrentTabModel().closeTab(
                        mTab));

        CriteriaHelper.pollUiThread(
                new Criteria("Form resubmission dialog not dismissed correctly") {
                    @Override
                    public boolean isSatisfied() {
                        return RepostFormWarningDialog.getCurrentDialogForTesting() == null;
                    }
                });
    }

    private AlertDialog waitForRepostFormWarningDialog() {
        CriteriaHelper.pollUiThread(
                new Criteria("Form resubmission warning not shown") {
                    @Override
                    public boolean isSatisfied() {
                        return RepostFormWarningDialog.getCurrentDialogForTesting() != null;
                    }
                });
        return ThreadUtils.runOnUiThreadBlockingNoException(
                () -> (AlertDialog) RepostFormWarningDialog.getCurrentDialogForTesting());
    }

    /** Performs a POST navigation in mTab. */
    private void postNavigation() throws Throwable {
        final String url = "/chrome/test/data/android/test.html";
        final byte[] postData = new byte[] { 42 };

        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                () -> mTab.loadUrl(LoadUrlParams.createLoadHttpPostParams(
                        mTestServer.getURL(url), postData)));
    }

    /** Reloads mTab. */
    private void reload() throws Throwable {
        InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> mTab.reload());
    }

    /** Clicks the given button in the given dialog. */
    private void clickButton(final AlertDialog dialog, final int buttonId) throws Throwable {
        InstrumentationRegistry.getInstrumentation().runOnMainSync(
                () -> dialog.getButton(buttonId).performClick());
    }
}
