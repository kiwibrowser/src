// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.v7.app.AlertDialog;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.share.ShareHelper;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.TouchCommon;
import org.chromium.net.test.EmbeddedTestServer;

/** Test suite for Web Share (navigator.share) functionality. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class WebShareTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static final String TEST_FILE = "/content/test/data/android/webshare.html";

    private EmbeddedTestServer mTestServer;

    private String mUrl;

    private Tab mTab;
    private WebShareUpdateWaiter mUpdateWaiter;

    private Intent mReceivedIntent;

    /** Waits until the JavaScript code supplies a result. */
    private class WebShareUpdateWaiter extends EmptyTabObserver {
        private CallbackHelper mCallbackHelper;
        private String mStatus;

        public WebShareUpdateWaiter() {
            mCallbackHelper = new CallbackHelper();
        }

        @Override
        public void onTitleUpdated(Tab tab) {
            String title = mActivityTestRule.getActivity().getActivityTab().getTitle();
            // Wait until the title indicates either success or failure.
            if (!title.equals("Success") && !title.startsWith("Fail:")) return;
            mStatus = title;
            mCallbackHelper.notifyCalled();
        }

        public String waitForUpdate() throws Exception {
            mCallbackHelper.waitForCallback(0);
            return mStatus;
        }
    }

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();

        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());

        mUrl = mTestServer.getURL(TEST_FILE);

        mTab = mActivityTestRule.getActivity().getActivityTab();
        mUpdateWaiter = new WebShareUpdateWaiter();
        mTab.addObserver(mUpdateWaiter);

        mReceivedIntent = null;
    }

    @After
    public void tearDown() throws Exception {
        mTab.removeObserver(mUpdateWaiter);
        mTestServer.stopAndDestroyServer();

        // Clean up some state that might have been changed by tests.
        ShareHelper.setForceCustomChooserForTesting(false);
        ShareHelper.setFakeIntentReceiverForTesting(null);

    }

    /**
     * Verify that WebShare fails if called without a user gesture.
     * @throws Exception
     */
    @Test
    @MediumTest
    @Feature({"WebShare"})
    public void testWebShareNoUserGesture() throws Exception {
        mActivityTestRule.loadUrl(mUrl);
        mActivityTestRule.runJavaScriptCodeInCurrentTab("initiate_share()");
        Assert.assertEquals("Fail: NotAllowedError: "
                        + "Must be handling a user gesture to perform a share request.",
                mUpdateWaiter.waitForUpdate());
    }

    /**
     * Verify WebShare fails if share is called from a user gesture, and canceled.
     * @throws Exception
     */
    @Test
    @MediumTest
    @Feature({"WebShare"})
    public void testWebShareCancel() throws Exception {
        // This test tests functionality that is only available post Lollipop MR1.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP_MR1) return;

        // Set up ShareHelper to ignore the intent (without showing a picker). This simulates the
        // user canceling the dialog.
        ShareHelper.setFakeIntentReceiverForTesting(new ShareHelper.FakeIntentReceiver() {
            @Override
            public void setIntentToSendBack(Intent intent) {}

            @Override
            public void onCustomChooserShown(AlertDialog dialog) {}

            @Override
            public void fireIntent(Context context, Intent intent) {
                // Click again to start another share. This is necessary to work around
                // https://crbug.com/636274 (callback is not canceled until next share is
                // initiated). This also serves as a regression test for https://crbug.com/640324.
                TouchCommon.singleClickView(mTab.getView());
            }
        });

        mActivityTestRule.loadUrl(mUrl);
        // Click (instead of directly calling the JavaScript function) to simulate a user gesture.
        TouchCommon.singleClickView(mTab.getView());
        Assert.assertEquals("Fail: AbortError: Share canceled", mUpdateWaiter.waitForUpdate());
    }

    /**
     * Verify WebShare succeeds if share is called from a user gesture, and app chosen.
     * @throws Exception
     */
    @Test
    @MediumTest
    @Feature({"WebShare"})
    public void testWebShareSuccess() throws Exception {
        // This test tests functionality that is only available post Lollipop MR1.
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP_MR1) return;

        // Set up ShareHelper to immediately succeed (without showing a picker).
        ShareHelper.setFakeIntentReceiverForTesting(new ShareHelper.FakeIntentReceiver() {
            private Intent mIntentToSendBack;

            @Override
            public void setIntentToSendBack(Intent intent) {
                mIntentToSendBack = intent;
            }

            @Override
            public void onCustomChooserShown(AlertDialog dialog) {}

            @Override
            public void fireIntent(Context context, Intent intent) {
                mReceivedIntent = intent;

                if (context == null) return;

                // Send the intent back, which indicates that the user made a choice. (Normally,
                // this would have EXTRA_CHOSEN_COMPONENT set, but for the test, we do not set any
                // chosen target app.)
                context.sendBroadcast(mIntentToSendBack);
            }
        });

        mActivityTestRule.loadUrl(mUrl);
        // Click (instead of directly calling the JavaScript function) to simulate a user gesture.
        TouchCommon.singleClickView(mTab.getView());
        Assert.assertEquals("Success", mUpdateWaiter.waitForUpdate());

        // The actual intent to be delivered to the target is in the EXTRA_INTENT of the chooser
        // intent.
        Assert.assertNotNull(mReceivedIntent);
        Assert.assertTrue(mReceivedIntent.hasExtra(Intent.EXTRA_INTENT));
        Intent innerIntent = mReceivedIntent.getParcelableExtra(Intent.EXTRA_INTENT);
        Assert.assertNotNull(innerIntent);
        Assert.assertEquals(Intent.ACTION_SEND, innerIntent.getAction());
        Assert.assertTrue(innerIntent.hasExtra(Intent.EXTRA_SUBJECT));
        Assert.assertEquals("Test Title", innerIntent.getStringExtra(Intent.EXTRA_SUBJECT));
        Assert.assertTrue(innerIntent.hasExtra(Intent.EXTRA_TEXT));
        Assert.assertEquals(
                "Test Text https://test.url/", innerIntent.getStringExtra(Intent.EXTRA_TEXT));
    }

    /**
     * Verify WebShare fails if share is called from a user gesture, and canceled.
     *
     * Simulates pre-Lollipop-LMR1 system (different intent picker).
     *
     * @throws Exception
     */
    @Test
    @MediumTest
    @Feature({"WebShare"})
    public void testWebShareCancelPreLMR1() throws Exception {
        ShareHelper.setFakeIntentReceiverForTesting(new ShareHelper.FakeIntentReceiver() {
            @Override
            public void setIntentToSendBack(Intent intent) {}

            @Override
            public void onCustomChooserShown(AlertDialog dialog) {
                // Cancel the chooser dialog.
                dialog.dismiss();
            }

            @Override
            public void fireIntent(Context context, Intent intent) {}
        });

        ShareHelper.setForceCustomChooserForTesting(true);

        mActivityTestRule.loadUrl(mUrl);
        // Click (instead of directly calling the JavaScript function) to simulate a user gesture.
        TouchCommon.singleClickView(mTab.getView());
        Assert.assertEquals("Fail: AbortError: Share canceled", mUpdateWaiter.waitForUpdate());
    }

    /**
     * Verify WebShare succeeds if share is called from a user gesture, and app chosen.
     *
     * Simulates pre-Lollipop-LMR1 system (different intent picker).
     *
     * @throws Exception
     */
    @Test
    @MediumTest
    @Feature({"WebShare"})
    public void testWebShareSuccessPreLMR1() throws Exception {
        ShareHelper.setFakeIntentReceiverForTesting(new ShareHelper.FakeIntentReceiver() {
            @Override
            public void setIntentToSendBack(Intent intent) {}

            @Override
            public void onCustomChooserShown(AlertDialog dialog) {
                // Click on an app (it doesn't matter which, because we will hook the intent).
                Assert.assertTrue(dialog.getListView().getCount() > 0);
                dialog
                    .getListView()
                    .performItemClick(null, 0, dialog.getListView().getItemIdAtPosition(0));
            }

            @Override
            public void fireIntent(Context context, Intent intent) {
                mReceivedIntent = intent;
            }
        });

        ShareHelper.setForceCustomChooserForTesting(true);
        mActivityTestRule.loadUrl(mUrl);
        // Click (instead of directly calling the JavaScript function) to simulate a user gesture.
        TouchCommon.singleClickView(mTab.getView());
        Assert.assertEquals("Success", mUpdateWaiter.waitForUpdate());

        Assert.assertNotNull(mReceivedIntent);
        Assert.assertEquals(Intent.ACTION_SEND, mReceivedIntent.getAction());
        Assert.assertTrue(mReceivedIntent.hasExtra(Intent.EXTRA_SUBJECT));
        Assert.assertEquals("Test Title", mReceivedIntent.getStringExtra(Intent.EXTRA_SUBJECT));
        Assert.assertTrue(mReceivedIntent.hasExtra(Intent.EXTRA_TEXT));
        Assert.assertEquals(
                "Test Text https://test.url/", mReceivedIntent.getStringExtra(Intent.EXTRA_TEXT));
    }
}
