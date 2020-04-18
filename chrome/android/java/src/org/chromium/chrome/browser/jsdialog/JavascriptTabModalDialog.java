// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.jsdialog;

import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.modaldialog.ModalDialogManager;
import org.chromium.chrome.browser.modaldialog.ModalDialogView;
import org.chromium.ui.base.WindowAndroid;

/**
 * The controller to communicate with native JavaScriptDialogAndroid for a tab modal JavaScript
 * dialog. This can be an alert dialog, a prompt dialog or a confirm dialog.
 */
public class JavascriptTabModalDialog implements ModalDialogView.Controller {
    private static final String TAG = "JsTabModalDialog";

    private final String mTitle;
    private final String mMessage;
    private final int mPositiveButtonTextId;
    private final int mNegativeButtonTextId;

    private ModalDialogManager mModalDialogManager;
    private String mDefaultPromptText;
    private long mNativeDialogPointer;
    private JavascriptModalDialogView mDialogView;

    /**
     * Constructor for initializing contents to be shown on the dialog.
     */
    private JavascriptTabModalDialog(
            String title, String message, int positiveButtonTextId, int negativeButtonTextId) {
        mTitle = title;
        mMessage = message;
        mPositiveButtonTextId = positiveButtonTextId;
        mNegativeButtonTextId = negativeButtonTextId;
    }

    /**
     * Constructor for creating prompt dialog only.
     */
    private JavascriptTabModalDialog(String title, String message, String defaultPromptText) {
        this(title, message, R.string.ok, R.string.cancel);
        mDefaultPromptText = defaultPromptText;
    }

    @CalledByNative
    private static JavascriptTabModalDialog createAlertDialog(String title, String message) {
        return new JavascriptTabModalDialog(title, message, R.string.ok, 0);
    }

    @CalledByNative
    private static JavascriptTabModalDialog createConfirmDialog(String title, String message) {
        return new JavascriptTabModalDialog(title, message, R.string.ok, R.string.cancel);
    }

    @CalledByNative
    private static JavascriptTabModalDialog createPromptDialog(
            String title, String message, String defaultPromptText) {
        return new JavascriptTabModalDialog(title, message, defaultPromptText);
    }

    @CalledByNative
    private void showDialog(WindowAndroid window, long nativeDialogPointer) {
        assert window != null;
        ChromeActivity activity = (ChromeActivity) window.getActivity().get();
        // If the activity has gone away, then just clean up the native pointer.
        if (activity == null) {
            nativeCancel(nativeDialogPointer);
            return;
        }

        // Cache the native dialog pointer so that we can use it to return the response.
        mNativeDialogPointer = nativeDialogPointer;

        mModalDialogManager = activity.getModalDialogManager();
        mDialogView = JavascriptModalDialogView.create(this, mTitle, mMessage, mDefaultPromptText,
                false, mPositiveButtonTextId, mNegativeButtonTextId);
        mModalDialogManager.showDialog(mDialogView, ModalDialogManager.TAB_MODAL);
    }

    @CalledByNative
    private String getUserInput() {
        return mDialogView.getPromptText();
    }

    @CalledByNative
    private void dismiss() {
        mModalDialogManager.dismissDialog(mDialogView);
        mNativeDialogPointer = 0;
    }

    @Override
    public void onClick(@ModalDialogView.ButtonType int buttonType) {
        switch (buttonType) {
            case ModalDialogView.BUTTON_POSITIVE:
                accept(mDialogView.getPromptText());
                mModalDialogManager.dismissDialog(mDialogView);
                break;
            case ModalDialogView.BUTTON_NEGATIVE:
                cancel();
                mModalDialogManager.dismissDialog(mDialogView);
                break;
            default:
                Log.e(TAG, "Unexpected button pressed in dialog: " + buttonType);
        }
    }

    @Override
    public void onCancel() {
        cancel();
    }

    @Override
    public void onDismiss() {}

    /**
     * Sends notification to native that the user accepts the dialog.
     * @param promptResult The text edited by user.
     */
    private void accept(String promptResult) {
        if (mNativeDialogPointer != 0) {
            nativeAccept(mNativeDialogPointer, promptResult);
        }
    }

    /**
     * Sends notification to native that the user cancels the dialog.
     */
    private void cancel() {
        if (mNativeDialogPointer != 0) {
            nativeCancel(mNativeDialogPointer);
        }
    }

    private native void nativeAccept(long nativeJavaScriptDialogAndroid, String prompt);
    private native void nativeCancel(long nativeJavaScriptDialogAndroid);
}
