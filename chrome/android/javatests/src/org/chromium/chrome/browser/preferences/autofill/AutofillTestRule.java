// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.autofill;

import android.view.KeyEvent;
import android.widget.EditText;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.CallbackHelper;
import org.chromium.chrome.browser.test.ChromeBrowserTestRule;
import org.chromium.chrome.browser.widget.prefeditor.EditorDialog;
import org.chromium.chrome.browser.widget.prefeditor.EditorObserverForTest;

import java.util.List;
import java.util.concurrent.TimeoutException;

/**
 * Custom ChromeBrowserTestRule to test Autofill.
 */
class AutofillTestRule extends ChromeBrowserTestRule implements EditorObserverForTest {
    final CallbackHelper mClickUpdate;
    final CallbackHelper mEditorTextUpdate;
    final CallbackHelper mPreferenceUpdate;
    final CallbackHelper mValidationUpdate;

    private EditorDialog mEditorDialog;

    AutofillTestRule() {
        mClickUpdate = new CallbackHelper();
        mEditorTextUpdate = new CallbackHelper();
        mPreferenceUpdate = new CallbackHelper();
        mValidationUpdate = new CallbackHelper();
        AutofillProfilesFragment.setObserverForTest(AutofillTestRule.this);
    }

    protected void setTextInEditorAndWait(final String[] values)
            throws InterruptedException, TimeoutException {
        int callCount = mEditorTextUpdate.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            List<EditText> fields = mEditorDialog.getEditableTextFieldsForTest();
            for (int i = 0; i < values.length; i++) {
                fields.get(i).setText(values[i]);
            }
        });
        mEditorTextUpdate.waitForCallback(callCount);
    }

    protected void clickInEditorAndWait(final int resourceId)
            throws InterruptedException, TimeoutException {
        int callCount = mClickUpdate.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                (Runnable) () -> mEditorDialog.findViewById(resourceId).performClick());
        mClickUpdate.waitForCallback(callCount);
    }

    protected void clickInEditorAndWaitForValidationError(final int resourceId)
            throws InterruptedException, TimeoutException {
        int callCount = mValidationUpdate.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(
                (Runnable) () -> mEditorDialog.findViewById(resourceId).performClick());
        mValidationUpdate.waitForCallback(callCount);
    }

    protected void sendKeycodeToTextFieldInEditorAndWait(final int keycode,
            final int textFieldIndex) throws InterruptedException, TimeoutException {
        int callCount = mClickUpdate.getCallCount();
        ThreadUtils.runOnUiThreadBlocking(() -> {
            List<EditText> fields = mEditorDialog.getEditableTextFieldsForTest();
            fields.get(textFieldIndex)
                    .dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, keycode));
            fields.get(textFieldIndex).dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, keycode));
        });
        mClickUpdate.waitForCallback(callCount);
    }

    protected void waitForThePreferenceUpdate() throws InterruptedException, TimeoutException {
        int callCount = mPreferenceUpdate.getCallCount();
        mPreferenceUpdate.waitForCallback(callCount);
    }

    protected void setEditorDialog(EditorDialog editorDialog) {
        mEditorDialog = editorDialog;
    }

    @Override
    public void onEditorDismiss() {
        ThreadUtils.assertOnUiThread();
        mPreferenceUpdate.notifyCalled();
    }

    @Override
    public void onEditorTextUpdate() {
        ThreadUtils.assertOnUiThread();
        mEditorTextUpdate.notifyCalled();
    }
    @Override
    public void onEditorReadyToEdit() {
        ThreadUtils.assertOnUiThread();
        mClickUpdate.notifyCalled();
    }

    @Override
    public void onEditorValidationError() {
        ThreadUtils.assertOnUiThread();
        mValidationUpdate.notifyCalled();
    }
}
