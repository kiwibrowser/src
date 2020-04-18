// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tab;

import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.externalnav.ExternalNavigationHandler;
import org.chromium.chrome.browser.externalnav.ExternalNavigationParams;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.components.navigation_interception.NavigationParams;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.net.test.EmbeddedTestServer;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.TimeoutException;

/**
 * Tests for InterceptNavigationDelegate
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@RetryOnFailure
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class InterceptNavigationDelegateTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static final String BASE_PAGE = "/chrome/test/data/navigation_interception/";
    private static final String NAVIGATION_FROM_TIMEOUT_PAGE =
            BASE_PAGE + "navigation_from_timer.html";
    private static final String NAVIGATION_FROM_USER_GESTURE_PAGE =
            BASE_PAGE + "navigation_from_user_gesture.html";
    private static final String NAVIGATION_FROM_XHR_CALLBACK_PAGE =
            BASE_PAGE + "navigation_from_xhr_callback.html";
    private static final String NAVIGATION_FROM_XHR_CALLBACK_AND_SHORT_TIMEOUT_PAGE =
            BASE_PAGE + "navigation_from_xhr_callback_and_short_timeout.html";
    private static final String NAVIGATION_FROM_XHR_CALLBACK_AND_LONG_TIMEOUT_PAGE =
            BASE_PAGE + "navigation_from_xhr_callback_and_long_timeout.html";
    private static final String NAVIGATION_FROM_IMAGE_ONLOAD_PAGE =
            BASE_PAGE + "navigation_from_image_onload.html";
    private static final String NAVIGATION_FROM_USER_GESTURE_IFRAME_PAGE =
            BASE_PAGE + "navigation_from_user_gesture_to_iframe_page.html";

    private static final long DEFAULT_MAX_TIME_TO_WAIT_IN_MS = 3000;
    private static final long LONG_MAX_TIME_TO_WAIT_IN_MS = 20000;

    private ChromeActivity mActivity;
    private List<NavigationParams> mNavParamHistory = new ArrayList<>();
    private List<ExternalNavigationParams> mExternalNavParamHistory = new ArrayList<>();
    private TestInterceptNavigationDelegate mInterceptNavigationDelegate;
    private EmbeddedTestServer mTestServer;

    class TestInterceptNavigationDelegate extends InterceptNavigationDelegateImpl {
        TestInterceptNavigationDelegate() {
            super(new TestExternalNavigationHandler(), mActivity.getActivityTab());
        }

        @Override
        public boolean shouldIgnoreNavigation(NavigationParams navigationParams) {
            mNavParamHistory.add(navigationParams);
            return super.shouldIgnoreNavigation(navigationParams);
        }
    }

    class TestExternalNavigationHandler extends ExternalNavigationHandler {
        public TestExternalNavigationHandler() {
            super(mActivity.getActivityTab());
        }

        @Override
        public OverrideUrlLoadingResult shouldOverrideUrlLoading(ExternalNavigationParams params) {
            mExternalNavParamHistory.add(params);
            return OverrideUrlLoadingResult.NO_OVERRIDE;
        }
    }

    private void waitTillExpectedCallsComplete(int count, long timeout) {
        CriteriaHelper.pollUiThread(
                Criteria.equals(count, new Callable<Integer>() {
                    @Override
                    public Integer call() {
                        return mNavParamHistory.size();
                    }
                }), timeout, CriteriaHelper.DEFAULT_POLLING_INTERVAL);
    }

    @Before
    public void setUp() throws Exception {
        mActivityTestRule.startMainActivityOnBlankPage();
        mActivity = mActivityTestRule.getActivity();
        mInterceptNavigationDelegate = new TestInterceptNavigationDelegate();
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Tab tab = mActivity.getActivityTab();
                tab.setInterceptNavigationDelegate(mInterceptNavigationDelegate);
            }
        });
        mTestServer = EmbeddedTestServer.createAndStartServer(InstrumentationRegistry.getContext());
    }

    @After
    public void tearDown() throws Exception {
        mTestServer.stopAndDestroyServer();
    }

    @Test
    @SmallTest
    public void testNavigationFromTimer() throws InterruptedException {
        mActivityTestRule.loadUrl(mTestServer.getURL(NAVIGATION_FROM_TIMEOUT_PAGE));
        Assert.assertEquals(1, mNavParamHistory.size());

        waitTillExpectedCallsComplete(2, DEFAULT_MAX_TIME_TO_WAIT_IN_MS);
        Assert.assertFalse(mNavParamHistory.get(1).hasUserGesture);
        Assert.assertFalse(mNavParamHistory.get(1).hasUserGestureCarryover);
    }

    @Test
    @SmallTest
    public void testNavigationFromUserGesture() throws InterruptedException, TimeoutException {
        mActivityTestRule.loadUrl(mTestServer.getURL(NAVIGATION_FROM_USER_GESTURE_PAGE));
        Assert.assertEquals(1, mNavParamHistory.size());

        DOMUtils.clickNode(mActivity.getActivityTab().getWebContents(), "first");
        waitTillExpectedCallsComplete(2, DEFAULT_MAX_TIME_TO_WAIT_IN_MS);
        Assert.assertTrue(mNavParamHistory.get(1).hasUserGesture);
        Assert.assertFalse(mNavParamHistory.get(1).hasUserGestureCarryover);
    }

    @Test
    @SmallTest
    public void testNavigationFromXHRCallback() throws InterruptedException, TimeoutException {
        mActivityTestRule.loadUrl(mTestServer.getURL(NAVIGATION_FROM_XHR_CALLBACK_PAGE));
        Assert.assertEquals(1, mNavParamHistory.size());

        DOMUtils.clickNode(mActivity.getActivityTab().getWebContents(), "first");
        waitTillExpectedCallsComplete(2, DEFAULT_MAX_TIME_TO_WAIT_IN_MS);
        Assert.assertFalse(mNavParamHistory.get(1).hasUserGesture);
        Assert.assertTrue(mNavParamHistory.get(1).hasUserGestureCarryover);
    }

    @Test
    @SmallTest
    public void testNavigationFromXHRCallbackAndShortTimeout()
            throws InterruptedException, TimeoutException {
        mActivityTestRule.loadUrl(
                mTestServer.getURL(NAVIGATION_FROM_XHR_CALLBACK_AND_SHORT_TIMEOUT_PAGE));
        Assert.assertEquals(1, mNavParamHistory.size());

        DOMUtils.clickNode(mActivity.getActivityTab().getWebContents(), "first");
        waitTillExpectedCallsComplete(2, DEFAULT_MAX_TIME_TO_WAIT_IN_MS);
        Assert.assertFalse(mNavParamHistory.get(1).hasUserGesture);
        Assert.assertTrue(mNavParamHistory.get(1).hasUserGestureCarryover);
    }

    @Test
    @SmallTest
    public void testNavigationFromXHRCallbackAndLongTimeout()
            throws InterruptedException, TimeoutException {
        mActivityTestRule.loadUrl(
                mTestServer.getURL(NAVIGATION_FROM_XHR_CALLBACK_AND_LONG_TIMEOUT_PAGE));
        Assert.assertEquals(1, mNavParamHistory.size());

        DOMUtils.clickNode(mActivity.getActivityTab().getWebContents(), "first");
        waitTillExpectedCallsComplete(2, LONG_MAX_TIME_TO_WAIT_IN_MS);
        Assert.assertFalse(mNavParamHistory.get(1).hasUserGesture);
        Assert.assertFalse(mNavParamHistory.get(1).hasUserGestureCarryover);
    }

    @Test
    @SmallTest
    public void testNavigationFromImageOnLoad() throws InterruptedException, TimeoutException {
        mActivityTestRule.loadUrl(mTestServer.getURL(NAVIGATION_FROM_IMAGE_ONLOAD_PAGE));
        Assert.assertEquals(1, mNavParamHistory.size());

        DOMUtils.clickNode(mActivity.getActivityTab().getWebContents(), "first");
        waitTillExpectedCallsComplete(2, DEFAULT_MAX_TIME_TO_WAIT_IN_MS);
        Assert.assertFalse(mNavParamHistory.get(1).hasUserGesture);
        Assert.assertTrue(mNavParamHistory.get(1).hasUserGestureCarryover);
    }

    @Test
    @MediumTest
    public void testExternalAppIframeNavigation() throws InterruptedException, TimeoutException {
        mActivityTestRule.loadUrl(mTestServer.getURL(NAVIGATION_FROM_USER_GESTURE_IFRAME_PAGE));
        Assert.assertEquals(1, mNavParamHistory.size());

        DOMUtils.clickNode(mActivity.getActivityTab().getWebContents(), "first");
        waitTillExpectedCallsComplete(3, DEFAULT_MAX_TIME_TO_WAIT_IN_MS);
        Assert.assertEquals(3, mExternalNavParamHistory.size());

        Assert.assertTrue(mNavParamHistory.get(2).isExternalProtocol);
        Assert.assertFalse(mNavParamHistory.get(2).isMainFrame);
        Assert.assertTrue(
                mExternalNavParamHistory.get(2).getRedirectHandler().shouldStayInChrome(true));
    }

}
