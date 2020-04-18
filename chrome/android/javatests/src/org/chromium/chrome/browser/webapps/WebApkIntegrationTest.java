// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.content.Intent;
import android.net.Uri;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.LargeTest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.CommandLine;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.ScalableTimeout;
import org.chromium.chrome.browser.ChromeSwitches;
import org.chromium.chrome.browser.DeferredStartupHandler;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.customtabs.CustomTabActivity;
import org.chromium.chrome.browser.customtabs.CustomTabIntentDataProvider;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.chrome.test.util.ChromeTabUtils;
import org.chromium.content.browser.test.NativeLibraryTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.JavaScriptUtils;
import org.chromium.content_public.common.ContentSwitches;
import org.chromium.net.test.EmbeddedTestServerRule;
import org.chromium.webapk.lib.client.WebApkValidator;
import org.chromium.webapk.lib.common.WebApkConstants;

/** Integration tests for WebAPK feature. */
@RunWith(ChromeJUnit4ClassRunner.class)
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class WebApkIntegrationTest {
    @Rule
    public final ChromeActivityTestRule<WebApkActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(WebApkActivity.class);

    @Rule
    public final NativeLibraryTestRule mNativeLibraryTestRule = new NativeLibraryTestRule();

    @Rule
    public EmbeddedTestServerRule mTestServerRule = new EmbeddedTestServerRule();

    private static final long STARTUP_TIMEOUT = ScalableTimeout.scaleTimeout(10000);

    public void startWebApkActivity(String webApkPackageName, final String startUrl)
            throws InterruptedException {
        Intent intent =
                new Intent(InstrumentationRegistry.getTargetContext(), WebApkActivity.class);
        intent.putExtra(WebApkConstants.EXTRA_WEBAPK_PACKAGE_NAME, webApkPackageName);
        intent.putExtra(ShortcutHelper.EXTRA_URL, startUrl);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        WebApkActivity webApkActivity =
                (WebApkActivity) InstrumentationRegistry.getInstrumentation().startActivitySync(
                        intent);
        mActivityTestRule.setActivity(webApkActivity);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();

        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mActivityTestRule.getActivity().getActivityTab() != null;
            }
        }, STARTUP_TIMEOUT, CriteriaHelper.DEFAULT_POLLING_INTERVAL);

        ChromeTabUtils.waitForTabPageLoaded(
                mActivityTestRule.getActivity().getActivityTab(), startUrl);
    }

    /** Waits for the splash screen to be hidden. */
    public void waitUntilSplashscreenHides() {
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mActivityTestRule.getActivity().getSplashScreenForTests() == null;
            }
        });
    }

    /** Register WebAPK with WebappDataStorage */
    private WebappDataStorage registerWithStorage(final String webappId) throws Exception {
        TestFetchStorageCallback callback = new TestFetchStorageCallback();
        WebappRegistry.getInstance().register(webappId, callback);
        callback.waitForCallback(0);
        return WebappRegistry.getInstance().getWebappDataStorage(webappId);
    }

    /** Returns URL for the passed-in host which maps to a page on the EmbeddedTestServer. */
    private String getUrlForHost(String host) {
        return "http://" + host + "/defaultresponse";
    }

    @Before
    public void setUp() throws Exception {
        WebApkUpdateManager.setUpdatesEnabledForTesting(false);
        Uri mapToUri = Uri.parse(mTestServerRule.getServer().getURL("/"));
        CommandLine.getInstance().appendSwitchWithValue(
                ContentSwitches.HOST_RESOLVER_RULES, "MAP * " + mapToUri.getAuthority());
    }

    /**
     * Tests that WebApkActivities are started properly by WebappLauncherActivity.
     */
    @Test
    @LargeTest
    @Feature({"Webapps"})
    public void testWebApkLaunchesByLauncherActivity() {
        String pwaRocksUrl = getUrlForHost("pwa.rocks");

        Intent intent = new Intent();
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setPackage(InstrumentationRegistry.getTargetContext().getPackageName());
        intent.setAction(WebappLauncherActivity.ACTION_START_WEBAPP);
        intent.putExtra(WebApkConstants.EXTRA_URL, pwaRocksUrl)
                .putExtra(WebApkConstants.EXTRA_WEBAPK_PACKAGE_NAME, "org.chromium.webapk.http");

        WebApkValidator.disableValidationForTesting();
        mActivityTestRule.startActivityCompletely(intent);

        WebApkActivity lastActivity = (WebApkActivity) mActivityTestRule.getActivity();
        Assert.assertEquals(pwaRocksUrl, lastActivity.getWebappInfo().uri().toString());
    }

    /**
     * Test launching a WebAPK. Test that loading the start page works and that the splashscreen
     * eventually hides.
     */
    @Test
    @LargeTest
    @Feature({"WebApk"})
    public void testLaunchAndNavigateOffOrigin() throws Exception {
        startWebApkActivity("org.chromium.webapk.http", getUrlForHost("pwa.rocks"));
        waitUntilSplashscreenHides();
        WebApkActivity webApkActivity = (WebApkActivity) mActivityTestRule.getActivity();
        WebappActivityTestRule.assertToolbarShowState(webApkActivity, false);

        // We navigate outside origin and expect CCT toolbar to show on top of WebApkActivity.
        String googleUrl = getUrlForHost("www.google.com");
        mActivityTestRule.runJavaScriptCodeInCurrentTab(
                "window.top.location = '" + googleUrl + "'");

        ChromeTabUtils.waitForTabPageLoaded(webApkActivity.getActivityTab(), googleUrl);
        WebappActivityTestRule.assertToolbarShowState(webApkActivity, true);
    }

    /**
     * Test launching a WebAPK. Test that open a url within scope through window.open() will open a
     * CCT.
     */
    @Test
    @LargeTest
    @Feature({"WebApk"})
    public void testLaunchAndOpenNewWindowInOrigin() throws Exception {
        String pwaRocksUrl = getUrlForHost("pwa.rocks");
        startWebApkActivity("org.chromium.webapk.http", pwaRocksUrl);
        waitUntilSplashscreenHides();

        WebappActivityTestRule.jsWindowOpen(mActivityTestRule.getActivity(), pwaRocksUrl);

        CustomTabActivity customTabActivity =
                ChromeActivityTestRule.waitFor(CustomTabActivity.class);
        ChromeTabUtils.waitForTabPageLoaded(customTabActivity.getActivityTab(), pwaRocksUrl);
        Assert.assertTrue(
                "Sending to external handlers needs to be enabled for redirect back (e.g. OAuth).",
                IntentUtils.safeGetBooleanExtra(customTabActivity.getIntent(),
                        CustomTabIntentDataProvider.EXTRA_SEND_TO_EXTERNAL_DEFAULT_HANDLER, false));
    }

    /**
     * Test launching a WebAPK. Test that open a url off scope through window.open() will open a
     * CCT, and in scope urls will stay in the CCT.
     */
    @Test
    @LargeTest
    @Feature({"WebApk"})
    public void testLaunchAndNavigationInNewWindowOffandInOrigin() throws Exception {
        String pwaRocksUrl = getUrlForHost("pwa.rocks");
        String googleUrl = getUrlForHost("www.google.com");
        startWebApkActivity("org.chromium.webapk.http", pwaRocksUrl);
        waitUntilSplashscreenHides();

        WebappActivityTestRule.jsWindowOpen(mActivityTestRule.getActivity(), googleUrl);
        CustomTabActivity customTabActivity =
                ChromeActivityTestRule.waitFor(CustomTabActivity.class);
        ChromeTabUtils.waitForTabPageLoaded(customTabActivity.getActivityTab(), googleUrl);

        JavaScriptUtils.executeJavaScriptAndWaitForResult(
                customTabActivity.getActivityTab().getWebContents(),
                String.format("window.location.href='%s'", pwaRocksUrl));
        ChromeTabUtils.waitForTabPageLoaded(customTabActivity.getActivityTab(), pwaRocksUrl);
    }

    /**
     * Test that on first launch:
     * - the "WebApk.LaunchInterval" histogram is not recorded (because there is no prevous launch
     *   to compute the interval from).
     * - the "last used" time is updated (to compute future "launch intervals").
     */
    @Test
    @LargeTest
    @Feature({"WebApk"})
    public void testLaunchIntervalHistogramNotRecordedOnFirstLaunch() throws Exception {
        final String histogramName = "WebApk.LaunchInterval";
        final String packageName = "org.chromium.webapk.http";
        startWebApkActivity(packageName, getUrlForHost("pwa.rocks"));

        CriteriaHelper.pollUiThread(new Criteria("Deferred startup never completed") {
            @Override
            public boolean isSatisfied() {
                return DeferredStartupHandler.getInstance().isDeferredStartupCompleteForApp();
            }
        });
        Assert.assertEquals(0, RecordHistogram.getHistogramTotalCountForTesting(histogramName));
        WebappDataStorage storage = WebappRegistry.getInstance().getWebappDataStorage(
                WebApkConstants.WEBAPK_ID_PREFIX + packageName);
        Assert.assertNotEquals(WebappDataStorage.TIMESTAMP_INVALID, storage.getLastUsedTimeMs());
    }

    /** Test that the "WebApk.LaunchInterval" histogram is recorded on susbequent launches. */
    @Test
    @LargeTest
    @Feature({"WebApk"})
    public void testLaunchIntervalHistogramRecordedOnSecondLaunch() throws Exception {
        mNativeLibraryTestRule.loadNativeLibraryNoBrowserProcess();

        final String histogramName = "WebApk.LaunchInterval2";
        final String packageName = "org.chromium.webapk.http";

        WebappDataStorage storage =
                registerWithStorage(WebApkConstants.WEBAPK_ID_PREFIX + packageName);
        storage.setHasBeenLaunched();
        storage.updateLastUsedTime();
        Assert.assertEquals(0, RecordHistogram.getHistogramTotalCountForTesting(histogramName));

        startWebApkActivity(packageName, getUrlForHost("pwa.rocks"));

        CriteriaHelper.pollUiThread(new Criteria("Deferred startup never completed") {
            @Override
            public boolean isSatisfied() {
                return DeferredStartupHandler.getInstance().isDeferredStartupCompleteForApp();
            }
        });

        Assert.assertEquals(1, RecordHistogram.getHistogramTotalCountForTesting(histogramName));
    }
}
