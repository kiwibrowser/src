// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.vr_shell;

import android.support.annotation.IntDef;
import android.view.View;

import org.junit.Assert;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.JavaScriptUtils;
import org.chromium.content_public.browser.ContentViewCore;
import org.chromium.content_public.browser.WebContents;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.concurrent.Callable;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Class containing most of the test framework for WebVR and WebXR testing, which requires
 * back-and-forth communication with JavaScript running in the browser. WebVR and WebXR-specific
 * functionality can be found in the VrTestFramework and XrTestFramework subclasses.
 *
 * The general test flow is:
 * - Load the HTML file containing the test, which:
 *   - Loads the WebVR boilerplate code and some test functions
 *   - Sets up common elements like the canvas and synchronization variable
 *   - Sets up any steps that need to be triggered by the Java code
 * - Check if any VRDisplay objects were found and fail the test if it doesn't
 *       match what we expect for that test
 * - Repeat:
 *   - Run any necessary Java-side code, e.g. trigger a user action
 *   - Trigger the next JavaScript test step and wait for it to finish
 *
 * The JavaScript code will automatically process test results once all
 * testharness.js tests are done, just like in layout tests. Once the results
 * are processed, the JavaScript code will automatically signal the Java code,
 * which can then grab the results and pass/fail the instrumentation test.
 */
public class TestFramework {
    public static final int PAGE_LOAD_TIMEOUT_S = 10;
    public static final int POLL_CHECK_INTERVAL_SHORT_MS = 50;
    public static final int POLL_CHECK_INTERVAL_LONG_MS = 100;
    public static final int POLL_TIMEOUT_SHORT_MS = 1000;
    public static final int POLL_TIMEOUT_LONG_MS = 10000;

    public static final String[] NATIVE_URLS_OF_INTEREST = {UrlConstants.BOOKMARKS_FOLDER_URL,
            UrlConstants.BOOKMARKS_UNCATEGORIZED_URL, UrlConstants.BOOKMARKS_URL,
            UrlConstants.DOWNLOADS_URL, UrlConstants.NATIVE_HISTORY_URL, UrlConstants.NTP_URL,
            UrlConstants.RECENT_TABS_URL};

    private static final String TAG = "TestFramework";
    static final String TEST_DIR = "chrome/test/data/vr/e2e_test_files";

    // Test status enum
    private static final int STATUS_RUNNING = 0;
    private static final int STATUS_PASSED = 1;
    private static final int STATUS_FAILED = 2;
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({STATUS_RUNNING, STATUS_PASSED, STATUS_FAILED})
    private @interface TestStatus {}

    private ChromeActivityTestRule mRule;
    private WebContents mFirstTabWebContents;
    private ContentViewCore mFirstTabCvc;
    private View mFirstTabContentView;

    /**
     * Must be constructed after the rule has been applied (e.g. in whatever method is
     * tagged with @Before)
     */
    public TestFramework(ChromeActivityTestRule rule) {
        mRule = rule;
        mFirstTabWebContents = mRule.getWebContents();
        mFirstTabCvc = mRule.getActivity().getActivityTab().getContentViewCore();
        mFirstTabContentView = mRule.getActivity().getActivityTab().getContentView();
        Assert.assertFalse("Test did not start in VR", VrShellDelegate.isInVr());
    }

    public WebContents getFirstTabWebContents() {
        return mFirstTabWebContents;
    }

    public View getFirstTabContentView() {
        return mFirstTabContentView;
    }

    public ChromeActivityTestRule getRule() {
        return mRule;
    }

    public void simulateRendererKilled() {
        final Tab tab = getRule().getActivity().getActivityTab();
        ThreadUtils.runOnUiThreadBlocking(() -> tab.simulateRendererKilledForTesting(true));

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return tab.isShowingSadTab();
            }
        });
    }

    /**
     * Gets the file:// URL to the test file
     * @param testName The name of the test whose file will be retrieved.
     * @return The file:// URL to the specified test file.
     */
    public static String getFileUrlForHtmlTestFile(String testName) {
        return "file://" + UrlUtils.getIsolatedTestFilePath(TEST_DIR) + "/html/" + testName
                + ".html";
    }

    /**
     * Gets the path to pass to an EmbeddedTestServer.getURL to load the given HTML test file.
     * @param testName The name of the test whose file will be retrieved.
     * @param A path that can be passed to EmbeddedTestServer.getURL to load the test file.
     */
    public static String getEmbeddedServerPathForHtmlTestFile(String testName) {
        return "/" + TEST_DIR + "/html/" + testName + ".html";
    }

    /**
     * Loads the given URL with the given timeout then waits for JavaScript to
     * signal that it's ready for testing.
     * @param url The URL of the page to load.
     * @param timeoutSec The timeout of the page load in seconds.
     * @return The return value of ChromeActivityTestRule.loadUrl()
     */
    public int loadUrlAndAwaitInitialization(String url, int timeoutSec)
            throws InterruptedException {
        int result = mRule.loadUrl(url, timeoutSec);
        Assert.assertTrue("JavaScript initialization successful",
                pollJavaScriptBoolean("isInitializationComplete()", POLL_TIMEOUT_LONG_MS,
                        mRule.getWebContents()));
        return result;
    }

    /**
     * Helper function to run the given JavaScript, return the return value,
     * and fail if a timeout/interrupt occurs so we don't have to catch or
     * declare exceptions all the time.
     * @param js The JavaScript to run.
     * @param timeout The timeout in milliseconds before a failure.
     * @param webContents The WebContents object to run the JavaScript in.
     * @return The return value of the JavaScript.
     */
    public static String runJavaScriptOrFail(String js, int timeout, WebContents webContents) {
        try {
            return JavaScriptUtils.executeJavaScriptAndWaitForResult(
                    webContents, js, timeout, TimeUnit.MILLISECONDS);
        } catch (InterruptedException | TimeoutException e) {
            Assert.fail("Fatal interruption or timeout running JavaScript: " + js);
        }
        return "Not reached";
    }

    /**
     * Retrieves the current status of the JavaScript test and returns an enum corresponding to it.
     * @param webContents The WebContents for the tab to check the status in.
     * @return A TestStatus integer corresponding to the current state of the JavaScript test
     */
    @TestStatus
    public static int checkTestStatus(WebContents webContents) {
        String resultString =
                runJavaScriptOrFail("resultString", POLL_TIMEOUT_SHORT_MS, webContents);
        boolean testPassed = Boolean.parseBoolean(
                runJavaScriptOrFail("testPassed", POLL_TIMEOUT_SHORT_MS, webContents));
        if (testPassed) {
            return STATUS_PASSED;
        } else if (!testPassed && resultString.equals("\"\"")) {
            return STATUS_RUNNING;
        } else {
            // !testPassed && !resultString.equals("\"\"")
            return STATUS_FAILED;
        }
    }

    /**
     * Helper function to end the test harness test and assert that it passed,
     * setting the failure reason as the description if it didn't.
     * @param webContents The WebContents for the tab to check test results in.
     */
    public static void endTest(WebContents webContents) {
        switch (checkTestStatus(webContents)) {
            case STATUS_PASSED:
                break;
            case STATUS_FAILED:
                String resultString =
                        runJavaScriptOrFail("resultString", POLL_TIMEOUT_SHORT_MS, webContents);
                Assert.fail("JavaScript testharness failed with result: " + resultString);
                break;
            case STATUS_RUNNING:
                Assert.fail("Attempted to end test in Java without finishing in JavaScript.");
                break;
            default:
                Assert.fail("Received unknown test status.");
        }
    }

    /**
     * Polls the provided JavaScript boolean until the timeout is reached or
     * the boolean is true.
     * @param boolName The name of the JavaScript boolean or expression to poll.
     * @param timeoutMs The polling timeout in milliseconds.
     * @param webContents The WebContents to run the JavaScript through.
     * @return True if the boolean evaluated to true, false if timed out.
     */
    public static boolean pollJavaScriptBoolean(
            final String boolName, int timeoutMs, final WebContents webContents) {
        try {
            CriteriaHelper.pollInstrumentationThread(Criteria.equals(true, new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    String result = "false";
                    try {
                        result = JavaScriptUtils.executeJavaScriptAndWaitForResult(webContents,
                                boolName, POLL_CHECK_INTERVAL_SHORT_MS, TimeUnit.MILLISECONDS);
                    } catch (InterruptedException | TimeoutException e) {
                        // Expected to happen regularly, do nothing
                    }
                    return Boolean.parseBoolean(result);
                }
            }), timeoutMs, POLL_CHECK_INTERVAL_LONG_MS);
        } catch (AssertionError e) {
            Log.d(TAG, "pollJavaScriptBoolean() timed out");
            return false;
        }
        return true;
    }

    /**
     * Waits for a JavaScript step to finish, asserting that the step finished
     * instead of timing out.
     * @param webContents The WebContents for the tab the JavaScript step is in.
     */
    public static void waitOnJavaScriptStep(WebContents webContents) {
        // Make sure we aren't trying to wait on a JavaScript test step without the code to do so.
        Assert.assertTrue("Attempted to wait on a JavaScript step without the code to do so. You "
                        + "either forgot to import webvr_e2e.js or are incorrectly using a Java "
                        + "method.",
                Boolean.parseBoolean(runJavaScriptOrFail("typeof javascriptDone !== 'undefined'",
                        POLL_TIMEOUT_SHORT_MS, webContents)));

        // Actually wait for the step to finish
        boolean success =
                pollJavaScriptBoolean("javascriptDone", POLL_TIMEOUT_LONG_MS, webContents);

        // Check what state we're in to make sure javascriptDone wasn't called because the test
        // failed.
        int testStatus = checkTestStatus(webContents);
        if (!success || testStatus == STATUS_FAILED) {
            // Failure states: Either polling failed or polling succeeded, but because the test
            // failed.
            String reason;
            if (!success) {
                reason = "Polling JavaScript boolean javascriptDone timed out.";
            } else {
                reason = "Polling JavaScript boolean javascriptDone succeeded, but test failed.";
            }
            String resultString =
                    runJavaScriptOrFail("resultString", POLL_TIMEOUT_SHORT_MS, webContents);
            if (resultString.equals("\"\"")) {
                reason += " Did not obtain specific reason from testharness.";
            } else {
                reason += " Testharness reported failure: " + resultString;
            }
            Assert.fail(reason);
        }

        // Reset the synchronization boolean
        runJavaScriptOrFail("javascriptDone = false", POLL_TIMEOUT_SHORT_MS, webContents);
    }

    /**
     * Executes a JavaScript step function using the given WebContents.
     * @param stepFunction The JavaScript step function to call.
     * @param webContents The WebContents for the tab the JavaScript is in.
     */
    public static void executeStepAndWait(String stepFunction, WebContents webContents) {
        // Run the step and block
        JavaScriptUtils.executeJavaScript(webContents, stepFunction);
        waitOnJavaScriptStep(webContents);
    }
}
