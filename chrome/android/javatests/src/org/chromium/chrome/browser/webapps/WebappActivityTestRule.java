// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import static org.chromium.base.test.util.ScalableTimeout.scaleTimeout;

import android.content.Intent;
import android.net.Uri;
import android.support.customtabs.TrustedWebUtils;
import android.support.test.InstrumentationRegistry;
import android.view.ViewGroup;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.customtabs.CustomTabsTestUtils;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.DOMUtils;
import org.chromium.content.browser.test.util.JavaScriptUtils;
import org.chromium.net.test.EmbeddedTestServer;
import org.chromium.net.test.EmbeddedTestServerRule;

/**
 * Custom {@link ChromeActivityTestRule} for tests using {@link WebappActivity}.
 */
public class WebappActivityTestRule extends ChromeActivityTestRule<WebappActivity0> {
    public static final String WEBAPP_ID = "webapp_id";
    public static final String WEBAPP_NAME = "webapp name";
    public static final String WEBAPP_SHORT_NAME = "webapp short name";

    private static final long STARTUP_TIMEOUT = scaleTimeout(10000);

    // Empty 192x192 image generated with:
    // ShortcutHelper.encodeBitmapAsString(Bitmap.createBitmap(192, 192, Bitmap.Config.ARGB_4444));
    public static final String TEST_ICON =
            "iVBORw0KGgoAAAANSUhEUgAAAMAAAADACAYAAABS3GwHAAAABHNCSVQICAgIfAhkiAAAAKZJREFU"
            + "eJztwTEBAAAAwqD1T20JT6AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD4GQN4AAe3mX6IA"
            + "AAAASUVORK5CYII=";

    // Empty 512x512 image generated with:
    // ShortcutHelper.encodeBitmapAsString(Bitmap.createBitmap(512, 512, Bitmap.Config.ARGB_4444));
    public static final String TEST_SPLASH_ICON =
            "iVBORw0KGgoAAAANSUhEUgAAAgAAAAIACAYAAAD0eNT6AAAABHNCSVQICAgIfAhkiAAABA9JREFU"
            + "eJztwTEBAAAAwqD1T20Hb6AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
            + "AAAAAAAAAOA3AvAAAdln8YgAAAAASUVORK5CYII=";

    @Rule
    private EmbeddedTestServerRule mTestServerRule = new EmbeddedTestServerRule();

    public WebappActivityTestRule() {
        super(WebappActivity0.class);
    }

    public EmbeddedTestServer getTestServer() {
        return mTestServerRule.getServer();
    }

    /**
     * Creates the Intent that starts the WebAppActivity. This is meant to be overriden by other
     * tests in order for them to pass some specific values, but it defaults to a web app that just
     * loads about:blank to avoid a network load.  This results in the URL bar showing because
     * {@link UrlUtils} cannot parse this type of URL.
     */
    public Intent createIntent() {
        Intent intent =
                new Intent(InstrumentationRegistry.getTargetContext(), WebappActivity0.class);
        intent.setData(Uri.parse(WebappActivity.WEBAPP_SCHEME + "://" + WEBAPP_ID));
        intent.putExtra(ShortcutHelper.EXTRA_ID, WEBAPP_ID);
        intent.putExtra(ShortcutHelper.EXTRA_URL, "about:blank");
        intent.putExtra(ShortcutHelper.EXTRA_NAME, WEBAPP_NAME);
        intent.putExtra(ShortcutHelper.EXTRA_SHORT_NAME, WEBAPP_SHORT_NAME);
        return intent;
    }

    /** Adds a mock Custom Tab session token to the intent. */
    public void addTwaExtrasToIntent(Intent intent) {
        Intent cctIntent = CustomTabsTestUtils.createMinimalCustomTabIntent(
                InstrumentationRegistry.getTargetContext(), "about:blank");
        intent.putExtras(cctIntent.getExtras());
        intent.putExtra(TrustedWebUtils.EXTRA_LAUNCH_AS_TRUSTED_WEB_ACTIVITY, true);
    }

    @Override
    public Statement apply(final Statement base, Description description) {
        Statement webappTestRuleStatement = new Statement() {
            @Override
            public void evaluate() throws Throwable {
                // We run the WebappRegistry calls on the UI thread to prevent
                // ConcurrentModificationExceptions caused by multiple threads iterating and
                // modifying its hashmap at the same time.
                final TestFetchStorageCallback callback = new TestFetchStorageCallback();
                ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                    @Override
                    public void run() {
                        // Register the webapp so when the data storage is opened, the test doesn't
                        // crash.
                        WebappRegistry.refreshSharedPrefsForTesting();
                        WebappRegistry.getInstance().register(WEBAPP_ID, callback);
                    }
                });

                // Running this on the UI thread causes issues, so can't group everything into one
                // runnable.
                callback.waitForCallback(0);

                ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                    @Override
                    public void run() {
                        callback.getStorage().updateFromShortcutIntent(createIntent());
                    }
                });

                base.evaluate();

                ThreadUtils.runOnUiThreadBlocking(new Runnable() {
                    @Override
                    public void run() {
                        WebappRegistry.getInstance().clearForTesting();
                    }
                });
            }
        };

        Statement testServerStatement = mTestServerRule.apply(webappTestRuleStatement, description);
        return super.apply(testServerStatement, description);
    }

    /**
     * Starts up the WebappActivity and sets up the test observer.
     */
    public final void startWebappActivity() throws Exception {
        startWebappActivity(createIntent());
    }

    /**
     * Starts up the WebappActivity with a specific Intent and sets up the test observer.
     */
    public final void startWebappActivity(Intent intent) throws Exception {
        launchActivity(intent);
        waitUntilIdle();
    }

    public static void assertToolbarShowState(
            final ChromeActivity activity, final boolean showState) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                Assert.assertEquals(showState, activity.getActivityTab().canShowBrowserControls());
            }
        });
    }

    /**
     * Executing window.open() through a click on a link, as it needs user gesture to avoid Chrome
     * blocking it as a popup.
     */
    static public void jsWindowOpen(ChromeActivity activity, String url) throws Exception {
        String injectedHtml = String.format("var aTag = document.createElement('testId');"
                        + "aTag.id = 'testId';"
                        + "aTag.innerHTML = 'Click Me!';"
                        + "aTag.onclick = function() {"
                        + "  window.open('%s');"
                        + "  return false;"
                        + "};"
                        + "document.body.insertAdjacentElement('afterbegin', aTag);",
                url);
        JavaScriptUtils.executeJavaScriptAndWaitForResult(
                activity.getActivityTab().getWebContents(), injectedHtml);
        DOMUtils.clickNode(activity.getActivityTab().getWebContents(), "testId");
    }

    /**
     * Waits until any loads in progress of the activity under test have completed.
     */
    protected void waitUntilIdle() {
        waitUntilIdle(getActivity());
    }

    /**
     * Waits until any loads in progress of a selected activity have completed.
     */
    protected void waitUntilIdle(final ChromeActivity activity) {
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return activity.getActivityTab() != null && !activity.getActivityTab().isLoading();
            }
        }, STARTUP_TIMEOUT, CriteriaHelper.DEFAULT_POLLING_INTERVAL);

        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
    }

    /**
     * Starts up the WebappActivity and sets up the test observer.
     * Wait till Splashscreen full loaded.
     */
    public final ViewGroup startWebappActivityAndWaitForSplashScreen() throws Exception {
        return startWebappActivityAndWaitForSplashScreen(createIntent());
    }

    /**
     * Starts up the WebappActivity and sets up the test observer.
     * Wait till Splashscreen full loaded.
     * Intent url is modified to one that takes more time to load.
     */
    public final ViewGroup startWebappActivityAndWaitForSplashScreen(Intent intent)
            throws Exception {
        // Reset the url to one that takes more time to load.
        // This is to make sure splash screen won't disappear during test.
        intent.putExtra(ShortcutHelper.EXTRA_URL, getTestServer().getURL("/slow?2"));
        launchActivity(intent);
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                // we are waiting for WebappActivity#getActivityTab() to be non-null because we want
                // to ensure that native has been loaded.
                // We also wait till the splash screen has finished initializing.
                ViewGroup splashScreen = getActivity().getSplashScreenForTests();
                return getActivity().getActivityTab() != null && splashScreen != null
                        && splashScreen.getChildCount() > 0;
            }
        }, STARTUP_TIMEOUT, CriteriaHelper.DEFAULT_POLLING_INTERVAL);

        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        ViewGroup splashScreen = getActivity().getSplashScreenForTests();
        if (splashScreen == null) {
            Assert.fail("No splash screen available.");
        }
        return splashScreen;
    }

    /**
     * Waits for the splash screen to be hidden.
     */
    public void waitUntilSplashscreenHides() {
        CriteriaHelper.pollInstrumentationThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return !isSplashScreenVisible();
            }
        }, STARTUP_TIMEOUT, CriteriaHelper.DEFAULT_POLLING_INTERVAL);
    }

    public boolean isSplashScreenVisible() {
        return getActivity().getSplashScreenForTests() != null;
    }
}
