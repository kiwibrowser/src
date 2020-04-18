// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.content.DialogInterface;
import android.support.test.filters.MediumTest;
import android.support.v7.app.AlertDialog;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.CommandLineFlags;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.RetryOnFailure;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.test.ChromeActivityTestRule;
import org.chromium.chrome.test.ChromeJUnit4ClassRunner;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer;
import org.chromium.content.browser.test.util.TestCallbackHelperContainer
        .OnEvaluateJavaScriptResultHelper;
import org.chromium.content.browser.test.util.TouchCommon;
import org.chromium.content_public.browser.GestureListenerManager;
import org.chromium.content_public.browser.GestureStateListener;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeoutException;

/**
 * Test suite for displaying and functioning of modal dialogs.
 */
@RunWith(ChromeJUnit4ClassRunner.class)
@RetryOnFailure
@CommandLineFlags.Add({ChromeSwitches.DISABLE_FIRST_RUN_EXPERIENCE})
public class ModalDialogTest {
    @Rule
    public ChromeActivityTestRule<ChromeActivity> mActivityTestRule =
            new ChromeActivityTestRule<>(ChromeActivity.class);

    private static final String TAG = "ModalDialogTest";
    private static final String EMPTY_PAGE = UrlUtils.encodeHtmlDataUri(
            "<html><title>Modal Dialog Test</title><p>Testcase.</p></title></html>");
    private static final String BEFORE_UNLOAD_URL = UrlUtils.encodeHtmlDataUri("<html>"
                    + "<head><script>window.onbeforeunload=function() {"
                    + "return 'Are you sure?';"
                    + "};</script></head></html>");

    @Before
    public void setUp() throws InterruptedException {
        mActivityTestRule.startMainActivityWithURL(EMPTY_PAGE);
    }

    /**
     * Verifies modal alert-dialog appearance and that JavaScript execution is
     * able to continue after dismissal.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testAlertModalDialog()
            throws InterruptedException, TimeoutException, ExecutionException {
        final OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog("alert('Hello Android!');");

        JavascriptAppModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        clickOk(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing prompt.",
                scriptEvent.waitUntilHasValue());
    }

    /**
     * Verifies that clicking on a button twice doesn't crash.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testAlertModalDialogWithTwoClicks()
            throws InterruptedException, TimeoutException, ExecutionException {
        OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog("alert('Hello Android');");
        JavascriptAppModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        clickOk(jsDialog);
        clickOk(jsDialog);

        Assert.assertTrue("JavaScript execution should continue after closing prompt.",
                scriptEvent.waitUntilHasValue());
    }

    /**
     * Verifies that modal confirm-dialogs display, two buttons are visible and
     * the return value of [Ok] equals true, [Cancel] equals false.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testConfirmModalDialog()
            throws InterruptedException, TimeoutException, ExecutionException {
        OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog("confirm('Android');");

        JavascriptAppModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        Button[] buttons = getAlertDialogButtons(jsDialog.getDialogForTest());
        Assert.assertNotNull("No cancel button in confirm dialog.", buttons[0]);
        Assert.assertEquals(
                "Cancel button is not visible.", View.VISIBLE, buttons[0].getVisibility());
        if (buttons[1] != null) {
            Assert.assertNotSame("Neutral button visible when it should not.", View.VISIBLE,
                    buttons[1].getVisibility());
        }
        Assert.assertNotNull("No OK button in confirm dialog.", buttons[2]);
        Assert.assertEquals("OK button is not visible.", View.VISIBLE, buttons[2].getVisibility());

        clickOk(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing dialog.",
                scriptEvent.waitUntilHasValue());

        String resultString = scriptEvent.getJsonResultAndClear();
        Assert.assertEquals("Invalid return value.", "true", resultString);

        // Try again, pressing cancel this time.
        scriptEvent = executeJavaScriptAndWaitForDialog("confirm('Android');");
        jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        clickCancel(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing dialog.",
                scriptEvent.waitUntilHasValue());

        resultString = scriptEvent.getJsonResultAndClear();
        Assert.assertEquals("Invalid return value.", "false", resultString);
    }

    /**
     * Verifies that modal prompt-dialogs display and the result is returned.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testPromptModalDialog()
            throws InterruptedException, TimeoutException, ExecutionException {
        final String promptText = "Hello Android!";
        final OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog("prompt('Android', 'default');");

        final JavascriptAppModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        // Set the text in the prompt field of the dialog.
        boolean result = ThreadUtils.runOnUiThreadBlocking(() -> {
            EditText prompt = (EditText) jsDialog.getDialogForTest().findViewById(
                    org.chromium.chrome.R.id.js_modal_dialog_prompt);
            if (prompt == null) return false;
            prompt.setText(promptText);
            return true;
        });
        Assert.assertTrue("Failed to find prompt view in prompt dialog.", result);

        clickOk(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing prompt.",
                scriptEvent.waitUntilHasValue());

        String resultString = scriptEvent.getJsonResultAndClear();
        Assert.assertEquals("Invalid return value.", '"' + promptText + '"', resultString);
    }

    /**
     * Verifies that message content in a dialog is only focusable if the message itself is long
     * enough to require scrolling.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testAlertModalDialogMessageFocus()
            throws InterruptedException, TimeoutException, ExecutionException {
        assertScrollViewFocusabilityInAlertDialog("alert('Short message!');", false);

        assertScrollViewFocusabilityInAlertDialog(
                "alert(new Array(200).join('Long message!'));", true);
    }

    private void assertScrollViewFocusabilityInAlertDialog(
            final String jsAlertScript, final boolean expectedFocusability)
            throws InterruptedException, TimeoutException, ExecutionException {
        final OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog(jsAlertScript);

        final JavascriptAppModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        final String errorMessage =
                "Scroll view focusability was incorrect. Expected: " + expectedFocusability;

        CriteriaHelper.pollUiThread(new Criteria(errorMessage) {
            @Override
            public boolean isSatisfied() {
                return jsDialog.getDialogForTest()
                               .findViewById(R.id.js_modal_dialog_scroll_view)
                               .isFocusable()
                        == expectedFocusability;
            }
        });

        clickOk(jsDialog);
        Assert.assertTrue("JavaScript execution should continue after closing prompt.",
                scriptEvent.waitUntilHasValue());
    }

    private static class TapGestureStateListener implements GestureStateListener {
        private CallbackHelper mCallbackHelper = new CallbackHelper();

        public int getCallCount() {
            return mCallbackHelper.getCallCount();
        }

        public void waitForTap(int currentCallCount) throws InterruptedException, TimeoutException {
            mCallbackHelper.waitForCallback(currentCallCount);
        }

        @Override
        public void onSingleTap(boolean consumed) {
            mCallbackHelper.notifyCalled();
        }
    }

    private GestureListenerManager getGestureListenerManager() {
        return GestureListenerManager.fromWebContents(
                mActivityTestRule.getActivity().getCurrentWebContents());
    }

    /**
     * Taps on a view and waits for a callback.
     */
    private void tapViewAndWait() throws InterruptedException, TimeoutException {
        final TapGestureStateListener tapGestureStateListener = new TapGestureStateListener();
        int callCount = tapGestureStateListener.getCallCount();
        getGestureListenerManager().addListener(tapGestureStateListener);

        TouchCommon.singleClickView(mActivityTestRule.getActivity().getActivityTab().getView());
        tapGestureStateListener.waitForTap(callCount);
    }

    /**
     * Verifies beforeunload dialogs are shown and they block/allow navigation
     * as appropriate.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testBeforeUnloadDialog()
            throws InterruptedException, TimeoutException, ExecutionException {
        mActivityTestRule.loadUrl(BEFORE_UNLOAD_URL);
        // JavaScript onbeforeunload dialogs require a user gesture.
        tapViewAndWait();
        executeJavaScriptAndWaitForDialog("history.back();");

        JavascriptAppModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);
        checkButtonPresenceVisibilityText(jsDialog, 0, R.string.cancel, "Cancel");
        clickCancel(jsDialog);

        Assert.assertEquals(BEFORE_UNLOAD_URL,
                mActivityTestRule.getActivity().getCurrentWebContents().getLastCommittedUrl());
        executeJavaScriptAndWaitForDialog("history.back();");

        jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);
        checkButtonPresenceVisibilityText(jsDialog, 2, R.string.leave, "Leave");

        final TestCallbackHelperContainer.OnPageFinishedHelper onPageLoaded =
                getActiveTabTestCallbackHelperContainer().getOnPageFinishedHelper();
        int callCount = onPageLoaded.getCallCount();
        clickOk(jsDialog);
        onPageLoaded.waitForCallback(callCount);
        Assert.assertEquals(EMPTY_PAGE,
                mActivityTestRule.getActivity().getCurrentWebContents().getLastCommittedUrl());
    }

    /**
     * Verifies that when showing a beforeunload dialogs as a result of a page
     * reload, the correct UI strings are used.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testBeforeUnloadOnReloadDialog()
            throws InterruptedException, TimeoutException, ExecutionException {
        mActivityTestRule.loadUrl(BEFORE_UNLOAD_URL);
        // JavaScript onbeforeunload dialogs require a user gesture.
        tapViewAndWait();
        executeJavaScriptAndWaitForDialog("window.location.reload();");

        JavascriptAppModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        checkButtonPresenceVisibilityText(jsDialog, 0, R.string.cancel, "Cancel");
        checkButtonPresenceVisibilityText(jsDialog, 2, R.string.reload, "Reload");
    }

    /**
     * Verifies that repeated dialogs give the option to disable dialogs
     * altogether and then that disabling them works.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testDisableRepeatedDialogs()
            throws InterruptedException, TimeoutException, ExecutionException {
        OnEvaluateJavaScriptResultHelper scriptEvent =
                executeJavaScriptAndWaitForDialog("alert('Android');");

        // Show a dialog once.
        JavascriptAppModalDialog jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);

        clickCancel(jsDialog);
        scriptEvent.waitUntilHasValue();

        // Show it again, it should have the option to suppress subsequent dialogs.
        scriptEvent = executeJavaScriptAndWaitForDialog("alert('Android');");
        jsDialog = getCurrentDialog();
        Assert.assertNotNull("No dialog showing.", jsDialog);
        final AlertDialog dialog = jsDialog.getDialogForTest();
        String errorMessage = ThreadUtils.runOnUiThreadBlocking(() -> {
            final CheckBox suppress = (CheckBox) dialog.findViewById(
                    org.chromium.chrome.R.id.suppress_js_modal_dialogs);
            if (suppress == null) return "Suppress checkbox not found.";
            if (suppress.getVisibility() != View.VISIBLE) {
                return "Suppress checkbox is not visible.";
            }
            suppress.setChecked(true);
            return null;
        });
        Assert.assertNull(errorMessage, errorMessage);
        clickCancel(jsDialog);
        scriptEvent.waitUntilHasValue();

        scriptEvent.evaluateJavaScriptForTests(
                mActivityTestRule.getActivity().getCurrentWebContents(), "alert('Android');");
        Assert.assertTrue(
                "No further dialog boxes should be shown.", scriptEvent.waitUntilHasValue());
    }

    /**
     * Displays a dialog and closes the tab in the background before attempting
     * to accept the dialog. Verifies that the dialog is dismissed when the tab
     * is closed.
     */
    @Test
    @MediumTest
    @Feature({"Browser", "Main"})
    public void testDialogDismissedAfterClosingTab() {
        executeJavaScriptAndWaitForDialog("alert('Android')");

        ThreadUtils.runOnUiThreadBlocking(() -> {
            ChromeActivity activity = mActivityTestRule.getActivity();
            activity.getCurrentTabModel().closeTab(activity.getActivityTab());
        });

        // Closing the tab should have dismissed the dialog.
        CriteriaHelper.pollInstrumentationThread(new JavascriptAppModalDialogShownCriteria(
                "The dialog should have been dismissed when its tab was closed.", false));
    }

    /**
     * Asynchronously executes the given code for spawning a dialog and waits
     * for the dialog to be visible.
     */
    private OnEvaluateJavaScriptResultHelper executeJavaScriptAndWaitForDialog(String script) {
        return executeJavaScriptAndWaitForDialog(new OnEvaluateJavaScriptResultHelper(), script);
    }

    /**
     * Given a JavaScript evaluation helper, asynchronously executes the given
     * code for spawning a dialog and waits for the dialog to be visible.
     */
    private OnEvaluateJavaScriptResultHelper executeJavaScriptAndWaitForDialog(
            final OnEvaluateJavaScriptResultHelper helper, String script) {
        helper.evaluateJavaScriptForTests(
                mActivityTestRule.getActivity().getCurrentWebContents(), script);
        CriteriaHelper.pollInstrumentationThread(new JavascriptAppModalDialogShownCriteria(
                "Could not spawn or locate a modal dialog.", true));
        return helper;
    }

    /**
     * Returns an array of the 3 buttons for this dialog, in the order
     * BUTTON_NEGATIVE, BUTTON_NEUTRAL and BUTTON_POSITIVE. Any of these values
     * can be null.
     */
    private Button[] getAlertDialogButtons(final AlertDialog dialog) throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(() -> {
            final Button[] buttons = new Button[3];
            buttons[0] = dialog.getButton(DialogInterface.BUTTON_NEGATIVE);
            buttons[1] = dialog.getButton(DialogInterface.BUTTON_NEUTRAL);
            buttons[2] = dialog.getButton(DialogInterface.BUTTON_POSITIVE);
            return buttons;
        });
    }

    /**
     * Returns the current JavaScript modal dialog showing or null if no such dialog is currently
     * showing.
     */
    private JavascriptAppModalDialog getCurrentDialog() throws ExecutionException {
        return ThreadUtils.runOnUiThreadBlocking(
                () -> JavascriptAppModalDialog.getCurrentDialogForTest());
    }

    private static class JavascriptAppModalDialogShownCriteria extends Criteria {
        private final boolean mShouldBeShown;

        public JavascriptAppModalDialogShownCriteria(String error, boolean shouldBeShown) {
            super(error);
            mShouldBeShown = shouldBeShown;
        }

        @Override
        public boolean isSatisfied() {
            try {
                return ThreadUtils.runOnUiThreadBlocking(() -> {
                    final boolean isShown =
                            JavascriptAppModalDialog.getCurrentDialogForTest() != null;
                    return mShouldBeShown == isShown;
                });
            } catch (ExecutionException e) {
                Log.e(TAG, "Failed to getCurrentDialog", e);
                return false;
            }
        }
    }

    /**
     * Simulates pressing the OK button of the passed dialog.
     */
    private void clickOk(JavascriptAppModalDialog dialog) {
        clickButton(dialog, DialogInterface.BUTTON_POSITIVE);
    }

    /**
     * Simulates pressing the Cancel button of the passed dialog.
     */
    private void clickCancel(JavascriptAppModalDialog dialog) {
        clickButton(dialog, DialogInterface.BUTTON_NEGATIVE);
    }

    private void clickButton(final JavascriptAppModalDialog dialog, final int whichButton) {
        ThreadUtils.runOnUiThreadBlocking(() -> dialog.onClick(null, whichButton));
    }

    private void checkButtonPresenceVisibilityText(
            JavascriptAppModalDialog jsDialog, int buttonIndex,
            int expectedTextResourceId, String readableName) throws ExecutionException {
        final Button[] buttons = getAlertDialogButtons(jsDialog.getDialogForTest());
        final Button button = buttons[buttonIndex];
        Assert.assertNotNull("No '" + readableName + "' button in confirm dialog.", button);
        Assert.assertEquals("'" + readableName + "' button is not visible.", View.VISIBLE,
                button.getVisibility());
        Assert.assertEquals("'" + readableName + "' button has wrong text",
                mActivityTestRule.getActivity().getResources().getString(expectedTextResourceId),
                button.getText().toString());
    }

    private TestCallbackHelperContainer getActiveTabTestCallbackHelperContainer() {
        return new TestCallbackHelperContainer(mActivityTestRule.getWebContents());
    }
}
