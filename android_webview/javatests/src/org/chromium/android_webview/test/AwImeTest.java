// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.content.Context;
import android.support.test.filters.SmallTest;
import android.view.KeyEvent;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.webkit.JavascriptInterface;
import android.widget.EditText;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.content.browser.test.util.TestInputMethodManagerWrapper;
import org.chromium.content_public.browser.ImeAdapter;

/**
 * Tests for IME (input method editor) on Android WebView.
 */
@RunWith(AwJUnit4ClassRunner.class)
public class AwImeTest {
    @Rule
    public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();

    private static class TestJavascriptInterface {
        private final CallbackHelper mFocusCallbackHelper = new CallbackHelper();

        @JavascriptInterface
        public void onEditorFocused() {
            mFocusCallbackHelper.notifyCalled();
        }

        public CallbackHelper getFocusCallbackHelper() {
            return mFocusCallbackHelper;
        }
    }

    private TestAwContentsClient mContentsClient;
    private AwTestContainerView mTestContainerView;
    private EditText mEditText;
    private final TestJavascriptInterface mTestJavascriptInterface = new TestJavascriptInterface();
    private TestInputMethodManagerWrapper mInputMethodManagerWrapper;

    @Before
    public void setUp() throws Exception {
        mContentsClient = new TestAwContentsClient();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            // Use detached container view to avoid focus request.
            mTestContainerView =
                    mActivityTestRule.createDetachedAwTestContainerView(mContentsClient);
            mEditText = new EditText(mActivityTestRule.getActivity());
            mActivityTestRule.getActivity().addView(mEditText);
            mActivityTestRule.getActivity().addView(mTestContainerView);
            mTestContainerView.getAwContents().addJavascriptInterface(
                    mTestJavascriptInterface, "test");
            // Let's not test against real input method.
            ImeAdapter imeAdapter = ImeAdapter.fromWebContents(mTestContainerView.getWebContents());
            imeAdapter.setInputMethodManagerWrapper(
                    TestInputMethodManagerWrapper.create(imeAdapter));
        });
    }

    private void loadContentEditableBody() throws Exception {
        final String mime = "text/html";
        final String htmlDocument = "<html><body contenteditable id='editor'></body></html>";
        final CallbackHelper loadHelper = mContentsClient.getOnPageFinishedHelper();

        mActivityTestRule.loadDataSync(
                mTestContainerView.getAwContents(), loadHelper, htmlDocument, mime, false);
    }

    private void focusOnEditTextAndShowKeyboard() {
        ThreadUtils.runOnUiThreadBlocking(() -> {
            mEditText.requestFocus();
            InputMethodManager imm =
                    (InputMethodManager) mActivityTestRule.getActivity().getSystemService(
                            Context.INPUT_METHOD_SERVICE);
            imm.showSoftInput(mEditText, 0);
        });
    }

    private void focusOnWebViewAndEnableEditing() throws Exception {
        ThreadUtils.runOnUiThreadBlocking((Runnable) () -> mTestContainerView.requestFocus());

        AwActivityTestRule.enableJavaScriptOnUiThread(mTestContainerView.getAwContents());
        // View focus may not have been propagated to the renderer process yet. If document is not
        // yet focused, and focusing on an element is an invalid operation. See crbug.com/622151
        // for details.
        mActivityTestRule.executeJavaScriptAndWaitForResult(mTestContainerView.getAwContents(),
                mContentsClient,
                "function onDocumentFocused() {\n"
                        + "  document.getElementById('editor').focus();\n"
                        + "  test.onEditorFocused();\n"
                        + "}\n"
                        + "(function() {\n"
                        + "if (document.hasFocus()) {\n"
                        + "  onDocumentFocused();"
                        + "} else {\n"
                        + "  window.addEventListener('focus', onDocumentFocused);\n"
                        + "}})();");
        mTestJavascriptInterface.getFocusCallbackHelper().waitForCallback(0);
    }

    private InputConnection getInputConnection() {
        return ImeAdapter.fromWebContents(mTestContainerView.getWebContents())
                .getInputConnectionForTest();
    }

    private void waitForNonNullInputConnection() {
        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return getInputConnection() != null;
            }
        });
    }

    /**
     * Tests that moving from EditText to WebView keeps the keyboard showing.
     */
    // https://crbug.com/569556
    @Test
    @SmallTest
    @Feature({"AndroidWebView", "TextInput"})
    public void testPressNextFromEditTextAndType() throws Throwable {
        loadContentEditableBody();
        focusOnEditTextAndShowKeyboard();
        focusOnWebViewAndEnableEditing();
        waitForNonNullInputConnection();
    }

    /**
     * Tests moving focus out of a WebView by calling InputConnection#sendKeyEvent() with a dpad
     * keydown event.
     */
    // https://crbug.com/787651
    // Flaky! - https://crbug.com/795423
    @Test
    // @SmallTest
    @DisabledTest
    public void testImeDpadMovesFocusOutOfWebView() throws Throwable {
        loadContentEditableBody();
        focusOnEditTextAndShowKeyboard();
        focusOnWebViewAndEnableEditing();
        waitForNonNullInputConnection();

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mActivityTestRule.getActivity().getCurrentFocus() == mTestContainerView;
            }
        });

        ThreadUtils.runOnUiThreadBlocking((Runnable) () -> {
            getInputConnection().sendKeyEvent(
                    new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_UP));
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mActivityTestRule.getActivity().getCurrentFocus() == mEditText;
            }
        });
    }

    /**
     * Tests moving focus out of a WebView by calling View#dispatchKeyEvent() with a dpad
     * keydown event.
     */
    @Test
    @SmallTest
    public void testDpadDispatchKeyEventMovesFocusOutOfWebView() throws Throwable {
        loadContentEditableBody();
        focusOnEditTextAndShowKeyboard();
        focusOnWebViewAndEnableEditing();
        waitForNonNullInputConnection();

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mActivityTestRule.getActivity().getCurrentFocus() == mTestContainerView;
            }
        });

        ThreadUtils.runOnUiThreadBlocking(() -> {
            mTestContainerView.dispatchKeyEvent(
                    new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DPAD_UP));
        });

        CriteriaHelper.pollUiThread(new Criteria() {
            @Override
            public boolean isSatisfied() {
                return mActivityTestRule.getActivity().getCurrentFocus() == mEditText;
            }
        });
    }
}
