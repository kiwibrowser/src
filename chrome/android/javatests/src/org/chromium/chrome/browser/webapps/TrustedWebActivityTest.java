// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import static org.chromium.base.ThreadUtils.runOnUiThreadBlocking;

import android.content.Intent;
import android.os.Bundle;
import android.support.customtabs.CustomTabsCallback;
import android.support.customtabs.CustomTabsIntent;
import android.support.customtabs.CustomTabsSession;
import android.support.customtabs.TrustedWebUtils;
import android.support.test.filters.SmallTest;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.customtabs.CustomTabsTestUtils;
import org.chromium.chrome.browser.firstrun.FirstRunStatus;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabTestUtils;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.content.browser.test.NativeLibraryTestRule;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.ui.base.PageTransition;

/**
 * Instrumentation tests for Trusted Web Activities (TWA) backed by {@link WebappActivity}.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class TrustedWebActivityTest {
    @Rule
    public final WebappActivityTestRule mWebappActivityRule = new WebappActivityTestRule();

    @Rule
    public final NativeLibraryTestRule mNativeLibraryTestRule = new NativeLibraryTestRule();

    private static final String TEST_PAGE = "/chrome/test/data/android/google.html";
    private static final String TEST_PAGE_2 = "/chrome/test/data/android/test.html";
    private static final String TEST_PAGE_3 = "/chrome/test/data/android/simple.html";

    // A generic failure occurred. Should match the value in net_error_list.h.
    private static final int NET_ERROR_FAILED = -2;

    private String mTestPage;
    private String mTestPage2;
    private String mTestPage3;

    @Before
    public void setUp() throws Exception {
        runOnUiThreadBlocking(() -> FirstRunStatus.setFirstRunFlowComplete(true));
        mNativeLibraryTestRule.loadNativeLibraryNoBrowserProcess();
        mTestPage = mWebappActivityRule.getTestServer().getURL(TEST_PAGE);
        mTestPage2 = mWebappActivityRule.getTestServer().getURL(TEST_PAGE_2);
        mTestPage3 = mWebappActivityRule.getTestServer().getURL(TEST_PAGE_3);
    }

    @After
    public void tearDown() throws Exception {
        runOnUiThreadBlocking(() -> FirstRunStatus.setFirstRunFlowComplete(false));
    }

    /**
     * Tests that the navigation callbacks run.
     */
    @Test
    @SmallTest
    @Feature({"Webapps"})
    @DisabledTest // TODO(https://crbug.com/842929): Re-enable after unflaking.
    public void testNavigationCallbacks() throws Exception {
        final CallbackHelper startedCallback = new CallbackHelper();
        final CallbackHelper finishedCallback = new CallbackHelper();
        final CallbackHelper failedCallback = new CallbackHelper();
        final CallbackHelper abortedCallback = new CallbackHelper();
        final CallbackHelper shownCallback = new CallbackHelper();
        final CallbackHelper hiddenCallback = new CallbackHelper();

        CustomTabsSession session = CustomTabsTestUtils.bindWithCallback(new CustomTabsCallback() {
            @Override
            public void onNavigationEvent(int navigationEvent, Bundle extras) {
                switch (navigationEvent) {
                    case CustomTabsCallback.NAVIGATION_STARTED:
                        startedCallback.notifyCalled();
                        break;
                    case CustomTabsCallback.NAVIGATION_FINISHED:
                        finishedCallback.notifyCalled();
                        break;
                    case CustomTabsCallback.NAVIGATION_FAILED:
                        failedCallback.notifyCalled();
                        break;
                    case CustomTabsCallback.NAVIGATION_ABORTED:
                        abortedCallback.notifyCalled();
                        break;
                    case CustomTabsCallback.TAB_SHOWN:
                        shownCallback.notifyCalled();
                        break;
                    case CustomTabsCallback.TAB_HIDDEN:
                        hiddenCallback.notifyCalled();
                        break;
                    default:
                        fail("Unknown navigation event: " + navigationEvent);
                }
            }
        });
        WebappActivity activity = runTrustedWebActivityAndWaitForIdle(session, mTestPage);
        Tab tab = activity.getActivityTab();

        startedCallback.waitForCallback(0);
        finishedCallback.waitForCallback(0);
        assertEquals(mTestPage, tab.getUrl());
        assertFalse(tab.isHidden());

        // Fake sending the activity to the background.
        runOnUiThreadBlocking(activity::onPause);
        runOnUiThreadBlocking(activity::onStop);
        runOnUiThreadBlocking(() -> activity.onWindowFocusChanged(false));
        hiddenCallback.waitForCallback(0);
        assertTrue(tab.isHidden());

        // Fake bringing the activity back to the foreground.
        runOnUiThreadBlocking(() -> activity.onWindowFocusChanged(true));
        runOnUiThreadBlocking(activity::onStart);
        runOnUiThreadBlocking(activity::onResume);
        shownCallback.waitForCallback(0);
        assertFalse(tab.isHidden());

        // Start two page loads, triggering navigation aborted and finished events.
        ChromeTabUtils.waitForTabPageLoadStart(tab, () -> runOnUiThreadBlocking(() -> {
            tab.loadUrl(new LoadUrlParams(mTestPage2, PageTransition.LINK));
        }), CallbackHelper.WAIT_TIMEOUT_SECONDS);
        ChromeTabUtils.waitForTabPageLoadStart(tab, () -> runOnUiThreadBlocking(() -> {
            tab.loadUrl(new LoadUrlParams(mTestPage3, PageTransition.LINK));
        }), CallbackHelper.WAIT_TIMEOUT_SECONDS);
        abortedCallback.waitForCallback(0);
        finishedCallback.waitForCallback(1);
        assertEquals(mTestPage3, tab.getUrl());

        // Simulate a generic network error, triggering the failed event.
        runOnUiThreadBlocking(() -> TabTestUtils.simulatePageLoadFailed(tab, NET_ERROR_FAILED));
        failedCallback.waitForCallback(0);
    }

    private WebappActivity runTrustedWebActivityAndWaitForIdle(
            CustomTabsSession session, String url) throws Exception {
        Intent intent = mWebappActivityRule.createIntent();
        addTwaExtrasToIntent(intent, session);
        mWebappActivityRule.startWebappActivity(intent.putExtra(ShortcutHelper.EXTRA_URL, url));
        mWebappActivityRule.waitUntilSplashscreenHides();
        mWebappActivityRule.waitUntilIdle();
        WebappActivity activity = mWebappActivityRule.getActivity();
        assertEquals(WebappActivity.ACTIVITY_TYPE_TWA, activity.getActivityType());
        return activity;
    }

    private void addTwaExtrasToIntent(Intent webappIntent, CustomTabsSession session) {
        CustomTabsIntent customTabsIntent = new CustomTabsIntent.Builder(session).build();
        Intent intent = customTabsIntent.intent;
        intent.setAction(Intent.ACTION_VIEW);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        webappIntent.putExtras(intent.getExtras());
        webappIntent.putExtra(TrustedWebUtils.EXTRA_LAUNCH_AS_TRUSTED_WEB_ACTIVITY, true);
    }
}
